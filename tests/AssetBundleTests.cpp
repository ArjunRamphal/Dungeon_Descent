#include "AssetBundle.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <stdexcept>

using namespace dungeon::portable;
namespace {
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
std::vector<unsigned char> read(const std::filesystem::path& file) {
    std::ifstream input(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
void write(const std::filesystem::path& file, const std::string& contents) {
    std::filesystem::create_directories(file.parent_path());
    std::ofstream out(file, std::ios::binary); out.write(contents.data(), contents.size());
    check(static_cast<bool>(out), "Write test fixture");
}
template<class F> void rejects(F function, const char* message) {
    bool failed = false;
    try { function(); } catch (const std::exception&) { failed = true; }
    check(failed, message);
}
struct Temporary {
    std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("DungeonDescent-bundle-test-" + std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    ~Temporary() { std::error_code ignored; std::filesystem::remove_all(root, ignored); }
};
}
int main() {
    try {
        Temporary temp;
        const auto source = temp.root / "source", notices = temp.root / "notices", archive = temp.root / "game.pak";
        const auto unicode = std::filesystem::u8path("r\xc3\xa9sum\xc3\xa9/\xe9\x9b\xaa.txt");
        write(source / "binary.dat", std::string("a\0b\xff\n", 5));
        write(source / "empty.txt", "");
        write(source / "nested/story.txt", "Aethoria\r\nFalkreath\n");
        write(source / unicode, "Unicode path");
        write(notices / "notice.txt", "Dependency notices");
        createArchive(source, notices, archive);
        auto bytes = read(archive);
        AssetBundle bundle({bytes.data(), bytes.size()});
        check(bundle.entries().size() == 5, "Package files, notices and empty files");
        const auto cache = bundle.materialize(temp.root / "cache");
        check(read(cache / "binary.dat") == read(source / "binary.dat"), "Binary data survives extraction");
        check(read(cache / "nested/story.txt") == read(source / "nested/story.txt"), "Story bytes and line endings survive extraction");
        check(read(cache / unicode) == read(source / unicode), "Unicode paths survive extraction");
        check(read(cache / "licenses/notice.txt") == read(notices / "notice.txt"), "Dependency notices are embedded");
        const auto old = std::filesystem::file_time_type::clock::now() - std::chrono::hours(1);
        std::filesystem::last_write_time(cache / "binary.dat", old);
        const auto written = std::filesystem::last_write_time(cache / "binary.dat");
        check(bundle.materialize(temp.root / "cache") == cache && std::filesystem::last_write_time(cache / "binary.dat") == written,
              "Valid cached files are reused without rewriting");
        write(cache / "binary.dat", std::string("bad!!", 5));
        bundle.materialize(temp.root / "cache");
        check(read(cache / "binary.dat") == read(source / "binary.dat"), "Same-size corrupt cached files are repaired");
        std::filesystem::remove(cache / "nested/story.txt");
        bundle.materialize(temp.root / "cache");
        check(read(cache / "nested/story.txt") == read(source / "nested/story.txt"), "Missing cached files are repaired");
        createArchive(source, notices, temp.root / "again.pak");
        check(read(temp.root / "again.pak") == bytes, "Asset archives are deterministic");
        write(source / "new.txt", "New version");
        createArchive(source, notices, temp.root / "new.pak");
        const auto newBytes = read(temp.root / "new.pak");
        AssetBundle updated({newBytes.data(), newBytes.size()});
        check(updated.id() != bundle.id() && updated.materialize(temp.root / "cache") != cache, "Asset changes get a separate cache identity");
        check(std::filesystem::exists(cache / ".complete"), "A new version does not delete an earlier cache");
        auto first = std::async(std::launch::async, [&] { return bundle.materialize(temp.root / "parallel"); });
        auto second = std::async(std::launch::async, [&] { return bundle.materialize(temp.root / "parallel"); });
        check(first.get() == second.get(), "Simultaneous launches safely share the asset cache");
        rejects([&] { AssetBundle bad({bytes.data(), 12}); }, "Reject truncated archives");
        auto broken = bytes; broken[0] ^= 1;
        rejects([&] { AssetBundle bad({broken.data(), broken.size()}); }, "Reject invalid archive magic");
        broken = bytes; broken[12] ^= 1;
        rejects([&] { AssetBundle bad({broken.data(), broken.size()}); }, "Reject damaged archive metadata");
        broken = bytes;
        const std::string escape = "../outside";
        check(bundle.entries()[0].name.size() == escape.size(), "Traversal fixture length");
        std::copy(escape.begin(), escape.end(), broken.begin() + 48);
        rejects([&] { AssetBundle bad({broken.data(), broken.size()}); }, "Reject archive path traversal before writing");
        broken = bytes; broken[bundle.entries()[0].offset] ^= 1;
        AssetBundle corrupt({broken.data(), broken.size()});
        rejects([&] { corrupt.materialize(temp.root / "corrupt"); }, "Reject damaged embedded file contents");
        check(!std::filesystem::exists(temp.root / "corrupt" / bundle.id() / ".complete"), "Failed extraction leaves no complete cache");
        for (const auto& entry : std::filesystem::directory_iterator(temp.root / "corrupt"))
            check(entry.path().filename().string().find(".preparing-") != 0, "Failed extraction cleans its temporary files");
        write(source / "licenses/notice.txt", "Duplicate destination");
        rejects([&] { createArchive(source, notices, temp.root / "duplicate.pak"); }, "Reject colliding archive paths");
#ifndef _WIN32
        std::filesystem::remove_all(cache);
        std::filesystem::create_directory_symlink(source, cache);
        bundle.materialize(temp.root / "cache");
        check(!std::filesystem::is_symlink(std::filesystem::symlink_status(cache)) && read(source / "binary.dat").size() == 5,
              "A cache symlink is replaced without modifying its target");
#endif
        std::cout << "Embedded asset round trips, Unicode, reuse, repair, concurrent launches, corruption and path checks passed.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
