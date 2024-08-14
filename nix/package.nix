{ stdenv }:
let
  pname = "puppyfetch";
  version = "0.1.0";
in
stdenv.mkDerivation {
  inherit pname version;
  src = ../.;

  installPhase = ''
    mkdir -p $out/bin
    cp puppyfetch $out/bin/puppyfetch
  '';
}
