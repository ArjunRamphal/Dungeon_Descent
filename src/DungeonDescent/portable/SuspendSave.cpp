#include "SuspendSave.h"
#include "FileLock.h"
#include "../sfml/DungeonRun.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace dungeon::portable {
namespace {
constexpr const char* SuspendFormat = "DUNGEON_DESCENT_SUSPEND";
constexpr std::uint64_t MaxSuspendBytes = 512u * 1024u;

std::filesystem::path lockPath(std::filesystem::path file) {
    file += ".lock";
    return file;
}

struct PendingDir {
    std::filesystem::path directory;
    ~PendingDir() {
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
    }
};

std::filesystem::path createStagingDir(const std::filesystem::path& parent) {
    static std::atomic<unsigned> serial{0};
    for (int attempt = 0; attempt < 100; ++attempt) {
        const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        const auto path = parent / (".suspend-pending-" + std::to_string(stamp) + "-" + std::to_string(serial++));
        if (std::filesystem::create_directory(path)) return path;
    }
    throw std::runtime_error("Cannot prepare staging directory for suspend save.");
}

std::string encodeHex(const std::string& input) {
    if (input.empty()) return "-";
    static constexpr char Hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(input.size() * 2);
    for (unsigned char c : input) {
        out.push_back(Hex[(c >> 4) & 0x0f]);
        out.push_back(Hex[c & 0x0f]);
    }
    return out;
}

std::string decodeHex(const std::string& hex) {
    if (hex == "-") return {};
    if (hex.size() % 2 != 0) throw std::runtime_error("Corrupt hex payload in suspend save.");
    auto nibble = [](char c) -> unsigned {
        if (c >= '0' && c <= '9') return static_cast<unsigned>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<unsigned>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<unsigned>(c - 'A' + 10);
        throw std::runtime_error("Invalid hex character in suspend save.");
    };
    std::string out;
    out.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        out.push_back(static_cast<char>((nibble(hex[i]) << 4) | nibble(hex[i + 1])));
    }
    return out;
}

std::uint64_t fnv1a64(const std::string& data) {
    std::uint64_t hash = 14695981039346656037ull;
    for (unsigned char c : data) {
        hash ^= c;
        hash *= 1099511628211ull;
    }
    return hash;
}

SuspendedRun parseSuspendFile(const std::filesystem::path& file) {
    if (std::filesystem::is_symlink(std::filesystem::symlink_status(file))) {
        throw std::runtime_error("Suspend save cannot be a symbolic link.");
    }
    if (!std::filesystem::is_regular_file(file)) {
        throw std::runtime_error("Suspend save is not a regular file.");
    }
    if (std::filesystem::file_size(file) > MaxSuspendBytes) {
        throw std::runtime_error("Suspend save exceeds maximum size.");
    }
    std::ifstream in(file, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open suspend save file.");

    std::string headerLine;
    if (!std::getline(in, headerLine)) throw std::runtime_error("Cannot read suspend save header.");
    std::istringstream header(headerLine);
    std::string magic, extra;
    int version = 0;
    std::uint64_t expectedHash = 0;
    if (!(header >> magic >> version >> expectedHash) || (header >> extra) ||
        magic != SuspendFormat || version != 1) {
        throw std::runtime_error("Invalid suspend save header.");
    }

    std::string metaLine;
    if (!std::getline(in, metaLine)) throw std::runtime_error("Missing metadata in suspend save.");
    std::istringstream meta(metaLine);
    std::string metaTag;
    double playTime = 0.0;
    std::uint32_t seed = 0;
    int dailyFlag = 0;
    std::string dateHex;
    if (!(meta >> metaTag >> playTime >> seed >> dailyFlag >> dateHex) || (meta >> extra) ||
        metaTag != "app" || !std::isfinite(playTime) || playTime < 0.0 || (dailyFlag != 0 && dailyFlag != 1)) {
        throw std::runtime_error("Invalid metadata in suspend save.");
    }

    std::ostringstream payloadStream;
    payloadStream << in.rdbuf();
    std::string runState = payloadStream.str();
    if (fnv1a64(runState) != expectedHash) {
        throw std::runtime_error("Suspend save checksum verification failed.");
    }

    SuspendedRun result;
    result.playTimeSeconds = playTime;
    result.seed = seed;
    result.dailyRun = dailyFlag != 0;
    result.dailyDate = decodeHex(dateHex);
    result.runState = std::move(runState);
    return result;
}

void writeSuspendFileAtomic(const std::filesystem::path& file, const SuspendedRun& data) {
    PendingDir pending{createStagingDir(file.parent_path())};
    const auto temporary = pending.directory / "suspend_run.sav";
    std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("Cannot create temporary suspend save.");
    const std::uint64_t hash = fnv1a64(data.runState);
    out << SuspendFormat << " 1 " << hash << '\n';
    out << "app " << std::setprecision(17) << data.playTimeSeconds << ' ' << data.seed << ' '
        << (data.dailyRun ? 1 : 0) << ' ' << encodeHex(data.dailyDate) << '\n';
    out.write(data.runState.data(), static_cast<std::streamsize>(data.runState.size()));
    out.close();
    if (!out) throw std::runtime_error("Cannot finish writing suspend save.");
#ifdef _WIN32
    if (!MoveFileExW(temporary.c_str(), file.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Cannot replace suspend save file.");
#else
    std::filesystem::rename(temporary, file);
#endif
}
} // namespace

SuspendSave::SuspendSave(std::filesystem::path file) : file_(std::filesystem::absolute(std::move(file))) {}

bool SuspendSave::exists() const {
    if (!std::filesystem::exists(file_)) return false;
    std::filesystem::create_directories(file_.parent_path());
    FileLock lock(lockPath(file_));
    return std::filesystem::is_regular_file(std::filesystem::symlink_status(file_));
}

void SuspendSave::save(const DungeonRun& run, double playTimeSeconds, bool dailyRun, const std::string& dailyDate) {
    if (!run.canSuspend()) {
        throw std::runtime_error("Cannot suspend run during a timed riddle or outside an active descent.");
    }
    if (!std::isfinite(playTimeSeconds) || playTimeSeconds < 0.0) {
        throw std::invalid_argument("Playtime must be finite and non-negative.");
    }
    SuspendedRun data;
    data.playTimeSeconds = playTimeSeconds;
    data.seed = run.seed();
    data.dailyRun = dailyRun;
    data.dailyDate = dailyDate;
    data.runState = run.serializeState();

    std::filesystem::create_directories(file_.parent_path());
    FileLock lock(lockPath(file_));
    if (std::filesystem::is_symlink(std::filesystem::symlink_status(file_))) {
        throw std::runtime_error("Suspend save path cannot be a symbolic link.");
    }
    writeSuspendFileAtomic(file_, data);
}

std::optional<SuspendedRun> SuspendSave::peek() const {
    if (!std::filesystem::exists(file_)) return std::nullopt;
    std::filesystem::create_directories(file_.parent_path());
    FileLock lock(lockPath(file_));
    if (!std::filesystem::exists(file_)) return std::nullopt;
    return parseSuspendFile(file_);
}

std::optional<SuspendedRun> SuspendSave::loadAndConsume(DungeonRun& run) {
    std::filesystem::create_directories(file_.parent_path());
    FileLock lock(lockPath(file_));
    if (!std::filesystem::exists(file_)) return std::nullopt;
    auto data = parseSuspendFile(file_);
    run.deserializeState(data.runState);
    if (!run.canSuspend()) {
        throw std::runtime_error("Saved run is not in a resumable state.");
    }
    std::error_code ignored;
    std::filesystem::remove(file_, ignored);
    if (std::filesystem::exists(file_)) {
        throw std::runtime_error("Cannot consume suspend save file upon resuming.");
    }
    return data;
}

void SuspendSave::clear() const {
    if (!std::filesystem::exists(file_.parent_path())) return;
    FileLock lock(lockPath(file_));
    std::error_code error;
    std::filesystem::remove(file_, error);
    if (error && std::filesystem::exists(file_)) {
        throw std::runtime_error("Cannot remove suspend save file.");
    }
}

} // namespace dungeon::portable
