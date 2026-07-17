#pragma once

#include <string>
#include <libssh/libssh.h>
#include "Interfaces/ISshService.h"

class SshService : public ISshService {
public:
    void startTask(const std::string& host, const std::string& user, const std::string& pass, int verbosity, int port) override;
    
    bool isConnected() const override;
    void writeChar(char c) override;
    std::string readOutput() override;
    std::string readOutputNonBlocking() override;
    void close() override;

private:
    // SSH Task, cause overflow if it runs in the main loop, so it must run in a dedicated FreeRTOS task with a larger stack
    static void connectTask(void* pvParams);

    bool connect(const std::string& host, const std::string& user, const std::string& pass, int verbosity, int port=22);
    bool authenticate(const std::string& password);
    bool openChannel();
    bool requestPty();
    bool startShell();

    ssh_session session = nullptr;
    ssh_channel channel = nullptr;
    bool connected = false;
};
