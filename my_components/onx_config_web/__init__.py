import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

CONF_HANDLER = "handler"

AUTO_LOAD = ["web_server_base", "json"]
DEPENDENCIES = ["network"]
ns = cg.esphome_ns.namespace("onx_config_web")
ConfigWeb = ns.class_("ConfigWeb", cg.Component)
Query = ns.struct("Query")
CONFIG_SCHEMA = cv.Schema({cv.GenerateID(): cv.declare_id(ConfigWeb), cv.Required(CONF_HANDLER): cv.returning_lambda}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    handler = await cg.process_lambda(config[CONF_HANDLER], [(Query.operator("const").operator("ref"), "query")], return_type=cg.std_string)
    cg.add(var.set_handler(handler))
