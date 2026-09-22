{
  description = "RAGE achievement enabler development environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { nixpkgs, ... }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in {
      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
          mingw = pkgs.pkgsCross.mingwW64;
        in {
          default = pkgs.mkShell {
            packages = [ pkgs.cmake pkgs.ninja mingw.stdenv.cc ];
          };
        });
    };
}
