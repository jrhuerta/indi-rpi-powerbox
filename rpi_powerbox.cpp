#include "config.h"
#include "rpi_powerbox.h"

// We declare an auto pointer to MyCustomDriver.
static std::unique_ptr<RPiPowerBox> mydriver(new RPiPowerBox());

RPiPowerBox::RPiPowerBox()
{
    setVersion(CDRIVER_VERSION_MAJOR, CDRIVER_VERSION_MINOR);
    chip = nullptr;
    mainPower = auxPower = heater0 = heater1 = nullptr;
}

RPiPowerBox::~RPiPowerBox()
{
    Disconnect();
}

const char *RPiPowerBox::getDefaultName()
{
    return "RPi Powerbox";
}

bool RPiPowerBox::Connect()
{
    LOG_INFO("Connecting to PowerBox...");

    // Open GPIO chip (assuming GPIOs are on gpiochip0)
    chip = gpiod_chip_open_by_name("gpiochip0");
    if (!chip)
    {
        LOG_ERROR("Failed to open GPIO chip.");
        return false;
    }

    // Request GPIO lines
    mainPower = gpiod_chip_get_line(chip, static_cast<int>(GPIOsNP[GPIO_PWR].value));
    auxPower = gpiod_chip_get_line(chip, static_cast<int>(GPIOsNP[GPIO_AUX].value));
    heater0 = gpiod_chip_get_line(chip, static_cast<int>(GPIOsNP[GPIO_HEATER0].value));
    heater1 = gpiod_chip_get_line(chip, static_cast<int>(GPIOsNP[GPIO_HEATER1].value));

    if (!mainPower || !auxPower || !heater0 || !heater1)
    {
        LOG_ERROR("Failed to request one or more GPIO lines.");
        gpiod_chip_close(chip);
        return false;
    }

    // Set GPIOs as outputs
    if (gpiod_line_request_output(mainPower, "indi_powerbox", 1) < 0 ||
        gpiod_line_request_output(auxPower, "indi_powerbox", 1) < 0 ||
        gpiod_line_request_output(heater0, "indi_powerbox", 0) < 0 ||
        gpiod_line_request_output(heater1, "indi_powerbox", 0) < 0)
    {
        LOG_ERROR("Failed to set GPIO directions.");
        gpiod_chip_close(chip);
        return false;
    }

    // LOG_INFO("GPIO successfully initialized.");
    return true;
}

bool RPiPowerBox::Disconnect()
{
    LOG_INFO("Disconnecting PowerBox...");

    if (mainPower) gpiod_line_set_value(mainPower, 0);
    if (auxPower) gpiod_line_set_value(auxPower, 0);
    if (heater0) gpiod_line_set_value(heater0, 0);
    if (heater1) gpiod_line_set_value(heater1, 0);

    if (chip)
    {
        gpiod_chip_close(chip);
        chip = nullptr;
    }

    mainPower = auxPower = heater0 = heater1 = nullptr;
    LOG_INFO("GPIO released and device disconnected.");
    return true;
}

bool RPiPowerBox::initProperties()
{
    INDI::DefaultDevice::initProperties();

    definePowerSwitch();
    defineAuxSwitch();
    definePwmNumber();
    defineGPIOs();

    return true;
}

bool RPiPowerBox::updateProperties()
{
    INDI::DefaultDevice::updateProperties();
    if (isConnected())
    {
        defineProperty(PowerSP);
        defineProperty(AuxSP);
        defineProperty(PwmNP);
        deleteProperty(GPIOsNP);
    }
    else
    {
        deleteProperty(PowerSP);
        deleteProperty(AuxSP);
        deleteProperty(PwmNP);
        defineProperty(GPIOsNP);
    }
    return true;
}

void RPiPowerBox::defineGPIOs()
{
    GPIOsNP[GPIO_PWR].fill(
        "GPIO_PWR",
        "Main Power",
        "%0.f",
        1,
        26,
        1,
        RPI_PB_GPIO_POWER);

    GPIOsNP[GPIO_AUX].fill(
        "GPIO_AUX",
        "Auxiliary Power",
        "%0.f",
        1,
        26,
        1,
        RPI_PB_GPIO_AUX);
    GPIOsNP[GPIO_HEATER0].fill(
        "GPIO_HEATER0",
        "Heater 0",
        "%0.f",
        1,
        26,
        1,
        RP_PB_GPIO_HEATER0);
    GPIOsNP[GPIO_HEATER1].fill(
        "GPIO_HEATER1",
        "Heater 1",
        "%0.f",
        1,
        26,
        1,
        RP_PB_GPIO_HEATER1);
    GPIOsNP.fill(
        getDeviceName(),
        "GPIO_PINS",
        "GPIOs Pin Numbers",
        OPTIONS_TAB,
        IP_RW,
        0,
        IPS_IDLE);

    defineProperty(AuxSP);
    // Define some properties here
}

void RPiPowerBox::handlePowerUpdate()
{
    switch (PowerSP.findOnSwitchIndex())
    {
    case PWR_ON:
        LOG_INFO("PWR_ON");
        PowerSP.setState(IPS_OK);
        break;
    case PWR_OFF:
        LOG_INFO("PWR_OFF");
        PowerSP.setState(IPS_IDLE);
        break;
    }
    PowerSP.apply();
}

void RPiPowerBox::definePowerSwitch()
{
    PowerSP[PWR_ON].fill(
        "PWR_ON",
        "On",
        ISS_ON);

    PowerSP[PWR_OFF].fill(
        "PWR_OFF",
        "Off",
        ISS_OFF);

    PowerSP.fill(
        getDeviceName(),
        "MAIN_POWER",
        "Main Power",
        MAIN_CONTROL_TAB,
        IP_RW,
        ISR_1OFMANY,
        60,
        IPS_OK);

    PowerSP.onUpdate([this]
                     { handlePowerUpdate(); });

    defineProperty(PowerSP);
}

void RPiPowerBox::handleAuxUpdate()
{
    switch (AuxSP.findOnSwitchIndex())
    {
    case AUX_ON:
        LOG_INFO("AUX_ON");
        AuxSP.setState(IPS_OK);
        break;
    case AUX_OFF:
        LOG_INFO("AUX_OFF");
        AuxSP.setState(IPS_IDLE);
        break;
    }
    AuxSP.apply();
}

void RPiPowerBox::defineAuxSwitch()
{
    AuxSP[AUX_ON].fill(
        "AUX_ON",
        "On",
        ISS_ON);

    AuxSP[AUX_OFF].fill(
        "AUX_OFF",
        "Off",
        ISS_OFF);

    AuxSP.fill(
        getDeviceName(),
        "AUX_POWER",
        "Auxiliary Power",
        MAIN_CONTROL_TAB,
        IP_RW,
        ISR_1OFMANY,
        60,
        IPS_OK);

    AuxSP.onUpdate([this]
                   { handleAuxUpdate(); });

    defineProperty(AuxSP);
}

void RPiPowerBox::handlePwmUpdate()
{
    // for (int i = 0; i < PWM_N; i++)
    // {
    //     LOG_INFO(PwmNP[i].getValue());
    // }
    // PwmNP.apply();
}

void RPiPowerBox::definePwmNumber()
{
    PwmNP[PWM_0].fill(
        "PWM_0",
        "PWM 0",
        "%0.f",
        0,
        100,
        5,
        0);

    PwmNP[PWM_1].fill(
        "PWM_1",
        "PWM 1",
        "%0.f",
        0,
        100,
        5,
        0);

    PwmNP.fill(
        getDeviceName(),
        "PWM",
        "PWM",
        MAIN_CONTROL_TAB,
        IP_RW,
        0,
        IPS_IDLE);

    PwmNP.onUpdate([this]
                   { handlePwmUpdate(); });

    defineProperty(PwmNP);
}
