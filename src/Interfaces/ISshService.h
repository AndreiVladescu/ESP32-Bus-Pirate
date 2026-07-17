#pragma once

#include <string>

class ISshService {
public:
    virtual ~ISshService() = default;

    virtual void startTask(const std::string& host, const std::string& user, const std::string& pass, int verbosity, int port) = 0;
    virtual bool isConnected() const = 0;
    virtual void writeChar(char c) = 0;
    virtual std::string readOutput() = 0;
    virtual std::string readOutputNonBlocking() = 0;
    virtual void close() = 0;
};

