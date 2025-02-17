use crate::defs::{
    EnumInfo, EnumMemberInfo, FieldInfo, FixedPointType, MethodInfo, MsgInfo, MsgRef,
    ParameterGroup, ParameterService, ServiceInfo, ServiceType,
};
use crate::naming;
use crate::protos::generated::vrpc::fix::Fix;
use crate::protos::generated::vrpc::{UFix, UFixQ};
use crate::protos::generated::vrpc_options::{
    VrpcFieldOptions, VrpcMessageOptions, VrpcMethodOptions, VrpcServiceOptions, VrpcServiceType,
};
use anyhow::{anyhow, Context};
use protobuf::descriptor::field_descriptor_proto::{Label, Type};
use protobuf::descriptor::{
    DescriptorProto, EnumDescriptorProto, EnumValueDescriptorProto, FieldDescriptorProto,
    FileDescriptorProto, MethodDescriptorProto, ServiceDescriptorProto,
};
use protobuf::{Message, SpecialFields, UnknownValueRef};
use std::collections::{HashMap, HashSet};

#[derive(Debug, Clone)]
pub struct ProtoFile {
    pub file_name: String,
    pub relative_dir: String,

    pub relative_path: String,
    pub absolute_path: String,
}

#[derive(Clone)]
pub struct DescriptorSet {
    pub packages: HashMap<String, String>,

    pub proto_files: Vec<ProtoFile>,

    pub messages: HashMap<String, MsgInfo>,
    pub services: HashMap<String, ServiceInfo>,
    pub enums: HashMap<String, EnumInfo>,

    pub messages_by_file: HashMap<String, HashMap<String, MsgInfo>>,
    pub services_by_file: HashMap<String, HashMap<String, ServiceInfo>>,
    pub enums_by_file: HashMap<String, HashMap<String, EnumInfo>>,
}

impl DescriptorSet {
    pub fn new() -> DescriptorSet {
        DescriptorSet {
            packages: Default::default(),
            proto_files: Default::default(),
            messages: Default::default(),
            services: Default::default(),
            enums: Default::default(),
            messages_by_file: Default::default(),
            services_by_file: Default::default(),
            enums_by_file: Default::default(),
        }
    }

    pub(crate) fn process_descriptors(
        &mut self,
        mut proto_files: Vec<ProtoFile>,
        descriptors: &Vec<FileDescriptorProto>,
    ) -> anyhow::Result<()> {
        self.proto_files.append(&mut proto_files);

        for fdesc in descriptors {
            self.packages
                .insert(fdesc.name().to_string(), fdesc.package().to_string());
        }

        for fdesc in descriptors {
            self.enums_by_file
                .insert(fdesc.name().to_string(), HashMap::new());

            for enumeration in &fdesc.enum_type {
                self.process_enum(fdesc.name(), fdesc.package(), &enumeration)?;
            }
        }

        for fdesc in descriptors {
            self.messages_by_file
                .insert(fdesc.name().to_string(), HashMap::new());

            for message in &fdesc.message_type {
                self.process_msg(fdesc.name(), fdesc.package(), &message)?;
            }
        }

        for fdesc in descriptors {
            self.services_by_file
                .insert(fdesc.name().to_string(), HashMap::new());

            for service in &fdesc.service {
                self.process_service(fdesc.name(), fdesc.package(), &service)?;
            }
        }

        Ok(())
    }

    fn process_msg(&mut self, file: &str, pkg: &str, msg: &DescriptorProto) -> anyhow::Result<()> {
        // Determine full name
        let full_name = if pkg.is_empty() {
            format!(".{}", msg.name())
        } else {
            format!(".{}.{}", pkg, msg.name())
        };

        let mut fields: HashMap<String, FieldInfo> = HashMap::new();
        for field_desc in &msg.field {
            let (name, field) = Self::process_field(&field_desc)?;
            fields.insert(name, field);
        }

        self.messages_by_file.get_mut(file).unwrap().insert(
            full_name.clone(),
            MsgInfo {
                descriptor: msg.clone(),
                name: msg.name().to_string(),
                package: pkg.to_string(),
                full_name: full_name.clone(),
                fields: fields.clone(),
            },
        );
        self.messages.insert(
            full_name.clone(),
            MsgInfo {
                descriptor: msg.clone(),
                name: msg.name().to_string(),
                package: pkg.to_string(),
                full_name,
                fields,
            },
        );

        Ok(())
    }

    fn process_field(field: &FieldDescriptorProto) -> anyhow::Result<(String, FieldInfo)> {
        let options = Self::find_vrpc_options::<VrpcFieldOptions>(&field.options.special_fields)?;

        let mut unit: Option<String> = None;
        let mut description: Option<String> = None;

        let fixed_point = if let Some(fopts) = &options {
            match &fopts.fp.fix {
                Some(Fix::Ufix(UFix { w, f, .. })) => Some(FixedPointType::UFix(*w, *f)),
                Some(Fix::Ufixq(UFixQ { w, f, q, .. })) => Some(FixedPointType::UFixQ(*w, *f, *q)),
                None => None,
            }
        } else {
            None
        };

        if let Some(fopts) = &options {
            if !fopts.unit.is_empty() {
                unit = Some(fopts.unit.clone());
            }
            if !fopts.description.is_empty() {
                description = Some(fopts.description.clone());
            }
        }

        Ok((
            field.name().to_string(),
            FieldInfo {
                descriptor: field.clone(),
                id: field.number(),
                name: field.name().to_string(),
                fixed_point,
                unit,
                description,
            },
        ))
    }

    fn process_service(
        &mut self,
        file: &str,
        pkg: &str,
        service: &ServiceDescriptorProto,
    ) -> anyhow::Result<()> {
        let full_name = if pkg.is_empty() {
            format!(".{}", service.name())
        } else {
            format!(".{}.{}", pkg, service.name())
        };

        let vrpc_service_options_opt =
            Self::find_vrpc_options::<VrpcServiceOptions>(&service.options.special_fields)?;

        if let Some(svc_opts) = vrpc_service_options_opt {
            let mut svc_info = ServiceInfo {
                descriptor: service.clone(),
                name: service.name().to_string(),
                full_name: full_name.clone(),
                id: svc_opts.id,
                svc_type: match svc_opts.type_.enum_value() {
                    Ok(VrpcServiceType::VRPC_SERVICE_TYPE_DEFAULT) => ServiceType::Default,
                    Ok(VrpcServiceType::VRPC_SERVICE_TYPE_PARAMETERS) => {
                        ServiceType::Parameters(self.process_parameter_service(service)?)
                    }
                    Err(_) => ServiceType::Default,
                },
                identifier: svc_opts.identifier.clone(),
                display_name: svc_opts.display_name.clone(),
                methods: HashMap::new(),
            };

            for method in &service.method {
                self.process_method(&mut svc_info, &method)?;
            }

            self.services_by_file
                .get_mut(file)
                .unwrap()
                .insert(full_name.clone(), svc_info.clone());
            self.services.insert(full_name, svc_info);
        }

        Ok(())
    }

    fn process_parameter_service(
        &self,
        service: &ServiceDescriptorProto,
    ) -> anyhow::Result<ParameterService> {
        let mut param_groups: Vec<ParameterGroup> = Default::default();
        let mut read_methods: HashMap<String, ReadMethod> = Default::default();
        let mut write_methods: HashMap<String, WriteMethod> = Default::default();

        for method in service.method.clone() {
            let input_msg = self
                .messages
                .get(method.input_type())
                .context(format!("Unknown method input type {}", method.input_type()))?;
            let output_msg = self.messages.get(method.output_type()).context(format!(
                "Unknown method output type {}",
                method.output_type()
            ))?;

            if input_msg.full_name == ".vrpc.ReadParameterGroupRequest" {
                // Read method
                read_methods.insert(
                    output_msg.full_name.clone(),
                    ReadMethod {
                        method: method.clone(),
                        input_type: input_msg.clone(),
                        output_type: output_msg.clone(),
                    },
                );
            } else if output_msg.full_name == ".vrpc.WriteParametersResponse" {
                // Write method
                let written_fields_candidates = input_msg
                    .fields
                    .iter()
                    .filter(|(_, f)| {
                        f.descriptor.label() == Label::LABEL_REPEATED
                            && f.descriptor.type_() == Type::TYPE_UINT32
                    })
                    .collect::<Vec<_>>();

                if written_fields_candidates.len() != 1 {
                    return Err(anyhow!(
                        "Zero or multiple candidates for written fields field in parameter message {}",
                        input_msg.full_name
                    ));
                }

                let body_candidates = input_msg
                    .fields
                    .iter()
                    .filter(|(_, f)| f.descriptor.type_() == Type::TYPE_MESSAGE)
                    .collect::<Vec<_>>();

                if body_candidates.len() != 1 {
                    return Err(anyhow!(
                        "Zero or multiple candidates for body field in parameter message {}",
                        input_msg.full_name
                    ));
                }

                let (_, written_fields_field) = written_fields_candidates.first().unwrap();
                let (_, body_field) = body_candidates.first().unwrap();

                let param_msg_name = body_field.descriptor.type_name();
                write_methods.insert(
                    param_msg_name.to_string(),
                    WriteMethod {
                        method: method.clone(),
                        input_type: input_msg.clone(),
                        output_type: output_msg.clone(),
                        written_fields_field: (*written_fields_field).clone(),
                        body_field: (*body_field).clone(),
                    },
                );
            }
        }

        let read_types: HashSet<String> = read_methods.keys().cloned().collect();
        let write_types: HashSet<String> = write_methods.keys().cloned().collect();

        let unmatched_types: Vec<String> = read_types
            .symmetric_difference(&write_types)
            .cloned()
            .collect();
        if !unmatched_types.is_empty() {
            return Err(anyhow!(
                "Service contains parameters that miss either a read or write message: {}",
                unmatched_types.join(", ")
            ));
        }

        for msg in read_types {
            let mr = read_methods.get(&msg).unwrap();
            let mw = write_methods.get(&msg).unwrap();

            if let (Some(r_opts), Some(w_opts), Some(pm_opts)) = (
                Self::find_vrpc_options::<VrpcMethodOptions>(mr.method.options.special_fields())?,
                Self::find_vrpc_options::<VrpcMethodOptions>(mw.method.options.special_fields())?,
                Self::find_vrpc_options::<VrpcMessageOptions>(
                    mr.output_type.descriptor.options.special_fields(),
                )?,
            ) {
                let read_method =
                    Self::make_method_info(&mr.method, &mr.input_type, &mr.output_type, r_opts);
                let write_method =
                    Self::make_method_info(&mw.method, &mw.input_type, &mw.output_type, w_opts);

                param_groups.push(ParameterGroup {
                    id: pm_opts.id,
                    read_method,
                    write_method,
                    write_msg_written_fields: mw.written_fields_field.clone(),
                    write_msg_body: mw.body_field.clone(),
                })
            } else {
                return Err(anyhow!(
                    "Either read method ({}) or write method ({}) doesnt contain vRPC options",
                    mr.method.name(),
                    mw.method.name()
                ));
            }
        }

        Ok(ParameterService { param_groups })
    }

    fn process_method(
        &self,
        service: &mut ServiceInfo,
        method: &MethodDescriptorProto,
    ) -> anyhow::Result<()> {
        let input_msg = self
            .messages
            .get(method.input_type())
            .context(format!("Unknown method input type {}", method.input_type()))?;
        let output_msg = self.messages.get(method.output_type()).context(format!(
            "Unknown method output type {}",
            method.output_type()
        ))?;

        if input_msg.full_name == ".vrpc.EnableSignalStreaming" {
            // Signal
        } else {
            // Method
            if let Some(method_opts) =
                Self::find_vrpc_options::<VrpcMethodOptions>(method.options.special_fields())?
            {
                service.methods.insert(
                    method.name().to_string(),
                    Self::make_method_info(method, input_msg, output_msg, method_opts),
                );
            } else {
                return Err(anyhow::anyhow!(
                    "No vRPC method options on method {}",
                    method.name()
                ));
            }
        }

        Ok(())
    }

    fn make_method_info(
        method: &MethodDescriptorProto,
        input_msg: &MsgInfo,
        output_msg: &MsgInfo,
        method_opts: VrpcMethodOptions,
    ) -> MethodInfo {
        MethodInfo {
            descriptor: method.clone(),
            id: method_opts.id,
            name: method.name().to_string(),
            input_type: MsgRef {
                name: input_msg.name.clone(),
                package: input_msg.package.clone(),
                full_name: input_msg.full_name.clone(),
            },
            output_type: MsgRef {
                name: output_msg.name.clone(),
                package: output_msg.package.clone(),
                full_name: output_msg.full_name.clone(),
            },
        }
    }

    fn process_enum(
        &mut self,
        file: &str,
        pkg: &str,
        enumeration: &EnumDescriptorProto,
    ) -> anyhow::Result<()> {
        let full_name = if pkg.is_empty() {
            format!(".{}", enumeration.name())
        } else {
            format!(".{}.{}", pkg, enumeration.name())
        };

        let members = enumeration
            .value
            .iter()
            .map(|member| Self::process_enum_member(pkg, enumeration.name(), &member))
            .collect::<Result<Vec<_>, _>>()?;

        self.enums_by_file.get_mut(file).unwrap().insert(
            full_name.clone(),
            EnumInfo {
                descriptor: enumeration.clone(),
                name: enumeration.name().to_string(),
                package: pkg.to_string(),
                full_name: full_name.clone(),
                members: members.clone(),
            },
        );
        self.enums.insert(
            full_name.clone(),
            EnumInfo {
                descriptor: enumeration.clone(),
                name: enumeration.name().to_string(),
                package: pkg.to_string(),
                full_name,
                members,
            },
        );

        Ok(())
    }

    fn process_enum_member(
        pkg: &str,
        enum_name: &str,
        member: &EnumValueDescriptorProto,
    ) -> anyhow::Result<EnumMemberInfo> {
        let name_prefix = naming::pascal_case_to_upper_snake_case(enum_name);
        let full_prefix = format!("{name_prefix}_");
        let parsed_name = member.name().replace(&full_prefix, "");

        Ok(EnumMemberInfo {
            descriptor: member.clone(),
            name: member.name().to_string(),
            package: pkg.to_string(),
            parsed_name,
        })
    }

    fn find_vrpc_options<T>(special_fields: &SpecialFields) -> anyhow::Result<Option<T>>
    where
        T: Default + Message,
    {
        let mut vrpc_options_found = false;
        let mut result = T::default();

        for (id, ufield) in special_fields.unknown_fields() {
            if id == 1020 {
                if let UnknownValueRef::LengthDelimited(bytes) = ufield {
                    result.merge_from_bytes(bytes)?;
                    vrpc_options_found = true;
                }
            }
        }

        if vrpc_options_found {
            Ok(Some(result))
        } else {
            Ok(None)
        }
    }
}

#[derive(Debug)]
struct ReadMethod {
    method: MethodDescriptorProto,
    input_type: MsgInfo,
    output_type: MsgInfo,
}

#[derive(Debug)]
struct WriteMethod {
    method: MethodDescriptorProto,
    input_type: MsgInfo,
    output_type: MsgInfo,
    written_fields_field: FieldInfo,
    body_field: FieldInfo,
}
