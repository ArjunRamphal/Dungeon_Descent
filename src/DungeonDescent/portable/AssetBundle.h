#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dungeon::portable {
struct Bytes { const unsigned char* data; std::size_t size; };
struct Entry {
    std::string name;
    std::uint64_t size, checksum;
    std::size_t offset;
};

// The view must outlive the bundle. Executable resources live for the process.
class AssetBundle {
public:
    explicit AssetBundle(Bytes bytes);
    const std::vector<Entry>& entries() const { return entries_; }
    const std::string& id() const { return id_; }
    std::filesystem::path materialize(const std::filesystem::path& cacheRoot) const;
private:
    bool validCache(const std::filesystem::path& folder) const;
    Bytes bytes_;
    std::vector<Entry> entries_;
    std::string id_;
};

void createArchive(const std::filesystem::path& assets, const std::filesystem::path& notices,
                   const std::filesystem::path& output);
Bytes embeddedAssets();
std::filesystem::path defaultCacheDirectory();
} // namespace dungeon::portable
