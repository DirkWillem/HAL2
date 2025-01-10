use crate::pin_data_parser::{PinData, PinId};
use regex::Regex;
use serde::Serialize;
use tera::Tera;

#[derive(Serialize, Clone, Debug)]
struct TimPinMapping {
    pin: PinId,
    tim: String,
    ch: String,
    af: u32,
}

#[derive(Serialize, Clone, Debug)]
struct TimPinMappings {
    ch: Vec<TimPinMapping>,
    chn: Vec<TimPinMapping>,
}

fn get_tim_pin_mappings(pin_data: &PinData) -> anyhow::Result<TimPinMappings> {
    let af_re = Regex::new(r"^(?<tim>TIM[0-9]+)_CH(?<ch>[0-9])(?<suffix>N?)$")?;

    let mut ch: Vec<TimPinMapping> = Default::default();
    let mut chn: Vec<TimPinMapping> = Default::default();

    for pin in &pin_data.pins {
        for (af_name, af_num) in &pin.afs {
            if let Some(cap) = af_re.captures(af_name) {
                match cap.name("suffix").unwrap().as_str() {
                    "" => ch.push(TimPinMapping {
                        pin: pin.pin_id.clone(),
                        tim: cap.name("tim").unwrap().as_str().to_string(),
                        ch: cap.name("ch").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "N" => chn.push(TimPinMapping {
                        pin: pin.pin_id.clone(),
                        tim: cap.name("tim").unwrap().as_str().to_string(),
                        ch: cap.name("ch").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    suffix => return Err(anyhow::anyhow!("Invalid TIM suffix {suffix}")),
                }
            }
        }
    }

    Ok(TimPinMappings { ch, chn })
}

pub fn gen_mapping_header(pin_data: &PinData, family: &str, mcu: &str) -> anyhow::Result<String> {
    // Get TIM pin mappings
    let tim_pin_mappings = get_tim_pin_mappings(&pin_data)?;

    // Setup templates
    let mut tera = Tera::default();
    tera.add_raw_template("tim_macros", include_str!("templates/tim_macros.tera"))?;
    tera.add_raw_template(
        "tim_pin_mapping",
        include_str!("templates/tim_pin_mapping.h.tera"),
    )?;

    let mut context = tera::Context::new();
    context.insert("family", family);
    context.insert("mcu", mcu);
    context.insert("tim_pins", &tim_pin_mappings);

    Ok(tera.render("tim_pin_mapping", &context)?)
}

