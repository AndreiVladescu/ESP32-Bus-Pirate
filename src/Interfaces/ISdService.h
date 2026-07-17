#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#if __has_include("Vendors/SD.h")
#include "Vendors/SD.h"
#else
#include <SD.h>
#endif

class ISdService {
public:
    virtual ~ISdService() = default;

    virtual bool configure(uint8_t clkPin, uint8_t misoPin, uint8_t mosiPin, uint8_t csPin) = 0;
    virtual void end() = 0;
    virtual bool isFile(const std::string& filePath) = 0;
    virtual bool isDirectory(const std::string& path) = 0;
    virtual bool getSdState() = 0;
    virtual std::vector<std::string> listElements(const std::string& dirPath, size_t limit = 0) = 0;
    virtual std::vector<uint8_t> readBinaryFile(const std::string& filePath) = 0;
    virtual std::string readFile(const std::string& filePath) = 0;
    virtual std::string readFileChunk(const std::string& filePath, size_t offset, size_t maxBytes) = 0;
    virtual bool writeFile(const std::string& filePath, const std::string& data, bool append = false) = 0;
    virtual bool writeBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data) = 0;
    virtual bool appendToFile(const std::string& filePath, const std::string& data) = 0;
    virtual bool deleteFile(const std::string& filePath) = 0;
    virtual bool ensureDirectory(const std::string& directory) = 0;
    virtual bool deleteDirectory(const std::string& dirPath) = 0;
    virtual std::string getFileExt(const std::string& path) = 0;
    virtual std::string getParentDirectory(const std::string& path) = 0;
    virtual std::string getFileName(const std::string& path) = 0;
    virtual std::vector<std::string> listElementsCached(const std::string& path) = 0;
    virtual void setCachedDirectoryElements(const std::string& path, const std::vector<std::string>& elements) = 0;
    virtual void removeCachedPath(const std::string& path) = 0;
    virtual File openFileRead(const std::string& path) = 0;
    virtual File openFileWrite(const std::string& path) = 0;
};
