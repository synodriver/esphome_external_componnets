#include "ws_z.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ws_z {

static const char *const TAG = "dart_ws_z";
static const uint8_t DART_REQUEST_LENGTH = 8;
static const uint8_t DART_RESPONSE_LENGTH = 9;
// static const uint8_t DART_COMMAND_GET_PPM[] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};//问答模式下，请求发送
// static const uint8_t DART_COMMAND_SET_QA[]  = {0xFF,0x01,0x78,0x41,0x00,0x00,0x00,0x00,0x46};//切换到问答模式
// static const uint8_t DART_COMMAND_SET_NQA[]  = {0xFF,0x01,0x78,0x40,0x00,0x00,0x00,0x00,0x47};//切换到主动上传模式

static const uint8_t DART_COMMAND_GET_PPM[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00};  // 问答模式下，请求发送
static const uint8_t DART_COMMAND_SET_QA[] = {0xFF, 0x01, 0x78, 0x41, 0x00, 0x00, 0x00, 0x00};  // 切换到问答模式
static const uint8_t DART_COMMAND_SET_NQA[] = {0xFF, 0x01, 0x78, 0x40, 0x00, 0x00, 0x00, 0x00};  // 切换到主动上传模式

uint8_t dart_checksum(const uint8_t *command) {
  uint8_t sum = 0;
  for (uint8_t i = 1; i < DART_REQUEST_LENGTH; i++) {
    sum += command[i];
  }
  sum = (~sum) + 1;
  return sum;
}

void DARTWSZComponent::setup() {
  uint8_t response[DART_RESPONSE_LENGTH];
  if (this->mode_ == DARTWS_MODE_PASSIVE) {
    if (!this->dart_write_command_(DART_COMMAND_SET_QA, response)) {
      ESP_LOGW(TAG, "Reading data from DART WS-Z failed!");
      this->status_set_warning();
      return;
    }
  }
}

void DARTWSZComponent::update() {
  if (this->mode_ == DARTWS_MODE_PASSIVE) {
    uint8_t response[DART_RESPONSE_LENGTH];
    if (!this->dart_write_command_(DART_COMMAND_GET_PPM, response)) {
      ESP_LOGW(TAG, "Reading data from DART WS-Z failed!");
      this->status_set_warning();
      return;
    }

    if (response[0] != 0xFF || response[1] != 0x86) {
      ESP_LOGW(TAG, "Invalid preamble from DART WS-Z!");
      this->status_set_warning();
      return;
    }

    uint8_t checksum = dart_checksum(response);
    if (response[8] != checksum) {
      ESP_LOGW(TAG, "DART WS-Z Checksum doesn't match: 0x%02X!=0x%02X", response[8], checksum);
      this->status_set_warning();
      return;
    }

    this->status_clear_warning();
    const uint16_t ch2oh_mg = uint16_t(response[2]) * 256 + response[3];
    const uint16_t ch20h_ppb = uint16_t(response[6]) * 256 + response[7];

    ESP_LOGD(TAG, "DART WS-Z Received HCHO=%u µg/m³, %u ppb, %X,%X,%X,%X,%X,%X,%X,%X,%X, ", ch2oh_mg, ch20h_ppb,
             response[0], response[1], response[2], response[3], response[4], response[5], response[6], response[7],
             response[8]);
    if (this->formaldehyde_sensor_ != nullptr) {
      this->formaldehyde_sensor_->publish_state(ch2oh_mg);
    }
    if (this->formaldehyde_ppb_sensor_ != nullptr) {
      this->formaldehyde_ppb_sensor_->publish_state(ch20h_ppb);
    }
  }
}

void DARTWSZComponent::loop() {
  if (this->mode_ == DARTWS_MODE_ACTIVE) {
    while (this->available()) {
      uint8_t c;
      if (!this->read_byte(&c)) {
        return;
      }
      // 寻找 0xFF 帧头
      if (c != 0xFF) {
        continue;
      }

      // 找到帧头后，读取后续的8个字节数据
      uint8_t data[8];
      if (!this->read_array(data, 8)) {
        return;
      }

      // 校验和检查
      uint8_t sum = 0;
      for (int i = 0; i < 8; i++) {
        sum += data[i];
      }

      if (sum != 0) {
        // 校验失败的数据包直接丢弃
        ESP_LOGW(TAG, "Checksum failed! Discarding packet. Sum: 0x%02X", sum);
        continue;
      }

      // 检查 甲醛ID(0x17) 和 Ppb单位(0x04)
      if (data[0] == 0x17 && data[1] == 0x04) {
        // 浓度值位置在第4和第5字节 (数组索引为 data[3] 和 data[4])
        uint32_t val = (uint32_t(data[3]) << 8) | uint32_t(data[4]);
        if (this->formaldehyde_ppb_sensor_ != nullptr) {
          this->formaldehyde_ppb_sensor_->publish_state(val);
        }
      } else {
        // 如果收到其他ID或单位的数据，也记录下来
        ESP_LOGW(TAG, "Ignoring frame with unknown ID/Unit. ID: 0x%02X, Unit: 0x02X", data[0], data[1]);
      }
    }
  }
}


bool DARTWSZComponent::dart_write_command_(const uint8_t *command, uint8_t *response) {
  // Empty RX Buffer
  while (this->available()) {
    this->read();
  }
  this->write_array(command, DART_REQUEST_LENGTH);
  this->write_byte(dart_checksum(command));
  this->flush();
  if (response == nullptr)
    return true;

  return this->read_array(response, DART_RESPONSE_LENGTH);
}

float DARTWSZComponent::get_setup_priority() const { return setup_priority::DATA; }

void DARTWSZComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "DART WS-Z:");
  LOG_SENSOR("  ", "HCHO ", this->formaldehyde_sensor_);
  LOG_SENSOR("  ", "HCHO PPB", this->formaldehyde_ppb_sensor_);
  this->check_uart_settings(9600);
}

}  // namespace ws_z
}  // namespace esphome