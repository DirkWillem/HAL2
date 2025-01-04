use protobuf::descriptor::{
    DescriptorProto, EnumDescriptorProto, EnumValueDescriptorProto, FieldDescriptorProto,
    MethodDescriptorProto, ServiceDescriptorProto,
};
use serde::Serialize;
use std::collections::HashMap;

#[derive(Serialize, Clone, Debug)]
pub struct MsgInfo {
    #[serde(skip_serializing)]
    pub descriptor: DescriptorProto,

    pub name: String,
    pub package: String,
    pub full_name: String,
    pub fields: HashMap<String, FieldInfo>,
}

#[derive(Serialize, Clone, Debug)]
pub struct MsgRef {
    pub name: String,
    pub package: String,
    pub full_name: String,
}

#[derive(Serialize, Clone, Debug)]
pub struct EnumMemberInfo {
    #[serde(skip_serializing)]
    pub descriptor: EnumValueDescriptorProto,

    pub name: String,
    pub package: String,
    pub parsed_name: String,
}

#[derive(Serialize, Clone, Debug)]
pub struct EnumInfo {
    #[serde(skip_serializing)]
    pub descriptor: EnumDescriptorProto,

    pub name: String,
    pub package: String,
    pub full_name: String,
    pub members: Vec<EnumMemberInfo>,
}

#[derive(Serialize, Clone, Debug)]
pub enum FixedPointType {
    UFix(u32, u32),
    UFixQ(u32, u32, i32),
    SFix(u32, u32),
    SFixQ(u32, u32, i32),
}

#[derive(Serialize, Clone, Debug)]
pub struct FieldInfo {
    #[serde(skip_serializing)]
    pub descriptor: FieldDescriptorProto,

    pub id: i32,
    pub name: String,
    pub fixed_point: Option<FixedPointType>,
    pub unit: Option<String>,
    pub description: Option<String>,
}

#[derive(Serialize, Clone, Debug)]
pub struct MethodInfo {
    #[serde(skip_serializing)]
    pub descriptor: MethodDescriptorProto,

    pub id: u32,
    pub name: String,
    pub input_type: MsgRef,
    pub output_type: MsgRef,
}

#[derive(Serialize, Clone, Debug)]
pub struct ParameterGroup {
    pub read_method: MethodInfo,
    pub write_method: MethodInfo,
    pub write_msg_written_fields: FieldInfo,
    pub write_msg_body: FieldInfo,
}

#[derive(Serialize, Clone, Debug)]
pub struct ParameterService {
    pub param_groups: Vec<ParameterGroup>,
}

#[derive(Serialize, Clone, Debug)]
pub enum ServiceType {
    Default,
    Parameters(ParameterService),
}

#[derive(Serialize, Clone, Debug)]
pub struct ServiceInfo {
    #[serde(skip_serializing)]
    pub descriptor: ServiceDescriptorProto,

    pub id: u32,
    pub identifier: String,
    pub svc_type: ServiceType,
    pub name: String,
    pub display_name: String,
    pub full_name: String,
    pub methods: HashMap<String, MethodInfo>,
}
