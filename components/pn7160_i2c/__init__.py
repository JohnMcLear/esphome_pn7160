import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.components import i2c
from esphome.const import (
    CONF_ID,
    CONF_ON_TAG,
    CONF_ON_TAG_REMOVED,
    CONF_TRIGGER_ID,
    CONF_UPDATE_INTERVAL,
)
from .. import pn7160  # Import base pn7160 component

PN7160OnEmulatedTagScanTrigger = cg.esphome_ns.namespace("pn7160").class_(
    "PN7160OnEmulatedTagScanTrigger",
    automation.Trigger.template(),  # no parameters
)

# Import constants from parent pn7160 component
CONF_DWL_REQ_PIN = "dwl_req_pin"
CONF_IRQ_PIN = "irq_pin"
CONF_VEN_PIN = "ven_pin"
CONF_WKUP_REQ_PIN = "wkup_req_pin"
CONF_EMULATION_MESSAGE = "emulation_message"
CONF_TAG_TTL = "tag_ttl"
CONF_ON_EMULATED_TAG_SCAN = "on_emulated_tag_scan"

# Health check specific options
CONF_HEALTH_CHECK_ENABLED = "health_check_enabled"
CONF_HEALTH_CHECK_INTERVAL = "health_check_interval"
CONF_MAX_FAILED_CHECKS = "max_failed_checks"
CONF_AUTO_RESET_ON_FAILURE = "auto_reset_on_failure"

DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["pn7160"]

pn7160_i2c_ns = cg.esphome_ns.namespace("pn7160_i2c")
PN7160I2C = pn7160_i2c_ns.class_("PN7160I2C", pn7160.PN7160, i2c.I2CDevice)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PN7160I2C),
            # Required pins
            cv.Required(CONF_IRQ_PIN): pins.gpio_input_pin_schema,
            cv.Required(CONF_VEN_PIN): pins.gpio_output_pin_schema,
            # Optional pins (from official API)
            cv.Optional(CONF_DWL_REQ_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_WKUP_REQ_PIN): pins.gpio_output_pin_schema,
            # Tag emulation options
            cv.Optional(CONF_EMULATION_MESSAGE): cv.string,
            cv.Optional(CONF_TAG_TTL, default="250ms"): cv.positive_time_period_milliseconds,
            # Health check options (custom enhancement)
            cv.Optional(CONF_HEALTH_CHECK_ENABLED, default=True): cv.boolean,
            cv.Optional(CONF_HEALTH_CHECK_INTERVAL, default="60s"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_MAX_FAILED_CHECKS, default=3): cv.int_range(min=1, max=10),
            cv.Optional(CONF_AUTO_RESET_ON_FAILURE, default=True): cv.boolean,
            # Standard options
            cv.Optional(CONF_UPDATE_INTERVAL, default="1s"): cv.update_interval,
            cv.Optional(CONF_ON_TAG): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        pn7160.PN7160Trigger
                    ),
                }
            ),
            cv.Optional(CONF_ON_TAG_REMOVED): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        pn7160.PN7160TagRemovedTrigger
                    ),
                }
            ),
            cv.Optional(CONF_ON_EMULATED_TAG_SCAN): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PN7160OnEmulatedTagScanTrigger),
                }
            ),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(i2c.i2c_device_schema(0x28))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    await pn7160.setup_pn7160(var, config)

    # Set pins
    irq_pin = await cg.gpio_pin_expression(config[CONF_IRQ_PIN])
    cg.add(var.set_irq_pin(irq_pin))
    
    ven_pin = await cg.gpio_pin_expression(config[CONF_VEN_PIN])
    cg.add(var.set_ven_pin(ven_pin))
    
    # Optional pins
    if CONF_DWL_REQ_PIN in config:
        dwl_req_pin = await cg.gpio_pin_expression(config[CONF_DWL_REQ_PIN])
        cg.add(var.set_dwl_req_pin(dwl_req_pin))
    
    if CONF_WKUP_REQ_PIN in config:
        wkup_req_pin = await cg.gpio_pin_expression(config[CONF_WKUP_REQ_PIN])
        cg.add(var.set_wkup_req_pin(wkup_req_pin))
    
    # Tag TTL
    if CONF_TAG_TTL in config:
        cg.add(var.set_tag_ttl(config[CONF_TAG_TTL]))
    
    # Emulation message
    if CONF_EMULATION_MESSAGE in config:
        cg.add(var.set_tag_emulation_message(config[CONF_EMULATION_MESSAGE]))
    
    # Health check settings (custom enhancement)
    if CONF_HEALTH_CHECK_ENABLED in config:
        cg.add(var.set_health_check_enabled(config[CONF_HEALTH_CHECK_ENABLED]))
    
    if CONF_HEALTH_CHECK_INTERVAL in config:
        cg.add(var.set_health_check_interval(config[CONF_HEALTH_CHECK_INTERVAL]))
    
    if CONF_MAX_FAILED_CHECKS in config:
        cg.add(var.set_max_failed_checks(config[CONF_MAX_FAILED_CHECKS]))
    
    if CONF_AUTO_RESET_ON_FAILURE in config:
        cg.add(var.set_auto_reset_on_failure(config[CONF_AUTO_RESET_ON_FAILURE]))
