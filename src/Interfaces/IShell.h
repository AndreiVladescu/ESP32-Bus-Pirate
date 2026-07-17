#pragma once

class IShell {
public:
    virtual ~IShell() = default;
    virtual void run() = 0;
};
