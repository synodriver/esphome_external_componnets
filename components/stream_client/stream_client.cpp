#include "stream_client.h"
#include "esphome/core/log.h"

namespace esphome {
namespace stream_client {

static const char *const TAG = "stream_client";

void StreamClientComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up stream client...");
  this->client_ = std::make_unique<AsyncClient>();
  this->client_->onConnect([this](void *arg, AsyncClient *client) {
    ESP_LOGI(TAG, "Connected to %s:%u", this->address_.c_str(), this->port_);
    this->status_clear_warning();
#ifdef USE_BINARY_SENSOR
    if (this->connected_binary_sensor_ != nullptr) {
      this->connected_binary_sensor_->publish_state(true);
    }
#endif
  });
  this->client_->onDisconnect([this](void *arg, AsyncClient *client) {
    ESP_LOGW(TAG, "Disconnected from %s:%u", this->address_.c_str(), this->port_);
#ifdef USE_BINARY_SENSOR
if (this->connected_binary_sensor_ != nullptr) {
  this->connected_binary_sensor_->publish_state(false);
}
#endif
  });
  this->client_->onData([this](void *arg, AsyncClient *client, void *data, size_t len) {
    ESP_LOGV(TAG, "Received %zu bytes of data", len);
    this->write_array(static_cast<uint8_t *>(data), len);
    this->flush();
  });
  this->client_->onError([this](void *arg, AsyncClient *client, int error) {
    ESP_LOGE(TAG, "Error connecting to %s:%u", this->address_.c_str(), this->port_);
    this->status_set_warning();
  });
  if (this->client_->connect(this->address_.c_str(), this->port_)) {
    ESP_LOGI(TAG, "Connecting to %s:%u...", this->address_.c_str(), this->port_);
  } else {
    ESP_LOGE(TAG, "Failed to initiate connection to %s:%u", this->address_.c_str(), this->port_);
    this->status_set_warning();
  }
}

void StreamClientComponent::loop() {
  if (this->client_) {
#if !defined(USE_ESP32) && !defined(USE_ESP8266) && !defined(USE_RP2040) && !defined(USE_LIBRETINY) && \
(defined(USE_SOCKET_IMPL_LWIP_SOCKETS) || defined(USE_SOCKET_IMPL_BSD_SOCKETS))
    this->client_->loop();
#endif
    while (this->available()) {
      uint8_t byte = this->read();
      this->client_->write(reinterpret_cast<const char *>(&byte), 1);
    }
  }
}

void StreamClientComponent::dump_config() {
  ESP_LOGCONFIG(TAG,
              "stream client:\n "
              "  Host: %s\n "
              "  Port: %u",
              this->address_.c_str(), this->port_);
#ifdef USE_BINARY_SENSOR
  LOG_BINARY_SENSOR("  ", "Connected Binary Sensor", this->connected_binary_sensor_);
#endif
}

}  // namespace stream_client
}