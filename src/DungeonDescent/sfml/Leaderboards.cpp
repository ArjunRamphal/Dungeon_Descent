#include "Leaderboards.h"
#include "../portable/FileLock.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>
#ifdef _WIN32
#include <shlobj.h>
#endif

namespace dungeon {
namespace {
constexpr const char* Format = "DUNGEON_DESCENT_SCORES";
bool better(const LeaderboardEntry& a, const LeaderboardEntry& b) {
    return a.score != b.score ? a.score > b.score : a.id < b.id;
}
std::filesystem::path lockFile(std::filesystem::path file) { file += ".lock"; return file; }
struct Pending {
    std::filesystem::path directory;
    ~Pending() { std::error_code ignored; std::filesystem::remove_all(directory, ignored); }
};
std::filesystem::path pendingDirectory(const std::filesystem::path& parent) {
    static std::atomic<unsigned> serial{0};
    for (int attempt = 0; attempt < 100; ++attempt) {
        const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        const auto path = parent / (".scores-pending-" + std::to_string(stamp) + "-" + std::to_string(serial++));
        if (std::filesystem::create_directory(path)) return path;
    }
    throw std::runtime_error("Cannot prepare the local leaderboard file.");
}
}

Leaderboards::Leaderboards(std::filesystem::path file) : file_(std::filesystem::absolute(std::move(file))) {}

std::filesystem::path Leaderboards::dataDirectory() {
#ifdef _WIN32
    std::wstring overridePath(32768, L'\0');
    const auto length = GetEnvironmentVariableW(L"DUNGEON_DESCENT_DATA_DIR", overridePath.data(), static_cast<DWORD>(overridePath.size()));
    if (length && length < overridePath.size()) { overridePath.resize(length); return overridePath; }
    PWSTR folder = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &folder))) {
        const auto result = std::filesystem::path(folder) / "DungeonDescent";
        CoTaskMemFree(folder);
        return result;
    }
#else
    if (const auto path = std::getenv("DUNGEON_DESCENT_DATA_DIR"); path && *path) return std::filesystem::u8path(path);
    if (const auto path = std::getenv("XDG_DATA_HOME"); path && *path && std::filesystem::path(path).is_absolute())
        return std::filesystem::path(path) / "DungeonDescent";
    if (const auto path = std::getenv("HOME"); path && *path) return std::filesystem::path(path) / ".local/share/DungeonDescent";
#endif
    return std::filesystem::temp_directory_path() / "DungeonDescent";
}

bool Leaderboards::validInitials(const std::string& initials) {
    return initials.size() == 3 && std::all_of(initials.begin(), initials.end(), [](char c) { return c >= 'A' && c <= 'Z'; });
}

Leaderboards::Data Leaderboards::read() const {
    Data data;
    if (!std::filesystem::exists(file_)) return data;
    if (std::filesystem::file_size(file_) > 65536) throw std::runtime_error("The local leaderboard file is too large.");
    std::ifstream input(file_, std::ios::binary);
    std::string line, format, extra;
    int version = 0;
    if (!std::getline(input, line)) throw std::runtime_error("Cannot read the local leaderboard file.");
    std::istringstream header(line);
    if (!(header >> format >> version >> data.nextId) || (header >> extra) || format != Format || version != 1 ||
        data.nextId == 0 || data.nextId == std::numeric_limits<std::uint64_t>::max())
        throw std::runtime_error("The local leaderboard header is invalid.");
    std::set<std::uint64_t> ids;
    while (std::getline(input, line)) {
        if (line.empty() || line == "\r") continue;
        std::istringstream row(line);
        LeaderboardEntry entry;
        int difficulty = -1, hero = -1, ending = 0;
        if (!(row >> difficulty >> entry.id >> entry.score >> entry.initials >> hero >> ending) || (row >> extra) ||
            difficulty < 0 || difficulty > 3 || hero < 0 || hero > 3 || ending < 1 || ending > 4 || entry.score < 0 ||
            !validInitials(entry.initials) || entry.id == 0 || entry.id >= data.nextId || !ids.insert(entry.id).second)
            throw std::runtime_error("The local leaderboard contains an invalid entry.");
        entry.hero = static_cast<Hero>(hero); entry.ending = static_cast<Ending>(ending);
        auto& table = data.tables[static_cast<std::size_t>(difficulty)];
        table.push_back(entry);
        if (table.size() > Limit) throw std::runtime_error("The local leaderboard contains too many entries.");
    }
    if (!input.eof()) throw std::runtime_error("Cannot finish reading the local leaderboard file.");
    for (auto& table : data.tables) std::sort(table.begin(), table.end(), better);
    return data;
}

void Leaderboards::write(const Data& data) const {
    Pending pending{pendingDirectory(file_.parent_path())};
    const auto temporary = pending.directory / "leaderboards.tsv";
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    output << Format << " 1 " << data.nextId << '\n';
    for (std::size_t mode = 0; mode < data.tables.size(); ++mode)
        for (const auto& entry : data.tables[mode])
            output << mode << '\t' << entry.id << '\t' << entry.score << '\t' << entry.initials << '\t'
                   << static_cast<int>(entry.hero) << '\t' << static_cast<int>(entry.ending) << '\n';
    output.close();
    if (!output) throw std::runtime_error("Cannot write the local leaderboard file.");
#ifdef _WIN32
    if (!MoveFileExW(temporary.c_str(), file_.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        throw std::runtime_error("Cannot replace the local leaderboard file.");
#else
    std::filesystem::rename(temporary, file_);
#endif
}

Leaderboards::Tables Leaderboards::load() const {
    std::filesystem::create_directories(file_.parent_path());
    portable::FileLock lock(lockFile(file_));
    return read().tables;
}

Leaderboards::Submission Leaderboards::submit(Difficulty difficulty, std::string initials, int score, Hero hero, Ending ending) const {
    for (auto& letter : initials) if (letter >= 'a' && letter <= 'z') letter = static_cast<char>(letter - 'a' + 'A');
    if (!validInitials(initials) || score < 0 || difficulty < Difficulty::Easy || difficulty > Difficulty::Impossible ||
        hero < Hero::Warrior || hero > Hero::Rogue || ending < Ending::Good || ending > Ending::Perfect)
        throw std::invalid_argument("A score requires three letters, a difficulty and a completed ending.");
    std::filesystem::create_directories(file_.parent_path());
    portable::FileLock lock(lockFile(file_));
    auto data = read();
    const auto id = data.nextId++;
    if (data.nextId == std::numeric_limits<std::uint64_t>::max()) throw std::runtime_error("The local leaderboard is full.");
    auto& table = data.tables[static_cast<std::size_t>(difficulty)];
    table.push_back({id, std::move(initials), score, hero, ending});
    std::sort(table.begin(), table.end(), better);
    const auto position = std::find_if(table.begin(), table.end(), [id](const auto& entry) { return entry.id == id; }) - table.begin();
    const auto rank = static_cast<std::size_t>(position) < Limit ? std::optional<std::size_t>(position + 1) : std::nullopt;
    if (table.size() > Limit) table.resize(Limit);
    write(data);
    return {std::move(data.tables), id, rank};
}
} // namespace dungeon
