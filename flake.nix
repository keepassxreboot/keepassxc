{
  description = "Flake for KeePassXC development";

  inputs = {
    flake-utils = {
      url = "github:numtide/flake-utils";
    };
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }: (
    flake-utils.lib.eachDefaultSystem (
      system: (
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in {
          devShell = pkgs.mkShell {
            buildInputs = with pkgs;
              [
                gnumake
                cmake
                gcc8
                curl
                botan2
                xorg.libXtst
                xorg.libXi
                libargon2
                minizip
                pcsclite
                qrencode
                asciidoctor
                libsForQt5.qt5.qtbase
                libsForQt5.qt5.qttools
                libsForQt5.qt5.qttranslations
                libsForQt5.qt5.qtsvg
                libsForQt5.qt5.qtx11extras
                libsForQt5.qt5.qtwayland
                readline
                zlib
              ]
              ++ lib.optional stdenv.isLinux pkgs.libusb1
              ++ lib.optional stdenv.isDarwin pkgs.libsForQt5.qt5.qtmacextras;
          };
        }
      )
    )
  );
}
