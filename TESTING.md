# PN7160 Enhanced Component Hardware Testing Procedure

This document outlines the validation steps required to ensure the stability and functionality of the enhanced PN7160 component across both I2C and SPI interfaces.

## 1. Requirements
- **Hardware:**
  - ESP32 (Required for sufficient GPIO count for simultaneous I2C + SPI testing).
  - **2 PN7160/PN7161 Modules:** One configured for I2C and one for SPI.
  - Each module requires an **IRQ pin** (interrupt) and a **VEN pin** (enable/reset).
  - **Tags Required:** You must have at least one of each of the following to verify formatting logic:
    - Mifare Classic 1k or 4k.
    - Mifare Ultralight or Ultralight C.
    - NTAG Series (e.g., NTAG213, 215, or 216).
- **Estimated Time:** 20–30 minutes (includes bus-switching and physical verification).

## 2. Test Configuration (Dual Interface Example)
This configuration tests the `pn7160` base logic by running both an I2C and an SPI hub simultaneously.

> **CRITICAL:** The PN7160 requires an I2C frequency of **≥ 100kHz**. ESPHome's default of 50kHz will cause IRQ timeouts (see [bug #6339](https://github.com/esphome/issues/issues/6339)). Always set `frequency: 400kHz` (or at least `100kHz`) in your `i2c:` config.

### GPIO Assignment Table (ESP32)
| Component | Bus/Pin | GPIO |
|---|---|---|
| **I2C Bus** | SDA / SCL | GPIO21 / GPIO22 |
| **SPI Bus** | CLK / MISO / MOSI | GPIO18 / GPIO19 / GPIO23 |
| **Hub 1 (I2C)** | Address | 0x28 |
| **Hub 1 (I2C)** | IRQ Pin | GPIO4 |
| **Hub 1 (I2C)** | VEN Pin | GPIO2 |
| **Hub 2 (SPI)** | CS Pin | GPIO5 |
| **Hub 2 (SPI)** | IRQ Pin | GPIO17 |
| **Hub 2 (SPI)** | VEN Pin | GPIO16 |

```yaml
i2c:
  sda: GPIO21
  scl: GPIO22
  frequency: 400kHz  # CRITICAL: Must be >= 100kHz

spi:
  clk_pin: GPIO18
  miso_pin: GPIO19
  mosi_pin: GPIO23

pn7160_i2c:
  - id: hub_i2c
    address: 0x28
    irq_pin: GPIO4
    ven_pin: GPIO2
    health_check_enabled: true
    health_check_interval: 30s
    max_failed_checks: 3
    auto_reset_on_failure: true
    on_tag:
      then:
        - logger.log:
            format: "I2C Tag: %s"
            args: ['x.c_str()']
    on_tag_removed:
      then:
        - logger.log:
            format: "I2C Tag removed: %s"
            args: ['x.c_str()']

pn7160_spi:
  - id: hub_spi
    cs_pin: GPIO5
    irq_pin: GPIO17
    ven_pin: GPIO16
    health_check_enabled: true
    health_check_interval: 30s
    max_failed_checks: 3
    auto_reset_on_failure: true
    on_tag:
      then:
        - logger.log:
            format: "SPI Tag: %s"
            args: ['x.c_str()']
    on_tag_removed:
      then:
        - logger.log:
            format: "SPI Tag removed: %s"
            args: ['x.c_str()']

binary_sensor:
  - platform: nfc
    pn7160_id: hub_i2c
    uid: "AA-BB-CC-DD"
    name: "I2C Tag"
  - platform: nfc
    pn7160_id: hub_spi
    uid: "AA-BB-CC-DD"
    name: "SPI Tag"
```

## 3. Test Cases

### Phase 1: Communication & Stability
| Test Case | Operator Action | Expected Result |
|---|---|---|
| **Mixed Boot** | **Readers:** Empty. **Action:** Power cycle ESP32. | Logs show initialization for both `pn7160_i2c` and `pn7160_spi`. |
| **I2C Recovery** | **Readers:** Empty. **Action:** Briefly disconnect I2C SDA wire. | `hub_i2c` enters backoff. `hub_spi` unaffected. Reconnect to see recovery via VEN reset. |
| **SPI Recovery** | **Readers:** Empty. **Action:** Briefly disconnect SPI CS wire. | `hub_spi` enters backoff. `hub_i2c` unaffected. Reconnect to see recovery via VEN reset. |
| **Health Check** | **Readers:** Empty. **Action:** Leave idle for 60s. | Both hubs log periodic health checks. No failures logged. |
| **I2C Frequency** | **Action:** Configure `i2c:` with `frequency: 50kHz`. | Component logs a warning about insufficient I2C frequency (bug #6339 guard). |
| **IRQ Stuck Fix** | **Action:** Place a tag, remove, repeat 6+ times rapidly. | IRQ line cleared correctly each cycle. No "stuck IRQ" lockup after ~5 reads. |

### Phase 2: Card Detection & Logic
| Test Case | Operator Action | Expected Result |
|---|---|---|
| **Cross-Bus Read** | **Action 1:** Place Mifare Classic on I2C. **Action 2:** Move same card to SPI. | Both readers correctly identify UID and trigger sensors. |
| **Anti-Collision** | **Action:** Place two different tags (e.g., NTAG and Ultralight) on I2C simultaneously. | One tag consistently read. No main thread blocking. |
| **Flapping Fix** | **Action:** Place a tag on SPI and leave it for 60s. | `on_tag` fires once. No removal/re-add logs during dwell. |
| **Dual Detection** | **Action:** Place one card on I2C and another on SPI simultaneously. | Both cards detected and held in ON state concurrently. |
| **UID Format** | **Action:** Configure binary sensor with `uid: "AA:BB:CC:DD"` (colon-separated). | Sensor matches tag correctly (colon and hyphen formats both accepted). |

### Phase 2b: `on_tag_removed` Lifecycle
| Test Case | Operator Action | Expected Result |
|---|---|---|
| **Basic Removal (I2C)** | **Action:** Place a tag on hub_i2c, wait for `on_tag` log, then remove tag. | `on_tag_removed` fires and logs the same UID that was reported by `on_tag`. |
| **Basic Removal (SPI)** | **Action:** Place a tag on hub_spi, wait for `on_tag` log, then remove tag. | `on_tag_removed` fires and logs the same UID that was reported by `on_tag`. |
| **UID Match** | **Action:** Use a tag with a known UID (e.g. `04-A3-B2-C1-D4-E5-F6`). Present then remove. | The UID string in `on_tag_removed` exactly matches the UID string from `on_tag`. |
| **Binary Sensor OFF** | **Action:** Present tag (binary sensor turns ON), then remove tag. | Binary sensor transitions to OFF state after tag is removed. No spurious ON/OFF flicker while tag is present. |
| **TTL Delay** | **Action:** Set `tag_ttl: 1s`. Present tag, remove, observe logs. | `on_tag_removed` fires approximately 1 second after the tag leaves the field (not immediately). |
| **Short TTL** | **Action:** Set `tag_ttl: 100ms`. Present tag, remove, observe logs. | `on_tag_removed` fires quickly (~100ms) after removal. Binary sensor reflects the faster timeout. |
| **Rapid Place/Remove** | **Action:** Rapidly place and remove the same tag 5+ times (< 1s per cycle). | Each cycle produces exactly one `on_tag` and one `on_tag_removed`. No missed events, no duplicate firings. |
| **Cross-Bus Removal** | **Action:** Present a tag on I2C, confirm `on_tag` on `hub_i2c`, remove tag. | Only `hub_i2c` fires `on_tag_removed`. `hub_spi` is unaffected and logs nothing. |
| **No Ghost Removal** | **Action:** Leave a tag stationary on the reader for 120s. | `on_tag_removed` does **not** fire during the dwell period. The tag stays in ON state. |

### Phase 3: NDEF Operations
| Test Case | Operator Action | Expected Result |
|---|---|---|
| **NDEF Read/Write** | **Action:** Place NTAG on I2C. Observe write verification logs. | Successful NDEF interaction logged. Main thread responsive. |
| **Mifare Format** | **Action:** Place "virgin" Mifare Classic on SPI. | Component authenticates, formats, and writes URI successfully. |
| **Card Emulation** | **Action:** Use `tag.emulation_on` and scan with a phone. | Phone reads the configured NDEF emulation message. |
| **Write Mode** | **Action:** Use `tag.set_write_message` + `tag.set_write_mode`, then present NTAG. | `on_finished_write` triggers; tag contains new NDEF data. |
| **Clean Mode** | **Action:** Use `tag.set_clean_mode`, then present a written NTAG. | Tag NDEF data erased. `on_tag` fires with empty payload. |

### Phase 4: Health Check & Auto-Recovery
| Test Case | Operator Action | Expected Result |
|---|---|---|
| **VEN Reset Trigger** | **Action:** Simulate 3+ consecutive health check failures (e.g., disconnect VEN briefly). | Component logs `auto_reset_on_failure` and toggles VEN pin to hard-reset the chip. |
| **Recovery After Reset** | **Action:** After a VEN reset, place a tag. | Component re-initializes and resumes normal tag detection. |
| **Disabled Health Check** | **Action:** Set `health_check_enabled: false`. | No health check logs appear. Component operates in polling-only mode. |

### Phase 5: Responsiveness & Main-Loop Non-Blocking
The PN7160 component must not stall the ESP32 main loop. This phase verifies that all NFC operations are non-blocking and that other firmware tasks remain responsive throughout.

**Setup:** Add a `interval:` component that logs every 500ms alongside the PN7160 config. This acts as a "heartbeat" to show the main loop is alive.

```yaml
interval:
  - interval: 500ms
    then:
      - logger.log: "Heartbeat"
```

| Test Case | Operator Action | Expected Result |
|---|---|---|
| **Idle Heartbeat** | **Action:** Boot with no tag present. Watch serial log for 30s. | "Heartbeat" logs appear every ~500ms without gaps or delays throughout. |
| **Tag Detection Latency** | **Action:** Present a tag to the reader. Note the time between physical contact and the `on_tag` log line. | Tag detected within 1 second of presentation. Heartbeat continues uninterrupted during detection. |
| **Removal Detection Latency** | **Action:** Remove a tag and note the time between physical removal and the `on_tag_removed` log line. | Removal detected within `tag_ttl` + 1 polling cycle. Heartbeat continues without gaps during removal. |
| **NDEF Read Non-Blocking** | **Action:** Place an NTAG with an NDEF payload. Watch the heartbeat while the read completes. | Heartbeat keeps firing at ~500ms during the entire NDEF read. No `took a long time` or `delay()` warnings in logs. |
| **NDEF Write Non-Blocking** | **Action:** Trigger a write via `tag.set_write_mode`, then present an NTAG. Watch the heartbeat. | Heartbeat continues during NDEF write. `on_finished_write` fires after write completes. No blocking warnings. |
| **WiFi Keepalive** | **Action:** Actively poll tags (place/remove 10 times) while monitoring WiFi connection (RSSI or ping). | WiFi remains connected throughout. No WiFi disconnect or reconnect events logged. |
| **Dual-Bus Polling Load** | **Action:** Run both `hub_i2c` and `hub_spi` simultaneously, placing/removing tags on each. Watch the heartbeat. | Heartbeat remains steady at ~500ms. Both hubs respond without starving each other. |
| **Long-Running Stability** | **Action:** Leave both hubs polling continuously for 10 minutes with tags alternating. | No crashes, reboots, or `took a long time` warnings. Heartbeat uninterrupted throughout. |

## 4. Success Criteria
- [x] **I2C Hardware:** PN7160 initializes and reads tags reliably over I2C at ≥ 100kHz without IRQ timeouts.
- [x] **SPI Hardware:** PN7160 initializes and reads tags reliably over SPI without timeouts or data corruption.
- [x] **Dual Bus Operation:** Simultaneous I2C and SPI readers function correctly on the same ESP32.
- [x] **IRQ Fix:** No "stuck IRQ" lockup after repeated reads (~5+ cycles).
- [x] **I2C Frequency Guard:** Warning logged when `i2c:` frequency is below 100kHz.
- [x] **Health Check:** Periodic health checks run at configured interval with correct pass/fail logging.
- [x] **Auto-Recovery:** VEN pin toggled for hard reset after `max_failed_checks` failures.
- [x] **Non-blocking:** No `delay()` or `took a long time` warnings during normal polling.
- [x] **Format Compatibility:** Both `AA-BB` and `AA:BB` UID formats accepted in YAML binary sensor config.
- [x] **Card Emulation:** NDEF emulation message readable by an external NFC reader.
- [x] **on_tag_removed:** Callback fires with the correct matching UID every time a tag leaves the field.
- [x] **Binary Sensor OFF:** Binary sensor correctly transitions to OFF state when a tag is removed.
- [x] **TTL Accuracy:** `tag_ttl` setting correctly controls the delay between physical removal and `on_tag_removed`.
- [x] **No Ghost Removals:** `on_tag_removed` does not fire spuriously while a tag remains on the reader.
- [x] **Rapid Cycle Stability:** Each place/remove cycle produces exactly one `on_tag` and one `on_tag_removed` with no missed or duplicate events.
- [x] **Main-Loop Responsiveness:** Heartbeat interval component fires at the expected rate throughout all NFC operations without gaps.
- [x] **Tag Detection Latency:** Tags detected within 1 second of presentation; removal detected within `tag_ttl` + 1 polling cycle.
- [x] **WiFi Keepalive:** WiFi connection remains stable during active NFC polling on both buses.
- [ ] **Mifare Authentication:** Resolve intermittent failures with non-standard keys.
- [ ] **Robust Counterfeit Detection:** Module correctly identifies emulated clones using hardware-level diagnostic checks.
- [ ] **NTAG216 Stability:** NDEF writing completes without timing out on high-capacity NTAG216 modules.
