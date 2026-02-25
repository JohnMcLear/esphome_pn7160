# ESPHome PN7160/PN7161 Component (Enhanced)

[![ESPHome Compile](../../actions/workflows/compile.yml/badge.svg)](../../actions/workflows/compile.yml)

An enhanced external ESPHome component for the PN7160/PN7161 NFC controller. Drop-in compatible with ESPHome's native `pn7160_spi` / `pn7160_i2c` API, with critical bug fixes and health checking.

## Improvements Over Native Component

### Bug Fixes

| Issue | Fix |
|---|---|
| **#6339 — I2C 50kHz IRQ timeout** at startup | Force I2C frequency >= 100kHz with validation warning |
| **IRQ blocking after ~5 reads** — IRQ line stuck HIGH | IRQ state clearing + hard reset via VEN pin |
| **No health check/recovery** — component fails permanently | Health check with auto-recovery via VEN toggle |

### New Features

- **Health check** with auto-reset: periodically validates NCI communication, resets via VEN pin if needed
- **IRQ handling fixes**: Exponential backoff polling + stuck IRQ detection/clearing
- **I2C frequency validation**: Warns if <100kHz configured (prevents bug #6339)
- Both SPI and I2C variants share common base with fixes

---

## Installation

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/JohnMcLear/esphome_pn7160
    components: [pn7160, pn7160_spi, pn7160_i2c]
    refresh: 1d
```

---

## Over I2C (IMPORTANT: Frequency >= 100kHz Required)

```yaml
# CRITICAL: PN7160 requires I2C frequency >= 100kHz
# ESPHome's default 50kHz will cause IRQ timeouts (bug #6339)
i2c:
  sda: GPIO21
  scl: GPIO22
  frequency: 400kHz  # ← Must be >= 100kHz

pn7160_i2c:
  id: pn7160_board
  address: 0x28
  irq_pin: GPIO18   # Required for PN7160
  ven_pin: GPIO19   # Required for PN7160
  update_interval: 1s
  on_tag:
    then:
      - logger.log:
          format: "Tag: %s"
          args: ['x.c_str()']
  on_tag_removed:
    then:
      - logger.log:
          format: "Tag removed: %s"
          args: ['x.c_str()']

binary_sensor:
  - platform: nfc
    name: "My NFC Tag"
    uid: "04-A3-B2-C1-D4-E5-F6"
```

### I2C Configuration Variables

- **`address`** (*Optional*, default `0x28`): I2C address (configurable via HIF pins: 0x28-0x2B).
- **`irq_pin`** (**Required**): IRQ (interrupt) pin — signals when data is ready to read.
- **`ven_pin`** (**Required**): VEN (enable) pin — powers device on/off, used for hard reset.
- **`update_interval`** (*Optional*, default `1s`): How often to check for tags.
- **`on_tag`** / **`on_tag_removed`**: Automation triggers (variable `x` is UID string).
- **`health_check_enabled`** (*Optional*, default `true`): Enable periodic health checks.
- **`health_check_interval`** (*Optional*, default `60s`): Health check frequency.
- **`max_failed_checks`** (*Optional*, default `3`): Failures before declaring unhealthy.
- **`auto_reset_on_failure`** (*Optional*, default `true`): Auto-reset via VEN pin on health failure.
- **`i2c_id`** (*Optional*): Manually specify I2C bus ID.
- **`id`** (*Optional*): Component ID.

**CRITICAL**: Your `i2c:` config **must** specify `frequency: 100kHz` or higher. The ESPHome default of 50kHz will cause IRQ timeouts (see bug #6339).

---

## Over SPI

```yaml
spi:
  clk_pin: GPIO18
  miso_pin: GPIO19
  mosi_pin: GPIO23

pn7160_spi:
  id: pn7160_board
  cs_pin: GPIO5
  irq_pin: GPIO17   # Required for PN7160
  ven_pin: GPIO16   # Required for PN7160
  update_interval: 1s
  on_tag:
    then:
      - homeassistant.tag_scanned: !lambda 'return x;'

binary_sensor:
  - platform: nfc
    name: "My NFC Tag"
    uid: "04-A3-B2-C1-D4-E5-F6"
```

### SPI Configuration Variables

All the same options as I2C above, plus:

- **`cs_pin`** (**Required**): Chip select pin.
- **`spi_id`** (*Optional*): Manually specify SPI bus ID.

---

## `pn7160` Binary Sensor

```yaml
binary_sensor:
  - platform: nfc
    pn7160_id: pn7160_board   # required if multiple PN7160 instances
    name: "My Tag"
    uid: "04-A3-B2-C1-D4-E5-F6"  # hyphen or colon separated hex
```

### Binary Sensor Configuration Variables

- **`uid`** (**Required**): UID to match. Hyphen-separated hex: `04-A3-B2-C1`. Colon-separated also accepted: `04:A3:B2:C1`.
- **`pn7160_id`** (*Optional*): ID of the `pn7160_spi` or `pn7160_i2c` hub.
- All other options from [Binary Sensor](https://esphome.io/components/binary_sensor/).

---

## Setting Up Tags

Same as PN7160 — configure without binary sensors first, scan a tag, copy the UID from the logs:

```
Found new tag '04-A3-B2-C1-D4-E5-F6'
```

Then add a `binary_sensor:` entry with that UID.

---

## Health Check

Unlike PN7160 (which uses `GetFirmwareVersion`), PN7160 health check uses:
- **`CORE_RESET_CMD`** to verify NCI communication
- Reads **`CORE_RESET_NTF`** to confirm IC responds
- On repeated failures, toggles **VEN pin** (hard reset) to recover

This resolves IRQ blocking and communication freeze bugs.

---

## Known Issues (Upstream) Addressed Here

| ESPHome Issue | Description | Fix in this component |
|---|---|---|
| [#6339](https://github.com/esphome/issues/issues/6339) | I2C 50kHz causes IRQ timeout at startup | I2C frequency validation, 100kHz minimum enforced |
| IRQ blocking (forum) | After ~5 reads, IRQ stuck HIGH, no more reads | IRQ clearing + hard reset via VEN |
| No recovery | Component marks itself failed permanently | Health check with VEN pin auto-reset |

---

## Differences from PN7160

| Feature | PN7160 | PN7160 |
|---|---|---|
| Protocol | Raw PN7160 commands | NCI 2.0 (standard) |
| Required pins | RSTPD_N (optional, SPI only) | IRQ + VEN (both required) |
| I2C address | 0x24 (fixed) | 0x28-0x2B (configurable via HIF) |
| I2C frequency | 50kHz OK | **100kHz minimum** |
| Capabilities | Reader only | Reader + card emulation |
| Hard reset | Optional via RSTPD_N | Required via VEN pin |

---

## Compatibility

- ESPHome 2024.x and later
- ESP32 (Arduino & ESP-IDF frameworks)
- ESP8266 (Arduino framework)
- PN7160/PN7161 NFC controllers over SPI or I2C

---

## License

MIT (same as ESPHome)
