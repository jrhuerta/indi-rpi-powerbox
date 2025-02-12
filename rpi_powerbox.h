#pragma once

#include "libindi/defaultdevice.h"
#include <gpiod.h>

#define RPI_PB_MSG_LEN 256
#define RPI_PB_GPIO_CHIP "gpiochip0"
#define RPI_PB_GPIO_POWER 8
#define RPI_PB_GPIO_AUX 7
#define RP_PB_GPIO_HEATER0 12
#define RP_PB_GPIO_HEATER1 13

class RPiPowerBox : public INDI::DefaultDevice
{
public:
    RPiPowerBox();
    virtual ~RPiPowerBox();

    bool Connect() override;
    bool Disconnect() override;
    virtual const char *getDefaultName() override;
    virtual bool initProperties() override;
    virtual bool updateProperties() override;

private:
    void definePowerSwitch();
    void handlePowerUpdate();
    void defineAuxSwitch();
    void handleAuxUpdate();
    void definePwmNumber();
    void handlePwmUpdate();
    void defineGPIOs();

private:
    struct gpiod_chip *chip;
    struct gpiod_line *mainPower;
    struct gpiod_line *auxPower;
    struct gpiod_line *heater0;
    struct gpiod_line *heater1;

private:
    enum
    {
        GPIO_PWR,
        GPIO_AUX,
        GPIO_HEATER0,
        GPIO_HEATER1,
        GPIO_N
    };
    INDI::PropertyNumber GPIOsNP{GPIO_N};

    enum
    {
        PWR_ON,
        PWR_OFF,
        PWR_N
    };
    INDI::PropertySwitch PowerSP{PWR_N};

    enum
    {
        AUX_ON,
        AUX_OFF,
        AUX_N
    };
    INDI::PropertySwitch AuxSP{AUX_N};

    enum
    {
        PWM_0,
        PWM_1,
        PWM_N
    };
    INDI::PropertyNumber PwmNP{PWM_N};
};
