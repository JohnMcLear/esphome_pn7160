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
  tag_ttl: 500ms    # Optional, default 250ms
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
- **`dwl_req_pin`** (*Optional*): Pin used for firmware download mode.
- **`wkup_req_pin`** (*Optional*): Pin used to wake up the chip.
- **`tag_ttl`** (*Optional*, default `250ms`): Time-to-live for a tag to be considered "present" since last seen.
- **`emulation_message`** (*Optional*): NDEF message to use for card emulation.
- **`on_tag`** / **`on_tag_removed`**: Automation triggers (variable `x` is UID string).
- **`on_emulated_tag_scan`**: Trigger when an emulated tag is scanned by an external reader.
- **`on_finished_write`**: Trigger when a tag write operation completes.
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
  on_tag:
    then:
      - homeassistant.tag_scanned: !lambda 'return x;'
```

### SPI Configuration Variables

All the same options as I2C above, plus:

- **`cs_pin`** (**Required**): Chip select pin.
- **`spi_id`** (*Optional*): Manually specify SPI bus ID.

---

## Actions

### `tag.set_emulation_message`
Sets the NDEF message to be used during card emulation.
```yaml
on_...:
  then:
    - tag.set_emulation_message:
        id: pn7160_board
        message: "https://esphome.io"
```

### `tag.emulation_on` / `tag.emulation_off`
Enables or disables card emulation mode.
```yaml
on_...:
  then:
    - tag.emulation_on: pn7160_board
```

### `tag.polling_on` / `tag.polling_off`
Enables or disables tag polling (reader mode).
```yaml
on_...:
  then:
    - tag.polling_off: pn7160_board
```

### `tag.set_write_message` / `tag.set_write_mode`
Prepares a message and enters write mode.
```yaml
on_...:
  then:
    - tag.set_write_message:
        id: pn7160_board
        message: "New Tag Content"
    - tag.set_write_mode: pn7160_board
```

### `tag.set_read_mode` / `tag.set_clean_mode` / `tag.set_format_mode`
Sets the operational mode for the next tag interaction.
- `set_read_mode`: (Default) Reads NDEF data from the tag.
- `set_clean_mode`: Erases NDEF data from the tag.
- `set_format_mode`: Formats the tag for NDEF.

---

## Conditions

### `pn7160.is_writing`
Returns true if the component is currently waiting to write to a tag.
```yaml
if:
  condition:
    pn7160.is_writing: pn7160_board
  then:
    ...
```

---

## `pn7160` Binary Sensor

```yaml
binary_sensor:
  - platform: nfc
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

## Health Check Implementation

The PN7160 health check periodically validates the internal state machine.
- It detects if the chip is stuck in transient initialization states (RESET, INIT, CONFIG, etc.).
- It monitors for "stuck" IRQ states where the chip fails to progress.
- If failures exceed `max_failed_checks`, it performs a hard reset by toggling the **VEN pin**.

---

## Known Issues (Upstream) Addressed Here

| ESPHome Issue | Description | Fix in this component |
|---|---|---|
| [#6339](https://github.com/esphome/issues/issues/6339) | I2C 50kHz causes IRQ timeout at startup | I2C frequency validation, 100kHz minimum enforced |
| IRQ blocking (forum) | After ~5 reads, IRQ stuck HIGH, no more reads | IRQ clearing + hard reset via VEN |
| No recovery | Component marks itself failed permanently | Health check with VEN pin auto-reset |

---

## Differences from Native ESPHome Component

| Feature | Native `pn7160` | This Component |
|---|---|---|
| Health Check | None | **Periodic state validation** |
| Auto-Recovery | None | **VEN pin hard reset** |
| I2C Freq Check | None | **Safety warning if < 100kHz** |
| Card Emulation | Minimal | **Full NDEF message emulation** |
| Tag TTL | Fixed | **Configurable via `tag_ttl`** |
| Stuck IRQ fix | No | **Yes** |

---

## Compatibility

- ESPHome 2024.x and later
- ESP32 (Arduino & ESP-IDF frameworks)
- ESP8266 (Arduino framework)
- PN7160/PN7161 NFC controllers over SPI or I2C

---

## License

MIT (same as ESPHome)
