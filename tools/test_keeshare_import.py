#!/usr/bin/env python3

import base64
import io
import shutil
import struct
import tempfile
import zipfile
from pathlib import Path

import pytest
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, rsa
from cryptography.hazmat.backends import default_backend
from pykeepass import PyKeePass, create_database

import keeshare_import


class TestFixtures:

    @staticmethod
    def generate_rsa_keypair():
        private_key = rsa.generate_private_key(
            public_exponent=65537,
            key_size=2048,
            backend=default_backend()
        )
        return private_key, private_key.public_key()

    @staticmethod
    def create_ssh_rsa_public_key(public_key: rsa.RSAPublicKey) -> bytes:
        numbers = public_key.public_numbers()

        def encode_mpint(value: int) -> bytes:
            byte_length = (value.bit_length() + 8) // 8
            value_bytes = value.to_bytes(byte_length, "big")
            return struct.pack(">I", len(value_bytes)) + value_bytes

        result = io.BytesIO()
        key_type = b"ssh-rsa"
        result.write(struct.pack(">I", len(key_type)))
        result.write(key_type)
        result.write(encode_mpint(numbers.e))
        result.write(encode_mpint(numbers.n))

        return result.getvalue()

    @staticmethod
    def create_signature_xml(signature_hex: str, signer: str, public_key_bytes: bytes) -> str:
        return f"""<?xml version="1.0" encoding="UTF-8"?>
<KeeShare>
    <Signature>rsa|{signature_hex}</Signature>
    <Certificate>
        <Signer>{signer}</Signer>
        <Key>{base64.b64encode(public_key_bytes).decode()}</Key>
    </Certificate>
</KeeShare>"""

    @staticmethod
    def sign_data(data: bytes, private_key: rsa.RSAPrivateKey) -> str:
        signature = private_key.sign(
            data,
            padding.PKCS1v15(),
            hashes.SHA256()
        )
        return signature.hex()

    @staticmethod
    def save_public_key_pem(public_key: rsa.RSAPublicKey, path: Path):
        pem = public_key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )
        path.write_bytes(pem)

    @staticmethod
    def create_test_database(path: Path, password: str, groups: list[dict] = None):
        kp = create_database(str(path), password=password)

        if groups:
            for group_data in groups:
                group = kp.add_group(kp.root_group, group_data["name"])
                for entry in group_data.get("entries", []):
                    kp.add_entry(
                        group,
                        title=entry["title"],
                        username=entry.get("username", ""),
                        password=entry.get("password", ""),
                        url=entry.get("url", ""),
                        notes=entry.get("notes", "")
                    )

        kp.save()
        return kp

    @staticmethod
    def create_share_file(output_path: Path, source_db_path: Path,
                          private_key: rsa.RSAPrivateKey, signer: str = "TestSigner"):
        with open(source_db_path, "rb") as f:
            kdbx_data = f.read()

        signature_hex = TestFixtures.sign_data(kdbx_data, private_key)
        public_key = private_key.public_key()
        ssh_rsa_key = TestFixtures.create_ssh_rsa_public_key(public_key)
        signature_xml = TestFixtures.create_signature_xml(signature_hex, signer, ssh_rsa_key)

        with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as zf:
            zf.writestr(keeshare_import.CONTAINER_FILENAME, kdbx_data)
            zf.writestr(keeshare_import.SIGNATURE_FILENAME, signature_xml)


class TestDataDir:

    def __init__(self):
        self.dir = Path(tempfile.mkdtemp(prefix="keeshare_test_"))
        self.private_key, self.public_key = TestFixtures.generate_rsa_keypair()
        self.other_private_key, self.other_public_key = TestFixtures.generate_rsa_keypair()

    def cleanup(self):
        shutil.rmtree(self.dir, ignore_errors=True)

    @property
    def trusted_cert_path(self) -> Path:
        return self.dir / "trusted_cert.pem"

    @property
    def other_cert_path(self) -> Path:
        return self.dir / "other_cert.pem"

    @property
    def target_db_path(self) -> Path:
        return self.dir / "target.kdbx"

    @property
    def source_db_path(self) -> Path:
        return self.dir / "source.kdbx"

    @property
    def share_file_path(self) -> Path:
        return self.dir / "share.kdbx.share"


@pytest.fixture
def test_env():
    env = TestDataDir()
    TestFixtures.save_public_key_pem(env.public_key, env.trusted_cert_path)
    TestFixtures.save_public_key_pem(env.other_public_key, env.other_cert_path)
    yield env
    env.cleanup()


class TestZipExtraction:

    def test_extract_valid_share_file(self, test_env):
        password = "testpass123"
        TestFixtures.create_test_database(
            test_env.source_db_path, password,
            groups=[{"name": "TestGroup", "entries": [{"title": "Entry1", "password": "secret"}]}]
        )
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.private_key
        )

        kdbx_data, signature_xml = keeshare_import.extract_share_file(test_env.share_file_path)

        assert len(kdbx_data) > 0
        assert "KeeShare" in signature_xml
        assert "Signature" in signature_xml

    def test_extract_missing_kdbx(self, test_env):
        with zipfile.ZipFile(test_env.share_file_path, "w") as zf:
            zf.writestr(keeshare_import.SIGNATURE_FILENAME, "<KeeShare/>")

        with pytest.raises(ValueError, match="missing"):
            keeshare_import.extract_share_file(test_env.share_file_path)

    def test_extract_missing_signature(self, test_env):
        with zipfile.ZipFile(test_env.share_file_path, "w") as zf:
            zf.writestr(keeshare_import.CONTAINER_FILENAME, b"dummy")

        with pytest.raises(ValueError, match="missing"):
            keeshare_import.extract_share_file(test_env.share_file_path)

    def test_extract_corrupted_zip(self, test_env):
        test_env.share_file_path.write_bytes(b"not a zip file")

        with pytest.raises(Exception):
            keeshare_import.extract_share_file(test_env.share_file_path)


class TestSignatureVerification:

    def test_valid_signature(self, test_env):
        password = "testpass123"
        TestFixtures.create_test_database(test_env.source_db_path, password)
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.private_key
        )

        kdbx_data, signature_xml = keeshare_import.extract_share_file(test_env.share_file_path)
        signature_hex, signer, embedded_key = keeshare_import.parse_signature_xml(signature_xml)
        trusted_key = keeshare_import.load_trusted_certificate(test_env.trusted_cert_path)

        result = keeshare_import.verify_signature(kdbx_data, signature_hex, embedded_key, trusted_key)
        assert result is True

    def test_invalid_certificate(self, test_env):
        password = "testpass123"
        TestFixtures.create_test_database(test_env.source_db_path, password)
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.other_private_key
        )

        kdbx_data, signature_xml = keeshare_import.extract_share_file(test_env.share_file_path)
        signature_hex, signer, embedded_key = keeshare_import.parse_signature_xml(signature_xml)
        trusted_key = keeshare_import.load_trusted_certificate(test_env.trusted_cert_path)

        result = keeshare_import.verify_signature(kdbx_data, signature_hex, embedded_key, trusted_key)
        assert result is False

    def test_tampered_data(self, test_env):
        password = "testpass123"
        TestFixtures.create_test_database(test_env.source_db_path, password)

        with open(test_env.source_db_path, "rb") as f:
            original_data = f.read()

        signature_hex = TestFixtures.sign_data(original_data, test_env.private_key)
        ssh_rsa_key = TestFixtures.create_ssh_rsa_public_key(test_env.public_key)
        signature_xml = TestFixtures.create_signature_xml(signature_hex, "Test", ssh_rsa_key)

        tampered_data = original_data[:-10] + b"TAMPERED!!"
        with zipfile.ZipFile(test_env.share_file_path, "w") as zf:
            zf.writestr(keeshare_import.CONTAINER_FILENAME, tampered_data)
            zf.writestr(keeshare_import.SIGNATURE_FILENAME, signature_xml)

        kdbx_data, sig_xml = keeshare_import.extract_share_file(test_env.share_file_path)
        sig_hex, _, embedded_key = keeshare_import.parse_signature_xml(sig_xml)
        trusted_key = keeshare_import.load_trusted_certificate(test_env.trusted_cert_path)

        result = keeshare_import.verify_signature(kdbx_data, sig_hex, embedded_key, trusted_key)
        assert result is False


class TestGroupImport:

    def test_import_into_empty_database(self, test_env):
        password = "testpass123"
        TestFixtures.create_test_database(
            test_env.source_db_path, password,
            groups=[
                {
                    "name": "Group1",
                    "entries": [
                        {"title": "Entry1", "username": "user1", "password": "pass1"},
                        {"title": "Entry2", "username": "user2", "password": "pass2"}
                    ]
                },
                {
                    "name": "Group2",
                    "entries": [
                        {"title": "Entry3", "username": "user3", "password": "pass3"}
                    ]
                }
            ]
        )
        TestFixtures.create_test_database(test_env.target_db_path, password)
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.private_key
        )

        kdbx_data, _ = keeshare_import.extract_share_file(test_env.share_file_path)
        temp_source = test_env.dir / "temp_source.kdbx"
        temp_source.write_bytes(kdbx_data)

        source_kp = PyKeePass(str(temp_source), password=password)
        target_kp = PyKeePass(str(test_env.target_db_path), password=password)

        count = keeshare_import.import_groups(source_kp, target_kp)
        target_kp.save()

        assert count == 3

        target_kp = PyKeePass(str(test_env.target_db_path), password=password)
        group1 = target_kp.find_groups(name="Group1", first=True)
        group2 = target_kp.find_groups(name="Group2", first=True)

        assert group1 is not None
        assert group2 is not None
        assert len(target_kp.find_entries(title="Entry1")) == 1
        assert len(target_kp.find_entries(title="Entry2")) == 1
        assert len(target_kp.find_entries(title="Entry3")) == 1

    def test_import_no_duplicates(self, test_env):
        password = "testpass123"
        TestFixtures.create_test_database(
            test_env.source_db_path, password,
            groups=[
                {
                    "name": "SharedGroup",
                    "entries": [
                        {"title": "SharedEntry", "username": "shareduser", "password": "sharedpass"}
                    ]
                }
            ]
        )
        TestFixtures.create_test_database(
            test_env.target_db_path, password,
            groups=[
                {
                    "name": "SharedGroup",
                    "entries": [
                        {"title": "SharedEntry", "username": "shareduser", "password": "oldpass"}
                    ]
                }
            ]
        )
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.private_key
        )

        kdbx_data, _ = keeshare_import.extract_share_file(test_env.share_file_path)
        temp_source = test_env.dir / "temp_source.kdbx"
        temp_source.write_bytes(kdbx_data)

        source_kp = PyKeePass(str(temp_source), password=password)
        target_kp = PyKeePass(str(test_env.target_db_path), password=password)

        count = keeshare_import.import_groups(source_kp, target_kp)
        target_kp.save()

        assert count == 0

        target_kp = PyKeePass(str(test_env.target_db_path), password=password)
        entries = target_kp.find_entries(title="SharedEntry")
        assert len(entries) == 1


class TestPasswordHandling:

    def test_same_password(self, test_env):
        password = "samepassword"
        TestFixtures.create_test_database(
            test_env.source_db_path, password,
            groups=[{"name": "TestGroup", "entries": [{"title": "TestEntry"}]}]
        )
        TestFixtures.create_test_database(test_env.target_db_path, password)
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.private_key
        )

        kdbx_data, _ = keeshare_import.extract_share_file(test_env.share_file_path)
        temp_source = test_env.dir / "temp_source.kdbx"
        temp_source.write_bytes(kdbx_data)

        source_kp = PyKeePass(str(temp_source), password=password)
        target_kp = PyKeePass(str(test_env.target_db_path), password=password)

        count = keeshare_import.import_groups(source_kp, target_kp)
        assert count == 1

    def test_different_passwords(self, test_env):
        share_password = "sharepass"
        target_password = "targetpass"
        TestFixtures.create_test_database(
            test_env.source_db_path, share_password,
            groups=[{"name": "TestGroup", "entries": [{"title": "TestEntry"}]}]
        )
        TestFixtures.create_test_database(test_env.target_db_path, target_password)
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.private_key
        )

        kdbx_data, _ = keeshare_import.extract_share_file(test_env.share_file_path)
        temp_source = test_env.dir / "temp_source.kdbx"
        temp_source.write_bytes(kdbx_data)

        source_kp = PyKeePass(str(temp_source), password=share_password)
        target_kp = PyKeePass(str(test_env.target_db_path), password=target_password)

        count = keeshare_import.import_groups(source_kp, target_kp)
        assert count == 1

    def test_wrong_password(self, test_env):
        correct_password = "correct"
        wrong_password = "wrong"
        TestFixtures.create_test_database(test_env.source_db_path, correct_password)
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.private_key
        )

        kdbx_data, _ = keeshare_import.extract_share_file(test_env.share_file_path)
        temp_source = test_env.dir / "temp_source.kdbx"
        temp_source.write_bytes(kdbx_data)

        with pytest.raises(Exception):
            PyKeePass(str(temp_source), password=wrong_password)


class TestDataIntegrity:

    def test_no_changes_on_signature_failure(self, test_env):
        password = "testpass"
        TestFixtures.create_test_database(
            test_env.target_db_path, password,
            groups=[{"name": "OriginalGroup", "entries": [{"title": "OriginalEntry"}]}]
        )

        original_kp = PyKeePass(str(test_env.target_db_path), password=password)
        original_entries = len(original_kp.entries)

        TestFixtures.create_test_database(
            test_env.source_db_path, password,
            groups=[{"name": "NewGroup", "entries": [{"title": "NewEntry"}]}]
        )
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.other_private_key
        )

        kdbx_data, signature_xml = keeshare_import.extract_share_file(test_env.share_file_path)
        signature_hex, _, embedded_key = keeshare_import.parse_signature_xml(signature_xml)
        trusted_key = keeshare_import.load_trusted_certificate(test_env.trusted_cert_path)

        result = keeshare_import.verify_signature(kdbx_data, signature_hex, embedded_key, trusted_key)
        assert result is False

        target_kp = PyKeePass(str(test_env.target_db_path), password=password)
        assert len(target_kp.entries) == original_entries
        assert target_kp.find_entries(title="NewEntry", first=True) is None


def run_all_tests():
    print("=" * 60)
    print("KeeShare Import MVP - Test Suite")
    print("=" * 60)
    print()

    exit_code = pytest.main([
        __file__,
        "-v",
        "--tb=short",
        "-x"
    ])

    print()
    print("=" * 60)
    if exit_code == 0:
        print("ALL TESTS PASSED!")
    else:
        print(f"TESTS FAILED (exit code: {exit_code})")
    print("=" * 60)

    return exit_code


if __name__ == "__main__":
    import sys
    sys.exit(run_all_tests())
