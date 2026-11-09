import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["esp32", "network", "lvgl"]
AUTO_LOAD = ["image"]
ns = cg.esphome_ns.namespace("onx_icons")
IconDownloader = ns.class_("IconDownloader", cg.Component)
CONFIG_SCHEMA = cv.Schema({cv.GenerateID(): cv.declare_id(IconDownloader)}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
