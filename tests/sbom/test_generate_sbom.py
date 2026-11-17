#!/usr/bin/env python3
# Copyright (C) 2026 KeePassXC Team <team@keepassxc.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 or (at your option)
# version 3 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""Standalone deployment SBOM regression tests (Python + spdx-tools)."""

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from spdx_tools.spdx.parser.json.json_parser import JsonLikeDictParser
from spdx_tools.spdx.validation.document_validator import validate_full_spdx_document


sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location('generate_sbom', ROOT / 'utils/generate-sbom.py')
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)
RELEASE_SPEC = importlib.util.spec_from_file_location('release_tool', ROOT / 'release-tool.py')
RELEASE = importlib.util.module_from_spec(RELEASE_SPEC)
RELEASE_SPEC.loader.exec_module(RELEASE)


def document():
    return {'spdxVersion': 'SPDX-2.3', 'SPDXID': 'SPDXRef-DOCUMENT', 'dataLicense': 'CC0-1.0',
            'name': 'fixture', 'documentNamespace': 'https://example.org/fixture',
            'creationInfo': {'created': '2026-01-01T00:00:00Z', 'creators': ['Tool: test']},
            'packages': [], 'files': [], 'relationships': []}


def package(name, identifier=None):
    return {'SPDXID': identifier or f'SPDXRef-{name}', 'name': name, 'versionInfo': '1.2.3',
            'downloadLocation': 'NOASSERTION', 'filesAnalyzed': False,
            'licenseConcluded': 'NOASSERTION', 'licenseDeclared': 'NOASSERTION'}


class SbomTestCase(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix='sbom test ')
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        self.deploy = self.directory / 'deploy'
        self.deploy.mkdir()
        self.sbom = MODULE.DeploymentSbom(document(), self.deploy, '2.8.0')

    def deploy_binary(self, name, magic=b'\x7fELF'):
        path = self.deploy / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(magic + b'example deployed bytes')
        return path

    def collect_deployed_files(self):
        self.sbom.collect_deployed_files(self.directory / 'output.spdx.json')

    def qt_fixture(self, tag_value=False, ownership='CONTAINS', directory='sbom'):
        source = document()
        source['packages'] = [package('Core'), package('pcre2'), package('moc')]
        source['packages'][0].update(
            versionInfo='6.8.0', licenseDeclared='LicenseRef-Qt', downloadLocation='git://code.qt.io/qt/qtbase.git',
            externalRefs=[{'referenceCategory': 'PACKAGE-MANAGER', 'referenceType': 'purl',
                           'referenceLocator': 'pkg:generic/Qt/Core@6.8.0'}])
        source['hasExtractedLicensingInfos'] = [{'licenseId': 'LicenseRef-Qt', 'extractedText': 'Example license'}]
        source['files'] = [{'SPDXID': 'SPDXRef-file', 'fileName': './lib/libQt6Core.so.6',
                            'checksums': [{'algorithm': 'SHA1', 'checksumValue': '0' * 40}]}]
        source['relationships'] = [
            {'spdxElementId': 'SPDXRef-Core', 'relationshipType': 'CONTAINS', 'relatedSpdxElement': 'SPDXRef-file'},
            {'spdxElementId': 'SPDXRef-Core', 'relationshipType': 'DEPENDS_ON', 'relatedSpdxElement': 'SPDXRef-pcre2'}]
        if ownership == 'CONTAINED_BY':
            source['relationships'][0] = {'spdxElementId': 'SPDXRef-file', 'relationshipType': ownership,
                                          'relatedSpdxElement': 'SPDXRef-Core'}
        elif ownership == 'hasFiles':
            source['relationships'].pop(0)
            source['packages'][0]['hasFiles'] = ['SPDXRef-file']
        prefix = self.directory / f'Qt-{tag_value}-{ownership}'
        path = prefix / directory / ('qtbase.spdx' if tag_value else 'qtbase.spdx.json')
        path.parent.mkdir(parents=True)
        if tag_value:
            from spdx_tools.spdx.writer.tagvalue.tagvalue_writer import write_document_to_file
            write_document_to_file(JsonLikeDictParser().parse(source), str(path), validate=False)
        else:
            path.write_text(json.dumps(source))
        return prefix

    def vcpkg_fixture(self, port, triplet, filename):
        installed = self.directory / 'vcpkg installed'
        info = installed / 'vcpkg/info'
        info.mkdir(parents=True, exist_ok=True)
        path = installed / triplet / 'share' / port / 'vcpkg.spdx.json'
        path.parent.mkdir(parents=True)
        source = document()
        source['packages'] = [package(port, 'SPDXRef-port'), package('abi', 'SPDXRef-binary')]
        source['packages'][0]['licenseDeclared'] = 'LicenseRef-vcpkg-null'
        path.write_text(json.dumps(source))
        (info / f'{port}_1.2.3_{triplet}.list').write_text(f'{triplet}/{filename}\n')
        return installed


class InventoryTests(SbomTestCase):
    def test_inventory_preserves_licenses_and_hashes_deployed_bytes(self):
        for name, magic in [('keepassxc.exe', b'MZ\0\0'), ('QtCore.framework/QtCore', b'\xcf\xfa\xed\xfe'),
                            ('libQt6Core.so.6', b'\x7fELF')]:
            path = self.deploy_binary(name, magic)
            path.write_bytes(path.read_bytes() + b'signature')
        self.sbom.document['files'] = [{'SPDXID': 'SPDXRef-qt-core',
                                      'fileName': './lib/../libQt6Core.so.6',
                                      'licenseConcluded': 'LGPL-3.0-only', 'copyrightText': 'Qt contributors'}]
        self.collect_deployed_files()
        self.assertEqual(len(self.sbom.binary_files), 3)
        self.sbom.finalize_document()
        files = {file['fileName']: file for file in self.sbom.document['files']}
        self.assertEqual(len(files), 3)
        self.assertEqual(files['keepassxc.exe']['licenseConcluded'], 'GPL-2.0-or-later')
        self.assertEqual(files['QtCore.framework/QtCore']['licenseConcluded'], 'NOASSERTION')
        self.assertEqual(files['QtCore.framework/QtCore']['copyrightText'], 'NOASSERTION')
        self.assertEqual(files['libQt6Core.so.6']['SPDXID'], 'SPDXRef-qt-core')
        self.assertEqual(files['libQt6Core.so.6']['licenseConcluded'], 'LGPL-3.0-only')
        self.assertEqual(files['libQt6Core.so.6']['copyrightText'], 'Qt contributors')
        sha1s = []
        for name, entry in files.items():
            contents = (self.deploy / name).read_bytes()
            sha1s.append(hashlib.sha1(contents).hexdigest())
            self.assertEqual({c['algorithm']: c['checksumValue'] for c in entry['checksums']},
                             {'SHA1': sha1s[-1], 'SHA256': hashlib.sha256(contents).hexdigest()})
        self.assertEqual(self.sbom.root['packageVerificationCode']['packageVerificationCodeValue'],
                         hashlib.sha1(''.join(sorted(sha1s)).encode()).hexdigest())

    def test_assets_are_excluded_including_syft_file_records(self):
        self.deploy_binary('usr/lib/libexample.so.1')
        (self.deploy / 'AppRun').write_text('#!/bin/sh\nexec usr/bin/keepassxc\n')
        for name in ('share/translations/qt_de.qm', 'share/icons/example.png',
                     'share/applications/example.desktop', 'share/doc/manual.html',
                     'usr/share/doc/qt6-wayland/copyright', 'COPYING'):
            path = self.deploy / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b'asset content')
        self.sbom.document['files'] = [{'SPDXID': 'SPDXRef-asset', 'fileName': './share/icons/example.png'}]
        self.sbom.document['files'][0]['licenseConcluded'] = 'LicenseRef-asset'
        self.sbom.document['hasExtractedLicensingInfos'] = [
            {'licenseId': 'LicenseRef-asset', 'extractedText': 'Asset license'}]
        self.sbom.root['hasFiles'] = ['SPDXRef-asset']
        self.sbom.add_relationship(self.sbom.root_id, 'CONTAINS', 'SPDXRef-asset')
        self.sbom.add_relationship('SPDXRef-asset', 'OTHER', self.sbom.root_id)
        self.collect_deployed_files()
        exclusions = MODULE.syft_asset_exclusions(self.deploy)
        self.assertIn('./share/icons/example.png', exclusions)
        self.assertIn('./COPYING', exclusions)
        self.assertIn('./usr/share/doc/qt6-wayland/copyright', exclusions)
        self.assertNotIn('./usr/lib/libexample.so.1', exclusions)
        self.assertNotIn('./AppRun', exclusions)
        self.assertNotIn('./share/doc/manual.html', exclusions)
        self.sbom.finalize_document()
        self.assertEqual({file['fileName'] for file in self.sbom.document['files']},
                         {'usr/lib/libexample.so.1', 'AppRun', 'share/doc/manual.html'})
        documentation = next(file for file in self.sbom.document['files'] if file['fileName'].endswith('.html'))
        self.assertEqual(documentation['licenseConcluded'], self.sbom.root['licenseDeclared'])
        self.assertEqual(documentation['copyrightText'], self.sbom.root['copyrightText'])
        self.assertEqual(documentation['fileTypes'], ['DOCUMENTATION'])
        self.assertIn({'spdxElementId': self.sbom.root_id, 'relationshipType': 'CONTAINS',
                       'relatedSpdxElement': documentation['SPDXID']}, self.sbom.document['relationships'])
        self.assertEqual(self.sbom.root['versionInfo'], '2.8.0')
        self.assertFalse(any('SPDXRef-asset' in rel.values() for rel in self.sbom.document['relationships']))
        self.assertNotIn('SPDXRef-asset', self.sbom.root['hasFiles'])
        self.assertEqual(self.sbom.document['hasExtractedLicensingInfos'], [])

    def test_canonical_filename_resolves_symlinks_within_deployment(self):
        target = self.deploy_binary('lib/libexample.so.1.2')
        alias = target.with_name('libexample.so.1')
        outside = self.directory / 'outside-deployment.so'
        outside.write_bytes(target.read_bytes())
        try:
            alias.symlink_to(target.name)
            (self.deploy / 'outside.so').symlink_to(outside)
        except OSError:
            self.skipTest('Symlink creation unavailable')
        self.assertEqual(self.sbom.resolve_deployed_path('./lib/../lib/libexample.so.1'),
                         'lib/libexample.so.1.2')
        with self.assertRaises(ValueError):
            self.sbom.resolve_deployed_path('../outside-deployment')
        self.collect_deployed_files()
        self.assertNotIn('outside.so', self.sbom.binary_files)

    def test_unidentified_libraries_are_recorded_once_without_invented_metadata(self):
        names = ('helper.exe', 'example.dll', 'libexample.so.1', 'Example.framework/Example')
        for name in names:
            self.deploy_binary(name)
        self.collect_deployed_files()
        known = package('known supplier', 'SPDXRef-known')
        self.sbom.document['packages'].append(known)
        self.sbom.attach_binaries(known['SPDXID'], {'example.dll'})
        self.sbom.add_unidentified_libraries()
        self.sbom.add_unidentified_libraries()
        self.sbom.finalize_document()
        packages = {p['name']: p for p in self.sbom.document['packages']}
        self.assertEqual(set(packages), {'KeePassXC', 'known supplier', 'libexample.so.1', 'Example'})
        for name in ('libexample.so.1', 'Example'):
            self.assertEqual(packages[name]['licenseDeclared'], 'NOASSERTION')
            self.assertNotIn('versionInfo', packages[name])
        self.assertEqual(len(self.sbom.document['packages']), 4)

    def test_syft_native_metadata_preserves_binary_evidence(self):
        for name, metadata in [('example.dll', {'legalCopyright': 'DLL supplier'}),
                               ('Example.framework/Example', {'copyright': 'Framework supplier'})]:
            with self.subTest(name=name):
                self.deploy_binary(name)
                artifact_id = hashlib.sha256(name.encode()).hexdigest()[:16]
                component = package(name, 'SPDXRef-Package-generic-' + artifact_id)
                component['licenseDeclared'] = 'MIT'
                self.sbom.document['packages'].append(component)
                self.collect_deployed_files()
                self.sbom.apply_syft_metadata({'artifacts': [{
                    'id': artifact_id, 'version': '1.2.3', 'metadata': metadata,
                    'locations': [{'path': '/' + name}]}]})
                self.assertEqual(component['licenseDeclared'], 'MIT')
                self.assertEqual(component['copyrightText'], next(iter(metadata.values())))
                self.assertIn(Path(name).name.lower(), self.sbom.accounted)
        self.sbom.finalize_document()


class SupplierTests(SbomTestCase):
    def test_linux_build_id_handles_modified_deployment(self):
        deployed = self.deploy_binary('libexample.so.1')
        original = self.directory / 'original.so'
        original.write_bytes(deployed.read_bytes())
        with patch.object(MODULE, 'query_local_command') as readelf:
            self.assertTrue(MODULE.matches_binary(deployed, original))
            readelf.assert_not_called()
        original.write_bytes(deployed.read_bytes() + b'different RPATH')
        with patch.object(MODULE, 'query_local_command', return_value='Build ID: 123abc'):
            self.assertTrue(MODULE.matches_binary(deployed, original))
        with patch.object(MODULE, 'query_local_command', side_effect=['Build ID: 123abc', 'Build ID: other']):
            self.assertFalse(MODULE.matches_binary(deployed, original))
        with patch.object(MODULE, 'query_local_command', return_value=''):
            self.assertFalse(MODULE.matches_binary(deployed, original))

    def test_distro_scan_excludes_unmatched_packages(self):
        self.deploy_binary('libexample.so.1')
        self.collect_deployed_files()
        source = document()
        source['packages'] = [package('example'), package('unused')]
        for matches in (False, True):
            with patch.object(MODULE.sys, 'platform', 'linux'), \
                    patch.object(MODULE.shutil, 'which', return_value='/usr/bin/dpkg-query'), \
                    patch.object(MODULE, 'query_local_command', return_value='example: /lib/libexample.so.1'), \
                    patch.object(MODULE, 'matches_binary', return_value=matches), \
                    patch.object(MODULE.subprocess, 'run', return_value=subprocess.CompletedProcess(
                        [], 0, stdout=json.dumps(source))) as scan:
                self.sbom.import_distro_packages('syft')
                self.assertEqual(scan.called, matches)
        self.assertEqual({p['name'] for p in self.sbom.document['packages']}, {'KeePassXC', 'example'})
        self.sbom.finalize_document()

    def test_vcpkg_selects_target_binaries_and_linked_archives(self):
        self.deploy_binary('botan-3.dll', b'MZ\0\0')
        self.collect_deployed_files()
        for port, triplet, filename in [('botan', 'x64-windows', 'bin/botan-3.dll'),
                                        ('host-only', 'x64-linux', 'bin/botan-3.dll'),
                                        ('unused', 'x64-windows', 'lib/unused.a'),
                                        ('linked', 'x64-windows', 'debug/lib/linked.a')]:
            installed = self.vcpkg_fixture(port, triplet, filename)
        inputs = self.directory / 'inputs.txt'
        inputs.write_text(f'LIBRARY|{installed}/x64-windows/debug/lib/linked.a\n'
                          'VENDORED|ykcore|src/thirdparty/ykcore/ykcore.c|1.20.0+keepassxc\n'
                          'VENDORED|zxcvbn|src/thirdparty/zxcvbn/zxcvbn.c|2.3+keepassxc\n')
        self.sbom.load_link_inputs(inputs)
        self.sbom.import_vcpkg_packages(str(installed), 'x64-windows')
        packages = {p['name']: p for p in self.sbom.document['packages']}
        self.assertEqual(set(packages), {'KeePassXC', 'botan', 'linked', 'ykcore', 'zxcvbn'})
        for port in ('botan', 'linked'):
            self.assertEqual(packages[port]['versionInfo'], '1.2.3')
            self.assertEqual(packages[port]['licenseDeclared'], 'NOASSERTION')
        for name, license_id, version in [('ykcore', 'BSD-2-Clause', '1.20.0+keepassxc'),
                                          ('zxcvbn', 'MIT', '2.3+keepassxc')]:
            self.assertEqual(packages[name]['licenseDeclared'], license_id)
            self.assertEqual(packages[name]['versionInfo'], version)
            self.assertIn({'spdxElementId': self.sbom.root_id, 'relationshipType': 'STATIC_LINK',
                           'relatedSpdxElement': packages[name]['SPDXID']}, self.sbom.document['relationships'])
        self.sbom.finalize_document()
        self.assertTrue(packages['botan']['filesAnalyzed'])
        self.assertFalse(packages['linked']['filesAnalyzed'])

    def test_qt_imports_binary_modules_and_transitive_components(self):
        self.deploy_binary('libQt6Core.so.6')
        for tag_value in (False, True):
            for ownership, directory in [('CONTAINS', 'sbom'), ('CONTAINED_BY', 'share/qt6/sbom'),
                                         ('hasFiles', 'share/Qt6/sbom')]:
                with self.subTest(tag_value=tag_value, ownership=ownership, directory=directory):
                    sbom = MODULE.DeploymentSbom(document(), self.deploy, '2.8.0')
                    sbom.collect_deployed_files(self.directory / 'output.spdx.json')
                    prefix = self.qt_fixture(tag_value, ownership, directory)
                    sbom.import_qt_packages(str(prefix), '6.8.0')
                    packages = {p['name']: p for p in sbom.document['packages']}
                    self.assertEqual(set(packages), {'KeePassXC', 'Qt Core', 'pcre2'})
                    qt = packages['Qt Core']
                    self.assertEqual(qt['versionInfo'], '6.8.0')
                    license_id = sbom.document['hasExtractedLicensingInfos'][0]['licenseId']
                    self.assertEqual(qt['licenseDeclared'], license_id)
                    self.assertNotEqual(license_id, 'LicenseRef-Qt')
                    sbom.finalize_document()
                    self.assertEqual(len(sbom.document['files']), 1)
                    deployed = sbom.document['files'][0]
                    self.assertIn(deployed['SPDXID'], MODULE.package_files(sbom.document)[qt['SPDXID']])
                    self.assertNotEqual(deployed['checksums'][0]['checksumValue'], '0' * 40)

    def test_qt_skips_source_inventories_and_duplicate_serializations(self):
        self.deploy_binary('libQt6Core.so.6')
        self.collect_deployed_files()
        prefix = self.qt_fixture()
        # Invalid content ensures none of these inventories reaches the parser.
        for name in ('qtbase.source.spdx', 'qtbase.source.spdx.json', 'qtbase.spdx'):
            (prefix / 'sbom' / name).write_text('must not be parsed')
        with patch.object(MODULE, 'load_spdx', wraps=MODULE.load_spdx) as load:
            self.sbom.import_qt_packages(str(prefix), '6.8.0')
        self.assertEqual([call.args[0].name for call in load.call_args_list], ['qtbase.spdx.json'])
        self.assertEqual({p['name'] for p in self.sbom.document['packages']}, {'KeePassXC', 'Qt Core', 'pcre2'})

    def test_qt_metadata_replaces_overlapping_vcpkg_port(self):
        for name in ('libQt6Core.so.6', 'libQt6Gui.so.6'):
            self.deploy_binary(name)
        self.collect_deployed_files()
        port = package('qtbase')
        port.update(sourceInfo='Metadata source: vcpkg:x64-linux/qtbase',
                    homepage='https://www.qt.io/', licenseDeclared='MIT',
                    description='Qt Base (Core, Gui, Widgets, Network, ...)',
                    summary='Qt Base', comment='This is the port recipe.',
                    externalRefs=[{'referenceCategory': 'PACKAGE-MANAGER', 'referenceType': 'purl',
                                   'referenceLocator': 'pkg:vcpkg/qtbase@1.2.3'}])
        self.sbom.document['packages'].append(port)
        for identifiers in self.sbom.binary_files.values():
            self.sbom.add_relationship(port['SPDXID'], 'CONTAINS', identifiers[0])
        self.sbom.add_relationship(self.sbom.root_id, 'DEPENDS_ON', port['SPDXID'])
        prefix = self.qt_fixture()
        path = prefix / 'sbom/qtbase.spdx.json'
        source = json.loads(path.read_text())
        gui = package('Gui')
        gui['downloadLocation'] = 'git://code.qt.io/qt/qtbase.git'
        gui['licenseDeclared'] = 'LGPL-3.0-only'
        gui['versionInfo'] = '6.8.0'
        gui['description'] = 'Qt GUI module'
        source['packages'].append(gui)
        source['files'].append({'SPDXID': 'SPDXRef-gui-file', 'fileName': './lib/libQt6Gui.so.6'})
        source['relationships'].append({'spdxElementId': gui['SPDXID'], 'relationshipType': 'CONTAINS',
                                        'relatedSpdxElement': 'SPDXRef-gui-file'})
        path.write_text(json.dumps(source))
        self.sbom.import_qt_packages(str(prefix), '6.8.0')
        packages = self.sbom.document['packages']
        self.assertNotIn('qtbase', {p['name'] for p in packages})
        for module in (p for p in packages if p['name'] in ('Qt Core', 'Qt Gui')):
            self.assertEqual(module['homepage'], port['homepage'])
            self.assertIn(port['externalRefs'][0], module['externalRefs'])
            self.assertNotEqual(module['licenseDeclared'], 'MIT')
            self.assertIn('vcpkg:x64-linux/qtbase', module['sourceInfo'])
        self.assertEqual(next(p for p in packages if p['name'] == 'Qt Gui')['versionInfo'], '6.8.0')
        core = next(p for p in packages if p['name'] == 'Qt Core')
        self.assertEqual(len(core['externalRefs']), 2)
        self.assertIn('pkg:generic/Qt/Core@6.8.0', {ref['referenceLocator'] for ref in core['externalRefs']})
        for field in ('description', 'summary', 'comment'):
            self.assertNotIn(field, core)
        self.assertEqual(next(p for p in packages if p['name'] == 'Qt Gui')['description'], 'Qt GUI module')
        self.assertFalse(any(port['SPDXID'] in rel.values() for rel in self.sbom.document['relationships']))
        self.assertIn('pcre2', {p['name'] for p in packages})
        self.sbom.finalize_document()

    def test_qt_merge_preserves_unmatched_vcpkg_files(self):
        for name in ('libQt6Core.so.6', 'unmatched.so'):
            self.deploy_binary(name)
        self.collect_deployed_files()
        port = package('qtbase')
        port['sourceInfo'] = 'Metadata source: vcpkg:x64-linux/qtbase'
        self.sbom.document['packages'].append(port)
        for identifiers in self.sbom.binary_files.values():
            # Exercise the inverse form of file ownership too.
            self.sbom.add_relationship(identifiers[0], 'CONTAINED_BY', port['SPDXID'])
        self.sbom.import_qt_packages(str(self.qt_fixture()), '6.8.0')
        owned = {r['spdxElementId'] for r in self.sbom.document['relationships']
                 if r['relationshipType'] == 'CONTAINED_BY' and r['relatedSpdxElement'] == port['SPDXID']}
        self.assertEqual(owned, set(self.sbom.binary_files['unmatched.so']))
        self.assertIn(port, self.sbom.document['packages'])
        relationships = list(self.sbom.document['relationships'])
        self.sbom.merge_vcpkg_qt_packages()
        self.assertEqual(self.sbom.document['relationships'], relationships)
        self.sbom.finalize_document()

    def test_qt_cross_repository_dependencies(self):
        self.deploy_binary('libQt6Core.so.6')
        self.collect_deployed_files()
        prefix = self.qt_fixture()
        qtbase = prefix / 'sbom/qtbase.spdx.json'
        base = json.loads(qtbase.read_text())
        external = document()
        external['documentNamespace'] = 'https://example.org/qt-thirdparty'
        # Reuse a local package ID to catch missing cross-document namespacing.
        external['packages'] = [package('embedded-library', 'SPDXRef-pcre2')]
        base['externalDocumentRefs'] = [{'externalDocumentId': 'DocumentRef-thirdparty',
                                        'spdxDocument': external['documentNamespace'],
                                        'checksum': {'algorithm': 'SHA1', 'checksumValue': '0' * 40}}]
        base['relationships'].append({'spdxElementId': 'SPDXRef-Core', 'relationshipType': 'DEPENDS_ON',
                                      'relatedSpdxElement': 'DocumentRef-thirdparty:SPDXRef-pcre2'})
        qtbase.write_text(json.dumps(base))
        (prefix / 'sbom/qt-thirdparty.spdx.json').write_text(json.dumps(external))
        self.sbom.import_qt_packages(str(prefix), '6.8.0')
        packages = {p['name']: p for p in self.sbom.document['packages']}
        self.assertEqual(set(packages), {'KeePassXC', 'Qt Core', 'pcre2', 'embedded-library'})
        self.assertNotEqual(packages['pcre2']['SPDXID'], packages['embedded-library']['SPDXID'])
        self.assertIn({'spdxElementId': packages['Qt Core']['SPDXID'], 'relationshipType': 'DEPENDS_ON',
                       'relatedSpdxElement': packages['embedded-library']['SPDXID']}, self.sbom.document['relationships'])
        self.sbom.finalize_document()

    def test_qt_64_fallback_without_vcpkg(self):
        self.deploy_binary('libQt6Core.so.6')
        self.deploy_binary('plugins/platforms/libqxcb.so')
        self.collect_deployed_files()
        self.sbom.import_vcpkg_packages('', '')
        self.sbom.import_qt_packages(str(self.directory / 'Qt 6.4'), '6.4.2')
        qt = self.sbom.document['packages'][-1]
        self.assertEqual((qt['name'], qt['versionInfo']), ('Qt', '6.4.2'))
        self.assertEqual(qt['copyrightText'], 'NOASSERTION')
        self.assertNotIn('libqxcb.so', self.sbom.accounted)
        self.sbom.finalize_document()

    def test_supplier_import_isolates_ids_licenses_and_dependency_cycles(self):
        source = document()
        source['packages'] = [package('library'), package('embedded'), package('unused-tool')]
        source['packages'][0].update(licenseDeclared='LicenseRef-custom', filesAnalyzed=True,
                                     hasFiles=['SPDXRef-original-file'],
                                     checksums=[{'algorithm': 'SHA1', 'checksumValue': '0' * 40}],
                                     packageVerificationCode={'packageVerificationCodeValue': '0' * 40},
                                     licenseInfoFromFiles=['LicenseRef-custom'])
        source['hasExtractedLicensingInfos'] = [{'licenseId': 'LicenseRef-custom', 'extractedText': 'Supplier A'}]
        source['relationships'] = [
            {'spdxElementId': 'SPDXRef-embedded', 'relationshipType': 'CONTAINED_BY',
             'relatedSpdxElement': 'SPDXRef-library'},
            {'spdxElementId': 'SPDXRef-embedded', 'relationshipType': 'DEPENDS_ON',
             'relatedSpdxElement': 'SPDXRef-library'}]
        before = json.dumps(source, sort_keys=True)
        a = self.sbom.import_supplier_packages(source, {'SPDXRef-library'}, 'supplier-a')
        self.assertEqual(json.dumps(source, sort_keys=True), before)
        source['hasExtractedLicensingInfos'][0]['extractedText'] = 'Supplier B'
        b = self.sbom.import_supplier_packages(source, {'SPDXRef-library'}, 'supplier-b')
        self.assertNotEqual(a, b)
        packages = {p['SPDXID']: p for p in self.sbom.document['packages']}
        for mapping in (a, b):
            self.assertEqual(set(mapping), {'SPDXRef-library', 'SPDXRef-embedded'})
            library = packages[mapping['SPDXRef-library']]
            self.assertFalse(library['filesAnalyzed'])
            for field in ('checksums', 'hasFiles', 'packageVerificationCode', 'licenseInfoFromFiles'):
                self.assertNotIn(field, library)
        self.sbom.finalize_document()
        licenses = {item['licenseId']: item['extractedText']
                    for item in self.sbom.document['hasExtractedLicensingInfos']}
        self.assertEqual(licenses[packages[a['SPDXRef-library']]['licenseDeclared']], 'Supplier A')
        self.assertEqual(licenses[packages[b['SPDXRef-library']]['licenseDeclared']], 'Supplier B')


class GeneratorTests(SbomTestCase):
    def test_main_writes_valid_release_and_debug_documents(self):
        self.deploy_binary('keepassxc')
        self.deploy_binary('libQt6Core.so.6')
        prefix = self.qt_fixture()
        source_path = prefix / 'sbom/qtbase.spdx.json'
        source = json.loads(source_path.read_text())
        source['packages'][0]['comment'] = 'Qt module comment'
        source['hasExtractedLicensingInfos'][0]['comment'] = 'License comment'
        source['hasExtractedLicensingInfos'].append({'licenseId': 'LicenseRef-unused', 'extractedText': 'Unused'})
        source_path.write_text(json.dumps(source))
        output = self.directory / 'nested/output.spdx.json'
        for debug in (False, True):
            with self.subTest(debug=debug):
                argv = ['generate-sbom.py', '--syft', 'syft', '--deploy-dir', str(self.deploy),
                        '--output', str(output), '--version', '2.8.0', '--qt-prefix', str(prefix),
                        '--qt-version', '6.8.0'] + (['--debug'] if debug else [])
                results = [subprocess.CompletedProcess([], 0, stdout=json.dumps(content))
                           for content in ({'artifacts': []}, document())]
                with patch.object(sys, 'argv', argv), \
                        patch.object(MODULE.subprocess, 'run', side_effect=results), \
                        patch.object(MODULE.DeploymentSbom, 'import_distro_packages'):
                    MODULE.main()
                written = json.loads(output.read_text())
                self.assertEqual(validate_full_spdx_document(JsonLikeDictParser().parse(written)), [])
                self.assertEqual(written['creationInfo']['creators'], ['Tool: KeePassXC Build System'])
                self.assertEqual('comment' in written['creationInfo'], debug)
                qt = next(p for p in written['packages'] if p['name'] == 'Qt Core')
                self.assertEqual('sourceInfo' in qt, debug)
                self.assertEqual('comment' in qt, debug)
                self.assertEqual(len(written['hasExtractedLicensingInfos']), 1)
                license_info = written['hasExtractedLicensingInfos'][0]
                self.assertEqual('comment' in license_info, debug)
                self.assertEqual(license_info['extractedText'], 'Example license')

    def test_failed_generation_removes_stale_output(self):
        output = self.directory / 'output.spdx.json'
        for phase in ('scan', 'convert', 'supplier', 'validation'):
            with self.subTest(phase=phase):
                output.write_text('stale output')
                argv = ['generate-sbom.py', '--syft', 'syft', '--deploy-dir', str(self.deploy),
                        '--output', str(output), '--version', '2.8.0']
                results = [subprocess.CompletedProcess([], 0, stdout=json.dumps(content))
                           for content in ({'artifacts': []}, document())]
                if phase in ('scan', 'convert'):
                    results[0 if phase == 'scan' else 1] = subprocess.CalledProcessError(1, ['syft', phase])
                if phase == 'validation':
                    invalid = document()
                    invalid['dataLicense'] = 'MIT'
                    results[1].stdout = json.dumps(invalid)
                with patch.object(sys, 'argv', argv), \
                        patch.object(MODULE.subprocess, 'run', side_effect=results), \
                        patch.object(MODULE.DeploymentSbom, 'import_distro_packages'), \
                        patch.object(MODULE, 'merge_qt_documents', side_effect=ValueError('Invalid supplier')):
                    if phase == 'supplier':
                        argv += ['--qt-prefix', str(self.qt_fixture())]
                    message = {'scan': 'syft', 'convert': 'syft', 'supplier': 'Invalid supplier',
                               'validation': 'Invalid generated SPDX document'}[phase]
                    with self.assertRaisesRegex((subprocess.CalledProcessError, ValueError), message):
                        MODULE.main()
                self.assertFalse(output.exists())


class ReleaseTests(SbomTestCase):
    def test_release_sidecars_append_to_full_artifact_names(self):
        output = self.directory / 'release'
        output.mkdir()
        for generator, artifact in [('ZIP', 'KeePassXC-2.8.0.zip'), ('WIX', 'KeePassXC-2.8.0.msi'),
                                    ('DragNDrop', 'KeePassXC-2.8.0.dmg'), (None, 'KeePassXC-2.8.0.AppImage')]:
            suffix = f'-{generator}' if generator else ''
            (self.directory / f'keepassxc{suffix}.spdx.json').write_text(artifact)
            RELEASE.Build._copy_sbom(self.directory, output, artifact, generator)
            self.assertEqual((output / f'{artifact}.spdx.json').read_text(), artifact)
        self.assertEqual(len(list(output.glob('*.spdx.json'))), 4)
        with self.assertRaises(RELEASE.Error):
            RELEASE.Build._copy_sbom(self.directory, output, 'missing', 'missing')

    def test_release_sbom_default_and_opt_out(self):
        parser = argparse.ArgumentParser()
        RELEASE.Build.setup_arg_parser(parser)
        for platform, builder in [('linux', 'build_linux'), ('win32', 'build_windows'), ('darwin', 'build_macos')]:
            for flags, enabled in [([], True), (['--no-sbom'], False)]:
                with self.subTest(platform=platform, flags=flags):
                    args = vars(parser.parse_args([
                        '2.8.0', '--snapshot', '--use-system-deps', '--yes', *flags,
                        '--src-dir', str(ROOT), '--output-dir', str(self.directory / 'release')]))
                    self.assertEqual(args['no_sbom'], not enabled)
                    self.assertNotIn('sbom', args)
                    with patch.object(RELEASE.sys, 'platform', platform), \
                            patch.object(RELEASE.Check, 'perform_basic_checks'), \
                            patch.object(RELEASE.Check, 'check_version_in_cmake'), \
                            patch.object(RELEASE.Build, builder) as build:
                        RELEASE.Build(parser).run(**args)
                    self.assertIn('-DWITH_SBOM=' + ('ON' if enabled else 'OFF'),
                                  build.call_args.kwargs['cmake_opts'])
                    self.assertEqual(build.call_args.kwargs['sbom'], enabled)
                    self.assertNotIn('no_sbom', build.call_args.kwargs)

    def test_appimage_sbom_runs_after_deployment_only_when_enabled(self):
        for enabled in (False, True):
            with self.subTest(sbom=enabled):
                case = self.directory / str(enabled)
                case.mkdir()
                app_dir = case / 'AppDir'
                desktop = app_dir / 'usr/share/applications/org.keepassxc.KeePassXC.desktop'
                desktop.parent.mkdir(parents=True)
                desktop.write_text('[Desktop Entry]\n')
                executable = app_dir / 'usr/bin/keepassxc'
                executable.parent.mkdir(parents=True)
                executable.touch()
                libssl = app_dir / 'usr/lib/libssl.so.3'
                libssl.parent.mkdir(parents=True)
                libssl.touch()
                output = case / 'release'
                output.mkdir()
                commands = []

                def run(command, **kwargs):
                    commands.append(command)
                    if command[0] == 'cmake':
                        self.assertFalse(libssl.exists())
                        self.assertIn('-DSBOM_DEPLOY_DIR=' + str(app_dir), command)
                        (case / 'keepassxc.spdx.json').write_text('sbom')

                with patch.object(RELEASE.Build, '_prepare_appimage_tool',
                                  side_effect=lambda name, *args: case / name), \
                        patch.object(RELEASE, '_run', side_effect=run):
                    RELEASE.Build(argparse.ArgumentParser())._build_linux_appimage(
                        '2.8.0', ROOT, output, app_dir, case, '/usr', 'x86_64', {}, sbom=enabled)
                self.assertEqual([Path(command[0]).name for command in commands],
                                 ['linuxdeploy', 'linuxdeploy-plugin-qt'] + (['cmake'] if enabled else []) + ['appimagetool'])
                self.assertEqual((output / 'KeePassXC-2.8.0-x86_64.AppImage.spdx.json').is_file(), enabled)


class CMakeTests(SbomTestCase):
    @unittest.skipUnless(shutil.which('cmake'), 'CMake unavailable')
    def test_component_installs_defer_sbom_until_cpack_prebuild(self):
        source = self.directory / 'source'
        build = self.directory / 'build'
        staging = self.directory / 'staging'
        (source / 'cmake').mkdir(parents=True)
        (source / 'utils').mkdir()
        shutil.copy(ROOT / 'cmake/GenerateSbom.cmake.in', source / 'cmake')
        (source / 'cmake/FindSyft.cmake').write_text('set(Syft_EXECUTABLE unused)\n')
        # Assert that deployment completed before the generator is invoked.
        (source / 'utils/generate-sbom.py').write_text(
            'import pathlib, sys\n'
            'deployment = pathlib.Path(sys.argv[sys.argv.index("--deploy-dir") + 1])\n'
            'assert (deployment / "qt-deployed").is_file()\n'
            'output = pathlib.Path(sys.argv[sys.argv.index("--output") + 1])\n'
            'output.write_text("complete")\n')
        (source / 'CMakeLists.txt').write_text(
            'cmake_minimum_required(VERSION 3.16)\n'
            'project(SbomInstallTest NONE)\n'
            'list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")\n'
            'install(CODE "file(WRITE \\"${CMAKE_INSTALL_PREFIX}/qt-deployed\\" complete)" '
            'COMPONENT Unspecified)\n'
            f'include("{ROOT.as_posix()}/cmake/KPXCSbom.cmake")\n')
        def cmake(*args):
            subprocess.run(['cmake', *map(str, args)], check=True, capture_output=True, text=True)

        cmake('-S', source, '-B', build, f'-DPython3_EXECUTABLE={sys.executable}',
              f'-DCMAKE_INSTALL_PREFIX={staging}')
        cmake('--install', build, '--component', 'Runtime')
        self.assertFalse((build / 'keepassxc.spdx.json').exists())
        cmake('--install', build, '--component', 'Unspecified')
        self.assertFalse((build / 'keepassxc.spdx.json').exists())
        cmake(f'-DCPACK_TEMPORARY_INSTALL_DIRECTORY={staging}', '-DCPACK_GENERATOR=fixture',
              '-P', build / 'GenerateSbom.cmake')
        self.assertEqual((build / 'keepassxc-fixture.spdx.json').read_text(), 'complete')
        (staging / 'qt-deployed').unlink()
        cmake('--install', build)
        self.assertEqual((build / 'keepassxc.spdx.json').read_text(), 'complete')


if __name__ == '__main__':
    unittest.main()
