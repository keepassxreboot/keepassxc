with (import <nixpkgs> {});

mkShell {
  buildInputs = [
    curl
    botan2
    # libXi
    # libXtst
    # libargon2
    minizip
    pcsclite
    qrencode
    # qtbase
    # qtsvg
    # qtx11extras
    readline
    zlib
  ];
  # ++ optional stdenv.isLinux libusb1
  # ++ optional stdenv.isDarwin qtmacextras;
}
