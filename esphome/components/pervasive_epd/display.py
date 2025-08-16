import logging

from esphome import core, pins
import esphome.codegen as cg
from esphome.components import display, spi
import esphome.config_validation as cv
from esphome.const import (
    CONF_BUSY_PIN,
    CONF_CS_PIN,
    CONF_DC_PIN,
    CONF_FULL_UPDATE_EVERY,
    CONF_ID,
    CONF_LAMBDA,
    CONF_MODEL,
    CONF_PAGES,
    CONF_RESET_DURATION,
    CONF_RESET_PIN,
)

DEPENDENCIES = ["spi"]

# CONF_CS_PIN = "css_pin"

pervasive_epd_ns = cg.esphome_ns.namespace("pervasive_epd")
Pervasive_EPD = pervasive_epd_ns.class_(
    "Pervasive_EPD", cg.PollingComponent, spi.SPIDevice, display.DisplayBuffer
)

Pervasive_EPD_Model = pervasive_epd_ns.enum("Pervasive_EPD_Model")

SaverTemplate = pervasive_epd_ns.class_("COGDataSaver")

MODELS = {
    "epd_150_ks_0j": Pervasive_EPD_Model.EPD_150_KS_0J,
    "epd_152_ks_0j": Pervasive_EPD_Model.EPD_152_KS_0J,
    "epd_154_ks_0c": Pervasive_EPD_Model.EPD_154_KS_0C,
    "epd_206_ks_0e": Pervasive_EPD_Model.EPD_206_KS_0E,
    "epd_213_ks_0e": Pervasive_EPD_Model.EPD_213_KS_0E,
    "epd_266_ks_0c": Pervasive_EPD_Model.EPD_266_KS_0C,
    "epd_271_ks_09": Pervasive_EPD_Model.EPD_271_KS_09,
    "epd_271_ks_0c": Pervasive_EPD_Model.EPD_271_KS_0C,
    "epd_290_ks_0f": Pervasive_EPD_Model.EPD_290_KS_0F,
    "epd_370_ks_0c": Pervasive_EPD_Model.EPD_370_KS_0C,
    "epd_417_ks_0d": Pervasive_EPD_Model.EPD_417_KS_0D,
    "epd_437_ks_0c": Pervasive_EPD_Model.EPD_437_KS_0C,
}

_LOGGER = logging.getLogger(__name__)

# def validate_full_update_every_only_types_ac(value):
#    if CONF_FULL_UPDATE_EVERY not in value:
#        return value
#    if MODELS[value[CONF_MODEL]][0] == "b":
#        full_models = []
#        for key, val in sorted(MODELS.items()):
#            if val[0] != "b":
#                full_models.append(key)
#        raise cv.Invalid(
#            "The 'full_update_every' option is only available for models "
#            + ", ".join(full_models)
#        )
#    return value


# def validate_reset_pin_required(config):
#    if config[CONF_MODEL] in RESET_PIN_REQUIRED_MODELS and CONF_RESET_PIN not in config:
#        raise cv.Invalid(
#            f"'{CONF_RESET_PIN}' is required for model {config[CONF_MODEL]}"
#        )
#    return config


CONFIG_SCHEMA = cv.All(
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(Pervasive_EPD),
            cv.Required(CONF_DC_PIN): pins.gpio_output_pin_schema,
            # cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
            # cv.Required(CONF_CLK_PIN): pins.gpio_output_pin_schema,
            # cv.Required(CONF_MOSI_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_MODEL): cv.one_of(*MODELS, lower=True),
            cv.Optional(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_FULL_UPDATE_EVERY): cv.int_range(min=1, max=4294967295),
            cv.Optional(CONF_RESET_DURATION): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(max=core.TimePeriod(milliseconds=500)),
            ),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(spi.spi_device_schema()),
    # validate_full_update_every_only_types_ac,
    # validate_reset_pin_required,
    cv.has_at_most_one_key(CONF_PAGES, CONF_LAMBDA),
)

FINAL_VALIDATE_SCHEMA = spi.final_validate_device_schema(
    "pervasive_epd", require_miso=False, require_mosi=True
)


async def to_code(config):
    model = MODELS[config[CONF_MODEL]]
    rhs = Pervasive_EPD.new(model)
    var = cg.Pvariable(config[CONF_ID], rhs, Pervasive_EPD)

    await display.register_display(var, config)
    await spi.register_spi_device(var, config)

    _LOGGER.info("DC: %s", config[CONF_DC_PIN])
    _LOGGER.info("CS: %s", config[CONF_CS_PIN])
    # _LOGGER.info("CLK: %s", config[CONF_CLK_PIN])
    # _LOGGER.info("MOSI: %s", config[CONF_MOSI_PIN])
    dc = await cg.gpio_pin_expression(config[CONF_DC_PIN])
    cg.add(var.set_dc_pin(dc))
    saver = SaverTemplate.new()
    cg.add(var.set_value_saver(saver))

    if CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[CONF_LAMBDA], [(display.DisplayRef, "it")], return_type=cg.void
        )
        cg.add(var.set_writer(lambda_))
    if CONF_RESET_PIN in config:
        reset = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(var.set_reset_pin(reset))
    if CONF_BUSY_PIN in config:
        reset = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
        cg.add(var.set_busy_pin(reset))
    if CONF_FULL_UPDATE_EVERY in config:
        cg.add(var.set_full_update_every(config[CONF_FULL_UPDATE_EVERY]))
    if CONF_RESET_DURATION in config:
        cg.add(var.set_reset_duration(config[CONF_RESET_DURATION]))
