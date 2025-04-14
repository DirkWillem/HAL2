use crate::pin_data_parser::{PinData, PinId};
use regex::Regex;
use serde::Serialize;
use tera::Tera;

#[derive(Serialize, Clone, Debug)]
struct UartPinMapping {
    pin: PinId,
    instance: String,
    af: u32,
}

#[derive(Serialize, Clone, Debug)]
struct UartPinMappings {
    tx: Vec<UartPinMapping>,
    rx: Vec<UartPinMapping>,
    rts: Vec<UartPinMapping>,
    cts: Vec<UartPinMapping>,
}

fn get_uart_pin_mappings(pin_data: &PinData) -> anyhow::Result<UartPinMappings> {
    let af_re = Regex::new(r"^(?<inst>(LP)?US?ART[0-9]+)_(?<fn>(RX)|(TX)|(RTS)|(CTS))$")?;

    let mut tx: Vec<UartPinMapping> = Default::default();
    let mut rx: Vec<UartPinMapping> = Default::default();
    let mut rts: Vec<UartPinMapping> = Default::default();
    let mut cts: Vec<UartPinMapping> = Default::default();

    for pin in &pin_data.pins {
        for (af_name, af_num) in &pin.afs {
            if let Some(cap) = af_re.captures(af_name) {
                match cap.name("fn").unwrap().as_str() {
                    "TX" => tx.push(UartPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "RX" => rx.push(UartPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "RTS" => rts.push(UartPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    "CTS" => cts.push(UartPinMapping {
                        pin: pin.pin_id.clone(),
                        instance: cap.name("inst").unwrap().as_str().to_string(),
                        af: *af_num,
                    }),
                    ifn => return Err(anyhow::anyhow!("Invalid UART function {ifn}")),
                }
            }
        }
    }

    Ok(UartPinMappings { tx, rx, rts, cts })
}

pub fn gen_mapping_header(pin_data: &PinData, family: &str, mcu: &str) -> anyhow::Result<String> {
    // Get UART pin mappings
    let uart_pin_mappings = get_uart_pin_mappings(&pin_data)?;

    // Setup templates
    let mut tera = Tera::default();
    tera.add_raw_template("uart_macros", include_str!("templates/uart_macros.tera"))?;
    tera.add_raw_template(
        "uart_pin_mapping",
        include_str!("templates/uart_pin_mapping.h.tera"),
    )?;

    let mut context = tera::Context::new();
    context.insert("family", family);
    context.insert("mcu", mcu);
    context.insert("uart_pins", &uart_pin_mappings);

    Ok(tera.render("uart_pin_mapping", &context)?)
}

pub fn gen_mapping_module(pin_data: &PinData, family: &str, mcu: &str) -> anyhow::Result<String> {
    // Get UART pin mappings
    let uart_pin_mappings = get_uart_pin_mappings(&pin_data)?;

    // Setup templates
    let mut tera = Tera::default();
    tera.add_raw_template("uart_macros", include_str!("templates/uart_macros.tera"))?;
    tera.add_raw_template(
        "uart_pin_mapping",
        include_str!("templates/uart_pin_mapping.cppm.tera"),
    )?;

    let mut context = tera::Context::new();
    context.insert("family", family);
    context.insert("mcu", mcu);
    context.insert("uart_pins", &uart_pin_mappings);

    Ok(tera.render("uart_pin_mapping", &context)?)
}
