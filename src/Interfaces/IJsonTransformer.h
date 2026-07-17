#pragma once

#include <string>
#include <vector>

class IJsonTransformer {
public:
    virtual ~IJsonTransformer() = default;

    virtual std::vector<std::string> toLines(const std::string& json) = 0;
    virtual std::string dechunk(const std::string& chunked) = 0;
};

