"""PN7160/PN7161 NFC controller component base for ESPHome.

Exports shared C++ class declarations, hub schema, and setup helper.
binary_sensor platform is in binary_sensor.py.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.const import CONF_TRIGGER_ID

CODEOWNERS = ["@your-github-handle"]
AUTO_LOAD = ["binary_sensor"]

pn7160_ns = cg.esphome_ns.namespace("pn7160")

# ── C++ class declarations ────────────────────────────────────────────────────

PN7160 = pn7160_ns.class_("PN7160", cg.PollingComponent)

PN7160Trigger = pn7160_ns.class_(
    "PN7160Trigger", automation.Trigger.template(cg.std_string)
)
PN7160TagRemovedTrigger = pn7160_ns.class_(
    "PN7160TagRemovedTrigger", automation.Trigger.template(cg.std_string)
)

# ── Shared config keys ────────────────────────────────────────────────────────

CONF_IRQ_PIN = "irq_pin"
CONF_VEN_PIN = "ven_pin"
CONF_ON_TAG = "on_tag"
CONF_ON_TAG_REMOVED = "on_tag_removed"
CONF_PN7160_ID = "pn7160_id"
CONF_HEALTH_CHECK_ENABLED = "health_check_enabled"
CONF_HEALTH_CHECK_INTERVAL = "health_check_interval"
CONF_AUTO_RESET_ON_FAILURE = "auto_reset_on_failure"
CONF_MAX_FAILED_CHECKS = "max_failed_checks"

DEFAULT_UPDATE_INTERVAL = "1s"
DEFAULT_HEALTH_CHECK_INTERVAL = "60s"
DEFAULT_MAX_FAILED_CHECKS = 3

# ── Shared hub schema ─────────────────────────────────────────────────────────

PN7160_SCHEMA = cv.Schema(
    {
        # IRQ and VEN pins are REQUIRED for PN7160
        cv.Required(CONF_IRQ_PIN): pins.gpio_input_pin_schema,
        cv.Required(CONF_VEN_PIN): pins.gpio_output_pin_schema,
        cv.Optional(CONF_ON_TAG): automation.validate_automation(
            {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PN7160Trigger)}
        ),
        cv.Optional(CONF_ON_TAG_REMOVED): automation.validate_automation(
            {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PN7160TagRemovedTrigger)}
        ),
        cv.Optional(CONF_HEALTH_CHECK_ENABLED, default=True): cv.boolean,
        cv.Optional(
            CONF_HEALTH_CHECK_INTERVAL, default=DEFAULT_HEALTH_CHECK_INTERVAL
        ): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_AUTO_RESET_ON_FAILURE, default=True): cv.boolean,
        cv.Optional(
            CONF_MAX_FAILED_CHECKS, default=DEFAULT_MAX_FAILED_CHECKS
        ): cv.int_range(min=1, max=10),
    }
).extend(cv.polling_component_schema(DEFAULT_UPDATE_INTERVAL))


# ── Shared hub code-gen helper ────────────────────────────────────────────────

async def setup_pn7160_core_(var, config):
    """Wire triggers, pins, and health-check settings for any PN7160 hub."""
    irq = await cg.gpio_pin_expression(config[CONF_IRQ_PIN])
    cg.add(var.set_irq_pin(irq))

    ven = await cg.gpio_pin_expression(config[CONF_VEN_PIN])
    cg.add(var.set_ven_pin(ven))

    cg.add(var.set_health_check_enabled(config[CONF_HEALTH_CHECK_ENABLED]))
    cg.add(var.set_health_check_interval(config[CONF_HEALTH_CHECK_INTERVAL]))
    cg.add(var.set_auto_reset_on_failure(config[CONF_AUTO_RESET_ON_FAILURE]))
    cg.add(var.set_max_failed_checks(config[CONF_MAX_FAILED_CHECKS]))

    for conf in config.get(CONF_ON_TAG, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "x")], conf)

    for conf in config.get(CONF_ON_TAG_REMOVED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "x")], conf)
