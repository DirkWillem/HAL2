use crate::codegen::filters::add_vrpc_filters;
use std::path::PathBuf;
use std::process::Command;
use tera::Tera;
use vrpc_reflection::descriptor_set::DescriptorSet;

mod filters;

fn add_macros(tera: &mut Tera) -> anyhow::Result<()> {
    tera.add_raw_template("macros", include_str!("codegen/templates/macros.tera"))?;
    Ok(())
}

pub fn gen_service_header(
    processor: &DescriptorSet,
    proto_file_name: &str,
) -> anyhow::Result<String> {
    let mut tera = Tera::default();
    add_macros(&mut tera)?;
    tera.add_raw_template(
        "service_header",
        include_str!("codegen/templates/service.h.tera"),
    )?;
    tera.add_raw_template(
        "service_parameter_bases",
        include_str!("codegen/templates/service_parameter_bases.tera"),
    )?;

    add_vrpc_filters(&mut tera);

    let proto_base_name = proto_file_name.strip_suffix(".proto").unwrap();

    let mut context = tera::Context::new();
    context.insert("name", &proto_base_name);
    context.insert("package", &processor.packages[proto_file_name]);
    context.insert("messages", &processor.messages_by_file[proto_file_name]);
    context.insert("services", &processor.services_by_file[proto_file_name]);
    context.insert("enums", &processor.enums_by_file[proto_file_name]);

    Ok(tera.render("service_header", &context)?)
}

pub fn gen_protocol_core_module(
    processor: &DescriptorSet,
    proto_file_name: &str,
    module_name: &str,
) -> anyhow::Result<String> {
    let mut tera = Tera::default();
    add_macros(&mut tera)?;
    tera.add_raw_template(
        "protocol_core",
        include_str!("codegen/templates/protocol_core.cppm.tera"),
    )?;
    tera.add_raw_template(
        "service_parameter_bases",
        include_str!("codegen/templates/service_parameter_bases.tera"),
    )?;

    add_vrpc_filters(&mut tera);

    let proto_base_name = proto_file_name.strip_suffix(".proto").unwrap();

    let mut context = tera::Context::new();
    context.insert("module_name", &module_name);
    context.insert("name", &proto_base_name);
    context.insert("package", &processor.packages[proto_file_name]);
    context.insert("messages", &processor.messages_by_file[proto_file_name]);
    context.insert("services", &processor.services_by_file[proto_file_name]);
    context.insert("enums", &processor.enums_by_file[proto_file_name]);

    Ok(tera.render("protocol_core", &context)?)
}

pub fn gen_server_module(
    processor: &DescriptorSet,
    proto_file_name: &str,
    module_name: &str,
) -> anyhow::Result<String> {
    let mut tera = Tera::default();
    add_macros(&mut tera)?;
    tera.add_raw_template(
        "server_module",
        include_str!("codegen/templates/protocol_server.cppm.tera"),
    )?;
    tera.add_raw_template(
        "service_parameter_bases",
        include_str!("codegen/templates/service_parameter_bases.tera"),
    )?;

    add_vrpc_filters(&mut tera);

    let proto_base_name = proto_file_name.strip_suffix(".proto").unwrap();

    let mut context = tera::Context::new();
    context.insert("module_name", &module_name);
    context.insert("name", &proto_base_name);
    context.insert("package", &processor.packages[proto_file_name]);
    context.insert("messages", &processor.messages_by_file[proto_file_name]);
    context.insert("services", &processor.services_by_file[proto_file_name]);
    context.insert("enums", &processor.enums_by_file[proto_file_name]);

    Ok(tera.render("server_module", &context)?)
}

pub fn gen_client_module(
    processor: &DescriptorSet,
    proto_file_name: &str,
    module_name: &str,
) -> anyhow::Result<String> {
    let mut tera = Tera::default();
    add_macros(&mut tera)?;
    tera.add_raw_template(
        "client_module",
        include_str!("codegen/templates/protocol_client.cppm.tera"),
    )?;
    tera.add_raw_template(
        "service_parameter_bases",
        include_str!("codegen/templates/service_parameter_bases.tera"),
    )?;

    add_vrpc_filters(&mut tera);

    let proto_base_name = proto_file_name.strip_suffix(".proto").unwrap();

    let mut context = tera::Context::new();
    context.insert("module_name", &module_name);
    context.insert("name", &proto_base_name);
    context.insert("package", &processor.packages[proto_file_name]);
    context.insert("messages", &processor.messages_by_file[proto_file_name]);
    context.insert("services", &processor.services_by_file[proto_file_name]);
    context.insert("enums", &processor.enums_by_file[proto_file_name]);

    Ok(tera.render("client_module", &context)?)
}

pub fn gen_uart_service_header(
    processor: &DescriptorSet,
    proto_file_name: &str,
) -> anyhow::Result<String> {
    let mut tera = Tera::default();
    add_macros(&mut tera)?;
    tera.add_raw_template(
        "service_header",
        include_str!("codegen/templates/uart_service.h.tera"),
    )?;

    add_vrpc_filters(&mut tera);

    let proto_base_name = proto_file_name.strip_suffix(".proto").unwrap();

    let mut context = tera::Context::new();
    context.insert("name", &proto_base_name);
    context.insert("package", &processor.packages[proto_file_name]);
    context.insert("messages", &processor.messages_by_file[proto_file_name]);
    context.insert("services", &processor.services_by_file[proto_file_name]);
    context.insert("enums", &processor.enums_by_file[proto_file_name]);

    Ok(tera.render("service_header", &context)?)
}

pub fn gen_uart_server_module(
    processor: &DescriptorSet,
    proto_file_name: &str,
    module_name: &str,
) -> anyhow::Result<String> {
    let mut tera = Tera::default();
    add_macros(&mut tera)?;
    tera.add_raw_template(
        "uart_server_module",
        include_str!("codegen/templates/uart_server_module.cppm.tera"),
    )?;

    add_vrpc_filters(&mut tera);

    let proto_base_name = proto_file_name.strip_suffix(".proto").unwrap();

    let mut context = tera::Context::new();
    context.insert("module_name", &module_name);
    context.insert("name", &proto_base_name);
    context.insert("package", &processor.packages[proto_file_name]);
    context.insert("messages", &processor.messages_by_file[proto_file_name]);
    context.insert("services", &processor.services_by_file[proto_file_name]);
    context.insert("enums", &processor.enums_by_file[proto_file_name]);

    Ok(tera.render("uart_server_module", &context)?)
}

pub fn gen_uart_service_client_header(
    processor: &DescriptorSet,
    proto_file_name: &str,
) -> anyhow::Result<String> {
    let mut tera = Tera::default();
    add_macros(&mut tera)?;
    tera.add_raw_template(
        "service_client_macros",
        include_str!("codegen/templates/uart_service_client_macros.tera"),
    )?;
    tera.add_raw_template(
        "service_client_header",
        include_str!("codegen/templates/uart_service_client.h.tera"),
    )?;

    add_vrpc_filters(&mut tera);

    let proto_base_name = proto_file_name.strip_suffix(".proto").unwrap();

    let mut context = tera::Context::new();
    context.insert("name", &proto_base_name);
    context.insert("package", &processor.packages[proto_file_name]);
    context.insert("messages", &processor.messages_by_file[proto_file_name]);
    context.insert("services", &processor.services_by_file[proto_file_name]);
    context.insert("enums", &processor.enums_by_file[proto_file_name]);

    Ok(tera.render("service_client_header", &context)?)
}

pub fn gen_uart_client_module(
    processor: &DescriptorSet,
    proto_file_name: &str,
    module_name: &str,
) -> anyhow::Result<String> {
    let mut tera = Tera::default();
    add_macros(&mut tera)?;
    tera.add_raw_template(
        "service_client_macros",
        include_str!("codegen/templates/uart_service_client_macros.tera"),
    )?;
    tera.add_raw_template(
        "uart_client_module",
        include_str!("codegen/templates/uart_client_module.cppm.tera"),
    )?;

    add_vrpc_filters(&mut tera);

    let proto_base_name = proto_file_name.strip_suffix(".proto").unwrap();

    let mut context = tera::Context::new();
    context.insert("module_name", &module_name);
    context.insert("name", &proto_base_name);
    context.insert("package", &processor.packages[proto_file_name]);
    context.insert("messages", &processor.messages_by_file[proto_file_name]);
    context.insert("services", &processor.services_by_file[proto_file_name]);
    context.insert("enums", &processor.enums_by_file[proto_file_name]);

    Ok(tera.render("uart_client_module", &context)?)
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
