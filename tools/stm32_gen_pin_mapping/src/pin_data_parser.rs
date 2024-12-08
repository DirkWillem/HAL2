use anyhow::Context;
use regex::Regex;
use std::collections::HashMap;
use std::fs;
use std::path::Path;
use serde::Serialize;

#[derive(Debug, Serialize, Clone)]
pub struct Pin {
    pub pin_id: PinId,
    pub afs: HashMap<String, u32>,
}

#[derive(Debug, Hash, PartialEq, PartialOrd, Eq, Ord, Serialize, Clone)]
pub struct PinId(char, u32);

#[derive(Debug, Serialize)]
pub struct PinData {
    pub pins: Vec<Pin>
}

pub fn parse_pin_data(mcu_xml_path: &str) -> anyhow::Result<PinData> {
    let xml_str = fs::read_to_string(mcu_xml_path)?;
    let doc = roxmltree::Document::parse(&xml_str)?;

    // Find the GPIO IP node
    let mcu_node = doc
        .descendants()
        .find(|n| n.has_tag_name("Mcu"))
        .context("Document must contain 'Mcu' node")?;
    let gpio_ip_node = mcu_node
        .descendants()
        .find(|n| n.has_tag_name("IP") && n.attribute("Name") == Some("GPIO"))
        .context("XML file must contain GPIO IP node")?;
    let gpio_version = gpio_ip_node
        .attribute("Version")
        .context("GPIO IP node must have 'Version' attribute")?;

    let xml_path = Path::new(mcu_xml_path);
    let xml_path_root = xml_path.parent().context("XML path must have a parent")?;
    let mut gpio_ip_path = xml_path_root.to_path_buf();
    gpio_ip_path.push("IP");
    gpio_ip_path.push(format!("GPIO-{gpio_version}_Modes.xml"));
    if !gpio_ip_path.exists() {
        return Err(anyhow::anyhow!(format!(
            "File {} must exist",
            gpio_ip_path.to_str().context("")?
        )));
    }

    parse_gpio_xml(gpio_ip_path.into())
}

fn parse_gpio_xml(xml_path: Box<Path>) -> anyhow::Result<PinData> {
    let pin_re = Regex::new("^P(?<port>[A-Z])(?<num>[0-9]+)")?;
    let af_re = Regex::new("^GPIO_AF(?<num>[0-9]+)_[A-Za-z0-9_]+$")?;
    let xml_str = fs::read_to_string(xml_path)?;
    let doc = roxmltree::Document::parse(&xml_str)?;

    let ip_node = doc
        .descendants()
        .find(|n| n.has_tag_name("IP"))
        .context("Document must contain 'IP' node")?;
    let gpio_pin_nodes = ip_node.descendants().filter(|n| n.has_tag_name("GPIO_Pin"));

    let mut pins =Vec::<Pin>::new();

    for gpio_pin_node in gpio_pin_nodes {
        let mut afs = HashMap::<String, u32>::new();

        let pin_name = gpio_pin_node.attribute("Name").unwrap();
        let pin_cap = pin_re.captures(pin_name);

        if pin_cap.is_none() {
            continue;
        }

        let pin_cap = pin_cap.unwrap();

        let port = pin_cap
            .name("port")
            .context("")
            .unwrap()
            .as_str()
            .chars()
            .next()
            .unwrap();
        let num = pin_cap
            .name("num")
            .context("")
            .unwrap()
            .as_str()
            .parse::<u32>()
            .unwrap();

        let pin_signal_nodes = gpio_pin_node.descendants().filter(|n| {
            n.has_tag_name("PinSignal")
                && n.descendants().any(|n2| {
                    n2.has_tag_name("SpecificParameter")
                        && n2.attribute("Name") == Some("GPIO_AF")
                        && n2.descendants().any(|n3| n3.has_tag_name("PossibleValue"))
                })
        });

        for pin_signal_node in pin_signal_nodes {
            let af_name = pin_signal_node
                .descendants()
                .find(|n| n.has_tag_name("SpecificParameter"))
                .unwrap()
                .descendants()
                .find(|n| n.has_tag_name("PossibleValue"))
                .unwrap()
                .text()
                .context("PossibleValue element must have text content")?;
            if let Some(captures) = af_re.captures(af_name) {
                let af_num = captures.name("num").unwrap().as_str().parse::<u32>()?;
                let af = pin_signal_node.attribute("Name").context("")?;

                afs.insert(af.into(), af_num);
            }
        }

        pins.push(Pin { pin_id: PinId(port, num), afs });
    }

    Ok(PinData { pins })
}
