use crate::codegen::{
    format_generated_file, gen_client_module, gen_protocol_core_module, gen_server_module,
    gen_service_header, gen_uart_server_module, gen_uart_service_client_header,
    gen_uart_service_header,
};
use anyhow::{anyhow, Context};
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
    CoreModule,
    ServerModule,
    ClientModule,
    UartServerService,
    UartServiceClient,
    UartServerModule,
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

    #[arg(long)]
    module_name: Option<String>,
}

fn main() -> anyhow::Result<()> {
    let cli = Cli::parse();

    let mut proto_reader = ProtoReader::new();
    proto_reader.add_include_dirs(cli.common_dir.clone());
    proto_reader.parse_dir(&cli.src_dir)?;
    let descriptor_set = proto_reader.get_descriptor_set()?;

    for proto_file in &descriptor_set.proto_files {
        let proto_name = proto_file.relative_path.strip_suffix(".proto").unwrap();

        let (file_name, file_contents) = match cli.gen {
            GenType::Service => (
                format!("{proto_name}.h"),
                gen_service_header(&descriptor_set, &proto_file.relative_path)?,
            ),
            GenType::CoreModule => (
                format!("{proto_name}.cppm"),
                gen_protocol_core_module(
                    &descriptor_set,
                    &proto_file.relative_path,
                    cli.module_name
                        .as_ref()
                        .ok_or(anyhow!("module name is required when generating a module"))?,
                )?,
            ),
            GenType::ServerModule => (
                format!("{proto_name}_server.cppm"),
                gen_server_module(
                    &descriptor_set,
                    &proto_file.relative_path,
                    cli.module_name
                        .as_ref()
                        .ok_or(anyhow!("module name is required when generating a module"))?,
                )?,
            ),
            GenType::ClientModule => (
                format!("{proto_name}_client.cppm"),
                gen_client_module(
                    &descriptor_set,
                    &proto_file.relative_path,
                    cli.module_name
                        .as_ref()
                        .ok_or(anyhow!("module name is required when generating a module"))?,
                )?,
            ),
            GenType::UartServerService => (
                format!("{proto_name}_uart_server_service.h"),
                gen_uart_service_header(&descriptor_set, &proto_file.relative_path)?,
            ),
            GenType::UartServiceClient => (
                format!("{proto_name}_uart_service_client.h"),
                gen_uart_service_client_header(&descriptor_set, &proto_file.relative_path)?,
            ),
            GenType::UartServerModule => (
                format!("{proto_name}_uart_server.cppm"),
                gen_uart_server_module(
                    &descriptor_set,
                    &proto_file.relative_path,
                    cli.module_name
                        .as_ref()
                        .ok_or(anyhow!("module name is required when generating a module"))?,
                )?,
            ),
        };

        let out_file_path = if proto_file.relative_path.is_empty() {
            file_name
        } else {
            format!("{}/{}", &cli.dst_dir, file_name)
        };

        let mut out_file = File::create(out_file_path.clone())?;
        out_file.write_all(&file_contents.as_bytes())?;

        format_generated_file(&out_file_path);
    }

    Ok(())
}
