#!/usr/bin/env python3

import argparse
import base64
import struct
import sys
import tempfile
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET

from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, rsa
from pykeepass import PyKeePass


CONTAINER_FILENAME = "container.share.kdbx"
SIGNATURE_FILENAME = "container.share.signature"


def parse_signature_xml(xml_content: str) -> tuple[str, str, bytes]:
    root = ET.fromstring(xml_content)

    signature_elem = root.find(".//Signature")
    if signature_elem is None or not signature_elem.text:
        raise ValueError("Signature element not found in signature file")

    signature_text = signature_elem.text
    if not signature_text.startswith("rsa|"):
        raise ValueError(f"Unsupported signature format: {signature_text[:20]}")

    signature_hex = signature_text[4:]

    signer_elem = root.find(".//Certificate/Signer")
    signer_name = signer_elem.text if signer_elem is not None and signer_elem.text else "Unknown"

    key_elem = root.find(".//Certificate/Key")
    if key_elem is None or not key_elem.text:
        raise ValueError("Certificate key not found in signature file")

    public_key_bytes = base64.b64decode(key_elem.text)

    return signature_hex, signer_name, public_key_bytes


def parse_ssh_rsa_key(key_bytes: bytes) -> tuple[int, int]:
    offset = 0

    def read_bytes():
        nonlocal offset
        length = struct.unpack(">I", key_bytes[offset:offset+4])[0]
        offset += 4
        data = key_bytes[offset:offset+length]
        offset += length
        return data

    key_type = read_bytes()
    if key_type != b"ssh-rsa":
        raise ValueError(f"Expected ssh-rsa key type, got: {key_type}")

    e_bytes = read_bytes()
    n_bytes = read_bytes()

    e = int.from_bytes(e_bytes, "big")
    n = int.from_bytes(n_bytes, "big")

    return e, n


def load_trusted_certificate(cert_path: Path) -> rsa.RSAPublicKey:
    with open(cert_path, "rb") as f:
        pem_data = f.read()

    try:
        return serialization.load_pem_public_key(pem_data)
    except ValueError:
        pass

    try:
        from cryptography import x509
        cert = x509.load_pem_x509_certificate(pem_data)
        return cert.public_key()
    except ValueError:
        pass

    raise ValueError("Could not load certificate: not a valid PEM public key or certificate")


def verify_signature(data: bytes, signature_hex: str, embedded_key_bytes: bytes,
                     trusted_key: rsa.RSAPublicKey) -> bool:
    e, n = parse_ssh_rsa_key(embedded_key_bytes)

    trusted_numbers = trusted_key.public_numbers()
    if trusted_numbers.e != e or trusted_numbers.n != n:
        print("Warning: Embedded certificate does not match trusted certificate")
        return False

    signature = bytes.fromhex(signature_hex)

    try:
        trusted_key.verify(
            signature,
            data,
            padding.PKCS1v15(),
            hashes.SHA256()
        )
        return True
    except Exception as e:
        print(f"Signature verification failed: {e}")
        return False


def extract_share_file(share_path: Path) -> tuple[bytes, str]:
    with zipfile.ZipFile(share_path, "r") as zf:
        names = zf.namelist()

        if CONTAINER_FILENAME not in names:
            raise ValueError(f"Share file missing {CONTAINER_FILENAME}")
        if SIGNATURE_FILENAME not in names:
            raise ValueError(f"Share file missing {SIGNATURE_FILENAME}")

        kdbx_data = zf.read(CONTAINER_FILENAME)
        signature_xml = zf.read(SIGNATURE_FILENAME).decode("utf-8")

    return kdbx_data, signature_xml


def import_groups(source_kp: PyKeePass, target_kp: PyKeePass) -> int:
    imported_count = 0

    def import_group_recursive(source_group, target_parent):
        nonlocal imported_count

        target_group = target_kp.find_groups(name=source_group.name, group=target_parent, first=True)
        if target_group is None:
            target_group = target_kp.add_group(target_parent, source_group.name,
                                                icon=source_group.icon,
                                                notes=source_group.notes)
            print(f"  Created group: {source_group.name}")

        for entry in source_group.entries:
            existing = target_kp.find_entries(title=entry.title, username=entry.username,
                                               group=target_group, first=True)
            if existing is None:
                target_kp.add_entry(
                    target_group,
                    title=entry.title,
                    username=entry.username or "",
                    password=entry.password or "",
                    url=entry.url or "",
                    notes=entry.notes or "",
                    icon=entry.icon
                )
                print(f"  Imported entry: {entry.title}")
                imported_count += 1
            else:
                print(f"  Skipped existing entry: {entry.title}")

        for subgroup in source_group.subgroups:
            import_group_recursive(subgroup, target_group)

    source_root = source_kp.root_group
    target_root = target_kp.root_group

    for entry in source_root.entries:
        existing = target_kp.find_entries(title=entry.title, username=entry.username,
                                           group=target_root, first=True)
        if existing is None:
            target_kp.add_entry(
                target_root,
                title=entry.title,
                username=entry.username or "",
                password=entry.password or "",
                url=entry.url or "",
                notes=entry.notes or "",
                icon=entry.icon
            )
            print(f"  Imported entry: {entry.title}")
            imported_count += 1

    for subgroup in source_root.subgroups:
        import_group_recursive(subgroup, target_root)

    return imported_count


def main():
    parser = argparse.ArgumentParser(description="Import KeePass KeeShare share-files into a database")
    parser.add_argument("share_file", type=Path, help="Path to the .kdbx.share file")
    parser.add_argument("database", type=Path, help="Path to the target KeePass database")
    parser.add_argument("certificate", type=Path, help="Path to trusted certificate (PEM format)")
    parser.add_argument("--password", required=True, help="Password for both databases")
    parser.add_argument("--share-password", help="Password for share file (if different from main)")

    args = parser.parse_args()

    if not args.share_file.exists():
        print(f"Error: Share file not found: {args.share_file}")
        return 1

    if not args.database.exists():
        print(f"Error: Database not found: {args.database}")
        return 1

    if not args.certificate.exists():
        print(f"Error: Certificate not found: {args.certificate}")
        return 1

    share_password = args.share_password or args.password

    print(f"Loading trusted certificate from {args.certificate}...")
    try:
        trusted_key = load_trusted_certificate(args.certificate)
    except Exception as e:
        print(f"Error loading certificate: {e}")
        return 1

    print(f"Extracting share file {args.share_file}...")
    try:
        kdbx_data, signature_xml = extract_share_file(args.share_file)
    except Exception as e:
        print(f"Error extracting share file: {e}")
        return 1

    print("Parsing signature...")
    try:
        signature_hex, signer_name, embedded_key_bytes = parse_signature_xml(signature_xml)
        print(f"  Signer: {signer_name}")
    except Exception as e:
        print(f"Error parsing signature: {e}")
        return 1

    print("Verifying signature...")
    if not verify_signature(kdbx_data, signature_hex, embedded_key_bytes, trusted_key):
        print("Error: Signature verification FAILED - share file may be tampered!")
        return 1
    print("  Signature verified successfully!")

    print("Opening share database...")
    try:
        with tempfile.NamedTemporaryFile(suffix=".kdbx", delete=False) as tmp:
            tmp.write(kdbx_data)
            tmp_path = tmp.name

        source_kp = PyKeePass(tmp_path, password=share_password)
        Path(tmp_path).unlink()
    except Exception as e:
        print(f"Error opening share database: {e}")
        return 1

    print(f"Opening target database {args.database}...")
    try:
        target_kp = PyKeePass(str(args.database), password=args.password)
    except Exception as e:
        print(f"Error opening target database: {e}")
        return 1

    print("Importing groups and entries...")
    try:
        count = import_groups(source_kp, target_kp)
    except Exception as e:
        print(f"Error importing: {e}")
        return 1

    print("Saving database...")
    try:
        target_kp.save()
    except Exception as e:
        print(f"Error saving database: {e}")
        return 1

    print(f"\nSuccess! Imported {count} entries from share file.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
