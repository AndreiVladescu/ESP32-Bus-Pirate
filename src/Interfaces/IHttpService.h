#pragma once

#include <string>

class IHttpService {
public:
    virtual ~IHttpService() = default;

    virtual void startGetTask(const std::string& url, int timeout_ms, int bodyMaxBytes, bool insecure,
                              int stack_bytes = 20000, int core = 1, bool onlyContent = false) = 0;
    virtual std::string fetchJson(const std::string& url, int bodyMaxBytes) = 0;
    virtual bool isResponseReady() const noexcept = 0;
    virtual std::string lastResponse() = 0;
    virtual void reset() = 0;
};

