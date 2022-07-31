with (import <nixpkgs> {});
mkShell {
  buildInputs = [
    cmake
    readline
    openssl
    # botan
    qrencode
    pcsclite
    libusb
  ];
}
