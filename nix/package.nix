{ stdenv }:
let
  pname = "puppyfetch";
  version = "0.1.0";
  cflags = "-Wall -Wextra -O2 -Wno-unused-result -Wno-unused-parameter";
in
stdenv.mkDerivation {
  inherit pname version;
  src = ../.;

  buildPhase = ''
    gcc ${cflags} puppyfetch.c
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp a.out $out/bin/puppyfetch
  '';
}
