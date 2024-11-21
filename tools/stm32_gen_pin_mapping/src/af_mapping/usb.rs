use crate::pin_data_parser::{PinData, PinId};
use regex::Regex;
use serde::Serialize;
use tera::Tera;

#[derive(Serialize, Clone, Debug)]
struct UsbPinMapping {
    pin: PinId,
    instance: String,
    af: u32,
}

#[derive(Serialize, Clone, Debug)]
struct UsbPinMappings {
    dp: Vec<UsbPinMapping>,
    dm: Vec<UsbPinMapping>,
    sof: Vec<UsbPinMapping>,
}

fn get_usb_pin_mappings(pin_data: &PinData) -> anyhow::Result<UsbPinMappings> {
    let af_re = Regex::new(r"^(?<inst>USB)_(?<fn>(DP)|(DM)|(SOF))$")?;

    let mut dp: Vec<UsbPinMapping> = Default::default();
    let mut dm: Vec<UsbPinMapping> = Default::default();
    let mut sof: Vec<UsbPinMapping> = Default::default();

    for pin in &pin_data.pins {
        for (af_name, af_num) in &pin.afs {
            if let Some(cap) = af_re.captures(af_name) {
                match cap.name("fn").unwrap().as_str() {
                    "DP" => dp.push(UsbPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "DM" => dm.push(UsbPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "SOF" => sof.push(UsbPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    ifn => return Err(anyhow::anyhow!("Invalid USB function {ifn}")),
                }
            }
        }
    }

    Ok(UsbPinMappings { dp, dm, sof })
}


pub fn gen_mapping_header(pin_data: &PinData, family: &str, mcu: &str) -> anyhow::Result<String> {
    // Get UART pin mappings
    let usb_pin_mappings = get_usb_pin_mappings(&pin_data)?;

    // Setup templates
    let mut tera = Tera::default();
    tera.add_raw_template("usb_macros", include_str!("templates/usb_macros.tera"))?;
    tera.add_raw_template(
        "usb_pin_mapping",
        include_str!("templates/usb_pin_mapping.h.tera"),
    )?;

    let mut context = tera::Context::new();
    context.insert("family", family);
    context.insert("mcu", mcu);
    context.insert("usb_pins", &usb_pin_mappings);

    Ok(tera.render("usb_pin_mapping", &context)?)
}
