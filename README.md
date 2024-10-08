# puppyfetch
puppyfetch is a system fetch for some Linux distributions.

![puppyfetch output preview](assets/updated-preview.png "puppyfetch preview")

### Building 
```sh
$ git clone https://github.com/makichiis/puppyfetch.git 
$ cd puppyfetch 
$ make 
```

### Installing 
```sh
$ sudo make install 
```

### Uninstalling 
```sh
$ sudo make uninstall 
```



### Nix
Just to run puppyfetch via Nix:
```sh
nix run "github:makichiis/puppyfetch"
```

to install on NixOS:
```nix
# flake.nix
{
  inputs = {
    puppyfetch.url = "github:makichiis/puppyfetch";
  };
}
```
```nix
# configuration.nix
{ inputs, pkgs, ... }: {
  environment.systemPackages = [
    inputs.puppyfetch.packages.${pkgs.stdenv.system}.puppyfetch
  ];
}
```
=======
# TODO
- [ ] Compatability with Android devices. 
- [ ] Compatability with Raspberry PI and other embedded chips running Linux.

# Contributions
If you want to add support for a certain OS/device(s) or add more aliases to 
the CPU list, feel free to PR and I'll review it.

