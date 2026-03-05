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
