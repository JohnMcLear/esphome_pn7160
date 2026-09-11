# PN7160 Technical Specifications & RF Tuning Guide

This document summarizes technical specifications and RF tuning parameters for the NXP PN7160 NFC Controller, based on NXP Datasheet (Rev 4.1) and AN13219 (Antenna Design Guide).

## 1. Device Overview
- **Interface:** I2C (up to 3.4 MBaud) and SPI (up to 7 MBaud).
- **Supply Voltage (VBAT):** 2.8V to 5.5V.
- **Transmitter Voltage (TXLDO):** Integrated LDO provides up to 5.25V (max 250mA).
- **Output Power:** Up to 1.3W.
- **NCI Version:** NCI 2.0 compliant.
- **Protocols:** ISO/IEC 14443 A/B, FeliCa, ISO/IEC 15693, MIFARE Classic/Ultralight, P2P, Card Emulation.

## 2. RF Performance Characteristics
- **Reader Mode Sensitivity:** Improved sensitivity compared to PN7150.
- **Card Mode Sensitivity:** 20 mV(p-p).
- **Dynamic Power Control (DPC):** Automatically adjusts transmitter power based on antenna loading (e.g., proximity to metal).
- **Dynamic Load Modulation Amplitude (DLMA):** Optimizes LMA based on external field strength for better card emulation range.

## 3. Critical RF Tuning Registers (via NCI Proprietary Tag 0xA0 0D)
RF tuning is performed using the `CORE_SET_CONFIG_CMD` with proprietary tag `0xA0 0D`.

### A. Receiver Sensitivity Tuning
| Register | Offset | Description |
| :--- | :--- | :--- |
| `CLIF_ANA_RX_REG` | `0x44` | Controls Analog Gain (AGC). Higher gain increases sensitivity to weak signals from small tags. |
| `CLIF_SIGPRO_RM_CONFIG1_REG` | `0x2D` | Sets the Minimum Level Detection threshold (`MIN_LEVEL`). Lowering this makes the receiver more sensitive. |

### B. Transmitter Performance
| Register | Offset | Description |
| :--- | :--- | :--- |
| `CLIF_ANA_TX_AMPLITUDE_REG` | `0x42` | Adjusts transmitter conductance and residual carrier level. Primary for Card Mode LMA tuning but affects field strength. |

### C. Common Transition IDs (for Tag 0xA0 0D)
Transition IDs define *when* the setting is applied:
- `0x01`: Reader Mode (General)
- `0x3C`: ISO14443-A (106 kbps) - Reader
- `0x4C`: ISO14443-B (106 kbps) - Reader
- `0x20`: ISO15693 - Reader
- `0x5E`: FeliCa (212 kbps) - Reader

## 4. Maintenance & Diagnostic Commands
Commands used during the tuning process (Group `2F`):

- **Measure AGC Value:** `2F 3D 04 02 C8 60 03`
  - *Expected Result:* AGC value should be between 500 and 800 (0x01F4 - 0x0320).
- **Antenna Self-Test:** `2F 3D 02 01 80` (Measures $I_{TVDD}$).
- **DPC Info:** `2F 3F 03 03 00 00` (Returns real-time current and LUT index).

## 5. Antenna Matching Recommendations
- **Target Impedance (Asymmetrical):** ~20 Ohm.
- **Target Impedance (Symmetrical + DPC):** ~16 Ohm.
- **Driver Current ($I_{TVDD}$):** Must not exceed 250mA. Recommended range: 160mA - 230mA.
- **Q-Factor:** Recommended value is ~20 for optimal bandwidth/performance balance.
- **$R_{rx}$ Resistors:** Typically 2.2kOhm (range 1k - 10k).
- **$C_{rx}$ Capacitors:** Typically 1nF.

## 6. Power Management (PMU_CFG Tag 0xA0 0E)
The PN7160 uses tag `0xA0 0E` for PMU configuration (11 bytes).
Current implementation in ESPHome sets TXLDO to 5.0V:
`0x01, 0xA0, 0x0E, 11, 0x11, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xD0, 0x0C`
*(Note: 0xFF in the 11th byte position corresponds to 5.0V).*

## 7. Supply Requirements, and the failure they cause

The transmitter regulator is the part most likely to bite you, and the symptom
does not look like a power problem.

### What the chip needs

| Rail | Requirement |
|---|---|
| VBAT | 2.8V to 5.5V |
| VUP / TVDD | Must be able to feed TXLDO at the configured output |
| TXLDO output | Set to 5.0V by this component, see section 6 |
| Driver current | Up to 250mA, so the supply must carry it during RF bursts |

Section 6 sets TXLDO to 5.0V unconditionally. That is the right default for
range, but it means the board has to be able to deliver it. A supply that merely
holds 3.3V steady, or one that sags under a 250mA burst, will not.

### 3.3V is not required when 5V is present

On a PN7161 there is no need to feed 3.3V/VDD separately if 5V is available.
Boards that bridge 3.3V to 5V to "help" can make things worse rather than better.
At least one reported failure was cured by removing exactly such a bridge.

### The symptom, and why it is misleading

A failing regulator does **not** report itself as a power problem. It looks like
a dead device:

```
[E][pn7160:718]: Too many initialization failures -- check device connections
[E][component:119]: Component pn7160 was marked as failed.
```

That message sends people to check wiring and I2C addresses, which is the wrong
place. Underneath, the chip is emitting this on repeat:

```
61 23 00
```

A control notification, group RF, OID 0x23, no payload. NXP UM11495 documents
0x23 as TxLdo failing to start, from a missing or bad supply on VUP/TVDD, or a
bad clock or power configuration.

**This component now decodes that** and logs it at ERROR:

```
[E][pn7160:...]: RF transmitter regulator did not start (TxLdo). Check the
VUP/TVDD supply and the clock/power configuration
```

Upstream still drops it into a verbose-level default branch, so on stock ESPHome
you will not see it at all unless logging is turned right up.

### Two unrelated causes, one symptom

Worth knowing, because they are easy to confuse and the fix for one does nothing
for the other:

| Cause | Tell | Fix |
|---|---|---|
| I2C bus below 100kHz | IRQ timeouts during init | Set `frequency: 100kHz` or higher |
| Supply cannot start TXLDO | `61 23 00` on repeat | Fix VUP/TVDD, remove any 3.3V to 5V bridge |

Both produce "Too many initialization failures". See esphome/issues#6339, where
the original report was the first and a later report was the second.

