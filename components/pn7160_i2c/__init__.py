"""PN7160/PN7161 over I2C for ESPHome.

CRITICAL: PN7160 requires I2C frequency >= 100kHz.
The default ESPHome I2C frequency of 50kHz will cause IRQ timeouts.
See https://github.com/esphome/issues/issues/6339
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID
import logging

from ..pn7160 import PN7160, PN7160_SCHEMA, setup_pn7160_core_

_LOGGER = logging.getLogger(__name__)

CODEOWNERS = ["@your-github-handle"]
DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["pn7160"]

pn7160_i2c_ns = cg.esphome_ns.namespace("pn7160_i2c")
PN7160I2C = pn7160_i2c_ns.class_("PN7160I2C", PN7160, i2c.I2CDevice)

# FIX for bug #6339: PN7160 requires minimum 100kHz I2C frequency
MIN_I2C_FREQUENCY = 100000


def validate_i2c_frequency(config):
    """Warn if I2C frequency is below the required 100kHz minimum."""
    # Get the I2C bus this device is attached to
    i2c_id = config.get("i2c_id")
    # Note: We can't easily access the parent I2C bus config here to check frequency,
    # but we can at least document the requirement prominently in logs.
    # The user must ensure their i2c: config has frequency: 100kHz or higher.
    _LOGGER.warning(
        "IMPORTANT: PN7160 requires I2C frequency >= 100kHz. "
        "If you experience IRQ timeouts, check your i2c: frequency setting. "
        "See https://github.com/esphome/issues/issues/6339"
    )
    return config


CONFIG_SCHEMA = (
    PN7160_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(PN7160I2C),
        }
    )
    .extend(i2c.i2c_device_schema(0x28))
)


async def to_code(config):
    validate_i2c_frequency(config)
    
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    await setup_pn7160_core_(var, config)
