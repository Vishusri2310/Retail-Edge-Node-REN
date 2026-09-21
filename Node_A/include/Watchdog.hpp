/**
 * @file Watchdog.hpp
 * @brief Hardware watchdog controller for deterministic uptime.
 * @details Interacts with the libgpiod subsystem to pulse a TI TPS3823 hardware timer.
 * If the application hangs and misses a pulse, the supervisor IC forces a hard system reset.
 */

#pragma once
#include <gpiod.h>
#include <string>

namespace REN {

class HardwareWatchdog {
public:
    HardwareWatchdog(const std::string& chip_name, unsigned int pin);
    ~HardwareWatchdog();

    // Pulses the designated GPIO line to reset the hardware timer decay
    void kick();
    
private:
    gpiod_chip* chip_ = nullptr;
    gpiod_line* line_ = nullptr;
    bool active_ = false;
};

} // namespace REN