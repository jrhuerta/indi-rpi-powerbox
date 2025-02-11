#pragma once

#include "libindi/defaultdevice.h"

class RPiPowerox : public INDI::DefaultDevice
{
public:
    RPiPowerbox();
    virtual ~RPiPowerbox() = default;

    // You must override this method in your class.
    virtual const char *getDefaultName() override;
};