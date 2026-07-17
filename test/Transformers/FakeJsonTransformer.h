#pragma once

#include <string>
#include <vector>

#include "Interfaces/IJsonTransformer.h"

class FakeJsonTransformer final : public IJsonTransformer {
public:
    std::vector<std::string> lines = {"json"};
    std::vector<std::string> toLinesInputs;
    std::vector<std::string> dechunkInputs;

    std::vector<std::string> toLines(const std::string& json) override {
        toLinesInputs.push_back(json);
        return lines;
    }

    std::string dechunk(const std::string& chunked) override {
        dechunkInputs.push_back(chunked);
        return chunked;
    }
};

