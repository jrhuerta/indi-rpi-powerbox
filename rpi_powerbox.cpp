#include "config.h"
#include "rpi_powerbox.h"

// We declare an auto pointer to MyCustomDriver.
static std::unique_ptr<RPiPowerox> mydriver(new RPiPowerox());

RPiPowerox::RPiPowerox()
{
    setVersion(CDRIVER_VERSION_MAJOR, CDRIVER_VERSION_MINOR);
}

const char *RPiPowerox::getDefaultName()
{
    return "RPi Powerbox";
}