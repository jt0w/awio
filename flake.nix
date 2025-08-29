{
  description = "A flake for c";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    chimera.url = "github:secretval/stc";
  };

  outputs =
    inputs@{ flake-parts, ... }:
    flake-parts.lib.mkFlake { inherit inputs; } {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
        "x86_64-darwin"
      ];

      perSystem =
        {
          self',
          pkgs,
          ...
        }:
        let
          name = "awio";
          version = "0.1.0";
        in
        {
          devShells.default = pkgs.mkShell {
            inputsFrom = [ self'.packages.default ];
          };

          packages = {
            default = pkgs.stdenv.mkDerivation {
              inherit version;
              pname = name;
              src = ./.;

              nativeBuildInputs = with pkgs; [
                wayland-scanner
              ];
              buildInputs = with pkgs; [
                gcc
                inputs.chimera.packages.${system}.default

                wayland
              ];

              buildPhase = ''
                gcc -o make make.c
                ./make
              '';

              installPhase = ''
                mkdir -p $out/bin
                mv build/${name} $out/bin
              '';
            };
          };
        };
    };
}
