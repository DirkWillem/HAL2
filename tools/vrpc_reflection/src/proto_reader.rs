use crate::descriptor_set::{DescriptorSet, ProtoFile};
use protobuf::descriptor::FileDescriptorProto;
use std::fs;
use std::path::Path;

fn list_proto_files<R: AsRef<Path>, P: AsRef<Path>>(
    root: R,
    dir: P,
) -> anyhow::Result<Vec<ProtoFile>> {
    let paths = fs::read_dir(dir.as_ref())?;
    let mut result: Vec<ProtoFile> = Vec::new();

    for entry_result in paths {
        let entry = entry_result?;
        let metadata = entry.metadata()?;

        if metadata.is_file() {
            let filename = entry.file_name().to_str().unwrap().to_string();
            if filename.ends_with(".proto") {
                let rel_path = entry.path().strip_prefix(root.as_ref())?.to_owned();

                result.push(ProtoFile {
                    file_name: entry.file_name().into_string().unwrap(),
                    relative_dir: format!("{}", rel_path.parent().unwrap().display()),
                    relative_path: format!("{}", rel_path.display()),
                    absolute_path: format!("{}", entry.path().display()),
                })
            }
        } else if metadata.is_dir() {
            let mut v2 = list_proto_files(root.as_ref(), entry.path())?;
            result.append(&mut v2);
        }
    }

    Ok(result)
}

pub struct ProtoReader {
    descriptors: Vec<FileDescriptorProto>,

    include_dirs: Vec<String>,
    proto_files: Vec<ProtoFile>,
}

impl ProtoReader {
    pub fn new() -> ProtoReader {
        ProtoReader {
            descriptors: Vec::default(),
            include_dirs: Vec::default(),
            proto_files: Vec::default(),
        }
    }

    pub fn add_include_dir(&mut self, dir: impl Into<String>) {
        self.include_dirs.push(dir.into());
    }

    pub fn add_include_dirs(&mut self, dirs: Vec<impl Into<String>>) {
        for dir in dirs {
            self.include_dirs.push(dir.into());
        }
    }

    pub fn parse_dir<R: AsRef<Path>>(&mut self, dir: R) -> anyhow::Result<()> {
        self.proto_files = list_proto_files(dir.as_ref(), dir.as_ref())?;

        let mut descriptors = protobuf_parse::Parser::new()
            .pure()
            .includes(self.include_dirs.iter())
            .include(dir.as_ref())
            .inputs(self.proto_files.iter().map(|pf| &pf.absolute_path))
            .parse_and_typecheck()?
            .file_descriptors;

        self.descriptors.append(&mut descriptors);

        Ok(())
    }

    pub fn get_descriptor_set(&self) -> anyhow::Result<DescriptorSet> {
        let mut processor = DescriptorSet::new();
        processor.process_descriptors(self.proto_files.clone(), &self.descriptors)?;
        Ok(processor)
    }
}
