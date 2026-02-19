#pragma once

#include "../pn7160/pn7160.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace pn7160_i2c {

static const uint8_t PN7160_I2C_DEFAULT_ADDRESS = 0x28;

class PN7160I2C : public pn7160::PN7160, public i2c::I2CDevice {
 public:
  void setup() override;
  void dump_config() override;

 protected:
  bool nci_write_(const std::vector<uint8_t> &data) override;
  bool nci_read_(std::vector<uint8_t> &data, uint8_t len) override;
};

}  // namespace pn7160_i2c
}  // namespace esphome
