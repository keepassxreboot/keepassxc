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

"""
Scan a deployment and enrich its SPDX 2.3 SBOM with supplier metadata.

Supplier file lists select components; their original checksums are deliberately
not imported, because deployqt, stripping and signing can modify those files.
"""

import argparse
from collections import defaultdict
from datetime import datetime
import fnmatch
from functools import lru_cache
import glob
import hashlib
import io
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import uuid


def transform_strings(value, transform):
    """Rewrite SPDX identifiers and license expressions without changing inputs."""
    if isinstance(value, dict):
        return {key: transform_strings(item, transform) for key, item in value.items()}
    if isinstance(value, list):
        return [transform_strings(item, transform) for item in value]
    return transform(value) if isinstance(value, str) else value


def load_spdx(path):
    """Read an SPDX 2.x supplier document in JSON or tag/value format."""
    if path.suffix == '.json':
        with path.open(encoding='utf-8') as stream:
            document = json.load(stream)
    else:
        # Qt ships both SPDX tag/value and JSON, depending on its version.
        from spdx_tools.spdx.parser.parse_anything import parse_file
        from spdx_tools.spdx.writer.json.json_writer import write_document_to_stream
        stream = io.StringIO()
        write_document_to_stream(parse_file(str(path)), stream, validate=False)
        document = json.loads(stream.getvalue())
    if not document.get('spdxVersion', '').startswith('SPDX-2.'):
        raise ValueError(f'Unsupported supplier SBOM format: {path}')
    return document


def is_binary_file(path):
    """Identify ELF, PE, and Mach-O files by their magic bytes."""
    with path.open('rb') as stream:
        magic = stream.read(4)
    return magic[:2] == b'MZ' or magic in (
        b'\x7fELF', b'\xfe\xed\xfa\xce', b'\xce\xfa\xed\xfe',
        b'\xfe\xed\xfa\xcf', b'\xcf\xfa\xed\xfe',
        b'\xca\xfe\xba\xbe', b'\xbe\xba\xfe\xca',
        b'\xca\xfe\xba\xbf', b'\xbf\xba\xfe\xca')


def keep_file(path):
    """Path filter for keeping only executables, libraries, scripts, and HTML documentation."""
    if not path.is_file():
        return False
    if path.suffix.lower() == '.html':
        return True
    with path.open('rb') as stream:
        header = stream.read(8)
    return header.startswith((b'#!', b'!<arch>\n')) or is_binary_file(path)


def syft_asset_exclusions(directory):
    """Exclude unwanted assets before Syft performs metadata or license analysis."""
    return [argument for path in sorted(directory.rglob('*'))
            if path.is_file() and not keep_file(path)
            for argument in ('--exclude', './' + glob.escape(path.relative_to(directory).as_posix()))]


def hash_file(path):
    """Hash the file's bytes for SPDX verification and integrity checks."""
    digests = {'SHA1': hashlib.sha1(), 'SHA256': hashlib.sha256()}
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            for digest in digests.values():
                digest.update(block)
    return [{'algorithm': name, 'checksumValue': digest.hexdigest()} for name, digest in digests.items()]


@lru_cache(maxsize=None)
def query_local_command(*args):
    """Return cached command output, or an empty string if the lookup fails."""
    try:
        result = subprocess.run(args, capture_output=True, text=True, errors='replace',
                                timeout=30, env=dict(os.environ, LC_ALL='C'))
        return result.stdout.strip() if result.returncode == 0 else ''
    except (OSError, subprocess.TimeoutExpired):
        return ''


def matches_binary(deployed, original):
    """Match a deployed file to its source using hashes or ELF build IDs."""
    if not original.is_file():
        return False
    if hash_file(deployed) == hash_file(original):
        return True
    ids = [re.search(r'Build ID:\s*([0-9a-f]+)', query_local_command('readelf', '-n', str(path)))
           for path in (deployed, original)]
    return bool(all(ids) and ids[0][1] == ids[1][1])


def package_files(document):
    """Index package ownership of file records, including inverse relationships."""
    files = {file['SPDXID'] for file in document.get('files', [])}
    owned = defaultdict(set)
    for rel in document.get('relationships', []):
        owner, child = rel['spdxElementId'], rel['relatedSpdxElement']
        if rel['relationshipType'] == 'CONTAINED_BY':
            owner, child = child, owner
        elif rel['relationshipType'] != 'CONTAINS':
            continue
        if child in files:
            owned[owner].add(child)
    for package in document.get('packages', []):
        identifiers = files.intersection(package.get('hasFiles', []))
        if identifiers:
            owned[package['SPDXID']].update(identifiers)
    return owned


def qt_sbom_paths(prefix):
    """Find Qt binary SBOMs, preferring JSON and excluding REUSE source inventories."""
    if not prefix:
        return []
    paths = {path for suffix in ('sbom', 'share/qt6/sbom', 'share/Qt6/sbom')
             for pattern in ('*.spdx', '*.spdx.json')
             for path in (Path(prefix) / suffix).glob(pattern)
             if not path.name.endswith(('.source.spdx', '.source.spdx.json'))}
    return sorted(path for path in paths if Path(str(path) + '.json') not in paths)


def merge_qt_documents(paths):
    """Resolve references between Qt repository SBOMs before selecting modules."""
    sources = [(path, load_spdx(path)) for path in paths]
    prefixes = {source['documentNamespace']: f'qt{i}' for i, (_, source) in enumerate(sources)}
    combined = {'packages': [], 'files': [], 'relationships': [], 'hasExtractedLicensingInfos': []}
    for path, source in sources:
        prefix = prefixes[source['documentNamespace']]
        external = {ref['externalDocumentId']: prefixes.get(ref['spdxDocument'])
                    for ref in source.get('externalDocumentRefs', [])}

        def namespace_qt_identifier(match):
            """Resolve an SPDX identifier against its local or referenced Qt document."""
            foreign, kind, suffix = match.groups()
            namespace = external.get(foreign) if foreign else prefix
            return match[0] if namespace is None else f'{kind}-{namespace}-{suffix}'

        for field in combined:
            items = transform_strings(source.get(field, []), lambda value: re.sub(
                r'(?:(DocumentRef-[\w.-]+):)?(SPDXRef|LicenseRef)-([\w.-]+)', namespace_qt_identifier, value))
            if field == 'packages':
                for package in items:
                    prefix_qt_package_name(package)
                    package['sourceInfo'] = (package.get('sourceInfo', '')
                                             + f' Qt supplier document: {path.name}').strip()
            combined[field].extend(items)
    return combined


def prefix_qt_package_name(package):
    """Identify Qt's own components without renaming bundled third-party code."""
    if re.match(r'^(?:git|https?)://code\.qt\.io/qt/', package.get('downloadLocation', '')):
        name = package['name']
        if not name.startswith('Qt '):
            package['name'] = 'Qt ' + re.sub(r'^Qt(?=[A-Z])', '', name)


class DeploymentSbom:
    def __init__(self, document, deploy_dir, version):
        """Initialize a deployment inventory from Syft's converted SPDX document."""
        self.document = document
        self.deploy_dir = deploy_dir
        self.binary_files = {}
        self.accounted = set()
        self.notes = []
        self.linked_files = set()
        self.copying_rules = self.load_copying_rules()
        described = document.get('documentDescribes', [])
        described += [r['relatedSpdxElement'] for r in document.get('relationships', [])
                      if r['spdxElementId'] == 'SPDXRef-DOCUMENT' and r['relationshipType'] == 'DESCRIBES']
        self.root = next((p for p in document.get('packages', []) if p['SPDXID'] in described), None)
        if self.root is None:
            self.root = {'SPDXID': 'SPDXRef-KeePassXC', 'downloadLocation': 'NOASSERTION',
                         'filesAnalyzed': False}
            document.setdefault('packages', []).append(self.root)
        self.root.update(name='KeePassXC', versionInfo=version, primaryPackagePurpose='APPLICATION')
        self.root['licenseDeclared'] = 'GPL-2.0-or-later'
        self.root['copyrightText'] = f'Copyright (C) 2016-{datetime.now().year} KeePassXC Team <team@keepassxc.org>'
        self.root['comment'] = 'License and copyright notice are from the project COPYING file.'
        self.root_id = self.root['SPDXID']
        document['documentDescribes'] = [self.root_id]
        document['relationships'] = [r for r in document.get('relationships', [])
                                     if r['spdxElementId'] != 'SPDXRef-DOCUMENT']
        self.add_relationship('SPDXRef-DOCUMENT', 'DESCRIBES', self.root_id)
        document['documentNamespace'] = f'https://keepassxc.org/spdx/{uuid.uuid4()}'
        document['name'] = f'KeePassXC-{version}'

    @staticmethod
    def load_copying_rules():
        """Read file patterns and their copyright and license declarations from COPYING."""
        text = Path(__file__).resolve().parents[1].joinpath('COPYING').read_text(encoding='utf-8')
        rules = []
        current = {'patterns': []}
        for line in text.splitlines() + ['']:
            if not line.strip():
                if current['patterns']:
                    rules.append(current)
                current = {'patterns': []}
            elif line.startswith('Files: '):
                current['patterns'].append(line[7:].strip())
            elif line.startswith('       '):
                current['patterns'].append(line.strip())
            elif line.startswith('Copyright: '):
                current['copyright'] = line[11:].strip()
            elif line.startswith('           '):
                current['copyright'] = (current.get('copyright', '') + ' ' + line.strip()).strip()
            elif line.startswith('License: '):
                current['license'] = line[9:].strip()
        return rules

    def get_copying_metadata(self, relative):
        """Find COPYING declarations or the project notice for a KeePassXC executable."""
        # Installed share paths add a product subdirectory, so match suffixes.
        for rule in reversed(self.copying_rules):
            if any(fnmatch.fnmatch(relative, pattern) or relative.endswith('/' + pattern)
                   or relative == pattern for pattern in rule['patterns']):
                return rule
        if Path(relative).name.startswith('keepassxc'):
            return {'copyright': self.root['copyrightText'], 'license': self.root['licenseDeclared']}
        return {}

    def load_link_inputs(self, path):
        """Read CMake link inputs and register the reachable vendored components."""
        vendored = set()
        for line in path.read_text(encoding='utf-8').splitlines():
            if line.startswith('LIBRARY|'):
                self.linked_files.add(Path(line.split('|', 1)[1]).resolve())
            elif line.startswith('VENDORED|'):
                _, name, source, *version = line.split('|')
                vendored.add((name, source, version[0] if version else ''))
        for name, source, version in sorted(vendored):
            metadata = self.get_copying_metadata(source)
            identifier = 'SPDXRef-vendored-' + hashlib.sha256(name.encode()).hexdigest()[:16]
            license_value = metadata.get('license', 'NOASSERTION')
            component = {
                'SPDXID': identifier, 'name': name, 'downloadLocation': 'NOASSERTION',
                'filesAnalyzed': False, 'licenseDeclared': license_value,
                'copyrightText': metadata.get('copyright', 'NOASSERTION'),
                'sourceInfo': f'Vendored component linked by CMake; source: {source}. '
                              'Link inputs do not establish which archive members were retained.'}
            if version:
                component['versionInfo'] = version
            self.document['packages'].append(component)
            self.add_relationship(self.root_id, 'DEPENDS_ON', identifier)
            self.add_relationship(self.root_id, 'STATIC_LINK', identifier)

    def add_relationship(self, source, kind, target):
        """Add an SPDX relationship unless the same relationship already exists."""
        relationship = dict(spdxElementId=source, relationshipType=kind, relatedSpdxElement=target)
        if relationship not in self.document['relationships']:
            self.document['relationships'].append(relationship)

    def attach_binaries(self, package_id, names):
        """Link a supplier package to the application and its deployed binaries."""
        self.add_relationship(self.root_id, 'DEPENDS_ON', package_id)
        self.accounted.update(names)
        for name in names:
            for file_id in self.binary_files[name]:
                self.add_relationship(package_id, 'CONTAINS', file_id)

    def resolve_deployed_path(self, filename):
        """Resolve a path within the deployment and return its relative POSIX name."""
        return (self.deploy_dir / filename).resolve().relative_to(self.deploy_dir.resolve()).as_posix()

    def collect_deployed_files(self, output):
        """Record included deployment files and discard unwanted Syft asset records."""
        excluded = {file['SPDXID'] for file in self.document.get('files', [])
                    if not keep_file(self.deploy_dir / self.resolve_deployed_path(file['fileName']))}
        self.document['files'] = [file for file in self.document.get('files', [])
                                  if file['SPDXID'] not in excluded]
        self.document['relationships'] = [rel for rel in self.document['relationships']
                                         if rel['spdxElementId'] not in excluded
                                         and rel['relatedSpdxElement'] not in excluded]
        for package in self.document['packages']:
            if 'hasFiles' in package:
                package['hasFiles'] = [identifier for identifier in package['hasFiles'] if identifier not in excluded]
        # Keep Syft's file identifiers/relationships, updating hashes from the
        # deployed bytes and adding files that its package catalogers missed.
        existing = {self.resolve_deployed_path(f['fileName']): f for f in self.document.get('files', [])}
        for path in sorted(self.deploy_dir.rglob('*')):
            if not path.is_file() or path.resolve() == output.resolve():
                continue
            # Do not read symlinks pointing outside the deployment.
            try:
                path.resolve().relative_to(self.deploy_dir.resolve())
            except ValueError:
                continue
            if not keep_file(path):
                continue
            relative = path.relative_to(self.deploy_dir).as_posix()
            metadata = self.get_copying_metadata(relative)
            if path.suffix.lower() == '.html':
                metadata = {'license': self.root['licenseDeclared'], 'copyright': self.root['copyrightText']}
            entry = existing.get(self.resolve_deployed_path(relative))
            if entry is None:
                entry = {'SPDXID': 'SPDXRef-deployed-' + hashlib.sha256(relative.encode()).hexdigest(),
                         'fileName': './' + relative, 'licenseConcluded': 'NOASSERTION',
                         'copyrightText': 'NOASSERTION'}
                self.document.setdefault('files', []).append(entry)
            if metadata.get('license'):
                entry['licenseConcluded'] = metadata['license'].replace('GPL-2+', 'GPL-2.0-or-later')
                entry['licenseInfoInFile'] = [metadata['license'].replace('GPL-2+', 'GPL-2.0-or-later')]
            if metadata.get('copyright'):
                entry['copyrightText'] = metadata['copyright']
            if path.suffix.lower() == '.html':
                entry['fileTypes'] = ['DOCUMENTATION']
            entry['checksums'] = hash_file(path)
            self.add_relationship(self.root_id, 'CONTAINS', entry['SPDXID'])
            if is_binary_file(path):
                self.binary_files.setdefault(path.name.lower(), []).append(entry['SPDXID'])
                if metadata:
                    self.accounted.add(path.name.lower())

    def import_supplier_packages(self, source, selected, origin):
        """Import package metadata only, with collision-free SPDX/license IDs."""
        prefix = hashlib.sha256((origin + source.get('documentNamespace', '')).encode()).hexdigest()[:16]
        packages = {p['SPDXID']: p for p in source.get('packages', [])}
        selected = selected & packages.keys()
        # Retain nested components, including code embedded in Qt libraries.
        forward = {'DEPENDS_ON', 'CONTAINS', 'STATIC_LINK', 'DYNAMIC_LINK'}
        reverse = {'DEPENDENCY_OF', 'CONTAINED_BY'}
        dependencies = defaultdict(set)
        for rel in source.get('relationships', []):
            a, b = rel['spdxElementId'], rel['relatedSpdxElement']
            if rel['relationshipType'] in forward:
                dependencies[a].add(b)
            elif rel['relationshipType'] in reverse:
                dependencies[b].add(a)
        pending = list(selected)
        while pending:
            added = (dependencies[pending.pop()] & packages.keys()) - selected
            selected.update(added)
            pending.extend(added)
        mapping = {key: f'SPDXRef-{prefix}-{hashlib.sha256(key.encode()).hexdigest()[:16]}'
                   for key in selected}
        licenses = {item['licenseId']: f'LicenseRef-{prefix}-{item["licenseId"].removeprefix("LicenseRef-")}'
                    for item in source.get('hasExtractedLicensingInfos', [])}

        def namespace_identifiers(value):
            """Namespace imported package identifiers and custom license references."""
            return transform_strings(value, lambda text: mapping.get(text, re.sub(
                r'(?<![\w:-])LicenseRef-[\w.-]+', lambda m: licenses.get(m[0], m[0]), text)))

        existing_ids = {package['SPDXID'] for package in self.document['packages']}
        for key in sorted(selected):
            package = namespace_identifiers(packages[key])
            # Upstream verification codes and file lists describe the supplier's
            # installation, not our deployment. Preserve only component metadata.
            for field in ('packageVerificationCode', 'hasFiles', 'licenseInfoFromFiles', 'checksums'):
                package.pop(field, None)
            package['filesAnalyzed'] = False
            # vcpkg uses this private marker for a port that declares no
            # license. In a merged document it has no extracted definition;
            # SPDX NOASSERTION preserves that meaning without an invalid ref.
            for field in ('licenseConcluded', 'licenseDeclared'):
                if package.get(field) == 'LicenseRef-vcpkg-null':
                    package[field] = 'NOASSERTION'
            package['sourceInfo'] = (package.get('sourceInfo', '') + f' Metadata source: {origin}').strip()
            if package['SPDXID'] not in existing_ids:
                self.document['packages'].append(package)
                existing_ids.add(package['SPDXID'])
        for rel in source.get('relationships', []):
            if rel['spdxElementId'] in selected and rel['relatedSpdxElement'] in selected:
                self.document['relationships'].append(namespace_identifiers(rel))
            elif rel['spdxElementId'] in selected and rel['relatedSpdxElement'].startswith('DocumentRef-'):
                self.notes.append(f'Unavailable supplier dependency document: {rel["relatedSpdxElement"]} ({origin}).')
        for item in source.get('hasExtractedLicensingInfos', []):
            self.document.setdefault('hasExtractedLicensingInfos', []).append(namespace_identifiers(item))
        return mapping

    def import_vcpkg_packages(self, installed, triplet):
        """Import target packages with deployed-file or CMake link evidence."""
        if not installed or not triplet:
            self.notes.append('No vcpkg target installation configured.')
            return
        prefix = Path(installed) / triplet
        info = Path(installed) / 'vcpkg/info'
        count = 0
        for path in sorted(prefix.glob('share/*/vcpkg.spdx.json')):
            port = path.parent.name
            # Target-triplet file ownership avoids importing host tools or every
            # package installed in a shared vcpkg tree.
            names = set()
            linked = False
            for listing in info.glob(f'{port}_*_{triplet}.list'):
                for line in listing.read_text(encoding='utf-8').splitlines():
                    names.add(Path(line).name.lower())
                    if (Path(installed) / line).resolve() in self.linked_files:
                        linked = True
            matched = names & self.binary_files.keys()
            if not matched and not linked:
                continue
            source = load_spdx(path)
            package = next((p for p in source.get('packages', []) if p['SPDXID'] == 'SPDXRef-port'), None)
            if package is None:
                raise ValueError(f'Missing vcpkg port metadata: {path}')
            # The binary package's version is an ABI hash, whereas the port
            # records the upstream version required for vulnerability matching.
            mapping = self.import_supplier_packages(source, {package['SPDXID']}, f'vcpkg:{triplet}/{port}')
            self.attach_binaries(mapping[package['SPDXID']], matched)
            count += 1
        self.notes.append(f'Enriched {count} deployed vcpkg components for {triplet}; '
                          'packages without deployed-file or CMake link evidence were excluded.')

    def add_unidentified_libraries(self):
        """Represent shipped dynamic libraries as components when no supplier SBOM identifies them."""
        packages = self.document['packages']
        known_file_ids = {r['relatedSpdxElement'] for r in self.document['relationships']
                          if r['relationshipType'] == 'CONTAINS'
                          and r['spdxElementId'] != self.root_id}
        files = {f['SPDXID']: f for f in self.document.get('files', [])}
        for name, file_ids in self.binary_files.items():
            shared = (name.endswith(('.dll', '.dylib')) or re.search(r'\.so(?:\.|$)', name)
                      or any('.framework/' in files[file_id]['fileName'] for file_id in file_ids))
            if not shared:
                continue
            for file_id in file_ids:
                if file_id in known_file_ids:
                    continue
                identifier = 'SPDXRef-deployed-library-' + hashlib.sha256(
                    files[file_id]['fileName'].encode()).hexdigest()[:16]
                component = {'SPDXID': identifier, 'name': Path(files[file_id]['fileName']).name,
                             'downloadLocation': 'NOASSERTION', 'filesAnalyzed': True,
                             'licenseConcluded': 'NOASSERTION', 'licenseDeclared': 'NOASSERTION',
                             'copyrightText': 'NOASSERTION',
                             'sourceInfo': 'Bundled shared library.'}
                packages.append(component)
                self.add_relationship(self.root_id, 'DEPENDS_ON', identifier)
                self.add_relationship(identifier, 'CONTAINS', file_id)
                known_file_ids.add(file_id)

    def apply_syft_metadata(self, native):
        """Preserve native evidence that Syft's SPDX conversion omits."""
        files = {self.resolve_deployed_path(file['fileName']): file for file in self.document.get('files', [])}
        packages = {package['SPDXID'].rsplit('-', 1)[-1]: package for package in self.document['packages']}
        for artifact in native.get('artifacts', []):
            package = packages.get(artifact['id'])
            if package is None:
                continue
            metadata = artifact.get('metadata') or {}
            copyright = metadata.get('legalCopyright') or metadata.get('copyright')
            if copyright and package.get('copyrightText', 'NOASSERTION') == 'NOASSERTION':
                package['copyrightText'] = copyright
            for location in artifact.get('locations', []):
                file = files.get(self.resolve_deployed_path(location['path'].lstrip('/')))
                if file is None:
                    continue
                if copyright and file.get('copyrightText', 'NOASSERTION') == 'NOASSERTION':
                    file['copyrightText'] = copyright
                # Syft may encode evidence as OTHER/evident-by rather than ownership.
                self.add_relationship(package['SPDXID'], 'CONTAINS', file['SPDXID'])
                self.add_relationship(self.root_id, 'DEPENDS_ON', package['SPDXID'])
                self.accounted.add(Path(file['fileName']).name.lower())

    def import_distro_packages(self, syft):
        """Let Syft parse package metadata unavailable inside an AppDir."""
        if not sys.platform.startswith('linux'):
            return
        debian = bool(shutil.which('dpkg-query'))
        if not debian and not shutil.which('rpm'):
            return
        files = {file['SPDXID']: file for file in self.document.get('files', [])}
        matches = {}
        rpm_paths = query_local_command('rpm', '-qal').splitlines() if not debian else []
        for name, identifiers in self.binary_files.items():
            candidates = [line.rsplit(': ', 1) for line in
                          query_local_command('dpkg-query', '-S', f'*/{name}').splitlines()
                          if ': ' in line and not line.startswith('diversion ')] if debian else [
                              (query_local_command('rpm', '-qf', '--qf', '%{NAME}', path), path)
                              for path in rpm_paths if Path(path).name.lower() == name]
            for identifier in identifiers:
                deployed = self.deploy_dir / files[identifier]['fileName']
                for owner, original in candidates:
                    if owner and matches_binary(deployed, Path(original)):
                        matches.setdefault(owner, set()).add(identifier)
                        break
        if not matches:
            return
        with tempfile.TemporaryDirectory(prefix='keepassxc-distro-sbom-') as temporary:
            root = Path(temporary)
            if debian:
                status = root / 'var/lib/dpkg/status'
                status.parent.mkdir(parents=True)
                status.write_text(query_local_command('dpkg-query', '--status', *sorted(matches)) + '\n', encoding='utf-8')
                cataloger = 'dpkg-db-cataloger'
            else:
                database = next((path for path in (Path('/usr/lib/sysimage/rpm'), Path('/var/lib/rpm'))
                                 if path.is_dir()), None)
                if database is None:
                    return
                shutil.copytree(database, root / database.relative_to('/'))
                cataloger = 'rpm-db-cataloger'
            result = subprocess.run([syft, 'scan', f'dir:{root}', '--override-default-catalogers', cataloger,
                                     '-o', 'spdx-json'], check=True, capture_output=True, text=True,
                                    env=dict(os.environ, SYFT_CHECK_FOR_APP_UPDATE='false'))
            source = json.loads(result.stdout)
        selected = {package['SPDXID']: matches.get(package['name'], set())
                    or set().union(*(ids for owner, ids in matches.items()
                                     if owner.split(':')[0] == package['name']))
                    for package in source.get('packages', [])}
        selected = {key: ids for key, ids in selected.items() if ids}
        mapping = self.import_supplier_packages(source, set(selected), 'Syft distro metadata (matched bytes/build ID)')
        for package, identifiers in selected.items():
            self.add_relationship(self.root_id, 'DEPENDS_ON', mapping[package])
            for identifier in identifiers:
                self.add_relationship(mapping[package], 'CONTAINS', identifier)
                self.accounted.add(Path(files[identifier]['fileName']).name.lower())

    def import_qt_packages(self, prefix, version):
        """Import matching Qt components or use the configured version as a fallback."""
        paths = qt_sbom_paths(prefix)
        imported = 0
        if paths:
            print(f'SBOM: Loading {len(paths)} Qt binary supplier SBOMs...', file=sys.stderr, flush=True)
            source = merge_qt_documents(paths)
            matches = {f['SPDXID']: Path(f['fileName']).name.lower() for f in source.get('files', [])
                       if Path(f['fileName']).name.lower() in self.binary_files}
            owners = {owner: {matches[file_id] for file_id in identifiers & matches.keys()}
                      for owner, identifiers in package_files(source).items() if identifiers & matches.keys()}
            mapping = self.import_supplier_packages(source, set(owners), 'Qt supplier SBOMs') if owners else {}
            for owner, names in owners.items():
                if owner in mapping:
                    self.attach_binaries(mapping[owner], names)
            imported = len(owners)
        if not imported:
            self.notes.append(f'No matching Qt supplier SBOM available (Qt {version}); using Syft and configured Qt version.')
            # Qt 6.4 and installations built without -sbom still have a known
            # Qt version. Identify framework binaries and Qt DLLs/SOs, not plugins
            # whose names alone cannot establish their supplier.
            names = {name for name in self.binary_files
                     if re.match(r'^(lib)?qt6?[a-z].*', name)}
            if names:
                component = {'SPDXID': 'SPDXRef-configured-Qt', 'name': 'Qt',
                             'versionInfo': version, 'downloadLocation': 'https://download.qt.io/',
                             'filesAnalyzed': False, 'licenseConcluded': 'NOASSERTION',
                             'licenseDeclared': 'NOASSERTION', 'copyrightText': 'NOASSERTION',
                             'comment': 'Version from the Qt installation used by CMake; no supplier SBOM available.'}
                self.document['packages'].append(component)
                self.attach_binaries(component['SPDXID'], names)
        else:
            self.notes.append(f'Enriched {imported} deployed Qt components from supplier SBOMs.')
        self.merge_vcpkg_qt_packages()

    def merge_vcpkg_qt_packages(self):
        """Fold vcpkg metadata into Qt modules that own the same deployed files."""
        packages = self.document['packages']
        relationships = self.document['relationships']
        owned = package_files(self.document)
        qt = [p for p in packages if 'Metadata source: Qt supplier SBOMs' in p.get('sourceInfo', '')]
        replacements = {}
        transferred = {}
        for port in packages:
            if ('Metadata source: vcpkg:' not in port.get('sourceInfo', '')
                    or 'Metadata source: Qt supplier SBOMs' in port.get('sourceInfo', '')):
                continue
            port_id = port['SPDXID']
            port_files = owned.get(port_id, set())
            modules = [p for p in qt if port_files & owned.get(p['SPDXID'], set())]
            if not modules:
                continue
            covered = set()
            for module in modules:
                covered |= port_files & owned[module['SPDXID']]
                # Qt values take precedence; keep additional vcpkg identifiers
                # and provenance even when both suppliers populate these fields.
                for field, value in port.items():
                    # Port prose describes the entire repository or recipe, rather than the individual Qt module.
                    if field not in ('SPDXID', 'hasFiles', 'packageVerificationCode', 'licenseInfoFromFiles',
                                     'description', 'summary', 'comment'):
                        module.setdefault(field, value)
                for ref in port.get('externalRefs', []):
                    if ref not in module.setdefault('externalRefs', []):
                        module['externalRefs'].append(ref)
                module['sourceInfo'] += '\n' + port['sourceInfo']
            transferred[port_id] = covered
            if covered == port_files:
                replacements[port_id] = [p['SPDXID'] for p in modules]
            else:
                # Retain evidence for binaries not described by Qt's SBOM.
                if 'hasFiles' in port:
                    port['hasFiles'] = [key for key in port['hasFiles'] if key not in covered]
        self.document['packages'] = [p for p in packages if p['SPDXID'] not in replacements]
        self.document['relationships'] = []
        for rel in relationships:
            a, b = rel['spdxElementId'], rel['relatedSpdxElement']
            kind = rel['relationshipType']
            if ((kind == 'CONTAINS' and b in transferred.get(a, set()))
                    or (kind == 'CONTAINED_BY' and a in transferred.get(b, set()))):
                continue
            for source in replacements.get(a, [a]):
                for target in replacements.get(b, [b]):
                    if source != target:
                        self.add_relationship(source, kind, target)
        if transferred:
            self.notes.append(f'Merged {len(transferred)} overlapping vcpkg packages into Qt supplier components; '
                              'Qt metadata takes precedence.')

    def finalize_document(self, debug=False):
        """Canonicalize paths, calculate verification codes, filter comments, and validate."""
        license_fields = {'licenseDeclared', 'licenseConcluded', 'licenseInfoInFile', 'licenseInfoFromFiles'}
        expressions = [{key: value for key, value in record.items() if key in license_fields}
                       for field in ('packages', 'files', 'snippets') for record in self.document.get(field, [])]
        referenced = set(re.findall(r'(?<![\w:-])LicenseRef-[\w.-]+', json.dumps(expressions)))
        if 'hasExtractedLicensingInfos' in self.document:
            self.document['hasExtractedLicensingInfos'] = [
                license for license in self.document['hasExtractedLicensingInfos']
                if license['licenseId'] in referenced]
        for file in self.document.get('files', []):
            file['fileName'] = self.resolve_deployed_path(file['fileName'])
        # SPDX 2.x requires SHA1 for its verification code; retain SHA256 too.
        files = {f['SPDXID']: f for f in self.document.get('files', [])}
        owned = package_files(self.document)
        for package in self.document['packages']:
            contained = owned.get(package['SPDXID'], set())
            if contained:
                hashes = sorted(c['checksumValue'] for key in contained
                                for c in files[key]['checksums'] if c['algorithm'] == 'SHA1')
                package['filesAnalyzed'] = True
                package['licenseInfoFromFiles'] = ['NOASSERTION']
                package['packageVerificationCode'] = {
                    'packageVerificationCodeValue': hashlib.sha1(''.join(hashes).encode()).hexdigest()}
            else:
                package['filesAnalyzed'] = False
                package.pop('packageVerificationCode', None)
                package.pop('licenseInfoFromFiles', None)
        self.notes.append('Generation context: deployed artifact scanned by Syft; supplier metadata selected by '
                          'deployed binary basenames and CMake link inputs. Vendored targets carry source metadata. '
                          'Static/vendored code without build or supplier evidence may be missing. '
                          'External runtime dependencies and versions may be incomplete.')
        unknown = sorted(self.binary_files.keys() - self.accounted)
        if unknown:
            self.notes.append('Binaries without COPYING or supplier metadata (Syft evidence retained): '
                              + ', '.join(unknown))
        self.document['creationInfo']['creators'] = ['Tool: KeePassXC Build System']
        self.document['creationInfo']['comment'] = '\n'.join(self.notes)
        for note in self.notes:
            print(f'SBOM: {note}', file=sys.stderr)
        if not debug:
            def strip_debug_metadata(value):
                """Remove sourceInfo and comment fields recursively from release output."""
                if isinstance(value, dict):
                    return {key: strip_debug_metadata(item) for key, item in value.items()
                            if key not in ('sourceInfo', 'comment')}
                if isinstance(value, list):
                    return [strip_debug_metadata(item) for item in value]
                return value
            self.document = strip_debug_metadata(self.document)
        # SPDX parsers validate IDs, license references and document semantics.
        from spdx_tools.spdx.parser.json.json_parser import JsonLikeDictParser
        from spdx_tools.spdx.validation.document_validator import validate_full_spdx_document
        messages = validate_full_spdx_document(JsonLikeDictParser().parse(self.document))
        if messages:
            raise ValueError('Invalid generated SPDX document: ' + '; '.join(str(m) for m in messages))


def main():
    """Scan and enrich a deployment, then write the requested SPDX JSON output."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--syft', required=True)
    parser.add_argument('--deploy-dir', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--version', required=True)
    parser.add_argument('--qt-prefix', default='')
    parser.add_argument('--qt-version', default='')
    parser.add_argument('--vcpkg-installed', default='')
    parser.add_argument('--triplet', default='')
    parser.add_argument('--link-inputs', type=Path)
    parser.add_argument('--debug', action='store_true', help='Include sourceInfo and comment metadata.')
    args = parser.parse_args()
    if not args.deploy_dir.is_dir():
        parser.error(f'Deployment directory does not exist: {args.deploy_dir}')
    # Never leave a previous successful output looking like this run succeeded.
    args.output.unlink(missing_ok=True)
    result = subprocess.run(
        [args.syft, 'scan', f'dir:{args.deploy_dir.resolve()}',
         '--source-name', 'KeePassXC', '--source-version', args.version, '-o', 'syft-json',
         *syft_asset_exclusions(args.deploy_dir)],
        check=True, stdout=subprocess.PIPE, text=True,
        env=dict(os.environ, SYFT_CHECK_FOR_APP_UPDATE='false'))
    converted = subprocess.run([args.syft, 'convert', '-', '-o', 'spdx-json'],
                               input=result.stdout, check=True, stdout=subprocess.PIPE, text=True,
                               env=dict(os.environ, SYFT_CHECK_FOR_APP_UPDATE='false'))
    sbom = DeploymentSbom(json.loads(converted.stdout), args.deploy_dir, args.version)
    sbom.collect_deployed_files(args.output)
    sbom.apply_syft_metadata(json.loads(result.stdout))
    if args.link_inputs:
        sbom.load_link_inputs(args.link_inputs)
    sbom.import_vcpkg_packages(args.vcpkg_installed, args.triplet)
    sbom.import_qt_packages(args.qt_prefix, args.qt_version)
    sbom.import_distro_packages(args.syft)
    sbom.add_unidentified_libraries()
    sbom.finalize_document(debug=args.debug)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(sbom.document, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        sys.exit(f'SBOM generation failed: {error}')
