{ mkShell, gnumake, puppyfetch }:
mkShell {
  name = "Puppyfetch Developer Environment";
  inputsFrom = [
    puppyfetch
  ];
  packages = [
    gnumake
  ];
}
