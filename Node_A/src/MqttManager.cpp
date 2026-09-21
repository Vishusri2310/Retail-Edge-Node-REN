#include "MqttManager.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

// Inside connect() try block, add the new subscription
void MqttManager::connect() {
    // ... setup ...
    try {
        client_->connect(conn_opts)->wait();
        client_->subscribe(Config::TOPIC_SYNC_REQUEST, 1)->wait();
        client_->subscribe(Config::TOPIC_NODE_ACTIVATE, 1)->wait(); // Add this line
    } catch (const mqtt::exception& e) {
        std::cerr << "[MQTT] Connection failed: " << e.what() << "\n";
    }
}

// Add the handler setter
void MqttManager::set_activation_handler(ActivationHandler handler) {
    activation_handler_ = std::move(handler);
}

// Update message_arrived to route the activation command
void MqttManager::message_arrived(mqtt::const_message_ptr msg) {
    std::string topic = msg->get_topic();
    std::string payload = msg->get_payload_str();

    if (topic == Config::TOPIC_SYNC_REQUEST) {
        if (sync_handler_) {
            std::string response_payload = sync_handler_();
            auto reply = mqtt::make_message(Config::TOPIC_SYNC_RESPONSE, response_payload, 2, false);
            client_->publish(reply);
        }
    } else if (topic == Config::TOPIC_NODE_ACTIVATE) {
        if (activation_handler_) {
            try {
                auto j = nlohmann::json::parse(payload);
                if (j.value("active", false)) {
                    long long sync_time = j.value("timestamp", 0LL);
                    if (sync_time > 0) activation_handler_(sync_time);
                }
            } catch (const std::exception& e) {
                std::cerr << "[MQTT] Failed to parse activation payload: " << e.what() << "\n";
            }
        }
    }
}

namespace REN {

MqttManager::MqttManager(const std::string& broker_uri, const std::string& client_id) {
    client_ = std::make_unique<mqtt::async_client>(broker_uri, client_id);
    client_->set_callback(*this);
}

MqttManager::~MqttManager() {
    if (client_ && client_->is_connected()) {
        client_->disconnect()->wait();
    }
}

void MqttManager::connect() {
    mqtt::connect_options conn_opts;
    conn_opts.set_clean_session(true);
    conn_opts.set_automatic_reconnect(true);

    try {
        client_->connect(conn_opts)->wait();
        // Subscribe to the sync topic to listen for data extraction triggers
        client_->subscribe(Config::TOPIC_SYNC_REQUEST, 1)->wait();
    } catch (const mqtt::exception& e) {
        std::cerr << "[MQTT] Connection exception: " << e.what() << "\n";
    }
}

void MqttManager::set_sync_request_handler(RequestHandler handler) {
    sync_handler_ = std::move(handler);
}

void MqttManager::publish_realtime_surge(const std::string& payload) {
    if (!client_ || !client_->is_connected()) return;
    
    // Dispatch surge alerts at QoS 1 to guarantee at-least-once delivery to Node C
    auto msg = mqtt::make_message(Config::TOPIC_SURGE_ALERT, payload, 1, false);
    client_->publish(msg);
}

void MqttManager::message_arrived(mqtt::const_message_ptr msg) {
    if (msg->get_topic() == Config::TOPIC_SYNC_REQUEST) {
        if (sync_handler_) {
            // Retrieve structured JSON telemetry from the Analytics module
            std::string payload = sync_handler_();
            
            // Dispatch the response with QoS 2 to prevent duplicated metrics aggregation
            auto reply = mqtt::make_message(Config::TOPIC_SYNC_RESPONSE, payload, 2, false);
            client_->publish(reply);
        }
    }
}

void MqttManager::connection_lost(const std::string& cause) {
    std::cerr << "[MQTT] Subsystem connectivity lost. Cause: " << cause << "\n";
}

} // namespace REN