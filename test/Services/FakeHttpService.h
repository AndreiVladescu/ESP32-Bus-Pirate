#pragma once

#include <string>
#include <vector>

#include "Interfaces/IHttpService.h"

class FakeHttpService final : public IHttpService {
public:
    struct GetCall {
        std::string url;
        int timeoutMs = 0;
        int bodyMaxBytes = 0;
        bool insecure = false;
        int stackBytes = 0;
        int core = 0;
        bool onlyContent = false;
    };

    struct FetchCall {
        std::string url;
        int bodyMaxBytes = 0;
    };

    bool responseReady = true;
    std::string response = "HTTP/1.1 200 OK\r\n\r\nbody";
    std::string fetchResponse = "{\"ok\":true}";
    std::vector<GetCall> getCalls;
    std::vector<FetchCall> fetchCalls;
    uint32_t resetCalls = 0;

    void startGetTask(const std::string& url, int timeout_ms, int bodyMaxBytes, bool insecure,
                      int stack_bytes = 20000, int core = 1, bool onlyContent = false) override {
        getCalls.push_back({url, timeout_ms, bodyMaxBytes, insecure, stack_bytes, core, onlyContent});
    }

    std::string fetchJson(const std::string& url, int bodyMaxBytes) override {
        fetchCalls.push_back({url, bodyMaxBytes});
        return fetchResponse;
    }

    bool isResponseReady() const noexcept override { return responseReady; }
    std::string lastResponse() override { return response; }
    void reset() override { ++resetCalls; }
};

