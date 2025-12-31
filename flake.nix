{
  description = "A flake for c";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    chimera.url = "github:jt0w/chimera";
  };

  outputs = inputs @ {flake-parts, ...}:
    flake-parts.lib.mkFlake {inherit inputs;} {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "aarch64-darwin"
        "x86_64-darwin"
      ];

      perSystem = {
        self',
        pkgs,
        ...
      }: let
        name = "awio";
        version = "0.1.0";
      in {
        devShells.default = pkgs.mkShell {
          inputsFrom = [self'.packages.default];
        };

        packages = {
          default = pkgs.stdenv.mkDerivation {
            inherit version;
            pname = name;
            src = ./.;

            nativeBuildInputs = with pkgs; [
              gcc
            ];
            buildInputs = with pkgs;[
              inputs.chimera.packages.${system}.default
              sdl3
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
