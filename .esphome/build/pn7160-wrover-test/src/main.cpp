// Auto generated code by esphome
// ========== AUTO GENERATED INCLUDE BLOCK BEGIN ===========
#include "esphome.h"
using namespace esphome;
using std::isnan;
using std::min;
using std::max;
using namespace binary_sensor;
static logger::Logger *logger_logger_id;
static preferences::IntervalSyncer *preferences_intervalsyncer_id;
using namespace i2c;
static i2c::IDFI2CBus *i2c_idfi2cbus_id;
static pn7160_i2c::PN7160I2C *pn7160_board;
static esp32::ESP32InternalGPIOPin *esp32_esp32internalgpiopin_id;
static esp32::ESP32InternalGPIOPin *esp32_esp32internalgpiopin_id_2;
static nfc::NfcOnTagTrigger *nfc_nfcontagtrigger_id;
static Automation<std::string, nfc::NfcTag> *automation_id;
static StatelessLambdaAction<std::string, nfc::NfcTag> *lambdaaction_id;
static nfc::NfcOnTagTrigger *nfc_nfcontagtrigger_id_2;
static Automation<std::string, nfc::NfcTag> *automation_id_2;
static StatelessLambdaAction<std::string, nfc::NfcTag> *lambdaaction_id_2;
static nfc::NfcTagBinarySensor *nfc_nfctagbinarysensor_id;
// ========== AUTO GENERATED INCLUDE BLOCK END ==========="

void setup() {
  // ========== AUTO GENERATED CODE BEGIN ===========
  // esphome:
  //   name: pn7160-wrover-test
  //   friendly_name: PN7160 Wrover Test
  //   min_version: 2026.2.4
  //   build_path: build/pn7160-wrover-test
  //   platformio_options: {}
  //   environment_variables: {}
  //   includes: []
  //   includes_c: []
  //   libraries: []
  //   name_add_mac_suffix: false
  //   debug_scheduler: false
  //   areas: []
  //   devices: []
  App.pre_setup("pn7160-wrover-test", "PN7160 Wrover Test", false);
  // binary_sensor:
  // logger:
  //   level: VERY_VERBOSE
  //   id: logger_logger_id
  //   baud_rate: 115200
  //   tx_buffer_size: 512
  //   deassert_rts_dtr: false
  //   task_log_buffer_size: 768
  //   hardware_uart: UART0
  //   logs: {}
  //   runtime_tag_levels: false
  logger_logger_id = new logger::Logger(115200, 512);
  logger_logger_id->create_pthread_key();
  logger_logger_id->init_log_buffer(768);
  logger_logger_id->set_log_level(ESPHOME_LOG_LEVEL_VERY_VERBOSE);
  logger_logger_id->set_uart_selection(logger::UART_SELECTION_UART0);
  logger_logger_id->pre_setup();
  logger_logger_id->set_component_source(LOG_STR("logger"));
  App.register_component(logger_logger_id);
  // preferences:
  //   id: preferences_intervalsyncer_id
  //   flash_write_interval: 60s
  preferences_intervalsyncer_id = new preferences::IntervalSyncer();
  preferences_intervalsyncer_id->set_write_interval(60000);
  preferences_intervalsyncer_id->set_component_source(LOG_STR("preferences"));
  App.register_component(preferences_intervalsyncer_id);
  // i2c:
  //   sda: 32
  //   scl: 33
  //   frequency: 400000.0
  //   scan: false
  //   id: i2c_idfi2cbus_id
  //   sda_pullup_enabled: true
  //   scl_pullup_enabled: true
  i2c_idfi2cbus_id = new i2c::IDFI2CBus();
  i2c_idfi2cbus_id->set_component_source(LOG_STR("i2c"));
  App.register_component(i2c_idfi2cbus_id);
  i2c_idfi2cbus_id->set_sda_pin(32);
  i2c_idfi2cbus_id->set_sda_pullup_enabled(true);
  i2c_idfi2cbus_id->set_scl_pin(33);
  i2c_idfi2cbus_id->set_scl_pullup_enabled(true);
  i2c_idfi2cbus_id->set_frequency(400000);
  i2c_idfi2cbus_id->set_scan(false);
  // esp32:
  //   board: esp32dev
  //   framework:
  //     type: esp-idf
  //     version: 5.5.2
  //     sdkconfig_options: {}
  //     log_level: ERROR
  //     advanced:
  //       compiler_optimization: SIZE
  //       enable_idf_experimental_features: false
  //       enable_lwip_assert: true
  //       ignore_efuse_custom_mac: false
  //       ignore_efuse_mac_crc: false
  //       enable_lwip_dhcp_server: false
  //       enable_lwip_mdns_queries: true
  //       enable_lwip_bridge_interface: false
  //       enable_lwip_tcpip_core_locking: true
  //       enable_lwip_check_thread_safety: true
  //       disable_libc_locks_in_iram: true
  //       disable_vfs_support_termios: true
  //       disable_vfs_support_select: true
  //       disable_vfs_support_dir: true
  //       freertos_in_iram: false
  //       ringbuf_in_iram: false
  //       heap_in_iram: false
  //       execute_from_psram: false
  //       loop_task_stack_size: 8192
  //       enable_ota_rollback: false
  //       use_full_certificate_bundle: false
  //       include_builtin_idf_components: []
  //       disable_debug_stubs: true
  //       disable_ocd_aware: true
  //       disable_usb_serial_jtag_secondary: true
  //       disable_dev_null_vfs: true
  //       disable_mbedtls_peer_cert: true
  //       disable_mbedtls_pkcs7: true
  //       disable_regi2c_in_iram: true
  //       disable_fatfs: true
  //     components: []
  //     platform_version: https:github.com/pioarduino/platform-espressif32/releases/download/55.03.37/platform-espressif32.zip
  //     source: pioarduino/framework-espidf@https:github.com/pioarduino/esp-idf/releases/download/v5.5.2/esp-idf-v5.5.2.tar.xz
  //   flash_size: 4MB
  //   variant: ESP32
  //   cpu_frequency: 160MHZ
  // external_components:
  //   - source:
  //       path: /home/jose/esphome_pn7160/components
  //       type: local
  //     components:
  //       - pn7160
  //       - pn7160_spi
  //       - pn7160_i2c
  //     refresh: 1d
  // pn7160_i2c:
  //   id: pn7160_board
  //   address: 0x28
  //   irq_pin:
  //     number: 26
  //     mode:
  //       input: true
  //       output: false
  //       open_drain: false
  //       pullup: false
  //       pulldown: false
  //     id: esp32_esp32internalgpiopin_id
  //     inverted: false
  //     ignore_pin_validation_error: false
  //     ignore_strapping_warning: false
  //     drive_strength: 20.0
  //   ven_pin:
  //     number: 27
  //     mode:
  //       output: true
  //       input: false
  //       open_drain: false
  //       pullup: false
  //       pulldown: false
  //     id: esp32_esp32internalgpiopin_id_2
  //     inverted: false
  //     ignore_pin_validation_error: false
  //     ignore_strapping_warning: false
  //     drive_strength: 20.0
  //   sensitivity: high
  //   on_tag:
  //     - then:
  //         - logger.log:
  //             format: 'Tag scanned: %s'
  //             args:
  //               - !lambda |-
  //                 x.c_str()
  //             tag: main
  //             logger_id: logger_logger_id
  //             level: DEBUG
  //           type_id: lambdaaction_id
  //       automation_id: automation_id
  //       trigger_id: nfc_nfcontagtrigger_id
  //   on_tag_removed:
  //     - then:
  //         - logger.log:
  //             format: 'Tag removed: %s'
  //             args:
  //               - !lambda |-
  //                 x.c_str()
  //             tag: main
  //             logger_id: logger_logger_id
  //             level: DEBUG
  //           type_id: lambdaaction_id_2
  //       automation_id: automation_id_2
  //       trigger_id: nfc_nfcontagtrigger_id_2
  //   health_check_enabled: true
  //   health_check_interval: 60s
  //   max_failed_checks: 3
  //   auto_reset_on_failure: true
  //   i2c_id: i2c_idfi2cbus_id
  pn7160_board = new pn7160_i2c::PN7160I2C();
  pn7160_board->set_i2c_bus(i2c_idfi2cbus_id);
  pn7160_board->set_i2c_address(0x28);
  pn7160_board->set_component_source(LOG_STR("pn7160"));
  App.register_component(pn7160_board);
  esp32_esp32internalgpiopin_id = new esp32::ESP32InternalGPIOPin();
  esp32_esp32internalgpiopin_id->set_pin(::GPIO_NUM_26);
  esp32_esp32internalgpiopin_id->set_drive_strength(::GPIO_DRIVE_CAP_2);
  esp32_esp32internalgpiopin_id->set_flags(gpio::Flags::FLAG_INPUT);
  pn7160_board->set_irq_pin(esp32_esp32internalgpiopin_id);
  esp32_esp32internalgpiopin_id_2 = new esp32::ESP32InternalGPIOPin();
  esp32_esp32internalgpiopin_id_2->set_pin(::GPIO_NUM_27);
  esp32_esp32internalgpiopin_id_2->set_drive_strength(::GPIO_DRIVE_CAP_2);
  esp32_esp32internalgpiopin_id_2->set_flags(gpio::Flags::FLAG_OUTPUT);
  pn7160_board->set_ven_pin(esp32_esp32internalgpiopin_id_2);
  pn7160_board->set_health_check_enabled(true);
  pn7160_board->set_health_check_interval(60000);
  pn7160_board->set_max_failed_checks(3);
  pn7160_board->set_auto_reset_on_failure(true);
  pn7160_board->set_sensitivity(pn7160::PN7160_SENSITIVITY_HIGH);
  nfc_nfcontagtrigger_id = new nfc::NfcOnTagTrigger();
  pn7160_board->register_ontag_trigger(nfc_nfcontagtrigger_id);
  automation_id = new Automation<std::string, nfc::NfcTag>(nfc_nfcontagtrigger_id);
  lambdaaction_id = new StatelessLambdaAction<std::string, nfc::NfcTag>([](std::string x, nfc::NfcTag tag) -> void {
      ESP_LOGD("main", "Tag scanned: %s", x.c_str());
  });
  automation_id->add_actions({lambdaaction_id});
  nfc_nfcontagtrigger_id_2 = new nfc::NfcOnTagTrigger();
  pn7160_board->register_ontagremoved_trigger(nfc_nfcontagtrigger_id_2);
  automation_id_2 = new Automation<std::string, nfc::NfcTag>(nfc_nfcontagtrigger_id_2);
  lambdaaction_id_2 = new StatelessLambdaAction<std::string, nfc::NfcTag>([](std::string x, nfc::NfcTag tag) -> void {
      ESP_LOGD("main", "Tag removed: %s", x.c_str());
  });
  automation_id_2->add_actions({lambdaaction_id_2});
  // binary_sensor.nfc:
  //   platform: nfc
  //   name: Test Badge
  //   uid: 04-A3-B2-C1-D4-E5-F6
  //   disabled_by_default: false
  //   id: nfc_nfctagbinarysensor_id
  //   nfcc_id: pn7160_board
  nfc_nfctagbinarysensor_id = new nfc::NfcTagBinarySensor();
  App.register_binary_sensor(nfc_nfctagbinarysensor_id);
  nfc_nfctagbinarysensor_id->set_name("Test Badge", 1157280365);
  nfc_nfctagbinarysensor_id->set_trigger_on_initial_state(false);
  nfc_nfctagbinarysensor_id->set_component_source(LOG_STR("nfc.binary_sensor"));
  App.register_component(nfc_nfctagbinarysensor_id);
  nfc_nfctagbinarysensor_id->set_parent(pn7160_board);
  pn7160_board->register_listener(nfc_nfctagbinarysensor_id);
  nfc_nfctagbinarysensor_id->set_uid({0x04, 0xA3, 0xB2, 0xC1, 0xD4, 0xE5, 0xF6});
  // =========== AUTO GENERATED CODE END ============
  App.setup();
}

void loop() {
  App.loop();
}
