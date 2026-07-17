#ifndef SD_SERVICE_H
#define SD_SERVICE_H

#include <SD.h>
#include <SPI.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "Interfaces/ISdService.h"

class SdService : public ISdService {
private:
    bool sdCardMounted = false;
    std::unordered_map<std::string, std::vector<std::string>> cachedDirectoryElements;
public:
    SdService();

    bool configure(uint8_t clkPin, uint8_t misoPin, uint8_t mosiPin, uint8_t csPin);
    void end();
    bool isFile(const std::string& filePath);
    bool isDirectory(const std::string& path);
    bool getSdState();

    std::vector<std::string> listElements(const std::string& dirPath, size_t limit = 0);
    std::vector<uint8_t> readBinaryFile(const std::string& filePath);
    std::string readFile(const std::string& filePath);
    std::string readFileChunk(const std::string& filePath, size_t offset, size_t maxBytes);

    bool writeFile(const std::string& filePath, const std::string& data, bool append = false);
    bool writeBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data);
    bool appendToFile(const std::string& filePath, const std::string& data);
    bool deleteFile(const std::string& filePath);
    bool ensureDirectory(const std::string& directory);
    bool deleteDirectory(const std::string& dirPath);
    std::string getFileExt(const std::string& path);
    std::string getParentDirectory(const std::string& path);
    std::string getFileName(const std::string& path);
    std::vector<std::string> listElementsCached(const std::string& path);
    void setCachedDirectoryElements(const std::string& path, const std::vector<std::string>& elements);
    void removeCachedPath(const std::string& path);

    File openFileRead(const std::string& path);
    File openFileWrite(const std::string& path);

};

#endif // SD_SERVICE_H
