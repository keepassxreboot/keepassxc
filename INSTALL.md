# Build and Install KeePassXC

This document will guide you through the steps to build and install KeePassXC from source.
For more information, see also the [_Building KeePassXC_](https://github.com/keepassxreboot/keepassxc/wiki/Building-KeePassXC) page on the wiki.

The [QuickStart Guide](https://keepassxc.org/docs/KeePassXC_GettingStarted.html) gets you started using KeePassXC on your Windows, macOS, or Linux computer using pre-compiled binaries from the [downloads page](https://keepassxc.org/download).

## Toolchain and Build Dependencies

The following build tools must exist within your PATH:

* cmake (>= 3.16.0)
* make (>= 4.2) or ninja (>= 1.10)
* g++ (>= 9.3.0) or clang++ (>= 10.0)
* asciidoctor (>= 2.0)

The following minimum versions of libraries must exist:
* qt (>= 5.12.0)
* botan (>= 2.19.1 or >= 3.0.0)
* zlib (>= 1.2.0)

* Besides a working C++ toolchain, KeePassXC also has a number of direct build and runtime dependencies. For detailed information about how to install them, please refer to the GitHub wiki:

* [Set up Build Environment on Linux](https://github.com/keepassxreboot/keepassxc/wiki/Set-up-Build-Environment-on-Linux)
* [Set up Build Environment on Windows](https://github.com/keepassxreboot/keepassxc/wiki/Set-up-Build-Environment-on-Windows)
* [Set up Build Environment on macOS](https://github.com/keepassxreboot/keepassxc/wiki/Set-up-Build-Environment-on-macOS)

## Build Steps

We recommend using the release tool to perform builds, please read up-to-date instructions [on our wiki](https://github.com/keepassxreboot/keepassxc/wiki/Building-KeePassXC#building-using-the-release-tool).

To compile from source, open a **Terminal (Linux/MacOS)**, the **MSVC Tools Command Prompt (Windows)**, or **MSYS2-MinGW shell (Windows)**. For code development on Windows, you can use Visual Studio 2022, Visual Studio Code, or CLion.

1. Download the KeePassXC [source tarball](https://keepassxc.org/download#source) or check out the latest version from our [Git repository](https://github.com/keepassxreboot/keepassxc).

   To clone the project from Git, `cd` to a suitable location and run

   ```sh
   git clone https://github.com/keepassxreboot/keepassxc.git
   ```

   This will clone the entire contents of the repository and check out the current `develop` branch.

   To update the project from within the project's folder, you can run the following command:

   ```sh
   git pull
   ```

   For a stable build, it is recommended to check out the `latest` tag.

   ```sh
   git checkout latest
   cmake -B ./build
   cmake --build ./build
   ```

If you have `vcpkg` installed, add `-DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake` to the `cmake` command to automatically download and install all required build and runtime dependencies locally to your build directory before compiling KeePassXC. Using `vcpkg` is the preferred way to install dependencies on macOS and required on Windows if using the MSVC toolchain.

For more detailed build instructions for each platform, please refer to the [GitHub wiki](https://github.com/keepassxreboot/keepassxc/wiki/Building-KeePassXC).

Note: These steps place the compiled KeePassXC binary inside the `./build/src/` directory (`src/KeePassXC.app/Contents/MacOS` on macOS).

## MacOS Build Notes

If you installed Qt@5 via Homebrew and CMake fails to find your Qt installation, you can specify it manually by adding the following parameter:

`-DCMAKE_PREFIX_PATH=$(brew --prefix qt)/lib/cmake`

When building with ASAN support on macOS, you need to use `export ASAN_OPTIONS=detect_leaks=0` before running the tests (LSAN is no supported on macOS).

## Windows Build Notes

If you are using MSYS2, you have to add ```-G "MSYS Makefiles"``` at the beginning of the cmake command.

## Additional CMake Parameters

KeePassXC comes with a variety of build options that can turn on/off features. Each of these build options are supplied at the time of calling cmake:

```
-DKPXC_MINIMAL=[ON|OFF] Build KeePassXC with the minimal feature set required for basic usage (default: OFF)
-DKPXC_FEATURE_BROWSER=[ON|OFF] Browser integration and passkeys support (default: ON)
-DKPXC_FEATURE_SSHAGENT=[ON|OFF] SSH Agent integration (default: ON)
-DKPXC_FEATURE_FDOSECRETS=[ON|OFF] freedesktop.org Secret Service integration; replace system keyring (default: ON)
-DKPXC_FEATURE_NETWORK=[ON|OFF] Include code that reaches out to external networks (e.g. downloading icons) (default: ON)
-DKPXC_FEATURE_UPDATES=[ON|OFF] Include automatic update checks; disable for managed distributions (default: ON)
-DKPXC_FEATURE_DOCS=[ON|OFF] Build offline documentation; requires asciidoctor tool (default: ON)

-DWITH_TESTS=[ON|OFF] Enable building of unit tests (default: ON)
-DWITH_GUI_TESTS=[ON|OFF] Enable building of GUI tests (default: OFF)
-DWITH_WARN_DEPRECATED=[ON|OFF] Development only: warn about deprecated methods, including Qt. (default: OFF)
-DWITH_ASAN=[ON|OFF] Enable address sanitizer checks (Linux / macOS only) (default: OFF)
-DWITH_COVERAGE=[ON|OFF] Use to build with coverage tests (GCC only). (default: OFF)
-DWITH_APP_BUNDLE=[ON|OFF] Enable Application Bundle for macOS (default: ON)
-DWITH_CCACHE=[ON|OFF] Use ccache for build (default: OFF)
-DWITH_X11=[ON|OFF] Enable building with X11 dependencies (default: ON)
-DWITH_BUILD_QT=[ON|OFF] Build Qt6 as a dependency (default: OFF)

-DKEEPASSXC_BUILD_TYPE=[Snapshot|Release] Set the build type to show/hide stability warnings (default: "Snapshot")
-DOVERRIDE_VERSION=[X.X.X] Specify a version number when building. Used with snapshot builds (default: "")
-DKEEPASSXC_DIST_TYPE=[Native|Snap|AppImage|Flatpak] Specify the distribution method (default: "Native")
-DGIT_HEAD_OVERRIDE=[XXXXXXX] Specify the 7 digit git commit ref for this build. Used with distribution builds (default: "")
-DWITH_SBOM=[ON|OFF] Generate an SPDX SBOM of the deployment (requires Syft and Python spdx-tools) (default: OFF)
```

Note: Even though you can build the application with all TCP/IP networking code disabled, we still require and link against
Qt6's network library to use local named pipes on all operating systems.

## Installation

After you have successfully built KeePassXC, install the binary by executing the following:

```sh
sudo make install
```

## Packaging

You can create a package to redistribute KeePassXC (zip, deb, rpm, dmg, etc..). Refer to [keepassxc-packaging](https://github.com/keepassxreboot/keepassxc-packaging) for packaging scripts.

To package using CMake, run the following command using whichever [generators](https://cmake.org/cmake/help/latest/manual/cpack-generators.7.html) you would like to package with.

```sh
cpack -G "ZIP;WIX"
```

### Software bill of materials (SBOM)

Enable `-DWITH_SBOM=ON` to generate an SPDX 2.3 JSON document during deployment.  Install [Syft](https://github.com/anchore/syft) on PATH and Python 3.10 or newer with `python -m pip install 'spdx-tools>=0.8,<0.9'`. Use `-DSyft_EXECUTABLE=/path/to/syft` and `-DPython3_EXECUTABLE=/path/to/python` to select specific installations.

The generator scans deployed files with Syft and uses CMake's exported link inputs to identify static libraries and vendored components during CMake install. Qt's own SBOMs are used when available (needs Qt 6.8+). Use a fresh, isolated target directory as installation prefix, because the scan covers the entire installation prefix.

On Windows and macOS, CPack scans the deployment folder again after deployment and signing, and produces `keepassxc-<generator>.spdx.json` in the build directory. For an AppImage, linuxdeploy adds the runtime libraries outside CMake's install step, so set `SBOM_DEPLOY_DIR` to the AppDir and run the `sbom` target after linuxdeploy and library cleanup, before appimagetool:

```sh
cd build
cmake -DWITH_SBOM=ON -DKEEPASSXC_DIST_TYPE=AppImage -DSBOM_DEPLOY_DIR=/absolute/path/to/AppDir ..
cmake --build . --target sbom
```

`release-tool.py build <version>` generates an SBOM document by default and copies it into the release directory. Use `--no-sbom` to disable it. The exported SBOM document carries the same name as the generated release artifact, but with the extension `.spdx.json` appended.

## Testing

You can perform tests on the built executables with:

```sh
make test ARGS+="--output-on-failure"
```

On Linux, if you are not currently running on an X Server or Wayland, run the tests as follows:

```sh
make test ARGS+="-E test\(cli\|gui\) --output-on-failure"
xvfb-run -e errors -a --server-args="-screen 0 1024x768x24" make test ARGS+="-R test\(cli\|gui\) --output-on-failure"
```

Common parameters:

```sh
CTEST_OUTPUT_ON_FAILURE=1
ARGS+=-jX
ARGS+="-E testgui"
```
