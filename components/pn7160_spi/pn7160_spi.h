#pragma once

#include "../pn7160/pn7160.h"
#include "esphome/components/spi/spi.h"

namespace esphome {
namespace pn7160_spi {

class PN7160Spi : public pn7160::PN7160,
                  public spi::SPIDevice<spi::BIT_ORDER_MSB_FIRST,
                                        spi::CLOCK_POLARITY_LOW,
                                        spi::CLOCK_PHASE_LEADING,
                                        spi::DATA_RATE_5MHZ> {
 public:
  void setup() override;
  void dump_config() override;

 protected:
  bool nci_write_(const std::vector<uint8_t> &data) override;
  bool nci_read_(std::vector<uint8_t> &data, uint8_t len) override;
  void pn7160_pre_setup_() override;
};

}  // namespace pn7160_spi
}  // namespace esphome
