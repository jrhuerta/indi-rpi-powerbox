#pragma once

// ============================================================================
// INCLUDES
// ============================================================================
#include "libindi/connectionplugins/connectioninterface.h"
#include "libindi/defaultdevice.h"
#include <string>

// ============================================================================
// GPIOConnection Class
// ============================================================================

/**
 * @brief The GPIOConnection class provides a custom connection interface that
 * bypasses the default connection management and driver loop in INDI.
 *
 * This class implements stub functions for Connect/Disconnect and related
 * callbacks, essentially allowing the driver to use a custom connection type
 * without overriding the entire INDI connection lifecycle.
 */
class GPIOConnection : public Connection::Interface
{
public:
    /**
     * @brief Constructs a new GPIOConnection object.
     *
     * @param dev Pointer to the INDI::DefaultDevice associated with this connection.
     */
    explicit GPIOConnection(INDI::DefaultDevice *dev)
        : Connection::Interface(dev, CONNECTION_CUSTOM)
    {
    }

    // ------------------------------------------------------------------------
    // Connection Management Overrides
    // ------------------------------------------------------------------------

    /**
     * @brief Connect stub; always returns true.
     *
     * @return true indicating successful connection.
     */
    bool Connect() override { return true; }

    /**
     * @brief Disconnect stub; always returns true.
     *
     * @return true indicating successful disconnection.
     */
    bool Disconnect() override { return true; }

    // ------------------------------------------------------------------------
    // Activation Callbacks
    // ------------------------------------------------------------------------

    /**
     * @brief Called when the connection is activated.
     */
    void Activated() override {}

    /**
     * @brief Called when the connection is deactivated.
     */
    void Deactivated() override {}

    // ------------------------------------------------------------------------
    // Identification
    // ------------------------------------------------------------------------

    /**
     * @brief Returns the name of the connection.
     *
     * @return A string identifying the connection type.
     */
    std::string name() override { return "CONNECTION_GPIO"; }

    /**
     * @brief Returns the label of the connection.
     *
     * @return A string representing the connection label.
     */
    std::string label() override { return "GPIO"; }
};
