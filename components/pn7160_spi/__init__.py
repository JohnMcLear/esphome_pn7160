"""PN7160/PN7161 over SPI for ESPHome."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome.const import CONF_ID

from ..pn7160 import PN7160, PN7160_SCHEMA, setup_pn7160_core_

CODEOWNERS = ["@your-github-handle"]
DEPENDENCIES = ["spi"]
AUTO_LOAD = ["pn7160"]

pn7160_spi_ns = cg.esphome_ns.namespace("pn7160_spi")
PN7160Spi = pn7160_spi_ns.class_("PN7160Spi", PN7160, spi.SPIDevice)

CONFIG_SCHEMA = (
    PN7160_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(PN7160Spi),
        }
    )
    .extend(spi.spi_device_schema())
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)
    await setup_pn7160_core_(var, config)
