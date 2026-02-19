#include "pn7160_spi.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pn7160_spi {

static const char *const TAG = "pn7160_spi";

void PN7160Spi::pn7160_pre_setup_() {
  this->spi_setup();
  delay(10);
}

void PN7160Spi::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PN7160 (SPI)...");
  pn7160::PN7160::setup();
}

void PN7160Spi::dump_config() {
  ESP_LOGCONFIG(TAG, "PN7160 (SPI):");
  LOG_PIN("  CS Pin: ", this->cs_);
  pn7160::PN7160::dump_config();
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Communication with PN7160 failed!");
  }
}

bool PN7160Spi::nci_write_(const std::vector<uint8_t> &data) {
  this->enable();
  for (auto b : data)
    this->write_byte(b);
  this->disable();
  return true;
}

bool PN7160Spi::nci_read_(std::vector<uint8_t> &data, uint8_t len) {
  this->enable();
  data.resize(len);
  for (uint8_t i = 0; i < len; i++)
    data[i] = this->read_byte();
  this->disable();
  return true;
}

}  // namespace pn7160_spi
}  // namespace esphome
