mod af_mapping;
mod pin_data_parser;

use crate::af_mapping::{i2c, spi_i2s, tim, usb};
use crate::pin_data_parser::{PinData, PinId};
use af_mapping::uart;
use clap::{Parser, ValueEnum};
use regex::Regex;
use serde::Serialize;
use std::fs::File;
use std::io::Write;
use std::path::PathBuf;
use std::process::Command;
use tera::Tera;

#[derive(ValueEnum, Clone, Serialize)]
enum GenType {
    UartPinMapping,
    UartPinMappingModule,
    SpiI2sPinMapping,
    I2cPinMapping,
    UsbPinMapping,
    TimPinMapping,
    TimPinMappingModule,
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

pub fn format_generated_file(path: impl AsRef<str>) {
    if let Ok(clang_format_path) = which::which("clang-format") {
        let mut dir = PathBuf::from(String::from(path.as_ref()));
        let file_name = dir.file_name().unwrap().to_str().unwrap().to_string();
        dir.pop();

        let result = Command::new(clang_format_path)
            .arg("-i")
            .arg(file_name)
            .current_dir(&dir)
            .output()
            .expect("");
    }
}

fn main() -> anyhow::Result<()> {
    let cli = Cli::parse();

    // Parse input file
    let pin_data = pin_data_parser::parse_pin_data(&cli.mcu_xml_file)?;

    let header = match cli.gen {
        GenType::UartPinMapping => uart::gen_mapping_header(&pin_data, &cli.family, &cli.mcu)?,
        GenType::UartPinMappingModule => {
            uart::gen_mapping_module(&pin_data, &cli.family, &cli.mcu)?
        }
        GenType::SpiI2sPinMapping => spi_i2s::gen_mapping_header(&pin_data, &cli.family, &cli.mcu)?,
        GenType::I2cPinMapping => i2c::gen_mapping_header(&pin_data, &cli.family, &cli.mcu)?,
        GenType::UsbPinMapping => usb::gen_mapping_header(&pin_data, &cli.family, &cli.mcu)?,
        GenType::TimPinMapping => tim::gen_mapping_header(&pin_data, &cli.family, &cli.mcu)?,
        GenType::TimPinMappingModule => tim::gen_mapping_module(&pin_data, &cli.family, &cli.mcu)?,
    };

    let mut out_file = File::create(cli.out_file.as_str())?;
    out_file.write_all(&header.as_bytes())?;
    format_generated_file(cli.out_file);
    Ok(())
}
