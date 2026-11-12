#include "AssetBundle.h"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
int pack(const std::vector<std::filesystem::path>& args) {
    try {
        if (args.size() != 4) throw std::runtime_error("Usage: dungeon_asset_packer ASSETS NOTICES OUTPUT");
        dungeon::portable::createArchive(args[1], args[2], args[3]);
        std::cout << "Embedded game assets: " << std::filesystem::file_size(args[3]) << " bytes\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
}
#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
    std::vector<std::filesystem::path> args;
    for (int i = 0; i < argc; ++i) args.emplace_back(argv[i]);
    return pack(args);
}
