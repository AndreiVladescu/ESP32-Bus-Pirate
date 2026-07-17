#pragma once

#include <map>
#include <string>
#include <vector>

#include "Interfaces/ISdService.h"

class FakeSdService final : public ISdService {
public:
    bool configureResult = true;
    bool sdState = true;
    uint32_t configureCalls = 0;
    uint32_t endCalls = 0;
    std::string lastReadPath;
    std::string lastWritePath;
    std::vector<std::string> deletedFiles;
    std::map<std::string, std::vector<uint8_t>> binaryFiles;
    std::map<std::string, std::string> textFiles;

    bool configure(uint8_t, uint8_t, uint8_t, uint8_t) override {
        ++configureCalls;
        return configureResult;
    }

    void end() override { ++endCalls; }
    bool isFile(const std::string& filePath) override { return textFiles.count(filePath) || binaryFiles.count(filePath); }
    bool isDirectory(const std::string&) override { return false; }
    bool getSdState() override { return sdState; }
    std::vector<std::string> listElements(const std::string&, size_t = 0) override { return {}; }

    std::vector<uint8_t> readBinaryFile(const std::string& filePath) override {
        lastReadPath = filePath;
        auto it = binaryFiles.find(filePath);
        return it == binaryFiles.end() ? std::vector<uint8_t>{} : it->second;
    }

    std::string readFile(const std::string& filePath) override {
        lastReadPath = filePath;
        auto it = textFiles.find(filePath);
        return it == textFiles.end() ? "" : it->second;
    }

    std::string readFileChunk(const std::string& filePath, size_t offset, size_t maxBytes) override {
        const auto data = readFile(filePath);
        if (offset >= data.size()) return "";
        return data.substr(offset, maxBytes);
    }

    bool writeFile(const std::string& filePath, const std::string& data, bool append = false) override {
        lastWritePath = filePath;
        if (append) textFiles[filePath] += data;
        else textFiles[filePath] = data;
        return true;
    }

    bool writeBinaryFile(const std::string& filePath, const std::vector<uint8_t>& data) override {
        lastWritePath = filePath;
        binaryFiles[filePath] = data;
        return true;
    }

    bool appendToFile(const std::string& filePath, const std::string& data) override {
        return writeFile(filePath, data, true);
    }

    bool deleteFile(const std::string& filePath) override {
        deletedFiles.push_back(filePath);
        textFiles.erase(filePath);
        binaryFiles.erase(filePath);
        return true;
    }

    bool ensureDirectory(const std::string&) override { return true; }
    bool deleteDirectory(const std::string&) override { return true; }
    std::string getFileExt(const std::string& path) override {
        const auto pos = path.find_last_of('.');
        return pos == std::string::npos ? "" : path.substr(pos);
    }
    std::string getParentDirectory(const std::string& path) override {
        const auto pos = path.find_last_of('/');
        return pos == std::string::npos ? "" : path.substr(0, pos);
    }
    std::string getFileName(const std::string& path) override {
        const auto pos = path.find_last_of('/');
        return pos == std::string::npos ? path : path.substr(pos + 1);
    }
    std::vector<std::string> listElementsCached(const std::string&) override { return {}; }
    void setCachedDirectoryElements(const std::string&, const std::vector<std::string>&) override {}
    void removeCachedPath(const std::string&) override {}

    File openFileRead(const std::string& path) override {
        lastReadPath = path;
        const auto text = textFiles.find(path);
        if (text != textFiles.end()) return File(true, text->second.size());
        const auto binary = binaryFiles.find(path);
        if (binary != binaryFiles.end()) return File(true, binary->second.size());
        return File();
    }

    File openFileWrite(const std::string& path) override {
        lastWritePath = path;
        textFiles[path] = "";
        return File(true, 0);
    }
};
