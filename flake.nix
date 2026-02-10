{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
  };

  outputs = { self, nixpkgs }: let
    system = "x86_64-linux";
    target = "riscv64-linux";
    # pkgs = import nixpkgs { localSystem = host; crossSystem = target; };
    pkgs = nixpkgs.legacyPackages.${system};
    cross = pkgs.pkgsCross.riscv64;
    llvm = pkgs.llvmPackages_latest;
    sysroot = cross.stdenv.cc.libc;
  in {
    devShells.${system}.default = cross.mkShell {
      name = "rvv";
      sysroot = sysroot;
      packages = [
        pkgs.qemu
        pkgs.gdb
        pkgs.clang-tools
        (pkgs.writeShellScriptBin "rcc" ''
          riscv64-unknown-linux-gnu-gcc -march=rv64gcv $@
        '')
        (pkgs.writeShellScriptBin "r++" ''
          riscv64-unknown-linux-gnu-g++ -march=rv64gcv -std=c++23 $@
        '')
      ];
    };
  };
}
