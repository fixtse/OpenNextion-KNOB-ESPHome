import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import binary_sensor
from esphome.const import CONF_ID, CONF_PIN

DEPENDENCIES = ["esp32"]
ns = cg.esphome_ns.namespace("onx_button")
BufferedButton = ns.class_("BufferedButton", binary_sensor.BinarySensor, cg.Component)
CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(BufferedButton).extend({
    cv.Required(CONF_PIN): pins.internal_gpio_input_pin_schema,
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config)
    await cg.register_component(var, config)
    pin = await cg.gpio_pin_expression(config[CONF_PIN])
    cg.add(var.set_pin(pin))
