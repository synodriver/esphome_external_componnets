#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/socket/socket.h"
#include "esphome/components/async_tcp/async_tcp.h"
#ifdef USE_BINARY_SENSOR
#include "esphome/components/binary_sensor/binary_sensor.h"
#endif

namespace esphome {
namespace stream_client {

class StreamClientComponent: public Component, public uart::UARTDevice {
#ifdef USE_BINARY_SENSOR
  SUB_BINARY_SENSOR(connected)
#endif

  public:
    void setup() override;
    float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
    void dump_config() override;
    void loop() override;

    void set_address(const std::string& address) { this->address_ = address; } // some ip address or hostname
    // void set_buffer_size(size_t size) { this->buf_size_ = size; }
    void set_port(uint16_t port) { this->port_ = port; }

  protected:
    std::string address_;
    // size_t buf_size_{};
    uint16_t port_{};

    std::unique_ptr<AsyncClient> client_{};

};

}
}