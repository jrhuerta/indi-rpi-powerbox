#include "config.h"
#include "rpi_powerbox.h"
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================================
// Static Instance of the Driver
// ============================================================================
// We declare a unique_ptr to the RPiPowerBox driver to ensure automatic cleanup.
static std::unique_ptr<RPiPowerBox> mydriver(new RPiPowerBox());
// Alternatively, with C++14 or later:
// static auto mydriver = std::make_unique<RPiPowerBox>();

// ============================================================================
// Constructors and Destructor
// ============================================================================

RPiPowerBox::RPiPowerBox()
{
    setVersion(CDRIVER_VERSION_MAJOR, CDRIVER_VERSION_MINOR);
}

RPiPowerBox::~RPiPowerBox()
{
    // Ensure disconnection and cleanup when the driver is destroyed.
    Disconnect();
}

// ============================================================================
// INDI Device Interface Overrides
// ============================================================================

const char *RPiPowerBox::getDefaultName()
{
    return "RPi Powerbox";
}

bool RPiPowerBox::Connect()
{
    LOG_INFO("Connecting PowerBox...");

    // Initialize GPIO pins and check for errors.
    bool rv = initGPIO();
    if (!rv)
    {
        return false;
    }

    // Detect connected temperature sensors.
    detectSensors();

    // Proceed with the default connection process.
    return DefaultDevice::Connect();
}

bool RPiPowerBox::Disconnect()
{
    LOG_INFO("Disconnecting PowerBox...");
    LOG_INFO("Releasing GPIO...");

    // Stop the pigpio daemon connection.
    pigpio_stop(piId);

    LOG_INFO("Releasing temperature sensors...");
    sensors.clear();

    return DefaultDevice::Disconnect();
}

bool RPiPowerBox::initProperties()
{
    // Initialize base properties.
    INDI::DefaultDevice::initProperties();

    // Define device-specific properties.
    definePowerSwitch();
    defineAuxSwitch();
    defineHeater0DutyCycle();
    defineHeater1DutyCycle();
    defineTemperatureProbes();

    addAuxControls();

    // Create and register the custom GPIO connection using a smart pointer.
    connection = std::make_unique<GPIOConnection>(this);
    registerConnection(connection.get());

    return true;
}

bool RPiPowerBox::updateProperties()
{
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        // Define properties when connected.
        defineProperty(PowerSP);
        defineProperty(AuxSP);
        defineProperty(Heater0NP);
        defineProperty(Heater1NP);

        defineTemperatureProbes();
        defineProperty(TempNP);
    }
    else
    {
        // Delete properties when not connected.
        deleteProperty(PowerSP);
        deleteProperty(AuxSP);
        deleteProperty(Heater0NP);
        deleteProperty(Heater1NP);
        deleteProperty(TempNP);
    }

    return true;
}

void RPiPowerBox::TimerHit()
{
    LOG_DEBUG("Timer hit.");

    if (!isConnected())
    {
        return;
    }

    // Update sensor temperature readings.
    updateTemperatureReadings();

    // Reset the timer (POLLMS defined elsewhere).
    SetTimer(POLLMS);
}

// ============================================================================
// Property Update Handlers and Definitions
// ============================================================================

void RPiPowerBox::handlePowerUpdate()
{
    // Update the main power switch based on the selected switch.
    switch (PowerSP.findOnSwitchIndex())
    {
    case PWR_ON:
        LOG_INFO("PWR_ON");
        gpio_write(piId, RPI_PB_GPIO_POWER, PI_HIGH);
        PowerSP.setState(IPS_OK);
        break;
    case PWR_OFF:
        LOG_INFO("PWR_OFF");
        gpio_write(piId, RPI_PB_GPIO_POWER, PI_LOW);
        PowerSP.setState(IPS_IDLE);
        break;
    }
    PowerSP.apply();
}

void RPiPowerBox::definePowerSwitch()
{
    // Configure individual power switch options.
    PowerSP[PWR_ON].fill("PWR_ON", "On", ISS_ON);
    PowerSP[PWR_OFF].fill("PWR_OFF", "Off", ISS_OFF);

    // Configure the overall power switch property.
    PowerSP.fill(getDeviceName(),
                 "MAIN_POWER",
                 "Main Power",
                 MAIN_CONTROL_TAB,
                 IP_RW,
                 ISR_1OFMANY,
                 60,
                 IPS_OK);

    // Register the update callback.
    PowerSP.onUpdate([this]
                     { handlePowerUpdate(); });
}

void RPiPowerBox::handleAuxUpdate()
{
    // Update the auxiliary switch based on the selected switch.
    switch (AuxSP.findOnSwitchIndex())
    {
    case AUX_ON:
        LOG_INFO("AUX_ON");
        gpio_write(piId, RPI_PB_GPIO_AUX, PI_HIGH);
        AuxSP.setState(IPS_OK);
        break;
    case AUX_OFF:
        LOG_INFO("AUX_OFF");
        gpio_write(piId, RPI_PB_GPIO_AUX, PI_LOW);
        AuxSP.setState(IPS_IDLE);
        break;
    }
    AuxSP.apply();
}

void RPiPowerBox::defineAuxSwitch()
{
    // Configure individual auxiliary switch options.
    AuxSP[AUX_ON].fill("AUX_ON", "On", ISS_ON);
    AuxSP[AUX_OFF].fill("AUX_OFF", "Off", ISS_OFF);

    // Configure the overall auxiliary property.
    AuxSP.fill(getDeviceName(),
               "AUX_POWER",
               "Auxiliary Power",
               MAIN_CONTROL_TAB,
               IP_RW,
               ISR_1OFMANY,
               60,
               IPS_OK);

    // Register the update callback.
    AuxSP.onUpdate([this]
                   { handleAuxUpdate(); });
}

void RPiPowerBox::handleHeaterUpdate(INDI::PropertyNumber &heaterProp, int gpioPin, const std::string &heaterName)
{
    // Retrieve the heater value and update the corresponding PWM duty cycle.
    int heaterValue = static_cast<int>(heaterProp[0].getValue());
    set_PWM_dutycycle(piId, gpioPin, heaterValue * 2.55);
    LOGF_INFO("Setting %s to %d%%", heaterName.c_str(), heaterValue);

    // Update the property state based on the heater value.
    heaterProp.setState(heaterValue == 0 ? IPS_IDLE : IPS_OK);
    heaterProp.apply();
}

void RPiPowerBox::defineHeater0DutyCycle()
{
    // Configure Heater 0's numeric property.
    Heater0NP[0].fill("HEATER_0",
                      "Heater 0",
                      "%0.f",
                      0,
                      100,
                      5,
                      0);

    Heater0NP.fill(getDeviceName(),
                   "HEATER_0",
                   "Heater 0",
                   MAIN_CONTROL_TAB,
                   IP_RW,
                   0,
                   IPS_IDLE);

    // Register update callback with the common heater update handler.
    Heater0NP.onUpdate([this]
                       { handleHeaterUpdate(Heater0NP, RP_PB_GPIO_HEATER0, "HEATER_0"); });
}

void RPiPowerBox::defineHeater1DutyCycle()
{
    // Configure Heater 1's numeric property.
    Heater1NP[0].fill("HEATER_1",
                      "Heater 1",
                      "%0.f",
                      0,
                      100,
                      5,
                      0);

    Heater1NP.fill(getDeviceName(),
                   "HEATER_1",
                   "Heater 1",
                   MAIN_CONTROL_TAB,
                   IP_RW,
                   0,
                   IPS_IDLE);

    // Register update callback with the common heater update handler.
    Heater1NP.onUpdate([this]
                       { handleHeaterUpdate(Heater1NP, RP_PB_GPIO_HEATER1, "HEATER_1"); });
}

void RPiPowerBox::defineTemperatureProbes()
{
    // Resize the temperature property array to match the number of sensors.
    TempNP.resize(sensors.size());

    // Define each temperature probe property.
    for (size_t i = 0; i < sensors.size(); ++i)
    {
        std::string label = "TEMP_" + std::to_string(i);
        TempNP[i].fill(label.c_str(),
                       sensors[i].id.c_str(),
                       "%0.1f",
                       -50,
                       50,
                       0.5,
                       0);
    }

    // Configure the overall temperature property.
    TempNP.fill(getDeviceName(),
                "TEMP",
                "Temp Sensors",
                MAIN_CONTROL_TAB,
                IP_RO,
                sensors.size(),
                IPS_IDLE);
}

// ============================================================================
// Hardware Initialization and Sensor Handling
// ============================================================================

bool RPiPowerBox::initGPIO()
{
    // Connect to the pigpio daemon.
    piId = pigpio_start(NULL, NULL);
    if (piId < 0)
    {
        LOG_ERROR("Failed to connect to pigpio daemon.");
        return false;
    }
    LOGF_INFO("pigpio version: %d", get_pigpio_version(piId));
    LOGF_INFO("hardware revision:  %d", get_hardware_revision(piId));

    // Configure the main power GPIO pin.
    if (get_mode(piId, RPI_PB_GPIO_POWER) != PI_OUTPUT)
    {
        set_mode(piId, RPI_PB_GPIO_POWER, PI_OUTPUT);
        gpio_write(piId, RPI_PB_GPIO_POWER, PI_HIGH);
    }

    // Configure the auxiliary GPIO pin.
    if (get_mode(piId, RPI_PB_GPIO_AUX) != PI_OUTPUT)
    {
        set_mode(piId, RPI_PB_GPIO_AUX, PI_OUTPUT);
        gpio_write(piId, RPI_PB_GPIO_AUX, PI_HIGH);
    }

    // Configure the GPIO pin for Heater 0.
    if (get_mode(piId, RP_PB_GPIO_HEATER0) != PI_OUTPUT)
    {
        set_mode(piId, RP_PB_GPIO_HEATER0, PI_OUTPUT);
        set_PWM_frequency(piId, RP_PB_GPIO_HEATER0, RP_PB_PWM_FREQ);
        set_PWM_dutycycle(piId, RP_PB_GPIO_HEATER0, 0);
    }

    // Configure the GPIO pin for Heater 1.
    if (get_mode(piId, RP_PB_GPIO_HEATER1) != PI_OUTPUT)
    {
        set_mode(piId, RP_PB_GPIO_HEATER1, PI_OUTPUT);
        set_PWM_frequency(piId, RP_PB_GPIO_HEATER1, RP_PB_PWM_FREQ);
        set_PWM_dutycycle(piId, RP_PB_GPIO_HEATER1, 0);
    }

    LOG_INFO("GPIO successfully initialized.");
    return true;
}

void RPiPowerBox::detectSensors()
{
    std::error_code ec;
    std::vector<std::filesystem::directory_entry> entries;

    // Collect all entries from the W1 devices directory.
    for (const auto &entry : fs::directory_iterator(W1_DEVICES_PATH, ec))
    {
        if (ec)
        {
            LOGF_ERROR("Error reading directory: %s", ec.message().c_str());
            return;
        }
        if (entry.is_directory())
        {
            entries.push_back(entry);
        }
    }

    // Sort entries by filename to ensure a consistent order.
    std::sort(entries.begin(), entries.end(),
              [](const auto &a, const auto &b)
              {
                  return a.path().filename().string() < b.path().filename().string();
              });

    // Clear any existing sensors.
    sensors.clear();
    int count = 0;

    // Populate sensors that match the expected SENSOR_PREFIX.
    for (const auto &entry : entries)
    {
        std::string entryName = entry.path().filename().string();
        if (entryName.rfind(SENSOR_PREFIX, 0) == 0)
        {
            LOGF_INFO("Found sensor: %s", entryName.c_str());
            Sensor sensor;
            sensor.id = entryName;
            sensor.path = (entry.path() / "w1_slave").string();
            sensors.push_back(sensor);
            count++;
        }
    }
}

void RPiPowerBox::updateTemperatureReadings()
{
    // Read and update temperature for each detected sensor.
    for (size_t i = 0; i < sensors.size(); ++i)
    {
        std::ifstream sensorFile(sensors[i].path);
        if (!sensorFile.is_open())
        {
            LOGF_ERROR("Failed to open sensor file: %s", sensors[i].path.c_str());
            return;
        }

        std::string line;
        std::getline(sensorFile, line);
        // Check for valid sensor data.
        if (line.find("YES") == std::string::npos)
        {
            LOGF_ERROR("CRC check failed for sensor: %s", sensors[i].id.c_str());
            return;
        }

        std::getline(sensorFile, line);
        size_t pos = line.find("t=");
        if (pos == std::string::npos)
        {
            LOGF_ERROR("Failed to read temperature for sensor: %s", sensors[i].id.c_str());
            return;
        }

        // Convert the sensor reading to a temperature in degrees Celsius.
        float temp = std::stof(line.substr(pos + 2)) / 1000;
        TempNP[i].setValue(temp);
    }
    TempNP.apply();
}
