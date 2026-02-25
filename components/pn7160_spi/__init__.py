import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import spi
from esphome.const import CONF_ID, CONF_CS_PIN

from .. import pn7160
from esphome import pins

DEPENDENCIES = ["spi"]
AUTO_LOAD = ["pn7160"]

pn7160_spi_ns = cg.esphome_ns.namespace("pn7160_spi")
PN7160Spi = pn7160_spi_ns.class_("PN7160Spi", pn7160.PN7160, spi.SPIDevice)

CONFIG_SCHEMA = (
    pn7160.PN7160_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(PN7160Spi),
            cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
        }
    )
    .extend(spi.spi_device_schema())
    .extend(cv.polling_component_schema("1s"))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await spi.register_spi_device(var, config)

    cs_pin = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    cg.add(var.set_cs_pin(cs_pin))

    await pn7160.setup_pn7160(var, config)

