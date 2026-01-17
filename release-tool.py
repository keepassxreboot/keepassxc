#!/usr/bin/env python3
#
# Copyright (C) 2025 KeePassXC Team <team@keepassxc.org>
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


import argparse
from collections import defaultdict
import ctypes
from datetime import datetime
import hashlib
import json
import logging
import lzma
import os
from pathlib import Path
import platform
import re
import signal
import shutil
import stat
import subprocess
import sys
import tarfile
import tempfile
from urllib import request
from xml import sax


###########################################################################################
#                                       Ctrl+F TOC
###########################################################################################

# class Check(Command)
# class Tag(Command)
# class Build(Command)
# class BuildSrc(Command)
# class AppSign(Command)
# class GPGSign(Command)
# class I18N(Command)


###########################################################################################
#                                   Errors and Logging
###########################################################################################


class Error(Exception):
    def __init__(self, msg, *args, **kwargs):
        self.msg = msg
        self.args = args
        self.kwargs = kwargs
        self.__dict__.update(kwargs)

    def __str__(self):
        return self.msg % self.args


class SubprocessError(Error):
    pass


class LogFormatter(logging.Formatter):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._BOLD = '\x1b[1m' if self.term_colors_on() else ''
        self._RES_BOLD = '\x1b[22m' if self.term_colors_on() else ''
        self._RED = '\x1b[31m' if self.term_colors_on() else ''
        self._BRIGHT_RED = '\x1b[91m' if self.term_colors_on() else ''
        self._YELLOW = '\x1b[33m' if self.term_colors_on() else ''
        self._BLUE = '\x1b[34m' if self.term_colors_on() else ''
        self._GREEN = '\x1b[32m' if self.term_colors_on() else ''
        self._RES_CLR = '\x1b[39m' if self.term_colors_on() else ''
        self._RES = '\x1b[0m' if self.term_colors_on() else ''

        self._FMT = {
            logging.DEBUG: f'{self._BOLD}[%(levelname)s]{self._RES} %(message)s',
            logging.INFO: f'{self._BOLD}[{self._BLUE}%(levelname)s{self._RES_CLR}]{self._RES} %(message)s',
            logging.WARNING: f'{self._BOLD}[{self._YELLOW}%(levelname)s{self._RES_CLR}]'
                             f'{self._RES}{self._YELLOW} %(message)s{self._RES}',
            logging.ERROR: f'{self._BOLD}[{self._RED}%(levelname)s{self._RES_CLR}]'
                           f'{self._RES}{self._RED} %(message)s{self._RES}',
            logging.CRITICAL: f'{self._BOLD}[{self._BRIGHT_RED}%(levelname)s{self._RES_CLR}]'
                              f'{self._RES}{self._BRIGHT_RED} %(message)s{self._RES}',
        }

    @staticmethod
    def term_colors_on():
        return 'color' in os.getenv('TERM', '') or 'FORCE_COLOR' in os.environ or sys.platform == 'win32'

    def format(self, record):
        return logging.Formatter(self._FMT.get(record.levelno, '%(message)s')).format(record)

    # Function to make text bold in the terminal
    def bold(self, text):
        return f'{self._BOLD}{text}{self._RES_BOLD}'

    # Functions to color text in the terminal
    def red(self, text):
        return f'{self._RED}{text}{self._RES_CLR}'

    def bright_red(self, text):
        return f'{self._BRIGHT_RED}{text}{self._RES_CLR}'

    def yellow(self, text):
        return f'{self._YELLOW}{text}{self._RES_CLR}'

    def blue(self, text):
        return f'{self._BLUE}{text}{self._RES_CLR}'

    def green(self, text):
        return f'{self._GREEN}{text}{self._RES_CLR}'

    # Force a reset of terminal formatting
    def reset(self):
        return f'{self._RES}'


fmt = LogFormatter()
console_handler = logging.StreamHandler()
console_handler.setFormatter(fmt)
logger = logging.getLogger(__file__)
logger.setLevel(os.getenv('LOGLEVEL')
                if type(logging.getLevelName(os.environ.get('LOGLEVEL'))) is int else logging.INFO)
logger.addHandler(console_handler)

###########################################################################################
#                                      Helper Functions
###########################################################################################


_GIT_ORIG_BRANCH_CWD = None  # type: tuple[str, str] | None


def _get_bin_path(build_dir=None):
    if not build_dir:
        return os.getenv('PATH')
    build_dir = Path(build_dir).absolute()
    path_sep = ';' if sys.platform == 'win32' else ':'
    return path_sep.join(list(map(str, build_dir.rglob('vcpkg_installed/*/tools/**/bin'))) + [os.getenv('PATH')])


def _yes_no_prompt(prompt, default_no=True):
    sys.stderr.write(f'{prompt} {"[y/N]" if default_no else "[Y/n]"} ')
    yes_no = input().strip().lower()
    if default_no:
        return yes_no == 'y'
    return yes_no != 'n'


def _choice_prompt(prompt, choices):
    while True:
        sys.stderr.write(prompt + '\n')
        for i, c in enumerate(choices):
            sys.stderr.write(f'  {i + 1}) {c}\n')
        sys.stderr.write('\nYour choice: ')
        choice = input().strip()
        if not choice.isnumeric() or int(choice) < 1 or int(choice) > len(choices):
            logger.error('Invalid choice: %s', choice)
            continue
        return int(choice) - 1


def _run(cmd, *args, cwd, path=None, env=None, input=None, capture_output=True, timeout=None, check=True,
         docker_image=None, docker_privileged=False, docker_mounts=None, docker_platform=None, **run_kwargs):
    """
    Run a command and return its output.
    Raises an error if ``check`` is ``True`` and the process exited with a non-zero code.
    """
    if not cmd:
        raise ValueError('Empty command given.')

    if not env:
        env = os.environ.copy()
    if path:
        env['PATH'] = path
    if LogFormatter.term_colors_on():
        env['FORCE_COLOR'] = '1'

    if docker_image:
        cwd2 = Path(cwd or '.').absolute()
        docker_cmd = ['docker', 'run', '--rm', '--tty=true', f'--workdir={cwd2}', f'--user={os.getuid()}:{os.getgid()}']
        docker_cmd.extend([f'--env={k}={v}' for k, v in env.items() if k in ['FORCE_COLOR', 'CC', 'CXX']])
        if path:
            docker_cmd.append(f'--env=PATH={path}')
        docker_cmd.append(f'--volume={cwd2}:{cwd2}:rw')
        if docker_mounts:
            docker_cmd.extend([f'--volume={Path(d).absolute()}:{Path(d).absolute()}:rw' for d in docker_mounts])
        if docker_privileged:
            docker_cmd.extend(['--cap-add=SYS_ADMIN', '--security-opt=apparmor:unconfined', '--device=/dev/fuse'])
        if docker_platform:
            docker_cmd.append(f'--platform={docker_platform}')
        docker_cmd.append(docker_image)
        cmd = docker_cmd + cmd

    try:
        logger.debug('Running command: %s', ' '.join(map(str, cmd)))
        return subprocess.run(
            cmd, *args,
            input=input,
            capture_output=capture_output,
            cwd=cwd,
            env=env,
            timeout=timeout,
            check=check,
            **run_kwargs)
    except FileNotFoundError:
        raise Error('Command not found: %s', cmd[0] if type(cmd) in [list, tuple] else cmd)
    except subprocess.CalledProcessError as e:
        if e.stderr:
            err_txt = e.stderr
            if type(err_txt) is bytes:
                err_txt = err_txt.decode()
            raise SubprocessError('Command "%s" exited with non-zero code: %s',
                                  cmd[0], err_txt, **e.__dict__)
        else:
            raise SubprocessError('Command "%s" exited with non-zero code.', cmd[0], **e.__dict__)


def _cmd_exists(cmd, path=None):
    """Check if command exists."""
    return shutil.which(cmd, path=path) is not None


def _git_working_dir_clean(*, cwd):
    """Check whether the Git working directory is clean."""
    return _run(['git', 'diff-index', '--quiet', 'HEAD', '--'], check=False, cwd=cwd).returncode == 0


def _git_get_branch(*, cwd):
    """Get current Git branch."""
    return _run(['git', 'rev-parse', '--abbrev-ref', 'HEAD'], cwd=cwd, text=True).stdout.strip()


def _git_branches_related(branch1, branch2, *, cwd):
    """Check whether branch is ancestor or descendant of another."""
    return (_run(['git', 'merge-base', '--is-ancestor', branch1, branch2], cwd=cwd, check=False).returncode == 0 or
            _run(['git', 'merge-base', '--is-ancestor', branch2, branch1], cwd=cwd, check=False).returncode == 0)


def _git_checkout(branch, *, cwd):
    """Check out Git branch."""
    try:
        global _GIT_ORIG_BRANCH_CWD
        if not _GIT_ORIG_BRANCH_CWD:
            _GIT_ORIG_BRANCH_CWD = (_git_get_branch(cwd=cwd), cwd)
        logger.info('Checking out branch "%s"...', branch)
        _run(['git', 'checkout', branch], cwd=cwd, text=True)
    except SubprocessError as e:
        raise Error('Failed to check out branch "%s". %s', branch, e)


def _git_commit_files(files, message, *, cwd, sign_key=None):
    """Commit changes to files or directories."""
    _run(['git', 'reset'], cwd=cwd)
    _run(['git', 'add', *files], cwd=cwd)

    if _git_working_dir_clean(cwd=cwd):
        logger.info('No changes to commit.')
        return

    logger.info('Committing changes...')
    commit_args = ['git', 'commit', '--message', message]
    if sign_key:
        commit_args.extend(['--gpg-sign', sign_key])
    _run(commit_args, cwd=cwd, capture_output=False)


def _cleanup():
    """Post-execution cleanup."""
    try:
        if _GIT_ORIG_BRANCH_CWD:
            _git_checkout(_GIT_ORIG_BRANCH_CWD[0], cwd=_GIT_ORIG_BRANCH_CWD[1])
        return 0
    except Exception as e:
        logger.critical('Exception occurred during cleanup:', exc_info=e)
        return 1


def _split_version(version):
    if type(version) is not str or not re.match(r'^\d+\.\d+\.\d+$', version):
        raise Error('Invalid version number: %s', version)
    return version.split('.')


def _capture_vs_env(arch='amd64'):
    """
    Finds a valid Visual Studio environment using vswhere or common install locations.
    Run the VS developer batch script in a cmd shell and capture the environment it sets.
    Returns a dict suitable for passing as subprocess env or for updating os.environ.
    """
    vs_cmd = None
    # Try vswhere first, fall back to common install locations
    installer_path = Path(os.environ.get('ProgramFiles(x86)')) / 'Microsoft Visual Studio' / 'Installer'
    search_path = os.environ.get('PATH') + ';' + str(installer_path)
    vswhere = shutil.which('vswhere', path=search_path)
    if vswhere:
        vsdir = _run([vswhere, '-latest', '-products', '*', '-requires', 'Microsoft.Component.MSBuild',
                      '-property', 'installationPath'], cwd=None, text=True).stdout.strip()
        if vsdir:
            cand = Path(vsdir) / 'Common7' / 'Tools' / 'VsDevCmd.bat'
            if cand.exists():
                vs_cmd = str(cand)
    # Fallback to search common paths
    if not vs_cmd:
        program_files = [os.environ.get('ProgramFiles(x86)'), os.environ.get('ProgramFiles')]
        for pfdir in program_files:
            for vs in sorted(Path(pfdir).glob('Microsoft Visual Studio/*/*'), reverse=True):
                cand = vs / 'Common7' / 'Tools' / 'VsDevCmd.bat'
                if cand.exists():
                    vs_cmd = str(cand)
    # VS not found, raise error
    if not vs_cmd:
        raise Error('Visual Studio developer command script not found. Install VS or add vswhere to PATH.')

    logger.info('Using Visual Studio developer command script: %s', vs_cmd)

    # Use cmd.exe to run the batch file and then dump the environment with `set`
    try:
        out = _run(f'cmd /c "{vs_cmd}" -arch={arch} -no_logo && set', cwd=None, text=True).stdout
    except subprocess.CalledProcessError as e:
        raise Error('Failed to run Visual Studio dev script: %s', e.output or str(e))

    env = {}
    for line in out.splitlines():
        if len(kv := line.split('=', 1)) == 2:
            env[kv[0]] = kv[1]

    # VS has plenty of environment variables, so this is a basic sanity check
    if len(env) < 10:
        raise Error('Failed to capture environment from Visual Studio dev script.')

    return env


def _macos_get_codesigning_identity(user_choice=None):
    """
    Select an Apple codesigning certificate to be used for signing the macOS binaries.
    If only one identity was found on the system, it is returned automatically. If multiple identities are
    found, an interactive selection is shown. A user choice can be supplied to skip the selection.
    If the user choice refers to an invalid identity, an error is raised.
    """
    Check.check_xcode_setup()
    result = _run(['security', 'find-identity', '-v', '-p', 'codesigning'], cwd=None, text=True)
    identities = [i.strip() for i in result.stdout.strip().split('\n')[:-1]]
    identities = [i.split(' ', 2)[1:] for i in identities]
    if not identities:
        raise Error('No codesigning identities found.')

    if not user_choice and len(identities) == 1:
        logger.info('Using codesigning identity %s.', identities[0][1])
        return identities[0][0]
    elif not user_choice:
        return identities[_choice_prompt(
            'The following code signing identities were found. Which one do you want to use?',
            [' '.join(i) for i in identities])][0]
    else:
        for i in identities:
            # Exact match of ID or substring match of description
            if user_choice == i[0] or user_choice in i[1]:
                return i[0]
        raise Error('Invalid identity: %s', user_choice)


def _macos_validate_keychain_profile(keychain_profile):
    """
    Validate that a given keychain profile with stored notarization credentials exists and is valid.
    If no such profile is found, an error is raised with instructions on how to set one up.
    """
    if _run(['security', 'find-generic-password', '-a',
             f'com.apple.gke.notary.tool.saved-creds.{keychain_profile}'], cwd=None, check=False).returncode != 0:
        raise Error(f'Keychain profile "%s" not found! Run\n'
                    f'    {fmt.bold("xcrun notarytool store-credentials %s [...]" % keychain_profile)}\n'
                    f'to store your Apple notary service credentials in a keychain as "%s".',
                    keychain_profile, keychain_profile)


###########################################################################################
#                                      CLI Commands
###########################################################################################


class Command:
    """Command base class."""

    def __init__(self, arg_parser):
        self._arg_parser = arg_parser

    @classmethod
    def setup_arg_parser(cls, parser: argparse.ArgumentParser):
        raise NotImplementedError('setup_arg_parser not implemented in subclass: %s' % cls.__name__)

    def run(self, **kwargs):
        raise NotImplementedError('run not implemented in subclass: %s' % self.__class__.__name__)


class Check(Command):
    """Perform a pre-merge dry-run check, nothing is changed."""

    @classmethod
    def setup_arg_parser(cls, parser: argparse.ArgumentParser):
        parser.add_argument('version', help='Release version number or name.')
        parser.add_argument('-s', '--src-dir', help='Source directory.', default='.')
        parser.add_argument('-b', '--release-branch', help='Release source branch (default: inferred from --version).')

    def run(self, version, src_dir, release_branch):
        if not version:
            logger.warning('No version specified, performing only basic checks.')
        self.perform_basic_checks(src_dir)
        if version:
            self.perform_version_checks(version, src_dir, release_branch)
        logger.info('All checks passed.')

    @classmethod
    def perform_basic_checks(cls, src_dir):
        logger.info('Performing basic checks...')
        cls.check_src_dir_exists(src_dir)
        cls.check_git()
        cls.check_git_repository(src_dir)

        logger.info('Checking for required build tools...')
        cls.check_git()
        cls.check_gnupg()
        cls.check_xcode_setup()

    @classmethod
    def perform_version_checks(cls, version, src_dir, git_ref=None, version_exists=False, checkout=True):
        logger.info('Performing version checks...')
        major, minor, patch = _split_version(version)
        cls.check_working_tree_clean(src_dir)
        if version_exists:
            git_ref = git_ref or version
            cls.check_release_exists(git_ref, src_dir)
        else:
            git_ref = git_ref or f'release/{major}.{minor}.x'
            cls.check_release_does_not_exist(version, src_dir)
            cls.check_branch_exists(git_ref, src_dir)
        if checkout:
            _git_checkout(git_ref, cwd=src_dir)
            logger.debug('Attempting to find "%s" version string in source files...', version)
            cls.check_version_in_vcpkg_manifest(version, src_dir)
            cls.check_version_in_cmake(version, src_dir)
            cls.check_changelog(version, src_dir)
            cls.check_app_stream_info(version, src_dir)
        return git_ref

    @staticmethod
    def check_src_dir_exists(src_dir):
        if not src_dir:
            raise Error('Empty source directory given.')
        if not Path(src_dir).is_dir():
            raise Error(f'Source directory "{src_dir}" does not exist!')

    @staticmethod
    def check_git_repository(cwd=None):
        if _run(['git', 'rev-parse', '--is-inside-work-tree'], check=False, cwd=cwd).returncode != 0:
            raise Error('Not a valid Git repository: %s', cwd)

    @staticmethod
    def check_release_exists(tag_name, cwd=None):
        if not _run(['git', 'tag', '--list', tag_name], check=False, cwd=cwd).stdout:
            raise Error('Release tag does not exists: %s', tag_name)

    @staticmethod
    def check_release_does_not_exist(tag_name, cwd=None):
        if _run(['git', 'tag', '--list', tag_name], check=False, cwd=cwd).stdout:
            raise Error('Release tag already exists: %s', tag_name)

    @staticmethod
    def check_working_tree_clean(cwd=None):
        if not _git_working_dir_clean(cwd=cwd):
            raise Error('Current working tree is not clean! Please commit or unstage any changes.')

    @staticmethod
    def check_branch_exists(branch, cwd=None):
        if _run(['git', 'rev-parse', branch], check=False, cwd=cwd).returncode != 0:
            raise Error(f'Branch or tag "{branch}" does not exist!')

    @staticmethod
    def check_version_in_cmake(version, cwd=None):
        cmakelists = Path('CMakeLists.txt')
        if cwd:
            cmakelists = Path(cwd) / cmakelists
        if not cmakelists.is_file():
            raise Error('File not found: %s', cmakelists)
        cmakelists_text = cmakelists.read_text("UTF-8")
        major = re.search(r'^set\(KEEPASSXC_VERSION_MAJOR "(\d+)"\)$', cmakelists_text, re.MULTILINE).group(1)
        minor = re.search(r'^set\(KEEPASSXC_VERSION_MINOR "(\d+)"\)$', cmakelists_text, re.MULTILINE).group(1)
        patch = re.search(r'^set\(KEEPASSXC_VERSION_PATCH "(\d+)"\)$', cmakelists_text, re.MULTILINE).group(1)
        cmake_version = '.'.join([major, minor, patch])
        if cmake_version != version:
            raise Error(f'Version number in {cmakelists} not updated! Expected: %s, found: %s.', version, cmake_version)

    @staticmethod
    def check_version_in_vcpkg_manifest(version, cwd=None):
        manifest = Path('vcpkg.json')
        if cwd:
            manifest = Path(cwd) / manifest
        manifest_json = json.load(manifest.open('r'))
        if version != manifest_json['version-string']:
            raise Error(f'Version number in {manifest} not updated! Expected: %s, found: %s.',
                        version, manifest_json['version-string'])

    @staticmethod
    def check_changelog(version, cwd=None):
        changelog = Path('CHANGELOG.md')
        if cwd:
            changelog = Path(cwd) / changelog
        if not changelog.is_file():
            raise Error('File not found: %s', changelog)
        major, minor, patch = _split_version(version)
        if not re.search(rf'^## {major}\.{minor}\.{patch} \(.+?\)\n+', changelog.read_text("UTF-8"), re.MULTILINE):
            raise Error(f'{changelog} has not been updated to the "%s" release.', version)

    @staticmethod
    def check_app_stream_info(version, cwd=None):
        appstream = Path('share/linux/org.keepassxc.KeePassXC.appdata.xml')
        if cwd:
            appstream = Path(cwd) / appstream
        if not appstream.is_file():
            raise Error('File not found: %s', appstream)

        try:
            parser = sax.make_parser()
            parser.setContentHandler(sax.handler.ContentHandler())
            parser.parse(appstream)
        except sax.SAXParseException as e:
            raise Error(f'{appstream} is not well-formed. Error: %s at line %s, column %s',
                        e.getMessage(), e.getLineNumber(), e.getColumnNumber())

        regex = re.compile(rf'^\s*<release version="{version}" date=".+?">')
        with appstream.open('r', encoding='utf-8') as f:
            for line in f:
                if regex.search(line):
                    return
        raise Error(f'{appstream} has not been updated to the "%s" release.', version)

    @staticmethod
    def check_git():
        if not _cmd_exists('git'):
            raise Error('Git not installed.')

    @staticmethod
    def check_gnupg():
        if not _cmd_exists('gpg'):
            raise Error('GnuPG not installed.')

    @staticmethod
    def check_xcode_setup():
        if sys.platform != 'darwin':
            return
        if not _cmd_exists('xcrun'):
            raise Error('xcrun command not found! Please check that you have correctly installed Xcode.')


class Tag(Command):
    """Update translations and tag release."""

    @classmethod
    def setup_arg_parser(cls, parser: argparse.ArgumentParser):
        parser.add_argument('version', help='Release version number or name.')
        parser.add_argument('-s', '--src-dir', help='Source directory.', default='.')
        parser.add_argument('-b', '--release-branch', help='Release source branch (default: inferred from version).')
        parser.add_argument('-t', '--tag-name', help='Name of tag to create (default: same as version).')
        parser.add_argument('-l', '--no-latest', help='Don\'t advance "latest" tag.', action='store_true')
        parser.add_argument('-k', '--sign-key', help='PGP key for signing release tags (default: ask).')
        parser.add_argument('--no-sign', help='Don\'t sign release tags (for testing only!)', action='store_true')
        parser.add_argument('-y', '--yes', help='Bypass confirmation prompts.', action='store_true')
        parser.add_argument('--skip-translations', help='Skip pulling translations from Transifex', action='store_true')
        parser.add_argument('--tx-resource', help='Transifex resource name.', choices=['master', 'develop'])
        parser.add_argument('--tx-min-perc', choices=range(0, 101), metavar='[0-100]',
                            default=I18N.TRANSIFEX_PULL_PERC,
                            help='Minimum percent complete for Transifex pull (default: %(default)s).')

    def run(self, version, src_dir, release_branch, tag_name, no_latest, sign_key, no_sign, yes,
            skip_translations, tx_resource, tx_min_perc):
        major, minor, patch = _split_version(version)
        Check.perform_basic_checks(src_dir)
        release_branch = Check.perform_version_checks(version, src_dir, release_branch)
        Check.check_gnupg()
        sign_key = GPGSign.get_secret_key(sign_key)

        # Update translations
        if not skip_translations:
            i18n = I18N(self._arg_parser)
            i18n.run_tx_pull(src_dir, tx_resource, tx_min_perc, commit=True, yes=yes)

        changelog = re.search(rf'^## ({major}\.{minor}\.{patch} \(.*?\)\n\n+.+?)\n\n+## ',
                              (Path(src_dir) / 'CHANGELOG.md').read_text("UTF-8"), re.MULTILINE | re.DOTALL)
        if not changelog:
            raise Error(f'No changelog entry found for version {version}.')
        changelog = 'Release ' + changelog.group(1)

        tag_name = tag_name or version
        logger.info('Creating "%s" tag...', tag_name)
        tag_cmd = ['git', 'tag', '--annotate', tag_name, '--message', changelog]
        if not no_sign:
            tag_cmd.extend(['--sign', '--local-user', sign_key])
        _run(tag_cmd, cwd=src_dir)

        if not no_latest:
            logger.info('Advancing "latest" tag...')
            tag_cmd = ['git', 'tag', '--annotate', 'latest', '--message', 'Latest stable release', '--force']
            if not no_sign:
                tag_cmd.extend(['--sign', '--local-user', sign_key])
            _run(tag_cmd, cwd=src_dir)

        log_msg = ('All done! Don\'t forget to push the release branch and the new tags:\n'
                   f'    {fmt.bold(f"git push origin {release_branch}")}\n'
                   f'    {fmt.bold(f"git push origin tag {tag_name}")}')
        if not no_latest:
            log_msg += f'\n    {fmt.bold("git push origin tag latest --force")}'
        logger.info(log_msg)


class Build(Command):
    """Build and package binary release from sources."""

    @classmethod
    def setup_arg_parser(cls, parser: argparse.ArgumentParser):
        parser.add_argument('version', help='Release version number or name.')
        parser.add_argument('-s', '--src-dir', help='Source directory.', default='.')
        parser.add_argument('-t', '--tag-name', help='Name of the tag to check out (default: same as version).')
        parser.add_argument('-o', '--output-dir', default='release',
                            help='Build output directory (default: %(default)s.')
        parser.add_argument('-g', '--cmake-generator', help='Override default CMake generator.')
        parser.add_argument('-i', '--install-prefix', default='/usr/local',
                            help='Build install prefix (default: %(default)s).')
        parser.add_argument('--snapshot', help='Build snapshot from current HEAD.', action='store_true')
        parser.add_argument('--use-system-deps', help='Use system dependencies instead of vcpkg.', action='store_true')
        parser.add_argument('-j', '--parallelism', default=os.cpu_count(), type=int,
                            help='Build parallelism (default: %(default)s).')
        parser.add_argument('-y', '--yes', help='Bypass confirmation prompts.', action='store_true')
        parser.add_argument('--with-tests', help='Build and run tests.', action='store_true')

        if sys.platform == 'darwin':
            parser.add_argument('--macos-target', default=12, metavar='MACOSX_DEPLOYMENT_TARGET',
                                help='macOS deployment target version (default: %(default)s).')
            parser.add_argument('-p', '--platform-target', default=platform.uname().machine,
                                help='Build target platform (default: %(default)s).', choices=['x86_64', 'arm64'])
            parser.add_argument('--sign', help='Sign binaries prior to packaging.', action='store_true')
            parser.add_argument('--sign-identity',
                                help='Apple Developer identity name used for signing binaries (default: ask).')
            parser.add_argument('--notarize', help='Notarize signed file(s).', action='store_true')
            parser.add_argument('--keychain-profile', default='notarization-creds',
                                help='Read Apple credentials for notarization from a keychain (default: %(default)s).')
            parser.set_defaults(cmake_generator='Ninja')
        elif sys.platform == 'linux':
            parser.add_argument('-d', '--docker-image', help='Run build in Docker image (overrides --use-system-deps).')
            parser.add_argument('-p', '--platform-target', help='Build target platform (default: %(default)s).',
                                choices=['x86_64', 'aarch64'], default=platform.uname().machine)
            parser.add_argument('-a', '--appimage', help='Build an AppImage.', action='store_true')
        elif sys.platform == 'win32':
            parser.add_argument('-p', '--platform-target', help='Build target platform (default: %(default)s).',
                                choices=['amd64', 'arm64'], default='amd64')
            parser.add_argument('--sign', help='Sign binaries prior to packaging.', action='store_true')
            parser.add_argument('--sign-identity', help='SHA1 fingerprint of the signing certificate.')
            parser.add_argument('--sign-timestamp-url', help='Timestamp URL for signing binaries.',
                                default='http://timestamp.sectigo.com')
            parser.set_defaults(cmake_generator='Ninja')

        parser.add_argument('-c', '--cmake-opts', nargs=argparse.REMAINDER,
                            help='Additional CMake options (no other arguments can be specified after this).')

    def run(self, version, src_dir, output_dir, tag_name, snapshot, cmake_generator, yes, with_tests, **kwargs):
        Check.perform_basic_checks(src_dir)
        src_dir = Path(src_dir).resolve()
        output_dir = Path(output_dir)
        if output_dir.exists():
            logger.warning(f'Output directory "{output_dir}" already exists.')
            if not yes and not _yes_no_prompt('Reuse existing output directory?'):
                raise Error('Build aborted!')
        else:
            logger.debug('Creating output directory...')
            output_dir.mkdir(parents=True)

        tag_name = tag_name or version
        kwargs['with_tests'] = with_tests
        with_tests = 'ON' if with_tests else 'OFF'
        cmake_opts = [
            '-DWITH_XC_ALL=ON',
            '-DCMAKE_BUILD_TYPE=Release',
            '-DCMAKE_INSTALL_PREFIX=' + kwargs['install_prefix'],
            '-DWITH_TESTS=' + with_tests,
            '-DWITH_GUI_TESTS=' + with_tests,
        ]

        if not kwargs['use_system_deps'] and not kwargs.get('docker_image'):
            cmake_opts.append(f'-DCMAKE_TOOLCHAIN_FILE={self._get_vcpkg_toolchain_file()}')

        if snapshot:
            logger.info('Building a snapshot from HEAD.')
            try:
                Check.check_version_in_cmake(version, src_dir)
            except Error as e:
                logger.warning(e.msg, *e.args)
                cmake_opts.append(f'-DOVERRIDE_VERSION={version}-snapshot')
            cmake_opts.append('-DKEEPASSXC_BUILD_TYPE=Snapshot')
            version += '-snapshot'
        else:
            Check.perform_version_checks(version, src_dir, tag_name, version_exists=True, checkout=True)
            cmake_opts.append('-DKEEPASSXC_BUILD_TYPE=Release')

        if cmake_generator:
            cmake_opts.extend(['-G', cmake_generator])
        kwargs['cmake_opts'] = cmake_opts + (kwargs['cmake_opts'] or [])

        if sys.platform == 'win32':
            return self.build_windows(version, src_dir, output_dir, **kwargs)
        if sys.platform == 'darwin':
            return self.build_macos(version, src_dir, output_dir, **kwargs)
        if sys.platform == 'linux':
            return self.build_linux(version, src_dir, output_dir, **kwargs)
        raise Error('Unsupported build platform: %s', sys.platform)

    @staticmethod
    def _get_vcpkg_toolchain_file(path=None):
        vcpkg = shutil.which('vcpkg', path=path)
        if not vcpkg:
            # Check the VCPKG_ROOT environment variable
            if 'VCPKG_ROOT' in os.environ:
                vcpkg = Path(os.environ['VCPKG_ROOT']) / 'vcpkg'
            else:
                raise Error('vcpkg not found in PATH (use --use-system-deps to build with '
                            'system dependencies instead).')
        toolchain = Path(vcpkg).parent / 'scripts' / 'buildsystems' / 'vcpkg.cmake'
        if not toolchain.is_file():
            raise Error('Toolchain file not found in vcpkg installation directory.')
        return toolchain.resolve()

    @staticmethod
    def _run_tests(cwd, ctest_cmd='ctest', parallelism=4):
        logger.info('Running tests...')
        _run([ctest_cmd, '-E', 'gui|cli', '--output-on-failure', '-j', str(parallelism)], cwd=cwd, capture_output=False)
        _run([ctest_cmd, '-R', 'gui|cli', '--output-on-failure'], cwd=cwd, capture_output=False)

    # noinspection PyMethodMayBeStatic
    def build_windows(self, version, src_dir, output_dir, *, parallelism, cmake_opts, platform_target,
                      sign, sign_identity, sign_timestamp_url, with_tests, **_):
        # Check for required tools
        if not _cmd_exists('candle.exe') or not _cmd_exists('light.exe') or not _cmd_exists('heat.exe'):
            raise Error('WiX Toolset not found on the PATH (candle.exe, light.exe, heat.exe).')

        # Setup build signing if requested
        if sign:
            cmake_opts.append(f'-DWITH_XC_CODESIGN_IDENTITY={sign_identity}')
            cmake_opts.append(f'-WITH_XC_CODESIGN_TIMESTAMP_URL={sign_timestamp_url}')
        # Use vcpkg for dependency deployment
        cmake_opts.append('-DX_VCPKG_APPLOCAL_DEPS_INSTALL=ON')

        # Find Visual Studio and capture build environment
        vs_env = _capture_vs_env(arch=platform_target)

        # Use vs_env to resolve common tools
        cmake_cmd = shutil.which('cmake', path=vs_env.get('PATH'))
        cpack_cmd = shutil.which('cpack', path=vs_env.get('PATH'))
        ctest_cmd = shutil.which('ctest', path=vs_env.get('PATH'))

        # Start the build
        with tempfile.TemporaryDirectory() as build_dir:
            logger.info('Configuring build...')
            _run([cmake_cmd, *cmake_opts, str(src_dir)], cwd=build_dir, env=vs_env, capture_output=False)

            logger.info('Compiling sources...')
            _run([cmake_cmd, '--build', '.', f'--parallel', str(parallelism)],
                 cwd=build_dir, env=vs_env, capture_output=False)

            if with_tests:
                self._run_tests(cwd=build_dir, ctest_cmd=ctest_cmd)

            logger.info('Packaging application...')
            _run([cpack_cmd, '-G', 'ZIP;WIX'], cwd=build_dir, env=vs_env, capture_output=False)

            artifacts = list(Path(build_dir).glob("*.zip")) + list(Path(build_dir).glob("*.msi"))
            for artifact in artifacts:
                artifact.replace(output_dir / artifact.name)
                logger.info(f'Created artifact: {output_dir / artifact.name}')

    # noinspection PyMethodMayBeStatic
    def build_macos(self, version, src_dir, output_dir, *, use_system_deps, parallelism, cmake_opts,
                    macos_target, platform_target, with_tests, sign, sign_identity, notarize, keychain_profile, **_):
        if not use_system_deps:
            cmake_opts.append(f'-DVCPKG_TARGET_TRIPLET={platform_target.replace("86_", "")}-osx-dynamic-release')
        cmake_opts.append(f'-DCMAKE_OSX_DEPLOYMENT_TARGET={macos_target}')
        cmake_opts.append(f'-DCMAKE_OSX_ARCHITECTURES={platform_target}')

        with tempfile.TemporaryDirectory() as build_dir:
            if sign:
                sign_identity = _macos_get_codesigning_identity(sign_identity)
                cmake_opts.append(f'-DWITH_XC_CODESIGN_IDENTITY={sign_identity}')
                if notarize:
                    _macos_validate_keychain_profile(keychain_profile)
                    cmake_opts.append(f'-DWITH_XC_NOTARY_KEYCHAIN_PROFILE={keychain_profile}')

            logger.info('Configuring build...')
            _run(['cmake', *cmake_opts, str(src_dir)], cwd=build_dir, capture_output=False)

            logger.info('Compiling sources...')
            _run(['cmake', '--build', '.', f'--parallel', str(parallelism)], cwd=build_dir, capture_output=False)

            if with_tests:
                self._run_tests(cwd=build_dir, parallelism=parallelism)

            logger.info('Packaging application...')
            _run(['cpack', '-G', 'DragNDrop'], cwd=build_dir, capture_output=False)

            output_file = Path(build_dir) / f'KeePassXC-{version}.dmg'
            unsigned_suffix = '-unsigned' if not sign else ''
            output_file.rename(output_dir / f'KeePassXC-{version}-{platform_target}{unsigned_suffix}.dmg')

        if sign:
            logger.info('All done!')
        else:
            logger.info('All done! Please don\'t forget to sign the binaries before distribution.')

    @staticmethod
    def _download_tools_if_not_available(toolname, bin_dir, url, docker_args=None):
        if _run(['which', toolname], cwd=None, check=False, **(docker_args or {})).returncode != 0:
            logger.info(f'Downloading {toolname}...')
            outfile = bin_dir / toolname
            request.urlretrieve(url, outfile)
            outfile.chmod(outfile.stat().st_mode | stat.S_IEXEC)

    def build_linux(self, version, src_dir, output_dir, *, install_prefix, parallelism, cmake_opts, use_system_deps,
                    platform_target, appimage, docker_image, with_tests, **_):
        if use_system_deps and platform_target != platform.uname().machine and not docker_image:
            raise Error('Need --docker-image for cross-platform compilation when not building with vcpkg!')

        docker_args = dict(
            docker_image=docker_image,
            docker_mounts=[src_dir],
            docker_platform=f'linux/{platform_target}',
        )
        if docker_image:
            logger.info('Pulling Docker image...')
            _run(['docker', 'pull', f'--platform=linux/{platform_target}', docker_image],
                 cwd=None, capture_output=False)

        if appimage:
            cmake_opts.append('-DKEEPASSXC_DIST_TYPE=AppImage')
            # Force install prefix to ensure proper AppDir structure for linuxdeploy
            install_prefix = '/usr'

        with tempfile.TemporaryDirectory() as build_dir:
            logger.info('Configuring build...')
            _run(['cmake', *cmake_opts, str(src_dir)], cwd=build_dir, capture_output=False, **docker_args)

            logger.info('Compiling sources...')
            _run(['cmake', '--build', '.', '--parallel', str(parallelism)],
                 cwd=build_dir, capture_output=False, **docker_args)

            if with_tests:
                self._run_tests(cwd=build_dir, parallelism=parallelism)

            logger.info('Bundling AppDir...')
            app_dir = Path(build_dir) / f'KeePassXC-{version}-{platform_target}.AppDir'
            _run(['cmake', '--install', '.', '--strip',
                  '--prefix', (app_dir.absolute() / install_prefix.lstrip('/')).as_posix()],
                 cwd=build_dir, capture_output=False, **docker_args)
            shutil.copytree(app_dir, output_dir / app_dir.name, symlinks=True, dirs_exist_ok=True)

            if appimage:
                self._build_linux_appimage(
                    version, src_dir, output_dir, app_dir, build_dir, install_prefix, platform_target, docker_args)

    def _build_linux_appimage(self, version, src_dir, output_dir, app_dir, build_dir, install_prefix,
                              platform_target, docker_args):
        if (app_dir / 'AppRun').exists():
            raise Error('AppDir has already been run through linuxdeploy! Please create a fresh AppDir and try again.')

        bin_dir = Path(build_dir) / 'bin'
        bin_dir.mkdir()
        self._download_tools_if_not_available(
            'linuxdeploy', bin_dir,
            'https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/' +
            f'linuxdeploy-{platform_target}.AppImage',
            docker_args)
        self._download_tools_if_not_available(
            'linuxdeploy-plugin-qt', bin_dir,
            'https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/' +
            f'linuxdeploy-plugin-qt-{platform_target}.AppImage',
            docker_args)
        self._download_tools_if_not_available(
            'appimagetool', bin_dir,
            'https://github.com/AppImage/AppImageKit/releases/download/continuous/' +
            f'appimagetool-{platform_target}.AppImage',
            docker_args)

        env_path = ':'.join([bin_dir.as_posix(), _get_bin_path()])
        install_prefix = app_dir / install_prefix.lstrip('/')
        desktop_file = install_prefix / 'share/applications/org.keepassxc.KeePassXC.desktop'
        icon_file = install_prefix / 'share/icons/hicolor/256x256/apps/keepassxc.png'
        executables = (install_prefix / 'bin').glob('keepassxc*')
        app_run = src_dir / 'share/linux/appimage-apprun.sh'

        logger.info('Building AppImage...')
        logger.debug('Running linuxdeploy...')
        _run(['linuxdeploy', '--plugin=qt', f'--appdir={app_dir}', f'--custom-apprun={app_run}',
              f'--desktop-file={desktop_file}', f'--icon-file={icon_file}',
              *[f'--executable={ex}' for ex in executables]],
             cwd=build_dir, capture_output=False, path=env_path, **docker_args, docker_privileged=True)

        logger.debug('Running appimagetool...')
        appimage_name = f'KeePassXC-{version}-{platform_target}.AppImage'
        desktop_file.write_text(desktop_file.read_text().strip() + f'\nX-AppImage-Version={version}\n')
        _run(['appimagetool', '--updateinformation=gh-releases-zsync|keepassxreboot|keepassxc|latest|' +
              f'KeePassXC-*-{platform_target}.AppImage.zsync',
              app_dir.as_posix(), (output_dir.absolute() / appimage_name).as_posix()],
             cwd=build_dir, capture_output=False, path=env_path, **docker_args, docker_privileged=True)
        # Move appimage zsync file to output dir
        zsync_file = next(Path(build_dir).glob('*.AppImage.zsync'), None)
        if zsync_file and zsync_file.is_file():
            shutil.move(zsync_file.absolute(), output_dir.absolute() / zsync_file.name)


class BuildSrc(Command):
    """Build and package source tarball."""

    @classmethod
    def setup_arg_parser(cls, parser: argparse.ArgumentParser):
        parser.add_argument('version', help='Release version number or name.')
        parser.add_argument('-s', '--src-dir', help='Source directory.', default='.')
        parser.add_argument('-t', '--tag-name', help='Name of the tag to check out (default: same as version).')
        parser.add_argument('-o', '--output-dir', default='release',
                            help='Build output directory (default: %(default)s.')
        parser.add_argument('-y', '--yes', help='Bypass confirmation prompts.', action='store_true')

    def run(self, version, src_dir, output_dir, tag_name, yes, **kwargs):
        Check.perform_basic_checks(src_dir)
        src_dir = Path(src_dir).resolve()
        output_dir = Path(output_dir)
        if output_dir.exists():
            logger.warning(f'Output directory "{output_dir}" already exists.')
            if not yes and not _yes_no_prompt('Reuse existing output directory?'):
                raise Error('Build aborted!')

        logger.info('Exporting sources...')
        prefix = f'keepassxc-{version}'
        output_file = Path(output_dir) / f'{prefix}-src.tar.xz'
        tag_name = tag_name or version

        with tempfile.TemporaryDirectory() as tmp:
            # Export sources to temporary tarball
            tmp = Path(tmp)
            tmp_export = tmp / 'export.tar'
            _run(['git', 'archive', '--format=tar', f'--prefix={prefix}/', f'--output={tmp_export}', tag_name],
                 cwd=src_dir)

            # Append .version and .gitrev files to tarball
            fver = tmp / '.version'
            fver.write_text(version)
            frev = tmp / '.gitrev'
            git_rev = _run(['git', 'rev-parse', '--short=7', tag_name], cwd=src_dir, text=True).stdout.strip()
            frev.write_text(git_rev)
            with tarfile.open(tmp_export, 'a') as tf:
                tf.add(fver, Path(prefix) / fver.name)
                tf.add(frev, Path(prefix) / frev.name)

            logger.info('Compressing source tarball...')
            tmp_comp = tmp_export.with_suffix('.tar.xz')
            with lzma.open(tmp_comp, 'wb', preset=6) as f:
                f.write(tmp_export.read_bytes())
            tmp_comp.rename(output_file)


class Notarize(Command):
    """Notarize a signed macOS DMG app bundle."""

    @classmethod
    def setup_arg_parser(cls, parser: argparse.ArgumentParser):
        parser.add_argument('file', help='Input DMG file(s) to notarize.', nargs='+')
        parser.add_argument('-p', '--keychain-profile', default='notarization-creds',
                            help='Read Apple credentials for notarization from a keychain (default: %(default)s).')

    def run(self, file, keychain_profile, **_):
        if sys.platform != 'darwin':
            raise Error('Unsupported platform.')

        logger.warning('This tool is meant primarily for testing purposes. '
                       'For production use, add the --notarize flag to the build command.')

        _macos_validate_keychain_profile(keychain_profile)
        for i, f in enumerate(file):
            f = Path(f)
            if not f.exists():
                raise Error('Input file does not exist: %s', f)
            if f.suffix != '.dmg':
                raise Error('Input file is not a DMG image: %s', f)
            file[i] = f
            self.notarize_macos(f, keychain_profile)

        logger.info('All done.')

    # noinspection PyMethodMayBeStatic
    def notarize_macos(self, file, keychain_profile):

        logger.info('Submitting "%s" for notarization...', file)
        _run(['xcrun', 'notarytool', 'submit', f'--keychain-profile={keychain_profile}', '--wait',
              file.as_posix()], cwd=None, capture_output=False)

        logger.debug('Stapling notarization ticket...')
        _run(['xcrun', 'stapler', 'staple', file.as_posix()], cwd=None)
        _run(['xcrun', 'stapler', 'validate', file.as_posix()], cwd=None)

        logger.info('Notarization successful.')


class GPGSign(Command):
    """Sign previously compiled release packages with GPG."""

    @classmethod
    def setup_arg_parser(cls, parser: argparse.ArgumentParser):
        parser.add_argument('file', help='Input file(s) to sign', nargs='+')
        parser.add_argument('-k', '--gpg-key', help='GnuPG key for signing input files (default: ask).')

    @staticmethod
    def get_secret_key(user_choice):
        keys = _run(['gpg', '--list-secret-keys', '--keyid-format=long'], cwd=None, text=True)
        keys = re.findall(r'^sec#?\s+(.+?/[A-F0-9]+) .+?\n\s+(.+?)\nuid .+?] (.+?)\n', keys.stdout, re.MULTILINE)
        if not keys:
            raise Error('No secret keys found!')

        if not user_choice and len(keys) == 1:
            logger.info('Using secret key %s %s.', keys[0][0], keys[0][2])
            return keys[0][1]
        elif not user_choice:
            return keys[_choice_prompt(
                'The following secret keys were found. Which one do you want to use?',
                [' '.join([k[0], k[2]]) for k in keys])][1]
        else:
            for i in keys:
                if user_choice in i[1] or user_choice in i[2]:
                    return i[1]
            raise Error('Invalid key ID: %s', user_choice)

    def run(self, file, gpg_key):
        Check.check_gnupg()

        for i, f in enumerate(file):
            f = Path(f)
            if not f.is_file():
                raise Error('File "%s" does not exist or is not a file!', f)
            file[i] = f

        key_id = self.get_secret_key(gpg_key)
        for f in file:
            logger.info('Signing "%s"...', f)
            _run(['gpg', '--armor', f'--local-user={key_id}', '--detach-sig',
                  f'--output={f.with_suffix(f.suffix + ".sig")}', str(f)], cwd=None)

            logger.info('Creating digest file...')
            h = hashlib.sha256(f.read_bytes()).hexdigest()
            f.with_suffix(f.suffix + '.DIGEST').write_text(f'{h}  {f.name}\n')

        logger.info('All done.')


class I18N(Command):
    """Update translation files and pull from or push to Transifex."""

    TRANSIFEX_RESOURCE = 'share-translations-keepassxc-en-ts--{}'
    TRANSIFEX_PULL_PERC = 60

    @classmethod
    def setup_arg_parser(cls, parser: argparse.ArgumentParser):
        parser.add_argument('-s', '--src-dir', help='Source directory.', default='.')
        parser.add_argument('-b', '--branch', help='Branch to operate on.')

        subparsers = parser.add_subparsers(title='Subcommands', dest='subcmd')
        push = subparsers.add_parser('tx-push', help='Push source translation file to Transifex.')
        push.add_argument('-r', '--resource', help='Transifex resource name.', choices=['master', 'develop'])
        push.add_argument('-y', '--yes', help='Don\'t ask before pushing source file.', action='store_true')
        push.add_argument('tx_args', help='Additional arguments to pass to tx subcommand.', nargs=argparse.REMAINDER)

        pull = subparsers.add_parser('tx-pull', help='Pull updated translations from Transifex.')
        pull.add_argument('-r', '--resource', help='Transifex resource name.', choices=['master', 'develop'])
        pull.add_argument('-m', '--min-perc', help='Minimum percent complete for pull (default: %(default)s).',
                          choices=range(0, 101), metavar='[0-100]', default=cls.TRANSIFEX_PULL_PERC)
        pull.add_argument('-c', '--commit', help='Commit changes.', action='store_true')
        pull.add_argument('-y', '--yes', help='Don\'t ask before pulling translations.', action='store_true')
        pull.add_argument('tx_args', help='Additional arguments to pass to tx subcommand.', nargs=argparse.REMAINDER)

        list_translators = subparsers.add_parser('tx-list-translators',
                                                 help='Print a HTML-formatted list of translation contributors.')
        list_translators.add_argument('-o', '--org', help='Transifex org name.', default='keepassxc')
        list_translators.add_argument('-p', '--project', help='Transifex project name.', default='keepassxc')
        list_translators.add_argument('-r', '--resource', help='Transifex resource name.',
                                      choices=['master', 'develop'])
        list_translators.add_argument('-b', '--member-blacklist', nargs='+', help='Transifex users to ignore',
                                      default=['phoerious', 'droidmonkey'])

        lupdate = subparsers.add_parser('lupdate', help='Update source translation file from C++ sources.')
        lupdate.add_argument('-d', '--build-dir', help='Build directory for looking up lupdate binary.')
        lupdate.add_argument('-c', '--commit', help='Commit changes.', action='store_true')
        lupdate.add_argument('lupdate_args', help='Additional arguments to pass to lupdate subcommand.',
                             nargs=argparse.REMAINDER)

    @staticmethod
    def check_transifex_cmd_exists():
        if not _cmd_exists('tx'):
            raise Error(f'Transifex tool "tx" is not installed! Installation instructions: '
                        f'{fmt.bold("https://developers.transifex.com/docs/cli")}.')

    @staticmethod
    def check_transifex_config_exists(src_dir):
        if not (Path(src_dir) / '.tx' / 'config').is_file():
            raise Error('No Transifex config found in source dir.')
        if not (Path.home() / '.transifexrc').is_file():
            raise Error('Transifex API key not configured. Run "tx status" first.')

    @staticmethod
    def check_lupdate_exists(path):
        if _cmd_exists('lupdate', path=path):
            result = _run(['lupdate', '-version'], path=path, check=False, cwd=None, text=True)
            if result.returncode == 0 and result.stdout.startswith('lupdate version 5.'):
                return
        raise Error('lupdate command not found. Make sure it is installed and the correct version.')

    def run(self, subcmd, src_dir, branch, **kwargs):
        if not subcmd:
            logger.error('No subcommand specified.')
            self._arg_parser.parse_args(['i18n', '--help'])

        Check.perform_basic_checks(src_dir)
        if branch:
            Check.check_working_tree_clean(src_dir)
            Check.check_branch_exists(branch, src_dir)
            _git_checkout(branch, cwd=src_dir)

        if subcmd.startswith('tx-'):
            self.check_transifex_cmd_exists()
            self.check_transifex_config_exists(src_dir)

            if 'tx_args' in kwargs:
                kwargs['tx_args'] = kwargs['tx_args'][1:]
            if subcmd == 'tx-push':
                self.run_tx_push(src_dir, **kwargs)
            elif subcmd == 'tx-pull':
                self.run_tx_pull(src_dir, **kwargs)
            elif subcmd == 'tx-list-translators':
                self.run_tx_list_translators(src_dir, kwargs['org'], kwargs['project'], kwargs['resource'],
                                             kwargs['member_blacklist'])

        elif subcmd == 'lupdate':
            kwargs['lupdate_args'] = kwargs['lupdate_args'][1:]
            self.run_lupdate(src_dir, **kwargs)

    # noinspection PyMethodMayBeStatic
    def derive_resource_name(self, override_resource=None, *, cwd):
        if override_resource:
            res = override_resource
        elif _git_branches_related('develop', 'HEAD', cwd=cwd):
            logger.info('Branch derives from develop, using ' + fmt.bold('"develop"') + ' resource.')
            res = 'develop'
        else:
            logger.info('Release branch, using ' + fmt.bold('"master"') + ' resource.')
            res = 'master'
        return self.TRANSIFEX_RESOURCE.format(res)

    # noinspection PyMethodMayBeStatic
    def run_tx_push(self, src_dir, resource, yes, tx_args):
        resource = 'keepassxc.' + self.derive_resource_name(resource, cwd=src_dir)
        sys.stderr.write('\nAbout to push the ' + fmt.bold('"en"') +
                         ' source file from the current branch to Transifex:\n')
        sys.stderr.write(f'    {fmt.bold(_git_get_branch(cwd=src_dir))}'
                         f' -> {fmt.bold(resource)}\n')
        if not yes and not _yes_no_prompt('Continue?'):
            logger.error('Push aborted.')
            return
        logger.info('Pushing source file to Transifex...')
        _run(['tx', 'push', '--source', '--use-git-timestamps', *tx_args, resource],
             cwd=src_dir, capture_output=False)
        logger.info('Push successful.')

    # noinspection PyMethodMayBeStatic
    def run_tx_pull(self, src_dir, resource, min_perc, commit=False, yes=False, tx_args=None):
        resource = 'keepassxc.' + self.derive_resource_name(resource, cwd=src_dir)
        sys.stderr.write('\nAbout to pull translations for ' + fmt.bold(f'"{resource}"') + '.\n')
        if not yes and not _yes_no_prompt('Continue?'):
            logger.error('Pull aborted.')
            return
        logger.info('Pulling translations from Transifex...')
        tx_args = tx_args or []
        _run(['tx', 'pull', '--all', '--use-git-timestamps', f'--minimum-perc={min_perc}', *tx_args, resource],
             cwd=src_dir, capture_output=False)
        logger.info('Pull successful.')
        files = [f.relative_to(src_dir) for f in Path(src_dir).glob('share/translations/*.ts')]
        if commit:
            _git_commit_files(files, 'Update translations.', cwd=src_dir)

    # noinspection PyMethodMayBeStatic
    def run_tx_list_translators(self, src_dir, org, project, resource, member_blacklist):
        txrc = Path.home() / '.transifexrc'
        if not txrc.exists():
            raise Error('No Transifex config found. Run tx init first.')

        org = f'o:{org}'
        project = f'{org}:p:{project}'
        resource = f'{project}:r:{self.derive_resource_name(resource, cwd=src_dir)}'

        token = [l for l in open(txrc, 'r') if l.startswith('token')][0].split('=', 1)[1].strip()
        member_blacklist = [f'u:{m}' for m in member_blacklist]

        def get_url(url):
            req = request.Request(url)
            req.add_header('Content-Type', 'application/vnd.api+json')
            req.add_header('Authorization', f'Bearer {token}')
            with request.urlopen(req) as resp:
                return json.load(resp)

        logger.info('Fetching languages...',)
        languages_json = get_url(f'https://rest.api.transifex.com/projects/{project}/languages')
        languages = {}
        for lang in languages_json['data']:
            languages[lang['id']] = lang['attributes']['name']

        logger.info('Fetching language stats...')
        language_stats_json = get_url('https://rest.api.transifex.com/resource_language_stats?'
                                      f'filter[project]={project}&filter[resource]={resource}')
        for s in language_stats_json['data']:
            completion = s['attributes']['translated_strings'] / s['attributes']['total_strings']
            if completion < .6:
                languages.pop(s['relationships']['language']['data']['id'])

        logger.info('Fetching language members...')
        members_json = get_url(f'https://rest.api.transifex.com/team_memberships?filter[organization]={org}')
        members = defaultdict(set)
        for member in members_json['data']:
            print('.', end='', file=sys.stderr)
            sys.stderr.flush()
            if member['relationships']['user']['data']['id'] in member_blacklist:
                continue
            lid = member['relationships']['language']['data']['id']
            if lid not in languages:
                continue
            user = get_url(member['relationships']['user']['links']['related'])['data']['attributes']['username']
            members[lid].add(user)
        print(file=sys.stderr, flush=True)

        print('<ul>')
        for lang in sorted(languages, key=lambda x: languages[x]):
            if not members[lang]:
                continue
            lines = [f'    <li><strong>{languages[lang]}:</strong> ']
            for i, m in enumerate(sorted(members[lang], key=lambda x: x.lower())):
                if len(lines[-1]) + len(m) >= 120:
                    lines.append('        ')
                lines[-1] += m
                if i < len(members[lang]) - 1:
                    lines[-1] += ', '
            lines[-1] += '</li>'
            print('\n'.join(lines))
        print('</ul>')
        logger.info('Done. Please add the list to the About dialog and commit the changes.')

    def run_lupdate(self, src_dir, build_dir=None, commit=False, lupdate_args=None):
        path = _get_bin_path(build_dir)
        self.check_lupdate_exists(path)
        logger.info('Updating translation source files from C++ sources...')
        _run(['lupdate', '-no-ui-lines', '-disable-heuristic', 'similartext', '-locations', 'none',
              '-extensions', 'c,cpp,h,js,mm,qrc,ui', '-no-obsolete', 'src',
              '-ts', str(Path(f'share/translations/keepassxc_en.ts')), *(lupdate_args or [])],
             cwd=src_dir, path=path, capture_output=False)
        logger.info('Translation source files updated.')
        if commit:
            _git_commit_files([f'share/translations/keepassxc_en.ts'],
                              'Update translation sources.', cwd=src_dir)


###########################################################################################
#                                       CLI Main
###########################################################################################


def main():
    if sys.platform == 'win32':
        # Enable terminal colours
        # noinspection PyUnresolvedReferences
        ctypes.windll.kernel32.SetConsoleMode(ctypes.windll.kernel32.GetStdHandle(-11), 7)

    sys.stderr.write(fmt.bold(f'{fmt.green("KeePassXC")} Release Preparation Tool\n'))
    sys.stderr.write(f'Copyright (C) 2016-{datetime.now().year} KeePassXC Team <https://keepassxc.org/>\n\n')

    parser = argparse.ArgumentParser(add_help=True)
    subparsers = parser.add_subparsers(title='Commands')

    check_parser = subparsers.add_parser('check', help=Check.__doc__)
    Check.setup_arg_parser(check_parser)
    check_parser.set_defaults(_cmd=Check)

    merge_parser = subparsers.add_parser('tag', help=Tag.__doc__)
    Tag.setup_arg_parser(merge_parser)
    merge_parser.set_defaults(_cmd=Tag)

    build_parser = subparsers.add_parser('build', help=Build.__doc__)
    Build.setup_arg_parser(build_parser)
    build_parser.set_defaults(_cmd=Build)

    build_src_parser = subparsers.add_parser('build-src', help=BuildSrc.__doc__)
    BuildSrc.setup_arg_parser(build_src_parser)
    build_src_parser.set_defaults(_cmd=BuildSrc)

    if sys.platform == 'darwin':
        notarize_parser = subparsers.add_parser('notarize', help=Notarize.__doc__)
        Notarize.setup_arg_parser(notarize_parser)
        notarize_parser.set_defaults(_cmd=Notarize)

    gpgsign_parser = subparsers.add_parser('gpgsign', help=GPGSign.__doc__)
    GPGSign.setup_arg_parser(gpgsign_parser)
    gpgsign_parser.set_defaults(_cmd=GPGSign)

    i18n_parser = subparsers.add_parser('i18n', help=I18N.__doc__)
    I18N.setup_arg_parser(i18n_parser)
    i18n_parser.set_defaults(_cmd=I18N)

    args = parser.parse_args()
    if '_cmd' not in args:
        parser.print_help()
        return 1
    # noinspection PyProtectedMember
    return args._cmd(parser).run(**{k: v for k, v in vars(args).items() if k != '_cmd'}) or 0


def _sig_handler(_, __):
    logger.error('Process interrupted.')
    sys.exit(3 | _cleanup())


signal.signal(signal.SIGINT, _sig_handler)
signal.signal(signal.SIGTERM, _sig_handler)

if __name__ == '__main__':
    ret = 0
    try:
        ret = main()
    except Error as e:
        logger.error(e.msg, *e.args, extra=e.kwargs)
        ret = e.kwargs.get('returncode', 1)
    except KeyboardInterrupt:
        logger.error('Process interrupted.')
        ret = 3
    except Exception as e:
        logger.critical('Unhandled exception:', exc_info=e)
        ret = 4
    except SystemExit as e:
        ret = e.code
    finally:
        ret |= _cleanup()
        sys.exit(ret)
