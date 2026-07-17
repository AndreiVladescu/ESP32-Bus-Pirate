#pragma once

#include <cstddef>

namespace fs {

class File {
public:
    File() = default;
    explicit File(bool open, size_t size = 0) : opened(open), fileSize(size) {}

    explicit operator bool() const { return opened; }
    size_t size() const { return fileSize; }
    void close() { opened = false; }

private:
    bool opened = false;
    size_t fileSize = 0;
};

}  // namespace fs

using fs::File;
