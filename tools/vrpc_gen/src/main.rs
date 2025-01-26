use crate::codegen::{
    format_generated_file, gen_service_header, gen_uart_service_client_header,
    gen_uart_service_header,
};
use anyhow::Context;
use clap::{Parser, ValueEnum};
use protobuf::Message;
use regex;
use serde::Serialize;
use std::fs::{create_dir_all, File};
use std::io::Write;
use vrpc_reflection::proto_reader::ProtoReader;

mod codegen;

#[derive(ValueEnum, Clone, Serialize, Debug)]
enum GenType {
    Service,
    UartServerService,
    UartServiceClient,
}

#[derive(Parser, Debug)]
#[command(name = "vrpc_gen")]
#[command(version = "1.0")]
struct Cli {
    src_dir: String,
    dst_dir: String,

    #[arg(long)]
    common_dir: Vec<String>,

    #[arg(long)]
    gen: GenType,
}

fn main() -> anyhow::Result<()> {
    let cli = Cli::parse();

    let mut proto_reader = ProtoReader::new();
    proto_reader.add_include_dirs(cli.common_dir.clone());
    proto_reader.parse_dir(&cli.src_dir)?;
    let descriptor_set = proto_reader.get_descriptor_set()?;

    match cli.gen {
        GenType::Service => {
            for proto_file in &descriptor_set.proto_files {
                let hdr_str = gen_service_header(&descriptor_set, &proto_file.relative_path)?;

                let out_dir_path = if proto_file.relative_path.is_empty() {
                    cli.dst_dir.clone()
                } else {
                    format!("{}/{}", &cli.dst_dir, proto_file.relative_dir)
                };
                create_dir_all(out_dir_path)?;

                let out_file_path = format!(
                    "{}/{}.h",
                    &cli.dst_dir,
                    proto_file.relative_path.strip_suffix(".proto").unwrap()
                );

                let mut out_file = File::create(out_file_path.clone())?;
                out_file.write_all(&hdr_str.as_bytes())?;

                format_generated_file(&out_file_path);
            }
        }
        GenType::UartServerService => {
            for proto_file in &descriptor_set.proto_files {
                let hdr_str = gen_uart_service_header(&descriptor_set, &proto_file.relative_path)?;
                let out_dir_path = if proto_file.relative_path.is_empty() {
                    cli.dst_dir.clone()
                } else {
                    format!("{}/{}", &cli.dst_dir, proto_file.relative_dir)
                };
                create_dir_all(out_dir_path)?;

                let out_file_path = format!(
                    "{}/{}_uart_server_service.h",
                    &cli.dst_dir,
                    proto_file.relative_path.strip_suffix(".proto").unwrap()
                );

                let mut out_file = File::create(out_file_path.clone())?;
                out_file.write_all(&hdr_str.as_bytes())?;

                format_generated_file(&out_file_path);
            }
        }
        GenType::UartServiceClient => {
            for proto_file in &descriptor_set.proto_files {
                let hdr_str =
                    gen_uart_service_client_header(&descriptor_set, &proto_file.relative_path)?;
                let out_dir_path = if proto_file.relative_path.is_empty() {
                    cli.dst_dir.clone()
                } else {
                    format!("{}/{}", &cli.dst_dir, proto_file.relative_dir)
                };
                create_dir_all(out_dir_path)?;

                let out_file_path = format!(
                    "{}/{}_uart_service_client.h",
                    &cli.dst_dir,
                    proto_file.relative_path.strip_suffix(".proto").unwrap()
                );

                let mut out_file = File::create(out_file_path.clone())?;
                out_file.write_all(&hdr_str.as_bytes())?;

                format_generated_file(&out_file_path);
            }
        }
    }

    Ok(())
}
