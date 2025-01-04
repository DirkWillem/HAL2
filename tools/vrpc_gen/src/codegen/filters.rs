use std::collections::HashMap;
use tera::{Filter, Tera, Value};
use vrpc_reflection::naming;

struct PackageToNamespaceFilter {}

impl Filter for PackageToNamespaceFilter {
    fn filter(&self, value: &Value, args: &HashMap<String, Value>) -> tera::Result<Value> {
        match value {
            Value::String(pkg_name) => Ok(Value::String(pkg_name.replace(".", "::"))),
            _ => Err(format!("Invalid package name {value}").into()),
        }
    }

    fn is_safe(&self) -> bool {
        true
    }
}

struct PackageToNanopbPrefixFilter {}

impl Filter for PackageToNanopbPrefixFilter {
    fn filter(&self, value: &Value, args: &HashMap<String, Value>) -> tera::Result<Value> {
        match value {
            Value::String(pkg_name) => Ok(Value::String(pkg_name.replace(".", "_"))),
            _ => Err(format!("Invalid package name {value}").into()),
        }
    }

    fn is_safe(&self) -> bool {
        true
    }
}

struct UpperSnakeCaseToPascalCase {}

impl Filter for UpperSnakeCaseToPascalCase {
    fn filter(&self, value: &Value, args: &HashMap<String, Value>) -> tera::Result<Value> {
        match value {
            Value::String(pkg_name) => Ok(Value::String(naming::upper_snake_case_to_pascal_case(
                &pkg_name,
            ))),
            _ => Err(format!("Invalid name {value}").into()),
        }
    }

    fn is_safe(&self) -> bool {
        true
    }
}

struct PascalCaseToLowerSnakeCase {}

impl Filter for crate::codegen::filters::PascalCaseToLowerSnakeCase {
    fn filter(&self, value: &Value, args: &HashMap<String, Value>) -> tera::Result<Value> {
        match value {
            Value::String(s) => Ok(Value::String(naming::pascal_case_to_lower_snake_case(&s))),
            _ => Err(format!("Invalid name {value}").into()),
        }
    }

    fn is_safe(&self) -> bool {
        true
    }
}

pub fn add_vrpc_filters(tera: &mut Tera) {
    tera.register_filter("package_to_namespace", PackageToNamespaceFilter {});
    tera.register_filter("package_to_nanopb_prefix", PackageToNanopbPrefixFilter {});
    tera.register_filter(
        "upper_snake_case_to_pascal_case",
        UpperSnakeCaseToPascalCase {},
    );
    tera.register_filter(
        "pascal_case_to_lower_snake_case",
        PascalCaseToLowerSnakeCase {},
    );
}
