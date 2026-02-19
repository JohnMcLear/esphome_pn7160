#include "pn7160.h"
#include "esphome/core/log.h"

namespace esphome {
namespace pn7160 {

static const char *const TAG = "pn7160";

// ─── NCI constants ────────────────────────────────────────────────────────────

// Message Types (MT) - bits 7:5 of header byte
static const uint8_t NCI_MT_DATA = 0x00;
static const uint8_t NCI_MT_CMD = 0x20;
static const uint8_t NCI_MT_RSP = 0x40;
static const uint8_t NCI_MT_NTF = 0x60;

// Group IDs (GID)
static const uint8_t NCI_GID_CORE = 0x00;
static const uint8_t NCI_GID_RF_MGMT = 0x01;
static const uint8_t NCI_GID_NFCEE_MGMT = 0x02;
static const uint8_t NCI_GID_PROPRIETARY = 0x0F;

// Opcodes (OID) - CORE group
static const uint8_t NCI_OID_CORE_RESET = 0x00;
static const uint8_t NCI_OID_CORE_INIT = 0x01;

// Opcodes - RF_MGMT group
static const uint8_t NCI_OID_RF_DISCOVER_MAP = 0x00;
static const uint8_t NCI_OID_RF_SET_LISTEN_MODE_ROUTING = 0x01;
static const uint8_t NCI_OID_RF_GET_LISTEN_MODE_ROUTING = 0x02;
static const uint8_t NCI_OID_RF_DISCOVER = 0x03;
static const uint8_t NCI_OID_RF_DISCOVER_SELECT = 0x04;
static const uint8_t NCI_OID_RF_INTF_ACTIVATED = 0x05;
static const uint8_t NCI_OID_RF_DEACTIVATE = 0x06;

// NCI Status codes
static const uint8_t NCI_STATUS_OK = 0x00;

// RF Technologies
static const uint8_t NCI_RF_TECH_A = 0x00;      // NFC-A (ISO14443A)
static const uint8_t NCI_RF_TECH_B = 0x01;      // NFC-B (ISO14443B)
static const uint8_t NCI_RF_TECH_F = 0x02;      // NFC-F (FeliCa)
static const uint8_t NCI_RF_TECH_15693 = 0x03;  // ISO15693

// RF Protocols
static const uint8_t NCI_RF_PROTOCOL_T1T = 0x01;
static const uint8_t NCI_RF_PROTOCOL_T2T = 0x02;
static const uint8_t NCI_RF_PROTOCOL_T3T = 0x03;
static const uint8_t NCI_RF_PROTOCOL_ISO_DEP = 0x04;
static const uint8_t NCI_RF_PROTOCOL_NFC_DEP = 0x05;
static const uint8_t NCI_RF_PROTOCOL_15693 = 0x06;

// ─── Trigger constructors ─────────────────────────────────────────────────────

PN7160Trigger::PN7160Trigger(PN7160 *parent) {
  parent->add_on_tag_callback([this](std::string uid) { this->trigger(uid); });
}

PN7160TagRemovedTrigger::PN7160TagRemovedTrigger(PN7160 *parent) {
  parent->add_on_tag_removed_callback([this](std::string uid) { this->trigger(uid); });
}

// ─── Binary sensor ───────────────────────────────────────────────────────────

bool PN7160BinarySensor::process(const std::vector<uint8_t> &uid) {
  if (uid.size() != this->uid_.size())
    return false;
  for (size_t i = 0; i < uid.size(); i++) {
    if (uid[i] != this->uid_[i])
      return false;
  }
  this->found_ = true;
  this->publish_state(true);
  return true;
}

// ─── IRQ handling (FIX for bugs #6339 and IRQ blocking) ──────────────────────

bool PN7160::wait_for_irq_(uint32_t timeout_ms) {
  if (this->irq_pin_ == nullptr) {
    ESP_LOGE(TAG, "IRQ pin not configured");
    return false;
  }

  uint32_t start = millis();
  uint32_t backoff = 1;  // Exponential backoff to reduce tight polling

  while (!this->irq_pin_->digital_read()) {
    if (millis() - start > timeout_ms) {
      ESP_LOGV(TAG, "Timed out waiting for IRQ HIGH");
      return false;
    }
    delay(backoff);
    backoff = std::min(backoff * 2, (uint32_t)10);  // Cap at 10ms
    yield();
  }
  return true;
}

void PN7160::clear_irq_() {
  // FIX for IRQ blocking bug: Read and discard pending notifications
  if (this->irq_pin_ == nullptr || !this->irq_pin_->digital_read())
    return;

  ESP_LOGV(TAG, "Clearing stuck IRQ state");
  uint8_t mt, gid, oid;
  std::vector<uint8_t> dummy;

  // Read up to 5 pending packets to clear IRQ
  for (int i = 0; i < 5 && this->irq_pin_->digital_read(); i++) {
    if (!this->nci_read_packet_(mt, gid, oid, dummy)) {
      break;
    }
    delay(1);
  }

  // If still stuck, hard reset via VEN pin
  if (this->irq_pin_->digital_read()) {
    ESP_LOGW(TAG, "IRQ still stuck after clearing, performing hard reset");
    this->hard_reset_();
  }
}

// ─── Hard reset ──────────────────────────────────────────────────────────────

void PN7160::hard_reset_() {
  if (this->ven_pin_ == nullptr) {
    ESP_LOGW(TAG, "Cannot perform hard reset: VEN pin not configured");
    return;
  }

  ESP_LOGD(TAG, "Hard reset via VEN pin");
  this->ven_pin_->digital_write(false);
  delay(10);
  this->ven_pin_->digital_write(true);
  delay(10);

  // Wait for CORE_RESET_NTF
  if (this->wait_for_irq_(1000)) {
    uint8_t gid, oid;
    std::vector<uint8_t> ntf;
    this->nci_read_notification_(gid, oid, ntf);
  }
}

// ─── NCI packet read/write ───────────────────────────────────────────────────

bool PN7160::nci_write_packet_(uint8_t gid, uint8_t oid, const std::vector<uint8_t> &payload) {
  // NCI packet: [Header] [Payload Length] [Payload...]
  // Header = MT | PBF | GID where MT=0x20 for CMD
  uint8_t header = NCI_MT_CMD | (gid & 0x0F);

  std::vector<uint8_t> packet;
  packet.push_back(header);
  packet.push_back(oid);
  packet.push_back((uint8_t)payload.size());
  for (auto b : payload)
    packet.push_back(b);

  ESP_LOGVV(TAG, "NCI write: GID=0x%02X OID=0x%02X len=%d", gid, oid, payload.size());
  return this->nci_write_(packet);
}

bool PN7160::nci_read_packet_(uint8_t &mt, uint8_t &gid, uint8_t &oid, std::vector<uint8_t> &payload) {
  std::vector<uint8_t> header;
  if (!this->nci_read_(header, 3)) {
    return false;
  }

  mt = header[0] & 0xE0;
  gid = header[0] & 0x0F;
  oid = header[1];
  uint8_t len = header[2];

  if (len == 0) {
    payload.clear();
    return true;
  }

  if (!this->nci_read_(payload, len)) {
    return false;
  }

  ESP_LOGVV(TAG, "NCI read: MT=0x%02X GID=0x%02X OID=0x%02X len=%d", mt, gid, oid, len);
  return true;
}

bool PN7160::nci_read_notification_(uint8_t &gid, uint8_t &oid, std::vector<uint8_t> &payload) {
  uint8_t mt;
  if (!this->nci_read_packet_(mt, gid, oid, payload)) {
    return false;
  }
  if (mt != NCI_MT_NTF) {
    ESP_LOGW(TAG, "Expected notification (0x60), got MT=0x%02X", mt);
    return false;
  }
  return true;
}

bool PN7160::nci_read_response_(uint8_t gid_exp, uint8_t oid_exp, std::vector<uint8_t> &payload) {
  uint8_t mt, gid, oid;
  if (!this->nci_read_packet_(mt, gid, oid, payload)) {
    return false;
  }
  if (mt != NCI_MT_RSP) {
    ESP_LOGW(TAG, "Expected response (0x40), got MT=0x%02X", mt);
    return false;
  }
  if (gid != gid_exp || oid != oid_exp) {
    ESP_LOGW(TAG, "Response mismatch: expected GID=0x%02X OID=0x%02X, got GID=0x%02X OID=0x%02X",
             gid_exp, oid_exp, gid, oid);
    return false;
  }
  return true;
}

// ─── NCI Core Reset ──────────────────────────────────────────────────────────

bool PN7160::nci_core_reset_() {
  // CORE_RESET_CMD: reset type 0x01 (keep config)
  if (!this->nci_write_packet_(NCI_GID_CORE, NCI_OID_CORE_RESET, {0x01})) {
    ESP_LOGW(TAG, "CORE_RESET_CMD write failed");
    return false;
  }

  // Wait for IRQ and read CORE_RESET_RSP
  if (!this->wait_for_irq_(1000)) {
    ESP_LOGW(TAG, "Timeout waiting for CORE_RESET_RSP");
    return false;
  }

  std::vector<uint8_t> rsp;
  if (!this->nci_read_response_(NCI_GID_CORE, NCI_OID_CORE_RESET, rsp)) {
    ESP_LOGW(TAG, "CORE_RESET_RSP read failed");
    return false;
  }

  if (rsp.empty() || rsp[0] != NCI_STATUS_OK) {
    ESP_LOGW(TAG, "CORE_RESET_RSP status: 0x%02X", rsp.empty() ? 0xFF : rsp[0]);
    return false;
  }

  // Wait for CORE_RESET_NTF
  if (!this->wait_for_irq_(1000)) {
    ESP_LOGW(TAG, "Timeout waiting for CORE_RESET_NTF");
    return false;
  }

  uint8_t gid, oid;
  std::vector<uint8_t> ntf;
  if (!this->nci_read_notification_(gid, oid, ntf)) {
    ESP_LOGW(TAG, "CORE_RESET_NTF read failed");
    return false;
  }

  if (gid != NCI_GID_CORE || oid != NCI_OID_CORE_RESET) {
    ESP_LOGW(TAG, "Expected CORE_RESET_NTF, got GID=0x%02X OID=0x%02X", gid, oid);
    return false;
  }

  ESP_LOGV(TAG, "CORE_RESET completed");
  return true;
}

// ─── NCI Core Init ───────────────────────────────────────────────────────────

bool PN7160::nci_core_init_() {
  // CORE_INIT_CMD (no payload for NCI 2.0)
  if (!this->nci_write_packet_(NCI_GID_CORE, NCI_OID_CORE_INIT, {})) {
    ESP_LOGW(TAG, "CORE_INIT_CMD write failed");
    return false;
  }

  if (!this->wait_for_irq_(1000)) {
    ESP_LOGW(TAG, "Timeout waiting for CORE_INIT_RSP");
    return false;
  }

  std::vector<uint8_t> rsp;
  if (!this->nci_read_response_(NCI_GID_CORE, NCI_OID_CORE_INIT, rsp)) {
    ESP_LOGW(TAG, "CORE_INIT_RSP read failed");
    return false;
  }

  if (rsp.size() < 13 || rsp[0] != NCI_STATUS_OK) {
    ESP_LOGW(TAG, "CORE_INIT_RSP invalid or status error");
    return false;
  }

  // Parse firmware version from CORE_INIT_RSP
  // Byte 9: Model ID, Bytes 10-12: FW version (major.minor.patch)
  this->model_id_ = rsp[9];
  this->fw_major_ = rsp[10];
  this->fw_minor_ = rsp[11];
  this->fw_patch_ = rsp[12];

  ESP_LOGCONFIG(TAG, "  Model: 0x%02X  Firmware: %d.%d.%d",
                this->model_id_, this->fw_major_, this->fw_minor_, this->fw_patch_);
  return true;
}

// ─── Configure RF Discovery ──────────────────────────────────────────────────

bool PN7160::configure_rf_discovery_() {
  // RF_DISCOVER_MAP_CMD: Map NFC-A and NFC-F to poll mode
  std::vector<uint8_t> discover_map = {
    0x02,  // Number of mappings
    NCI_RF_TECH_A, NCI_RF_PROTOCOL_ISO_DEP, 0x01,  // NFC-A → ISO-DEP (poll)
    NCI_RF_TECH_F, NCI_RF_PROTOCOL_T3T, 0x01       // NFC-F → T3T (poll)
  };

  if (!this->nci_write_packet_(NCI_GID_RF_MGMT, NCI_OID_RF_DISCOVER_MAP, discover_map)) {
    ESP_LOGW(TAG, "RF_DISCOVER_MAP_CMD write failed");
    return false;
  }

  if (!this->wait_for_irq_(1000))
    return false;

  std::vector<uint8_t> rsp;
  if (!this->nci_read_response_(NCI_GID_RF_MGMT, NCI_OID_RF_DISCOVER_MAP, rsp))
    return false;

  if (rsp.empty() || rsp[0] != NCI_STATUS_OK) {
    ESP_LOGW(TAG, "RF_DISCOVER_MAP_RSP status error");
    return false;
  }

  // RF_DISCOVER_CMD: Start discovery for NFC-A and NFC-F
  std::vector<uint8_t> discover = {
    0x02,  // Number of configurations
    NCI_RF_TECH_A, 0x01,  // NFC-A, poll mode
    NCI_RF_TECH_F, 0x01   // NFC-F, poll mode
  };

  if (!this->nci_write_packet_(NCI_GID_RF_MGMT, NCI_OID_RF_DISCOVER, discover)) {
    ESP_LOGW(TAG, "RF_DISCOVER_CMD write failed");
    return false;
  }

  if (!this->wait_for_irq_(1000))
    return false;

  if (!this->nci_read_response_(NCI_GID_RF_MGMT, NCI_OID_RF_DISCOVER, rsp))
    return false;

  if (rsp.empty() || rsp[0] != NCI_STATUS_OK) {
    ESP_LOGW(TAG, "RF_DISCOVER_RSP status error");
    return false;
  }

  ESP_LOGV(TAG, "RF discovery configured and started");
  return true;
}

// ─── Setup ───────────────────────────────────────────────────────────────────

void PN7160::setup() {
  ESP_LOGCONFIG(TAG, "Setting up PN7160...");

  // Validate required pins
  if (this->irq_pin_ == nullptr || this->ven_pin_ == nullptr) {
    ESP_LOGE(TAG, "IRQ and VEN pins are required for PN7160");
    this->mark_failed();
    return;
  }

  this->irq_pin_->setup();
  this->ven_pin_->setup();

  // Transport-specific pre-setup (e.g. spi_setup())
  this->pn7160_pre_setup_();

  // Perform hard reset to init PN7160
  this->hard_reset_();

  // NCI init sequence
  if (!this->nci_core_reset_()) {
    ESP_LOGE(TAG, "CORE_RESET failed");
    this->mark_failed();
    return;
  }

  if (!this->nci_core_init_()) {
    ESP_LOGE(TAG, "CORE_INIT failed");
    this->mark_failed();
    return;
  }

  if (!this->configure_rf_discovery_()) {
    ESP_LOGE(TAG, "RF discovery configuration failed");
    this->mark_failed();
    return;
  }

  if (this->health_check_enabled_) {
    this->last_health_check_ms_ = millis();
  }

  ESP_LOGCONFIG(TAG, "PN7160 setup complete");
}

// ─── Dump config ─────────────────────────────────────────────────────────────

void PN7160::dump_config() {
  ESP_LOGCONFIG(TAG, "PN7160:");
  LOG_PIN("  IRQ Pin: ", this->irq_pin_);
  LOG_PIN("  VEN Pin: ", this->ven_pin_);
  LOG_UPDATE_INTERVAL(this);
  ESP_LOGCONFIG(TAG, "  Registered binary sensors: %d", this->binary_sensors_.size());
  if (this->health_check_enabled_) {
    ESP_LOGCONFIG(TAG, "  Health Check: interval=%dms, max_failures=%d, auto_reset=%s",
                  this->health_check_interval_, this->max_failed_checks_,
                  this->auto_reset_on_failure_ ? "yes" : "no");
  } else {
    ESP_LOGCONFIG(TAG, "  Health Check: disabled");
  }
}

// ─── Loop (health check) ──────────────────────────────────────────────────────

void PN7160::loop() {
  if (!this->health_check_enabled_)
    return;

  uint32_t now = millis();
  if (now - this->last_health_check_ms_ < this->health_check_interval_)
    return;
  this->last_health_check_ms_ = now;

  if (!this->perform_health_check_()) {
    this->consecutive_failures_++;
    ESP_LOGW(TAG, "Health check failed (%d/%d)", this->consecutive_failures_, this->max_failed_checks_);

    if (this->consecutive_failures_ >= this->max_failed_checks_) {
      if (this->is_healthy_) {
        this->is_healthy_ = false;
        this->status_set_error("PN7160 health check failed");
        ESP_LOGE(TAG, "PN7160 declared unhealthy after %d consecutive failures",
                 this->consecutive_failures_);
      }

      if (this->auto_reset_on_failure_) {
        ESP_LOGW(TAG, "Attempting automatic reset...");
        this->hard_reset_();
        delay(50);
        if (this->nci_core_reset_() && this->nci_core_init_() && this->configure_rf_discovery_()) {
          this->consecutive_failures_ = 0;
          this->is_healthy_ = true;
          this->retries_ = 0;
          this->throttle_ms_ = 0;
          this->status_clear_error();
          ESP_LOGI(TAG, "PN7160 reset successful");
        } else {
          ESP_LOGE(TAG, "PN7160 reset failed");
        }
      }
    }
  } else {
    if (this->consecutive_failures_ > 0)
      ESP_LOGI(TAG, "Health check recovered after %d failures", this->consecutive_failures_);
    this->consecutive_failures_ = 0;
    if (!this->is_healthy_) {
      this->is_healthy_ = true;
      this->status_clear_error();
      ESP_LOGI(TAG, "PN7160 health restored");
    }
  }
}

// ─── Update (tag scanning) ────────────────────────────────────────────────────

void PN7160::update() {
  if (this->health_check_enabled_ && !this->is_healthy_) {
    ESP_LOGD(TAG, "Skipping scan — PN7160 unhealthy");
    return;
  }

  // Throttled backoff after failures
  if (this->throttle_ms_ > 0) {
    if (millis() - this->last_update_ms_ < this->throttle_ms_)
      return;
  }
  this->last_update_ms_ = millis();

  // Mark all binary sensors unseen
  for (auto *sensor : this->binary_sensors_)
    sensor->on_scan_end();

  // Check for tag detection notification
  std::vector<uint8_t> uid;
  if (this->read_tag_(uid)) {
    this->process_tag_(uid);
  } else {
    if (this->tag_present_)
      this->tag_removed_();
  }
}

// ─── Tag processing ───────────────────────────────────────────────────────────

void PN7160::process_tag_(const std::vector<uint8_t> &uid) {
  // Same-tag still present — silent refresh
  if (uid == this->current_uid_ && this->tag_present_) {
    for (auto *sensor : this->binary_sensors_)
      sensor->process(uid);
    return;
  }

  this->current_uid_ = uid;
  this->tag_present_ = true;

  // Format UID as hyphen-separated hex
  std::string uid_str;
  for (size_t i = 0; i < uid.size(); i++) {
    if (i > 0) uid_str += '-';
    char buf[3];
    snprintf(buf, sizeof(buf), "%02X", uid[i]);
    uid_str += buf;
  }

  ESP_LOGD(TAG, "Found new tag '%s'", uid_str.c_str());

  for (auto *sensor : this->binary_sensors_)
    sensor->process(uid);

  this->on_tag_callbacks_.call(uid_str);
}

void PN7160::tag_removed_() {
  std::string uid_str;
  for (size_t i = 0; i < this->current_uid_.size(); i++) {
    if (i > 0) uid_str += '-';
    char buf[3];
    snprintf(buf, sizeof(buf), "%02X", this->current_uid_[i]);
    uid_str += buf;
  }

  ESP_LOGD(TAG, "Tag removed: '%s'", uid_str.c_str());

  this->on_tag_removed_callbacks_.call(uid_str);

  for (auto *sensor : this->binary_sensors_) {
    if (sensor->state)
      sensor->publish_state(false);
  }

  this->tag_present_ = false;
  this->current_uid_.clear();
}

// ─── Read tag UID ─────────────────────────────────────────────────────────────

bool PN7160::read_tag_(std::vector<uint8_t> &uid) {
  // Check if IRQ is high (notification pending)
  if (!this->irq_pin_->digital_read()) {
    return false;
  }

  uint8_t gid, oid;
  std::vector<uint8_t> ntf;
  if (!this->nci_read_notification_(gid, oid, ntf)) {
    return false;
  }

  // RF_INTF_ACTIVATED_NTF contains tag UID
  if (gid != NCI_GID_RF_MGMT || oid != NCI_OID_RF_INTF_ACTIVATED) {
    return false;
  }

  // Parse UID from RF_INTF_ACTIVATED_NTF (format varies by protocol)
  // Simplified: UID typically starts at byte 10 for ISO-DEP
  if (ntf.size() < 15) {
    ESP_LOGW(TAG, "RF_INTF_ACTIVATED_NTF too short");
    return false;
  }

  uint8_t uid_len = ntf[10];
  if (ntf.size() < (size_t)(11 + uid_len)) {
    ESP_LOGW(TAG, "UID length exceeds notification size");
    return false;
  }

  uid.assign(ntf.begin() + 11, ntf.begin() + 11 + uid_len);
  return true;
}

// ─── Health check ─────────────────────────────────────────────────────────────

bool PN7160::perform_health_check_() {
  // Check if IRQ is responsive
  if (!this->wait_for_irq_(50)) {
    ESP_LOGD(TAG, "Health check: IRQ not responding");
    return false;
  }

  // Verify we can still communicate via CORE_RESET
  if (!this->nci_core_reset_()) {
    ESP_LOGD(TAG, "Health check: CORE_RESET failed");
    return false;
  }

  ESP_LOGV(TAG, "Health check passed (Model: 0x%02X, FW: %d.%d.%d)",
           this->model_id_, this->fw_major_, this->fw_minor_, this->fw_patch_);
  return true;
}

}  // namespace pn7160
}  // namespace esphome
