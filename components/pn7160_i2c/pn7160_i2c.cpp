#include "pn7160_i2c.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pn7160_i2c {

static const char *const TAG = "pn7160_i2c";

void PN7160I2C::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PN7160 (I2C)...");
  pn7160::PN7160::setup();
}

void PN7160I2C::dump_config() {
  ESP_LOGCONFIG(TAG, "PN7160 (I2C):");
  LOG_I2C_DEVICE(this);
  pn7160::PN7160::dump_config();
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Communication with PN7160 failed!");
  }
}

bool PN7160I2C::nci_write_(const std::vector<uint8_t> &data) {
  auto err = this->write(data.data(), data.size());
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "I2C write error: %d", err);
    return false;
  }
  return true;
}

bool PN7160I2C::nci_read_(std::vector<uint8_t> &data, uint8_t len) {
  data.resize(len);
  auto err = this->read(data.data(), len);
  if (err != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "I2C read error: %d", err);
    return false;
  }
  return true;
}

}  // namespace pn7160_i2c
}  // namespace esphome
