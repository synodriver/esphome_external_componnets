import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_PLUG
)

from . import CONF_STREAM_CLIENT_ID, StreamClientComponent

DEPENDENCIES = ["stream_client"]

CONF_CONNECTED = "connected"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_STREAM_CLIENT_ID): cv.use_id(StreamClientComponent),
            cv.Optional(CONF_CONNECTED): binary_sensor.binary_sensor_schema(
                device_class=DEVICE_CLASS_PLUG,
            )
        }
    )
)

async def to_code(config):
    stream_client_component = await cg.get_variable(config[CONF_STREAM_CLIENT_ID])

    if connected_config := config.get(CONF_CONNECTED):
        sens = await binary_sensor.new_binary_sensor(connected_config)
        cg.add(stream_client_component.set_connected_binary_sensor(sens))