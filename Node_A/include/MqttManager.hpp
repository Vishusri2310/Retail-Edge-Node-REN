/**
 * @file MqttManager.hpp
 * @brief Asynchronous MQTT client interface.
 * @details Handles bidirectional local network communication, facilitating high-priority 
 * predictive surge alerts and responding to scheduled node synchronization requests.
 */

#pragma once
#include "Config.hpp"
#include <mqtt/async_client.h>
#include <functional>
#include <string>
#include <memory>

namespace REN {

class MqttManager : public virtual mqtt::callback {
public:
    using RequestHandler = std::function<std::string()>;

    MqttManager(const std::string& broker_uri, const std::string& client_id);
    ~MqttManager() override;

    void connect();
    
    // Registers the callback executed when Node C requests telemetry sync
    void set_sync_request_handler(RequestHandler handler);
    
    // Pushes immediate QoS-1 alerts when queue thresholds are breached
    void publish_realtime_surge(const std::string& payload);

    // Overridden MQTT asynchronous callbacks
    void message_arrived(mqtt::const_message_ptr msg) override;
    void connection_lost(const std::string& cause) override;
    // Add alongside RequestHandler
    using ActivationHandler = std::function<void(long long)>;

    // Add inside the MqttManager class
    void set_activation_handler(ActivationHandler handler);
    
private:
    std::unique_ptr<mqtt::async_client> client_;
    RequestHandler sync_handler_;
    ActivationHandler activation_handler_; // New handler
    
};

} // namespace REN