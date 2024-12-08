use crate::pin_data_parser::{PinData, PinId};
use regex::Regex;
use serde::Serialize;
use tera::Tera;

#[derive(Serialize, Clone, Debug)]
struct I2cPinMapping {
    pin: PinId,
    instance: String,
    af: u32,
}

#[derive(Serialize, Clone, Debug)]
struct I2cPinMappings {
    sda: Vec<I2cPinMapping>,
    scl: Vec<I2cPinMapping>,
    smba: Vec<I2cPinMapping>,
}

fn get_i2c_pin_mappings(pin_data: &PinData) -> anyhow::Result<I2cPinMappings> {
    let af_re = Regex::new(r"^(?<inst>I2C[0-9]+)_(?<fn>(SDA)|(SCL)|(SMBA))$")?;

    let mut sda: Vec<I2cPinMapping> = Default::default();
    let mut scl: Vec<I2cPinMapping> = Default::default();
    let mut smba: Vec<I2cPinMapping> = Default::default();

    for pin in &pin_data.pins {
        for (af_name, af_num) in &pin.afs {
            if let Some(cap) = af_re.captures(af_name) {
                match cap.name("fn").unwrap().as_str() {
                    "SDA" => sda.push(I2cPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "SCL" => scl.push(I2cPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "SMBA" => smba.push(I2cPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    ifn => return Err(anyhow::anyhow!("Invalid UART function {ifn}")),
                }
            }
        }
    }

    Ok(I2cPinMappings { sda, scl, smba })
}

pub fn gen_mapping_header(pin_data: &PinData, family: &str, mcu: &str) -> anyhow::Result<String> {
    // Get pin mappings
    let i2c_pin_mappings = get_i2c_pin_mappings(pin_data)?;

    // Setup templates
    let mut tera = Tera::default();
    tera.add_raw_template("i2c_macros", include_str!("templates/i2c_macros.tera"))?;
    tera.add_raw_template(
        "i2c_pin_mapping",
        include_str!("templates/i2c_pin_mapping.h.tera"),
    )?;

    let mut context = tera::Context::new();
    context.insert("family", family);
    context.insert("mcu", mcu);
    context.insert("i2c_pins", &i2c_pin_mappings);

    Ok(tera.render("i2c_pin_mapping", &context)?)
}

