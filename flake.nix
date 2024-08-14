{
  description = "puppyfetch";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
  };

  outputs = { self, nixpkgs }:
  let
    systems = [ "x86_64-linux" "aarch64-linux" ];
    forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f {
      pkgs = import nixpkgs {
        inherit system;
      };
      inherit system;
    });
  in
  {
    packages = forAllSystems ({ pkgs, system }: {
      default = pkgs.callPackage ./nix/package.nix { };
      puppyfetch = self.packages.${system}.default;
    });

    devShells = forAllSystems ({ pkgs, system }: {
      default =
        pkgs.callPackage ./nix/shell.nix { inherit (self.packages.${system}) puppyfetch; };
    });
  };
}
