#pragma once

class IUsbAdapterShell {
public:
    virtual ~IUsbAdapterShell() = default;
    virtual void run() = 0;
    virtual void rebootOpenOcdBusPirate() = 0;
};
