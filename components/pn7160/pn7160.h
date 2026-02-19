#pragma once

#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/core/hal.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

#include <functional>
#include <string>
#include <vector>

namespace esphome {
namespace pn7160 {

// ─── Forward declarations ────────────────────────────────────────────────────

class PN7160;

// ─── Binary Sensor ───────────────────────────────────────────────────────────

class PN7160BinarySensor : public binary_sensor::BinarySensor {
 public:
  void set_uid(const std::vector<uint8_t> &uid) { this->uid_ = uid; }
  bool process(const std::vector<uint8_t> &uid);
  void on_scan_end() {
    if (!this->found_) {
      this->publish_state(false);
    }
    this->found_ = false;
  }

 protected:
  std::vector<uint8_t> uid_;
  bool found_{false};
};

// ─── Triggers ────────────────────────────────────────────────────────────────

class PN7160Trigger : public Trigger<std::string> {
 public:
  explicit PN7160Trigger(PN7160 *parent);
};

class PN7160TagRemovedTrigger : public Trigger<std::string> {
 public:
  explicit PN7160TagRemovedTrigger(PN7160 *parent);
};

// ─── Main Base Component ─────────────────────────────────────────────────────

class PN7160 : public PollingComponent {
 public:
  void setup() override;
  void dump_config() override;
  void loop() override;
  void update() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // ── Pin configuration (IRQ and VEN are REQUIRED for PN7160) ──
  void set_irq_pin(GPIOPin *pin) { this->irq_pin_ = pin; }
  void set_ven_pin(GPIOPin *pin) { this->ven_pin_ = pin; }

  // ── Sensor registration ──
  void register_tag_sensor(PN7160BinarySensor *sensor) {
    this->binary_sensors_.push_back(sensor);
  }

  // ── Callbacks ──
  void add_on_tag_callback(std::function<void(std::string)> &&cb) {
    this->on_tag_callbacks_.add(std::move(cb));
  }
  void add_on_tag_removed_callback(std::function<void(std::string)> &&cb) {
    this->on_tag_removed_callbacks_.add(std::move(cb));
  }

  // ── Health check configuration ──
  void set_health_check_enabled(bool enabled) { this->health_check_enabled_ = enabled; }
  void set_health_check_interval(uint32_t ms) { this->health_check_interval_ = ms; }
  void set_auto_reset_on_failure(bool v) { this->auto_reset_on_failure_ = v; }
  void set_max_failed_checks(uint8_t v) { this->max_failed_checks_ = v; }

 protected:
  // ── Abstract transport interface (implemented by SPI/I2C subclasses) ──
  virtual bool nci_write_(const std::vector<uint8_t> &data) = 0;
  virtual bool nci_read_(std::vector<uint8_t> &data, uint8_t len) = 0;
  virtual void pn7160_pre_setup_() {}

  // ── NCI protocol helpers ──
  bool nci_write_packet_(uint8_t gid, uint8_t oid, const std::vector<uint8_t> &payload);
  bool nci_read_packet_(uint8_t &mt, uint8_t &gid, uint8_t &oid, std::vector<uint8_t> &payload);
  bool nci_read_notification_(uint8_t &gid, uint8_t &oid, std::vector<uint8_t> &payload);
  bool nci_read_response_(uint8_t gid_exp, uint8_t oid_exp, std::vector<uint8_t> &payload);

  // ── Core NCI init/reset ──
  bool nci_core_reset_();
  bool nci_core_init_();
  bool configure_rf_discovery_();
  void hard_reset_();

  // ── Tag detection ──
  void process_tag_(const std::vector<uint8_t> &uid);
  void tag_removed_();
  bool read_tag_(std::vector<uint8_t> &uid);

  // ── IRQ handling (FIX for bugs #6339 and IRQ blocking) ──
  bool wait_for_irq_(uint32_t timeout_ms);
  void clear_irq_();

  // ── Health check ──
  bool perform_health_check_();

  // ── Hardware pins ──
  GPIOPin *irq_pin_{nullptr};
  GPIOPin *ven_pin_{nullptr};

  // ── NCI state ──
  uint8_t model_id_{0};
  uint8_t fw_major_{0};
  uint8_t fw_minor_{0};
  uint8_t fw_patch_{0};

  // ── Tag state ──
  std::vector<uint8_t> current_uid_{};
  bool tag_present_{false};

  // ── Binary sensors ──
  std::vector<PN7160BinarySensor *> binary_sensors_;

  // ── Backoff state ──
  uint8_t retries_{0};
  uint32_t last_update_ms_{0};
  uint32_t throttle_ms_{0};

  // ── Health check state ──
  bool health_check_enabled_{true};
  uint32_t health_check_interval_{60000};
  uint32_t last_health_check_ms_{0};
  uint8_t consecutive_failures_{0};
  uint8_t max_failed_checks_{3};
  bool auto_reset_on_failure_{true};
  bool is_healthy_{true};

  // ── Callbacks ──
  CallbackManager<void(std::string)> on_tag_callbacks_;
  CallbackManager<void(std::string)> on_tag_removed_callbacks_;
};

}  // namespace pn7160
}  // namespace esphome
