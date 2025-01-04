use protobuf_codegen::{Codegen, CustomizeCallback};

fn main() {
    Codegen::new()
        .pure()
        .out_dir("src/protos/generated")
        .input("../../common/vrpc/vendor/nanopb/generator/proto/nanopb.proto")
        .input("../../common/vrpc/proto/common/vrpc_options.proto")
        .input("../../common/vrpc/proto/common/vrpc.proto")
        .include("../../common/vrpc/proto/common")
        .include("../../common/vrpc/vendor/nanopb/generator/proto")
        .run_from_script();
}
