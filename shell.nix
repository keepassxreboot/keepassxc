{
  withKeePassBrowser ? true,
  withKeePassKeeShare ? true,
  withKeePassSSHAgent ? true,
  withKeePassNetworking ? true,
  withKeePassTouchID ? true,
  withKeePassYubiKey ? true,
  withKeePassFDOSecrets ? true,
}:

with (import <nixpkgs> {});

mkShell {
  buildInputs = [
    curl
    botan2
    xorg.libXtst
    xorg.libXi
    libargon2
    minizip
    pcsclite
    qrencode
    libsForQt5.qt5.qtbase
    libsForQt5.qt5.qtsvg
    libsForQt5.qt5.qtx11extras
    readline
    zlib
  ]
  ++ lib.optional stdenv.isLinux libusb1
  ++ lib.optional stdenv.isDarwin libsForQt5.qt5.qtmacextras;
}
