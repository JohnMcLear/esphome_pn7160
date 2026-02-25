#include "pn7160_i2c.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pn7160_i2c {

static const char *const TAG = "pn7160_i2c";

void PN7160I2C::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PN7160 I2C...");
  pn7160::PN7160::setup();
}

void PN7160I2C::dump_config() {
  ESP_LOGCONFIG(TAG, "PN7160 I2C:");
  LOG_I2C_DEVICE(this);
  pn7160::PN7160::dump_config();
  if (this->is_failed()) {
    ESP_LOGE(TAG, "Communication with PN7160 failed!");
  }
}

uint8_t PN7160I2C::write_nfcc(nfc::NciMessage &tx) {
  const auto &data = tx.get_message();
  auto err = this->write(data.data(), data.size());
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "I2C write error %d", err);
    return nfc::STATUS_FAILED;
  }
  return nfc::STATUS_OK;
}

uint8_t PN7160I2C::read_nfcc(nfc::NciMessage &rx, uint16_t timeout) {
  uint8_t header[3];
  auto err = this->read(header, 3);
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "I2C read header error %d", err);
    return nfc::STATUS_FAILED;
  }
  uint8_t payload_len = header[2];
  std::vector<uint8_t> data(header, header + 3);
  if (payload_len > 0) {
    data.resize(3 + payload_len);
    err = this->read(data.data() + 3, payload_len);
    if (err != i2c::ERROR_OK) {
      ESP_LOGW(TAG, "I2C read payload error %d", err);
      return nfc::STATUS_FAILED;
    }
  }
  rx.set_message(std::move(data));
  return nfc::STATUS_OK;
}

}  // namespace pn7160_i2c
}  // namespace esphome

