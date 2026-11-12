#include "AssetBundle.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace dungeon::portable {
namespace {
constexpr char Magic[] = "DDAST001";
constexpr std::uint64_t Basis = 14695981039346656037ull;
constexpr std::uint64_t Prime = 1099511628211ull;
constexpr std::uint32_t MaxEntries = 100000, MaxName = 4096;
constexpr std::size_t Chunk = 65536;

void hashBytes(std::uint64_t& hash, const unsigned char* data, std::size_t size) {
    for (std::size_t i = 0; i < size; ++i) { hash ^= data[i]; hash *= Prime; }
}
void hashInteger(std::uint64_t& hash, std::uint64_t value, unsigned width) {
    for (unsigned i = 0; i < width; ++i) { const auto byte = static_cast<unsigned char>(value >> (i * 8)); hashBytes(hash, &byte, 1); }
}
void hashEntry(std::uint64_t& hash, const std::string& name, std::uint64_t size, std::uint64_t checksum) {
    hashInteger(hash, name.size(), 4); hashInteger(hash, size, 8); hashInteger(hash, checksum, 8);
    hashBytes(hash, reinterpret_cast<const unsigned char*>(name.data()), name.size());
}
std::uint64_t fileChecksum(const std::filesystem::path& file) {
    std::ifstream input(file, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot read bundled file: " + file.string());
    std::array<unsigned char, Chunk> buffer{};
    std::uint64_t hash = Basis;
    while (input) {
        input.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        hashBytes(hash, buffer.data(), static_cast<std::size_t>(input.gcount()));
    }
    if (!input.eof()) throw std::runtime_error("Cannot finish reading bundled file: " + file.string());
    return hash;
}
std::string lower(std::string name) {
    for (auto& c : name) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return name;
}
void validateName(const std::string& name) {
    if (name.empty() || name.size() > MaxName || name.front() == '/' || name.back() == '/' || lower(name) == ".complete")
        throw std::runtime_error("Invalid bundled asset path.");
    for (const unsigned char c : name) if (c < 32 || c == '\\' || c == ':' || c == 127)
        throw std::runtime_error("Invalid character in bundled asset path.");
    std::istringstream parts(name);
    std::string part;
    while (std::getline(parts, part, '/')) {
        if (part.empty() || part == "." || part == ".." || part.back() == '.' || part.back() == ' ')
            throw std::runtime_error("Bundled asset path escapes its directory.");
        const auto stem = lower(part.substr(0, part.find('.')));
        if (stem == "con" || stem == "prn" || stem == "aux" || stem == "nul" ||
            (stem.size() == 4 && (stem.substr(0, 3) == "com" || stem.substr(0, 3) == "lpt") && stem[3] >= '1' && stem[3] <= '9'))
            throw std::runtime_error("Reserved filename in bundled assets.");
    }
}
void writeInteger(std::ostream& out, std::uint64_t value, unsigned width) {
    for (unsigned i = 0; i < width; ++i) out.put(static_cast<char>(value >> (i * 8)));
}
struct Reader {
    Bytes bytes;
    std::size_t cursor = 0;
    std::uint64_t integer(unsigned width) {
        if (width > bytes.size - cursor) throw std::runtime_error("Truncated embedded assets.");
        std::uint64_t value = 0;
        for (unsigned i = 0; i < width; ++i) value |= static_cast<std::uint64_t>(bytes.data[cursor++]) << (8 * i);
        return value;
    }
};
struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() { if (!path.empty()) { std::error_code ignored; std::filesystem::remove_all(path, ignored); } }
};
class CacheLock {
public:
    explicit CacheLock(const std::filesystem::path& file) {
#ifdef _WIN32
        handle_ = CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (handle_ == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot open the game asset cache lock.");
        BY_HANDLE_FILE_INFORMATION info{};
        if (!GetFileInformationByHandle(handle_, &info) || (info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) ||
            !LockFileEx(handle_, LOCKFILE_EXCLUSIVE_LOCK, 0, 1, 0, &overlapped_)) {
            CloseHandle(handle_); throw std::runtime_error("Cannot lock the game asset cache.");
        }
#else
        handle_ = open(file.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (handle_ < 0) throw std::runtime_error("Cannot open the game asset cache lock.");
        if (flock(handle_, LOCK_EX) != 0) { close(handle_); throw std::runtime_error("Cannot lock the game asset cache."); }
#endif
    }
    ~CacheLock() {
#ifdef _WIN32
        UnlockFileEx(handle_, 0, 1, 0, &overlapped_); CloseHandle(handle_);
#else
        flock(handle_, LOCK_UN); close(handle_);
#endif
    }
    CacheLock(const CacheLock&) = delete;
    CacheLock& operator=(const CacheLock&) = delete;
private:
#ifdef _WIN32
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    OVERLAPPED overlapped_{};
#else
    int handle_ = -1;
#endif
};
std::filesystem::path stagingDirectory(const std::filesystem::path& root) {
    static std::atomic<unsigned> serial{0};
    for (int attempt = 0; attempt < 100; ++attempt) {
        const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        const auto folder = root / (".preparing-" + std::to_string(stamp) + "-" + std::to_string(serial++));
        std::error_code error;
        if (std::filesystem::create_directory(folder, error)) return folder;
        if (error) throw std::filesystem::filesystem_error("Cannot prepare game assets", folder, error);
    }
    throw std::runtime_error("Cannot create a game asset preparation directory.");
}
} // namespace

AssetBundle::AssetBundle(Bytes bytes) : bytes_(bytes) {
    if (!bytes.data || bytes.size < 28 || !std::equal(Magic, Magic + 8, bytes.data))
        throw std::runtime_error("The executable's embedded assets are missing or damaged.");
    Reader input{bytes, 8};
    const auto count = input.integer(4), expectedHash = input.integer(8), expectedSize = input.integer(8);
    if (!count || count > MaxEntries || expectedSize != bytes.size) throw std::runtime_error("Invalid embedded asset archive.");
    std::uint64_t hash = Basis;
    hashInteger(hash, count, 4);
    std::set<std::string> names;
    for (std::uint64_t i = 0; i < count; ++i) {
        const auto length = input.integer(4), size = input.integer(8), checksum = input.integer(8);
        if (length > MaxName || length > bytes.size - input.cursor) throw std::runtime_error("Invalid embedded asset filename.");
        std::string name(reinterpret_cast<const char*>(bytes.data + input.cursor), static_cast<std::size_t>(length));
        validateName(name);
        if (!names.insert(lower(name)).second) throw std::runtime_error("Duplicate embedded asset filename.");
        input.cursor += static_cast<std::size_t>(length);
        if (size > bytes.size - input.cursor) throw std::runtime_error("Truncated embedded asset contents.");
        entries_.push_back({name, size, checksum, input.cursor});
        hashEntry(hash, name, size, checksum);
        input.cursor += static_cast<std::size_t>(size);
    }
    if (input.cursor != bytes.size || hash != expectedHash) throw std::runtime_error("Embedded asset directory checksum failed.");
    std::ostringstream identity;
    identity << std::hex << std::setfill('0') << std::setw(16) << hash;
    id_ = identity.str();
}

bool AssetBundle::validCache(const std::filesystem::path& folder) const {
    try {
        const auto status = std::filesystem::symlink_status(folder);
        if (!std::filesystem::is_directory(status) || std::filesystem::is_symlink(status)) return false;
        const auto marker = folder / ".complete";
        if (!std::filesystem::is_regular_file(std::filesystem::symlink_status(marker))) return false;
        std::ifstream complete(marker);
        std::string identity;
        if (!std::getline(complete, identity) || identity != id_) return false;
        for (const auto& entry : entries_) {
            const auto relative = std::filesystem::u8path(entry.name);
            auto parent = folder;
            for (const auto& component : relative.parent_path()) {
                parent /= component;
                if (!std::filesystem::is_directory(std::filesystem::symlink_status(parent))) return false;
            }
            const auto file = folder / relative;
            if (!std::filesystem::is_regular_file(std::filesystem::symlink_status(file)) ||
                std::filesystem::file_size(file) != entry.size || fileChecksum(file) != entry.checksum) return false;
        }
        return true;
    } catch (const std::filesystem::filesystem_error&) { return false; }
      catch (const std::runtime_error&) { return false; }
}

std::filesystem::path AssetBundle::materialize(const std::filesystem::path& cacheRoot) const {
    const auto folder = cacheRoot / id_;
    std::filesystem::create_directories(cacheRoot);
    CacheLock lock(cacheRoot / (id_ + ".lock"));
    if (validCache(folder)) return folder;
    Cleanup staging{stagingDirectory(cacheRoot)};
    for (const auto& entry : entries_) {
        const auto file = staging.path / std::filesystem::u8path(entry.name);
        std::filesystem::create_directories(file.parent_path());
        std::ofstream output(file, std::ios::binary);
        if (!output) throw std::runtime_error("Cannot prepare game file: " + file.string());
        std::uint64_t checksum = Basis;
        for (std::uint64_t offset = 0; offset < entry.size;) {
            const auto length = static_cast<std::size_t>(std::min<std::uint64_t>(Chunk, entry.size - offset));
            const auto data = bytes_.data + entry.offset + static_cast<std::size_t>(offset);
            hashBytes(checksum, data, length);
            output.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(length));
            offset += length;
        }
        output.close();
        if (!output || checksum != entry.checksum) throw std::runtime_error("Embedded game file checksum failed: " + entry.name);
    }
    std::ofstream marker(staging.path / ".complete");
    marker << id_ << '\n'; marker.close();
    if (!marker) throw std::runtime_error("Cannot finish preparing game assets.");
    std::error_code error;
    if (std::filesystem::exists(std::filesystem::symlink_status(folder))) std::filesystem::remove_all(folder);
    std::filesystem::rename(staging.path, folder, error);
    if (error) {
        if (validCache(folder)) return folder;
        throw std::filesystem::filesystem_error("Cannot finish preparing game assets", folder, error);
    }
    staging.path.clear();
    return folder;
}

void createArchive(const std::filesystem::path& assets, const std::filesystem::path& notices,
                   const std::filesystem::path& output) {
    struct Source { std::filesystem::path file; std::string name; };
    std::vector<Source> files;
    const auto collect = [&](const std::filesystem::path& root, const std::string& prefix) {
        for (const auto& file : std::filesystem::recursive_directory_iterator(root)) {
            if (file.is_symlink()) throw std::runtime_error("Asset archives cannot include symbolic links.");
            if (!file.is_regular_file()) continue;
            const auto utf8 = file.path().lexically_relative(root).generic_u8string();
            std::string name = prefix + std::string(utf8.begin(), utf8.end());
            validateName(name);
            if (std::filesystem::weakly_canonical(file.path()) == std::filesystem::weakly_canonical(output))
                throw std::runtime_error("An asset archive cannot overwrite its input.");
            files.push_back({file.path(), std::move(name)});
        }
    };
    collect(assets, "");
    if (!notices.empty()) collect(notices, "licenses/");
    if (files.empty() || files.size() > MaxEntries) throw std::runtime_error("Invalid number of game assets.");
    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
    std::set<std::string> names;
    for (const auto& file : files) if (!names.insert(lower(file.name)).second) throw std::runtime_error("Duplicate asset archive path: " + file.name);
    const auto parent = output.has_parent_path() ? output.parent_path() : std::filesystem::current_path();
    std::filesystem::create_directories(parent);
    Cleanup temporary{stagingDirectory(parent)};
    const auto archiveFile = temporary.path / "assets.pak";
    std::ofstream archive(archiveFile, std::ios::binary | std::ios::trunc);
    if (!archive) throw std::runtime_error("Cannot create embedded asset archive.");
    archive.write(Magic, 8);
    writeInteger(archive, files.size(), 4); writeInteger(archive, 0, 8); writeInteger(archive, 0, 8);
    std::uint64_t identity = Basis;
    hashInteger(identity, files.size(), 4);
    std::array<unsigned char, Chunk> buffer{};
    for (const auto& file : files) {
        const auto size = std::filesystem::file_size(file.file);
        writeInteger(archive, file.name.size(), 4); writeInteger(archive, size, 8);
        const auto checksumPosition = archive.tellp();
        writeInteger(archive, 0, 8); archive.write(file.name.data(), static_cast<std::streamsize>(file.name.size()));
        std::ifstream input(file.file, std::ios::binary);
        if (!input) throw std::runtime_error("Cannot read game asset: " + file.file.string());
        std::uint64_t hash = Basis, copied = 0;
        while (input) {
            input.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            const auto length = static_cast<std::size_t>(input.gcount());
            hashBytes(hash, buffer.data(), length);
            archive.write(reinterpret_cast<const char*>(buffer.data()), static_cast<std::streamsize>(length));
            copied += length;
        }
        if (!input.eof() || copied != size) throw std::runtime_error("A game asset changed while it was being packaged.");
        const auto end = archive.tellp(); archive.seekp(checksumPosition); writeInteger(archive, hash, 8); archive.seekp(end);
        hashEntry(identity, file.name, size, hash);
    }
    const auto length = archive.tellp();
    archive.seekp(12); writeInteger(archive, identity, 8); writeInteger(archive, static_cast<std::uint64_t>(length), 8);
    archive.close();
    if (!archive) throw std::runtime_error("Cannot finish writing embedded asset archive.");
    std::error_code error;
    std::filesystem::remove(output, error);
    std::filesystem::rename(archiveFile, output);
}
} // namespace dungeon::portable
