#!/usr/bin/env python3
"""
Test suite for KeeShare Import MVP.

This script tests all scenarios from TEST_PLAN.md:
1. Valid signature verification
2. Invalid/unknown certificate handling
3. Corrupted share-file handling
4. Group import (empty db, existing groups)
5. Password handling (same, different, wrong)

Usage:
    pip install -r keeshare_import_requirements.txt
    pip install pytest
    python test_keeshare_import.py
"""

import base64
import io
import os
import shutil
import struct
import tempfile
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET

import pytest
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, rsa
from cryptography.hazmat.backends import default_backend
from pykeepass import PyKeePass, create_database

# Import the module under test
import keeshare_import


class TestFixtures:
    """Generate test fixtures for all test scenarios."""

    @staticmethod
    def generate_rsa_keypair():
        """Generate an RSA key pair for testing."""
        private_key = rsa.generate_private_key(
            public_exponent=65537,
            key_size=2048,
            backend=default_backend()
        )
        return private_key, private_key.public_key()

    @staticmethod
    def create_ssh_rsa_public_key(public_key: rsa.RSAPublicKey) -> bytes:
        """Serialize public key to ssh-rsa format (as KeeShare does)."""
        numbers = public_key.public_numbers()

        def encode_mpint(value: int) -> bytes:
            """Encode an integer as SSH mpint."""
            byte_length = (value.bit_length() + 8) // 8  # +8 for sign bit padding
            value_bytes = value.to_bytes(byte_length, "big")
            return struct.pack(">I", len(value_bytes)) + value_bytes

        result = io.BytesIO()
        # Key type
        key_type = b"ssh-rsa"
        result.write(struct.pack(">I", len(key_type)))
        result.write(key_type)
        # e (public exponent)
        result.write(encode_mpint(numbers.e))
        # n (modulus)
        result.write(encode_mpint(numbers.n))

        return result.getvalue()

    @staticmethod
    def create_signature_xml(signature_hex: str, signer: str, public_key_bytes: bytes) -> str:
        """Create KeeShare signature XML."""
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
        """Sign data using PKCS#1 v1.5 with SHA-256 (matching KeeShare)."""
        signature = private_key.sign(
            data,
            padding.PKCS1v15(),
            hashes.SHA256()
        )
        return signature.hex()

    @staticmethod
    def save_public_key_pem(public_key: rsa.RSAPublicKey, path: Path):
        """Save public key as PEM file."""
        pem = public_key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )
        path.write_bytes(pem)

    @staticmethod
    def create_test_database(path: Path, password: str, groups: list[dict] = None):
        """Create a test KeePass database with optional groups and entries."""
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
        """Create a valid KeeShare share file."""
        # Read the source database
        with open(source_db_path, "rb") as f:
            kdbx_data = f.read()

        # Sign the data
        signature_hex = TestFixtures.sign_data(kdbx_data, private_key)

        # Create public key in ssh-rsa format
        public_key = private_key.public_key()
        ssh_rsa_key = TestFixtures.create_ssh_rsa_public_key(public_key)

        # Create signature XML
        signature_xml = TestFixtures.create_signature_xml(signature_hex, signer, ssh_rsa_key)

        # Create ZIP file
        with zipfile.ZipFile(output_path, "w", zipfile.ZIP_DEFLATED) as zf:
            zf.writestr(keeshare_import.CONTAINER_FILENAME, kdbx_data)
            zf.writestr(keeshare_import.SIGNATURE_FILENAME, signature_xml)


class TestDataDir:
    """Manage test data directory."""

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
    """Create test environment with all necessary files."""
    env = TestDataDir()

    # Save certificates
    TestFixtures.save_public_key_pem(env.public_key, env.trusted_cert_path)
    TestFixtures.save_public_key_pem(env.other_public_key, env.other_cert_path)

    yield env

    env.cleanup()


# =============================================================================
# Test: ZIP Extraction
# =============================================================================

class TestZipExtraction:
    """Test ZIP file extraction functionality."""

    def test_extract_valid_share_file(self, test_env):
        """Test extracting a valid share file."""
        password = "testpass123"

        # Create source database
        TestFixtures.create_test_database(
            test_env.source_db_path, password,
            groups=[{"name": "TestGroup", "entries": [{"title": "Entry1", "password": "secret"}]}]
        )

        # Create share file
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.private_key
        )

        # Extract and verify
        kdbx_data, signature_xml = keeshare_import.extract_share_file(test_env.share_file_path)

        assert len(kdbx_data) > 0
        assert "KeeShare" in signature_xml
        assert "Signature" in signature_xml
        print("PASS: Valid share file extracted successfully")

    def test_extract_missing_kdbx(self, test_env):
        """Test extracting share file missing the KDBX."""
        # Create ZIP without KDBX
        with zipfile.ZipFile(test_env.share_file_path, "w") as zf:
            zf.writestr(keeshare_import.SIGNATURE_FILENAME, "<KeeShare/>")

        with pytest.raises(ValueError, match="missing"):
            keeshare_import.extract_share_file(test_env.share_file_path)
        print("PASS: Missing KDBX detected correctly")

    def test_extract_missing_signature(self, test_env):
        """Test extracting share file missing the signature."""
        # Create ZIP without signature
        with zipfile.ZipFile(test_env.share_file_path, "w") as zf:
            zf.writestr(keeshare_import.CONTAINER_FILENAME, b"dummy")

        with pytest.raises(ValueError, match="missing"):
            keeshare_import.extract_share_file(test_env.share_file_path)
        print("PASS: Missing signature detected correctly")

    def test_extract_corrupted_zip(self, test_env):
        """Test extracting corrupted ZIP file."""
        # Write garbage data
        test_env.share_file_path.write_bytes(b"not a zip file")

        with pytest.raises(Exception):
            keeshare_import.extract_share_file(test_env.share_file_path)
        print("PASS: Corrupted ZIP handled correctly")


# =============================================================================
# Test: Signature Verification
# =============================================================================

class TestSignatureVerification:
    """Test signature verification functionality."""

    def test_valid_signature(self, test_env):
        """Test verification with valid signature and matching certificate."""
        password = "testpass123"

        # Create source database
        TestFixtures.create_test_database(test_env.source_db_path, password)

        # Create share file signed with trusted key
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.private_key
        )

        # Extract and verify
        kdbx_data, signature_xml = keeshare_import.extract_share_file(test_env.share_file_path)
        signature_hex, signer, embedded_key = keeshare_import.parse_signature_xml(signature_xml)

        trusted_key = keeshare_import.load_trusted_certificate(test_env.trusted_cert_path)

        result = keeshare_import.verify_signature(kdbx_data, signature_hex, embedded_key, trusted_key)
        assert result is True
        print("PASS: Valid signature verified successfully")

    def test_invalid_certificate(self, test_env):
        """Test verification with non-matching certificate."""
        password = "testpass123"

        # Create source database
        TestFixtures.create_test_database(test_env.source_db_path, password)

        # Create share file signed with OTHER key (not trusted)
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.other_private_key  # Different key!
        )

        # Extract and verify against trusted cert
        kdbx_data, signature_xml = keeshare_import.extract_share_file(test_env.share_file_path)
        signature_hex, signer, embedded_key = keeshare_import.parse_signature_xml(signature_xml)

        trusted_key = keeshare_import.load_trusted_certificate(test_env.trusted_cert_path)

        result = keeshare_import.verify_signature(kdbx_data, signature_hex, embedded_key, trusted_key)
        assert result is False
        print("PASS: Invalid certificate detected correctly")

    def test_tampered_data(self, test_env):
        """Test verification with tampered KDBX data."""
        password = "testpass123"

        # Create source database
        TestFixtures.create_test_database(test_env.source_db_path, password)

        # Read original and sign it
        with open(test_env.source_db_path, "rb") as f:
            original_data = f.read()

        signature_hex = TestFixtures.sign_data(original_data, test_env.private_key)
        ssh_rsa_key = TestFixtures.create_ssh_rsa_public_key(test_env.public_key)
        signature_xml = TestFixtures.create_signature_xml(signature_hex, "Test", ssh_rsa_key)

        # Create ZIP with TAMPERED data
        tampered_data = original_data[:-10] + b"TAMPERED!!"
        with zipfile.ZipFile(test_env.share_file_path, "w") as zf:
            zf.writestr(keeshare_import.CONTAINER_FILENAME, tampered_data)
            zf.writestr(keeshare_import.SIGNATURE_FILENAME, signature_xml)

        # Verify should fail
        kdbx_data, sig_xml = keeshare_import.extract_share_file(test_env.share_file_path)
        sig_hex, _, embedded_key = keeshare_import.parse_signature_xml(sig_xml)
        trusted_key = keeshare_import.load_trusted_certificate(test_env.trusted_cert_path)

        result = keeshare_import.verify_signature(kdbx_data, sig_hex, embedded_key, trusted_key)
        assert result is False
        print("PASS: Tampered data detected correctly")


# =============================================================================
# Test: Group Import
# =============================================================================

class TestGroupImport:
    """Test group and entry import functionality."""

    def test_import_into_empty_database(self, test_env):
        """Test importing groups into an empty database."""
        password = "testpass123"

        # Create source database with groups
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

        # Create empty target database
        TestFixtures.create_test_database(test_env.target_db_path, password)

        # Create share file
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.private_key
        )

        # Extract and open databases
        kdbx_data, _ = keeshare_import.extract_share_file(test_env.share_file_path)

        # Write temp file for source
        temp_source = test_env.dir / "temp_source.kdbx"
        temp_source.write_bytes(kdbx_data)

        source_kp = PyKeePass(str(temp_source), password=password)
        target_kp = PyKeePass(str(test_env.target_db_path), password=password)

        # Import
        count = keeshare_import.import_groups(source_kp, target_kp)
        target_kp.save()

        # Verify
        assert count == 3  # 3 entries imported

        # Reload and check
        target_kp = PyKeePass(str(test_env.target_db_path), password=password)
        group1 = target_kp.find_groups(name="Group1", first=True)
        group2 = target_kp.find_groups(name="Group2", first=True)

        assert group1 is not None
        assert group2 is not None
        assert len(target_kp.find_entries(title="Entry1")) == 1
        assert len(target_kp.find_entries(title="Entry2")) == 1
        assert len(target_kp.find_entries(title="Entry3")) == 1
        print("PASS: Groups imported into empty database successfully")

    def test_import_no_duplicates(self, test_env):
        """Test that existing entries are not duplicated."""
        password = "testpass123"

        # Create source database
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

        # Create target database with SAME group and entry
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

        # Create share file
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.private_key
        )

        # Extract and import
        kdbx_data, _ = keeshare_import.extract_share_file(test_env.share_file_path)
        temp_source = test_env.dir / "temp_source.kdbx"
        temp_source.write_bytes(kdbx_data)

        source_kp = PyKeePass(str(temp_source), password=password)
        target_kp = PyKeePass(str(test_env.target_db_path), password=password)

        count = keeshare_import.import_groups(source_kp, target_kp)
        target_kp.save()

        # Verify no duplicates
        assert count == 0  # Entry already exists, should skip

        target_kp = PyKeePass(str(test_env.target_db_path), password=password)
        entries = target_kp.find_entries(title="SharedEntry")
        assert len(entries) == 1  # Only one entry, no duplicate
        print("PASS: No duplicates created for existing entries")


# =============================================================================
# Test: Password Handling
# =============================================================================

class TestPasswordHandling:
    """Test password scenarios."""

    def test_same_password(self, test_env):
        """Test import with same password for both databases."""
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

        # Extract and verify we can open both with same password
        kdbx_data, _ = keeshare_import.extract_share_file(test_env.share_file_path)
        temp_source = test_env.dir / "temp_source.kdbx"
        temp_source.write_bytes(kdbx_data)

        source_kp = PyKeePass(str(temp_source), password=password)
        target_kp = PyKeePass(str(test_env.target_db_path), password=password)

        count = keeshare_import.import_groups(source_kp, target_kp)
        assert count == 1
        print("PASS: Same password works correctly")

    def test_different_passwords(self, test_env):
        """Test import with different passwords for share and target."""
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

        # Extract and open with different passwords
        kdbx_data, _ = keeshare_import.extract_share_file(test_env.share_file_path)
        temp_source = test_env.dir / "temp_source.kdbx"
        temp_source.write_bytes(kdbx_data)

        source_kp = PyKeePass(str(temp_source), password=share_password)
        target_kp = PyKeePass(str(test_env.target_db_path), password=target_password)

        count = keeshare_import.import_groups(source_kp, target_kp)
        assert count == 1
        print("PASS: Different passwords work correctly")

    def test_wrong_password(self, test_env):
        """Test that wrong password is rejected."""
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
        print("PASS: Wrong password rejected correctly")


# =============================================================================
# Test: Data Integrity
# =============================================================================

class TestDataIntegrity:
    """Test that data is not modified on failure."""

    def test_no_changes_on_signature_failure(self, test_env):
        """Test that target database is not modified if signature fails."""
        password = "testpass"

        # Create target with known content
        TestFixtures.create_test_database(
            test_env.target_db_path, password,
            groups=[{"name": "OriginalGroup", "entries": [{"title": "OriginalEntry"}]}]
        )

        # Get original state
        original_kp = PyKeePass(str(test_env.target_db_path), password=password)
        original_entries = len(original_kp.entries)

        # Create source with different content
        TestFixtures.create_test_database(
            test_env.source_db_path, password,
            groups=[{"name": "NewGroup", "entries": [{"title": "NewEntry"}]}]
        )

        # Create share file signed with UNTRUSTED key
        TestFixtures.create_share_file(
            test_env.share_file_path,
            test_env.source_db_path,
            test_env.other_private_key  # Not trusted!
        )

        # Attempt import (should fail signature check)
        kdbx_data, signature_xml = keeshare_import.extract_share_file(test_env.share_file_path)
        signature_hex, _, embedded_key = keeshare_import.parse_signature_xml(signature_xml)
        trusted_key = keeshare_import.load_trusted_certificate(test_env.trusted_cert_path)

        result = keeshare_import.verify_signature(kdbx_data, signature_hex, embedded_key, trusted_key)
        assert result is False, "Signature should fail"

        # Verify target unchanged
        target_kp = PyKeePass(str(test_env.target_db_path), password=password)
        assert len(target_kp.entries) == original_entries
        assert target_kp.find_entries(title="NewEntry", first=True) is None
        print("PASS: Database unchanged after signature failure")


# =============================================================================
# Main Test Runner
# =============================================================================

def run_all_tests():
    """Run all tests and print summary."""
    print("=" * 60)
    print("KeeShare Import MVP - Test Suite")
    print("=" * 60)
    print()

    # Run pytest with verbose output
    exit_code = pytest.main([
        __file__,
        "-v",
        "--tb=short",
        "-x"  # Stop on first failure
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
