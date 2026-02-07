import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_PORT, CONF_ADDRESS

AUTO_LOAD = ["socket", "async_tcp"]
CODEOWNERS = ["@synodriver"]

DEPENDENCIES = ["uart", "network"]

CONF_STREAM_CLIENT_ID = "stream_client_id"

stream_client_ns = cg.esphome_ns.namespace("stream_client")
StreamClientComponent = stream_client_ns.class_("StreamClientComponent", cg.Component, uart.UARTDevice)

def validate_buffer_size(buffer_size):
    if buffer_size & (buffer_size - 1) != 0:
        raise cv.Invalid("Buffer size must be a power of two.")
    return buffer_size


CONFIG_SCHEMA = cv.All(
    cv.require_esphome_version(2025, 3, 0),
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(StreamClientComponent),
            cv.Required(CONF_ADDRESS): cv.string,
            cv.Optional(CONF_PORT, default=6638): cv.port,
            # cv.Optional(CONF_BUFFER_SIZE, default=128): cv.All(
            #     cv.positive_int, validate_buffer_size
            # ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA),
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "stream_client",
    require_tx=True,
    require_rx=True,
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_port(config[CONF_PORT]))