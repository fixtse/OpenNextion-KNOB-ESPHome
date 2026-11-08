import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from pathlib import Path

onx_app_state_ns = cg.esphome_ns.namespace("onx_app_state")
OnxAppState = onx_app_state_ns.class_("OnxAppState", cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(OnxAppState),
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    patch_script = Path(__file__).with_name("patch_api_batch.py")
    cg.add_platformio_option("extra_scripts", [f"pre:{patch_script}"])
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
