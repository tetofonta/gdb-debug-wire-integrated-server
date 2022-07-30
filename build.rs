extern crate bindgen;

fn main() {

    println!("cargo:rustc-link-search=./cmake-build-debug");
    println!("cargo:rustc-link-lib=cdc");
    println!("cargo:rerun-if-changed=./lufa_rust_binding/bindings.h");

    let bindings = bindgen::Builder::default()
        .header("./lufa_rust_binding/bindings.h")
        .parse_callbacks(Box::new(bindgen::CargoCallbacks))
        .generate()
        .expect("Unable to generate bindings");

    bindings
        .write_to_file("./src/usb/lufa/bindings.rs")
        .expect("Couldn't write bindings!");
}