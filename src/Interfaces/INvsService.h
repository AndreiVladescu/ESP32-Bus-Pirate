#pragma once

#include <string>

class INvsService {
public:
    virtual ~INvsService() = default;

    virtual void open() = 0;
    virtual void close() = 0;
    virtual void saveString(const std::string& key, const std::string& value) = 0;
    virtual std::string getString(const std::string& key, const std::string& defaultValue = "") = 0;
};

