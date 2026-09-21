#include "Watchdog.hpp"
#include <iostream>
#include <thread>
#include <chrono>

namespace REN {

HardwareWatchdog::HardwareWatchdog(const std::string& chip_name, unsigned int pin) {
    // Acquire a handle to the system GPIO controller
    chip_ = gpiod_chip_open_by_name(chip_name.c_str());
    if (!chip_) {
        std::cerr << "[Watchdog] Critical fault: Failed to open GPIO chip " << chip_name << "\n";
        return;
    }

    // Reserve the specific pin assigned to the TI TPS3823 WDI (Watchdog Input)
    line_ = gpiod_chip_get_line(chip_, pin);
    if (!line_) {
        std::cerr << "[Watchdog] Critical fault: Failed to reserve GPIO pin " << pin << "\n";
        gpiod_chip_close(chip_);
        return;
    }

    // Configure the pin for active output driving
    if (gpiod_line_request_output(line_, "REN_Watchdog", 0) < 0) {
        std::cerr << "[Watchdog] Critical fault: Failed to set GPIO direction to OUTPUT\n";
        gpiod_chip_close(chip_);
        return;
    }

    active_ = true;
}

HardwareWatchdog::~HardwareWatchdog() {
    if (active_) {
        gpiod_line_release(line_);
        gpiod_chip_close(chip_);
    }
}

void HardwareWatchdog::kick() {
    if (!active_) return;
    
    // Execute a standard 100-microsecond high-to-low pulse required by the TPS3823 logic
    gpiod_line_set_value(line_, 1);
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    gpiod_line_set_value(line_, 0);
}

} // namespace REN