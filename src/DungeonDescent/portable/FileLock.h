#pragma once
#include <filesystem>
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
class FileLock {
public:
    explicit FileLock(const std::filesystem::path& file) {
#ifdef _WIN32
        handle_ = CreateFileW(file.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (handle_ == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot open the local game data lock.");
        BY_HANDLE_FILE_INFORMATION info{};
        if (!GetFileInformationByHandle(handle_, &info) || (info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) ||
            !LockFileEx(handle_, LOCKFILE_EXCLUSIVE_LOCK, 0, 1, 0, &overlapped_)) {
            CloseHandle(handle_); throw std::runtime_error("Cannot lock the local game data.");
        }
#else
        handle_ = open(file.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (handle_ < 0) throw std::runtime_error("Cannot open the local game data lock.");
        if (flock(handle_, LOCK_EX) != 0) { close(handle_); throw std::runtime_error("Cannot lock the local game data."); }
#endif
    }
    ~FileLock() {
#ifdef _WIN32
        UnlockFileEx(handle_, 0, 1, 0, &overlapped_); CloseHandle(handle_);
#else
        flock(handle_, LOCK_UN); close(handle_);
#endif
    }
    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;
private:
#ifdef _WIN32
    HANDLE handle_ = INVALID_HANDLE_VALUE;
    OVERLAPPED overlapped_{};
#else
    int handle_ = -1;
#endif
};
} // namespace dungeon::portable
