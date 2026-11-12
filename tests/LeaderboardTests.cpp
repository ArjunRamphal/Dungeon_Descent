#include "Leaderboards.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <stdexcept>

using namespace dungeon;
namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class F> void rejects(F function, const char* message) {
    bool rejected = false;
    try { function(); } catch (const std::exception&) { rejected = true; }
    check(rejected, message);
}
std::string bytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
void write(const std::filesystem::path& path, const std::string& data) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary); out << data;
}
struct Temporary {
    std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("DungeonDescent-scores-" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    ~Temporary() { std::error_code ignored; std::filesystem::remove_all(root, ignored); }
};
}
int main() {
    try {
        Temporary temp;
        const auto file = temp.root / std::filesystem::u8path("r\xc3\xa9sum\xc3\xa9/\xe9\x9b\xaa/leaderboards.tsv");
        Leaderboards store(file);
        for (const auto& board : store.load()) check(board.empty(), "A new profile has four empty boards");
        for (int mode = 0; mode < 4; ++mode) {
            const auto ending = static_cast<Ending>(mode + 1);
            const auto result = store.submit(static_cast<Difficulty>(mode), "aBc", 100 + mode, static_cast<Hero>(mode), ending);
            check(result.rank == 1 && result.tables[mode][0].initials == "ABC", "Each difficulty accepts three letters and normalises case");
        }
        const auto reloaded = Leaderboards(file).load();
        for (int mode = 0; mode < 4; ++mode)
            check(reloaded[mode].size() == 1 && reloaded[mode][0].score == 100 + mode &&
                  reloaded[mode][0].ending == static_cast<Ending>(mode + 1), "All four endings and difficulty boards survive a fresh store instance");
        const auto original = bytes(file);
        for (const auto& name : {"", "AB", "ABCD", "A1B", "A B", "\xc3\xa9" "AB"})
            rejects([&] { store.submit(Difficulty::Easy, name, 99, Hero::Warrior, Ending::Good); }, "Reject invalid initials");
        rejects([&] { store.submit(Difficulty::Easy, "ABC", -1, Hero::Warrior, Ending::Good); }, "Reject negative scores");
        rejects([&] { store.submit(Difficulty::Easy, "ABC", 99, Hero::Warrior, Ending::None); }, "An unfinished run cannot enter a board");
        rejects([&] { store.submit(static_cast<Difficulty>(4), "ABC", 99, Hero::Warrior, Ending::Good); }, "Reject unknown difficulty");
        check(bytes(file) == original, "Rejected submissions leave existing scores intact");

        Leaderboards ranked(temp.root / "ranking/leaderboards.tsv");
        std::uint64_t firstTie = 0;
        for (int score = 0; score < 20; ++score) {
            const auto result = ranked.submit(Difficulty::Easy, "AAA", score, Hero::Mage, Ending::True);
            if (score == 19) firstTie = result.id;
        }
        auto tables = ranked.load();
        check(tables[0].size() == 10 && tables[0].front().score == 19 && tables[0].back().score == 10, "Only the ten highest overall scores are retained");
        const auto tied = ranked.submit(Difficulty::Easy, "AAA", 19, Hero::Rogue, Ending::Perfect);
        check(tied.rank == 2 && tied.tables[0][0].id == firstTie, "Ties keep the earlier run first and repeated initials are allowed");
        const auto low = ranked.submit(Difficulty::Easy, "LOW", 0, Hero::Mage, Ending::Bad);
        check(!low.rank && low.tables[0].size() == 10, "An outside-top-ten score does not displace higher scores");
        ranked.submit(Difficulty::Impossible, "ZER", 0, Hero::Mage, Ending::Bad);
        check(ranked.load()[3][0].score == 0 && ranked.load()[0].size() == 10, "Zero scores and independent difficulty tables are valid");

        std::vector<std::future<Leaderboards::Submission>> submissions;
        for (int score = 100; score < 108; ++score)
            submissions.push_back(std::async(std::launch::async, [&, score] {
                return Leaderboards(file).submit(Difficulty::Hard, "XYZ", score, Hero::Mage, Ending::Good);
            }));
        for (auto& task : submissions) task.get();
        const auto concurrent = store.load();
        check(concurrent[2].size() == 9 && concurrent[2].front().score == 107 && concurrent[2].back().score == 100,
              "Concurrent writers merge every run instead of replacing another game's scores");
        check(concurrent[0].size() == 1 && concurrent[1].size() == 1 && concurrent[3].size() == 1, "Concurrent writes preserve the other difficulties");

        const auto damaged = temp.root / "damaged/leaderboards.tsv";
        for (const auto& bad : {"DUNGEON_DESCENT_SCORES 2 1\n", "DUNGEON_DESCENT_SCORES 1 2\n0 1 100 AB 0 1\n",
                                "DUNGEON_DESCENT_SCORES 1 2\n0 1 100 ABC 0 1\n0 1 101 XYZ 0 1\n",
                                "DUNGEON_DESCENT_SCORES 1 3\n0 1 100 ABC 0 1\n0 2 99"}) {
            write(damaged, bad);
            rejects([&] { Leaderboards(damaged).load(); }, "Reject damaged, duplicated or unsupported leaderboard data");
            rejects([&] { Leaderboards(damaged).submit(Difficulty::Easy, "NEW", 20, Hero::Mage, Ending::Good); }, "Do not overwrite unreadable scores");
            check(bytes(damaged) == bad, "Unreadable score data is preserved for recovery");
        }
        const auto blocked = temp.root / "blocked";
        write(blocked, "keep this file");
        rejects([&] { Leaderboards(blocked / "leaderboards.tsv").submit(Difficulty::Easy, "ABC", 1, Hero::Mage, Ending::Good); }, "Report an unwritable data location");
        check(bytes(blocked) == "keep this file", "Storage failures do not delete existing files");
        for (const auto& entry : std::filesystem::recursive_directory_iterator(temp.root))
            check(entry.path().filename().generic_u8string().find(".scores-pending-") != 0, "Successful and failed writes clean their staging directories");
        std::cout << "Four boards, all endings, persistence, Unicode paths, ranking, ties, initials, concurrent writers and failure preservation passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
