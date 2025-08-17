use crate::pin_data_parser::{PinData, PinId};
use regex::Regex;
use serde::Serialize;
use tera::Tera;

#[derive(Serialize, Clone, Debug)]
struct SpiPinMapping {
    pin: PinId,
    instance: String,
    af: u32,
}

#[derive(Serialize, Clone, Debug)]
struct SpiPinMappings {
    sck: Vec<SpiPinMapping>,
    nss: Vec<SpiPinMapping>,
    mosi: Vec<SpiPinMapping>,
    miso: Vec<SpiPinMapping>,
}

#[derive(Serialize, Clone, Debug)]
struct I2sPinMapping {
    pin: PinId,
    instance: String,
    af: u32,
}

#[derive(Serialize, Clone, Debug)]
struct I2sPinMappings {
    sd: Vec<I2sPinMapping>,
    ws: Vec<I2sPinMapping>,
    ck: Vec<I2sPinMapping>,
    mck: Vec<I2sPinMapping>,
}

fn get_spi_pin_mappings(pin_data: &PinData) -> anyhow::Result<SpiPinMappings> {
    let af_re = Regex::new(r"^(?<inst>SPI[0-9]+)_(?<fn>(SCK)|(NSS)|(MOSI)|(MISO))$")?;

    let mut sck: Vec<SpiPinMapping> = Default::default();
    let mut nss: Vec<SpiPinMapping> = Default::default();
    let mut mosi: Vec<SpiPinMapping> = Default::default();
    let mut miso: Vec<SpiPinMapping> = Default::default();

    for pin in &pin_data.pins {
        for (af_name, af_num) in &pin.afs {
            if let Some(cap) = af_re.captures(af_name) {
                match cap.name("fn").unwrap().as_str() {
                    "SCK" => sck.push(SpiPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "NSS" => nss.push(SpiPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "MOSI" => mosi.push(SpiPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "MISO" => miso.push(SpiPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    ifn => return Err(anyhow::anyhow!("Invalid SPI function {ifn}")),
                }
            }
        }
    }

    Ok(SpiPinMappings {
        sck,
        nss,
        mosi,
        miso,
    })
}

fn get_i2s_pin_mappings(pin_data: &PinData) -> anyhow::Result<I2sPinMappings> {
    let af_re = Regex::new(r"^(?<inst>I2S[0-9]+)_(?<fn>(SD)|(WS)|(CK)|(MCK))$")?;

    let mut sd: Vec<I2sPinMapping> = Default::default();
    let mut ws: Vec<I2sPinMapping> = Default::default();
    let mut ck: Vec<I2sPinMapping> = Default::default();
    let mut mck: Vec<I2sPinMapping> = Default::default();

    for pin in &pin_data.pins {
        for (af_name, af_num) in &pin.afs {
            if let Some(cap) = af_re.captures(af_name) {
                match cap.name("fn").unwrap().as_str() {
                    "SD" => sd.push(I2sPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "WS" => ws.push(I2sPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "CK" => ck.push(I2sPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "MCK" => mck.push(I2sPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    ifn => return Err(anyhow::anyhow!("Invalid SPI function {ifn}")),
                }
            }
        }
    }

    Ok(I2sPinMappings { sd, ws, ck, mck })
}

pub fn gen_mapping_header(pin_data: &PinData, family: &str, mcu: &str) -> anyhow::Result<String> {
    // Get pin mappings
    let spi_pin_mappings = get_spi_pin_mappings(&pin_data)?;
    let i2s_pin_mappings = get_i2s_pin_mappings(&pin_data)?;

    // Setup templates
    let mut tera = Tera::default();
    tera.add_raw_template(
        "spi_i2s_macros",
        include_str!("templates/spi_i2s_macros.tera"),
    )?;
    tera.add_raw_template(
        "spi_i2s_pin_mapping",
        include_str!("templates/spi_i2s_pin_mapping.h.tera"),
    )?;

    let mut context = tera::Context::new();
    context.insert("family", family);
    context.insert("mcu", mcu);
    context.insert("spi_pins", &spi_pin_mappings);
    context.insert("i2s_pins", &i2s_pin_mappings);

    Ok(tera.render("spi_i2s_pin_mapping", &context)?)
}

pub fn gen_mapping_module(pin_data: &PinData, family: &str, mcu: &str) -> anyhow::Result<String> {
    // Get pin mappings
    let spi_pin_mappings = get_spi_pin_mappings(&pin_data)?;
    let i2s_pin_mappings = get_i2s_pin_mappings(&pin_data)?;

    // Setup templates
    let mut tera = Tera::default();
    tera.add_raw_template(
        "spi_i2s_macros",
        include_str!("templates/spi_i2s_macros.tera"),
    )?;
    tera.add_raw_template(
        "spi_i2s_pin_mapping",
        include_str!("templates/spi_i2s_pin_mapping.cppm.tera"),
    )?;

    let mut context = tera::Context::new();
    context.insert("family", family);
    context.insert("mcu", mcu);
    context.insert("spi_pins", &spi_pin_mappings);
    context.insert("i2s_pins", &i2s_pin_mappings);

    Ok(tera.render("spi_i2s_pin_mapping", &context)?)
}
