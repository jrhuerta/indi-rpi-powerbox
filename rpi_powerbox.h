#pragma once

// ============================================================================
// INCLUDES
// ============================================================================
#include "libindi/defaultdevice.h"
#include "gpioconnection.h"
#include <pigpiod_if2.h>

// ============================================================================
// MACROS & CONSTANTS
// ============================================================================
#define RPI_PB_GPIO_POWER 8
#define RPI_PB_GPIO_AUX 7
#define RP_PB_GPIO_HEATER0 12
#define RP_PB_GPIO_HEATER1 13
#define RP_PB_PWM_FREQ 8000

#define W1_DEVICES_PATH "/sys/bus/w1/devices"
#define SENSOR_PREFIX "28-"

// ============================================================================
// SENSOR STRUCTURE
// ============================================================================
// Structure representing a temperature sensor.
struct Sensor
{
    std::string id;
    std::string path;
};

// ============================================================================
// RPiPowerBox DEVICE CLASS
// ============================================================================

/**
 * @brief The RPiPowerBox class encapsulates the functionality to control
 * a Raspberry Pi Power Box via GPIO pins. It extends INDI::DefaultDevice
 * to integrate with the INDI framework.
 */
class RPiPowerBox : public INDI::DefaultDevice
{
public:
    // ------------------------------------------------------------------------
    // Constructors / Destructors
    // ------------------------------------------------------------------------
    RPiPowerBox();
    virtual ~RPiPowerBox();

    // ------------------------------------------------------------------------
    // INDI Device Overrides
    // ------------------------------------------------------------------------
    bool Connect() override;
    bool Disconnect() override;
    virtual const char *getDefaultName() override;
    virtual bool initProperties() override;
    virtual bool updateProperties() override;
    virtual void TimerHit() override;

private:
    // ------------------------------------------------------------------------
    // Connection Management
    // ------------------------------------------------------------------------
    // Smart pointer to manage the GPIO connection.
    std::unique_ptr<GPIOConnection> connection;

    // ------------------------------------------------------------------------
    // Hardware Initialization & Sensor Handling
    // ------------------------------------------------------------------------
    /**
     * @brief Initializes the GPIO pins and sets their modes.
     * @return true if successful, false otherwise.
     */
    bool initGPIO();

    /**
     * @brief Detects connected temperature sensors.
     */
    void detectSensors();

    /**
     * @brief Updates the temperature readings from detected sensors.
     */
    void updateTemperatureReadings();

    // ------------------------------------------------------------------------
    // INDI Property Definitions
    // ------------------------------------------------------------------------
    /**
     * @brief Defines the temperature probe properties.
     */
    void defineTemperatureProbes();

    /**
     * @brief Defines the power switch property and its update handler.
     */
    void definePowerSwitch();

    /**
     * @brief Handles updates for the power switch property.
     */
    void handlePowerUpdate();

    /**
     * @brief Defines the auxiliary switch property and its update handler.
     */
    void defineAuxSwitch();

    /**
     * @brief Handles updates for the auxiliary switch property.
     */
    void handleAuxUpdate();

    /**
     * @brief Defines the heater duty cycle property for Heater 0.
     */
    void defineHeater0DutyCycle();

    /**
     * @brief Defines the heater duty cycle property for Heater 1.
     */
    void defineHeater1DutyCycle();

    /**
     * @brief Common handler for updating heater properties.
     *
     * This function updates the PWM duty cycle for the specified heater,
     * logs the change, updates the heater's state, and applies the property.
     *
     * @param heaterProp The numeric property for the heater.
     * @param gpioPin The GPIO pin associated with the heater.
     * @param heaterName A string identifier for logging.
     */
    void handleHeaterUpdate(INDI::PropertyNumber &heaterProp, int gpioPin, const std::string &heaterName);

    // ------------------------------------------------------------------------
    // Private Data Members
    // ------------------------------------------------------------------------
    int piId = -1;               ///< Raspberry Pi connection ID (invalid until initialized).
    std::vector<Sensor> sensors; ///< List of detected temperature sensors.

    // ------------------------------------------------------------------------
    // INDI Property Enumerations & Instances
    // ------------------------------------------------------------------------

    // Enumerations for power switch states.
    enum
    {
        PWR_ON,
        PWR_OFF,
        PWR_N
    };
    INDI::PropertySwitch PowerSP{PWR_N}; ///< INDI property for the main power switch.

    // Enumerations for auxiliary switch states.
    enum
    {
        AUX_ON,
        AUX_OFF,
        AUX_N
    };
    INDI::PropertySwitch AuxSP{AUX_N}; ///< INDI property for the auxiliary switch.

    // INDI properties for heater duty cycle control.
    INDI::PropertyNumber Heater0NP{1}; ///< INDI property for Heater 0 duty cycle.
    INDI::PropertyNumber Heater1NP{1}; ///< INDI property for Heater 1 duty cycle.

    // INDI property for temperature sensor readings.
    INDI::PropertyNumber TempNP{0}; ///< INDI property for temperature probes.
};
