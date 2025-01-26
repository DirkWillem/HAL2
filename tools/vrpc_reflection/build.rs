use protobuf_codegen::{Codegen, CustomizeCallback};

fn main() {
    Codegen::new()
        .pure()
        .out_dir("src/protos/generated")
        .input("../../common/vrpc/vendor/nanopb/generator/proto/nanopb.proto")
        .input("../../common/vrpc/protos/common/vrpc_options.proto")
        .input("../../common/vrpc/protos/common/vrpc.proto")
        .include("../../common/vrpc/protos/common")
        .include("../../common/vrpc/vendor/nanopb/generator/proto")
        .run_from_script();
}
