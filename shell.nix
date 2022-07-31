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
    # libXtst
    # libXi
    # libargon2
    minizip
    pcsclite
    qrencode
    # qtbase
    # qtsvg
    # qtx11extras
    readline
    zlib
  ]
  ++ lib.optional stdenv.isLinux libusb1;
  # ++ lib.optional stdenv.isDarwin qtmacextras;
}
