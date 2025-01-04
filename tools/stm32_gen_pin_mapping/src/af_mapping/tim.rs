use crate::pin_data_parser::{PinData, PinId};
use regex::Regex;
use serde::Serialize;
use tera::Tera;

#[derive(Serialize, Clone, Debug)]
struct TimPinMapping {
    pin: PinId,
    tim: String,
    ch: u32,
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
                        ch: u32::from(cap.name("ch").unwrap().as_str()),
                        af: *af_num,
                    }),
                    "N" => chn.push(TimPinMapping {
                        pin: pin.pin_id.clone(),
                        tim: cap.name("tim").unwrap().as_str().to_string(),
                        ch: u32::from(cap.name("ch").unwrap().as_str()),
                        af: *af_num,
                    }),
                    suffix => return Err(anyhow::anyhow!("Invalid TIM suffix {suffix}")),
                }
            }
        }
    }

    Ok(TimPinMappings { ch, chn })
}
