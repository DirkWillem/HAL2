mod af_mapping;
mod pin_data_parser;

use crate::af_mapping::{i2c, spi_i2s, usb};
use crate::pin_data_parser::{PinData, PinId};
use af_mapping::uart;
use clap::{Parser, ValueEnum};
use regex::Regex;
use serde::Serialize;
use std::fs::File;
use std::io::Write;
use tera::Tera;

#[derive(ValueEnum, Clone, Serialize)]
enum GenType {
    UartPinMapping,
    SpiI2sPinMapping,
    I2cPinMapping,
    UsbPinMapping,
}

#[derive(Parser)]
#[command(name = "stm32_af_gen")]
#[command(version = "1.0")]
struct Cli {
    mcu_xml_file: String,
    out_file: String,

    #[arg(long)]
    gen: GenType,
    #[arg(long)]
    family: String,
    #[arg(long)]
    mcu: String,
}

fn main() -> anyhow::Result<()> {
    let cli = Cli::parse();

    // Parse input file
    let pin_data = pin_data_parser::parse_pin_data(&cli.mcu_xml_file)?;

    match cli.gen {
        GenType::UartPinMapping => {
            // Calculate UART pin mappings
            let header = uart::gen_mapping_header(&pin_data, &cli.family, &cli.mcu)?;

            let mut out_file = File::create(cli.out_file)?;
            out_file.write_all(&header.as_bytes())?;
            Ok(())
        }
        GenType::SpiI2sPinMapping => {
            // Calculate UART pin mappings
            let header = spi_i2s::gen_mapping_header(&pin_data, &cli.family, &cli.mcu)?;

            let mut out_file = File::create(cli.out_file)?;
            out_file.write_all(&header.as_bytes())?;
            Ok(())
        }
        GenType::I2cPinMapping => {
            let header = i2c::gen_mapping_header(&pin_data, &cli.family, &cli.mcu)?;

            let mut out_file = File::create(cli.out_file)?;
            out_file.write_all(&header.as_bytes())?;
            Ok(())
        },
        GenType::UsbPinMapping => {
            let header = usb::gen_mapping_header(&pin_data, &cli.family, &cli.mcu)?;

            let mut out_file = File::create(cli.out_file)?;
            out_file.write_all(&header.as_bytes())?;
            Ok(())
        }
    }
}
