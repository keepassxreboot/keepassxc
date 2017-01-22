#!/usr/bin/env bash
#  
# KeePassXC Release Preparation Helper
# Copyright (C) 2017 KeePassXC team <https://keepassxc.org/>
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

echo -e "\e[1m\e[32mKeePassXC\e[0m Release Preparation Helper"
echo -e "Copyright (C) 2017 KeePassXC Team <https://keepassxc.org/>\n"


# default values
RELEASE_NAME=""
APP_NAME="KeePassXC"
APP_NAME_LOWER="keepassxc"
SRC_DIR="."
GPG_KEY="CFB4C2166397D0D2"
GPG_GIT_KEY=""
OUTPUT_DIR="release"
BRANCH=""
RELEASE_BRANCH="master"
TAG_NAME=""
BUILD_SOURCES=false
DOCKER_IMAGE=""
DOCKER_CONTAINER_NAME="${APP_NAME_LOWER}-build-container"
CMAKE_OPTIONS=""
COMPILER="g++"
MAKE_OPTIONS="-j8"
BUILD_PLUGINS="autotype"
INSTALL_PREFIX="/usr/local"

ORIG_BRANCH="$(git rev-parse --abbrev-ref HEAD 2> /dev/null)"
ORIG_CWD="$(pwd)"


# helper functions
printUsage() {
    echo -e "\e[1mUsage:\e[0m $(basename $0) [options]"
    cat << EOF

Options:
  -v, --version        Release version number or name (required)
  -a, --app-name       Application name (default: '${APP_NAME}')
  -s, --source-dir     Source directory (default: '${SRC_DIR}')
  -k, --gpg-key        GPG key used to sign the release tarball
                       (default: '${GPG_KEY}')
  -g, --gpg-git-key    GPG key used to sign the merge commit and release tag,
                       leave empty to let Git choose your default key
                       (default: '${GPG_GIT_KEY}')
  -o, --output-dir     Output directory where to build the release
                       (default: '${OUTPUT_DIR}')
      --develop-branch Development branch to merge from (default: 'release/VERSION')
      --release-branch Target release branch to merge to (default: '${RELEASE_BRANCH}')
  -t, --tag-name       Override release tag name (defaults to version number)
  -b, --build          Build sources after exporting release
  -d, --docker-image   Use the specified Docker image to compile the application.
                       The image must have all required build dependencies installed.
                       This option has no effect if --build is not set.
      --container-name Docker container name (default: '${DOCKER_CONTAINER_NAME}')
                       The container must not exist already
  -c, --cmake-options  Additional CMake options for compiling the sources
      --compiler       Compiler to use (default: '${COMPILER}')
  -m, --make-options   Make options for compiling sources (default: '${MAKE_OPTIONS}')
  -i, --install-prefix Install prefix (default: '${INSTALL_PREFIX}')
  -p, --plugins        Space-separated list of plugins to build
                       (default: ${BUILD_PLUGINS})
  -h, --help           Show this help

EOF
}

logInfo() {
    echo -e "\e[1m[ \e[34mINFO\e[39m ]\e[0m $1"
}

logError() {
    echo -e "\e[1m[ \e[31mERROR\e[39m ]\e[0m $1" >&2
}

exitError() {
    logError "$1"
    if [ "" != "$ORIG_BRANCH" ]; then
        git checkout "$ORIG_BRANCH" > /dev/null 2>&1
    fi
    cd "$ORIG_CWD"
    exit 1
}


# parse command line options
while [ $# -ge 1 ]; do
    arg="$1"
    
    case "$arg" in
        -a|--app-name)
            APP_NAME="$2"
            shift ;;
        
        -s|--source-dir)
            SRC_DIR"$2"
            shift ;;
        
        -v|--version)
            RELEASE_NAME="$2"
            shift ;;
        
        -k|--gpg-key)
            GPG_KEY="$2"
            shift ;;
        
        -g|--gpg-git-key)
            GPG_GIT_KEY="$2"
            shift ;;
        
        -o|--output-dir)
            OUTPUT_DIR="$2"
            shift ;;
        
        --develop-branch)
            BRANCH="$2"
            shift ;;
        
        --release-branch)
            RELEASE_BRANCH="$2"
            shift ;;
        
        -t|--tag-name)
            TAG_NAME="$2"
            shift ;;
        
        -b|--build)
            BUILD_SOURCES=true ;;
        
        -d|--docker-image)
            DOCKER_IMAGE="$2"
            shift ;;
        
        --container-name)
            DOCKER_CONTAINER_NAME="$2"
            shift ;;
        
        -c|--cmake-options)
            CMAKE_OPTIONS="$2"
            shift ;;
        
        -m|--make-options)
            MAKE_OPTIONS="$2"
            shift ;;
        
        --compiler)
            COMPILER="$2"
            shift ;;
        
        -p|--plugins)
            BUILD_PLUGINS="$2"
            shift ;;
        
        -h|--help)
            printUsage
            exit ;;
        
        *)
            logError "Unknown option '$arg'\n"
            printUsage
            exit 1 ;;
    esac
    shift
done


if [ "" == "$RELEASE_NAME" ]; then
    logError "Missing arguments, --version is required!\n"
    printUsage
    exit 1
fi

if [ "" == "$TAG_NAME" ]; then
    TAG_NAME="$RELEASE_NAME"
fi
if [ "" == "$BRANCH" ]; then
    BRANCH="release/${RELEASE_NAME}"
fi
APP_NAME_LOWER="$(echo "$APP_NAME" | tr '[:upper:]' '[:lower:]')"
APP_NAME_UPPER="$(echo "$APP_NAME" | tr '[:lower:]' '[:upper:]')"

SRC_DIR="$(realpath "$SRC_DIR")"
OUTPUT_DIR="$(realpath "$OUTPUT_DIR")"
if [ ! -d "$SRC_DIR" ]; then
    exitError "Source directory '${SRC_DIR}' does not exist!"
fi

logInfo "Changing to source directory..."
cd "${SRC_DIR}"

logInfo "Performing basic checks..."

if [ -e "$OUTPUT_DIR" ]; then
    exitError "Output directory '$OUTPUT_DIR' already exists. Please choose a different location!"
fi

if [ ! -d .git ] || [ ! -f CHANGELOG ]; then
    exitError "Source directory is not a valid Git repository!"
fi

git tag | grep -q "$RELEASE_NAME"
if [ $? -eq 0 ]; then
    exitError "Release '$RELEASE_NAME' already exists!"
fi

git diff-index --quiet HEAD --
if [ $? -ne 0 ]; then
    exitError "Current working tree is not clean! Please commit or unstage any changes."
fi

git checkout "$BRANCH" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    exitError "Source branch '$BRANCH' does not exist!"
fi

grep -q "${APP_NAME_UPPER}_VERSION \"${RELEASE_NAME}\"" CMakeLists.txt
if [ $? -ne 0 ]; then
    exitError "${APP_NAME_UPPER}_VERSION version not updated to '${RELEASE_NAME}' in CMakeLists.txt!"
fi

grep -q "${APP_NAME_UPPER}_VERSION_NUM \"${RELEASE_NAME}\"" CMakeLists.txt
if [ $? -ne 0 ]; then
    exitError "${APP_NAME_UPPER}_VERSION_NUM version not updated to '${RELEASE_NAME}' in CMakeLists.txt!"
fi

if [ ! -f CHANGELOG ]; then
    exitError "No CHANGELOG file found!"
fi

grep -qPzo "${RELEASE_NAME} \(\d{4}-\d{2}-\d{2}\)\n=+\n" CHANGELOG
if [ $? -ne 0 ]; then
    exitError "CHANGELOG does not contain any information about the '${RELEASE_NAME}' release!"
fi

git checkout "$RELEASE_BRANCH" > /dev/null 2>&1
if [ $? -ne 0 ]; then
    exitError "Release branch '$RELEASE_BRANCH' does not exist!"
fi

logInfo "All checks pass, getting our hands dirty now!"

logInfo "Merging '${BRANCH}' into '${RELEASE_BRANCH}'..."

CHANGELOG=$(grep -Pzo "(?<=${RELEASE_NAME} \(\d{4}-\d{2}-\d{2}\)\n)=+\n\n?(?:.|\n)+?\n(?=\n)" \
    CHANGELOG | grep -Pzo '(?<=\n\n)(.|\n)+' | tr -d \\0)
COMMIT_MSG="Release ${RELEASE_NAME}"

git merge "$BRANCH" --no-ff -m "$COMMIT_MSG" -m "${CHANGELOG}" "$BRANCH" -S"$GPG_GIT_KEY"

logInfo "Creating tag '${RELEASE_NAME}'..."
if [ "" == "$GPG_GIT_KEY" ]; then
    git tag -a "$RELEASE_NAME" -m "$COMMIT_MSG" -m "${CHANGELOG}" -s
else
    git tag -a "$RELEASE_NAME" -m "$COMMIT_MSG" -m "${CHANGELOG}" -s -u "$GPG_GIT_KEY"
fi

logInfo "Merge done, creating target directory..."
mkdir -p "$OUTPUT_DIR"

if [ $? -ne 0 ]; then
    exitError "Failed to create output directory!"
fi

logInfo "Creating source tarball..."
TARBALL_NAME="${APP_NAME_LOWER}-${RELEASE_NAME}-src.tar.bz2"
git archive --format=tar "$RELEASE_BRANCH" --prefix="${APP_NAME_LOWER}-${RELEASE_NAME}/" \
    | bzip2 -9 >  "${OUTPUT_DIR}/${TARBALL_NAME}"


if $BUILD_SOURCES; then
    logInfo "Creating build directory..."
    mkdir -p "${OUTPUT_DIR}/build-release"
    mkdir -p "${OUTPUT_DIR}/bin-release"
    cd "${OUTPUT_DIR}/build-release"
    
    logInfo "Configuring sources..."
    for p in $BUILD_PLUGINS; do
        CMAKE_OPTIONS="${CMAKE_OPTIONS} -DWITH_XC_$(echo $p | tr '[:lower:]' '[:upper:]')=On"
    done
    
    if [ "$COMPILER" == "g++" ]; then
        export CC=gcc
    elif [ "$COMPILER" == "clang++" ]; then
        export CC=clang
    fi
    export CXX="$COMPILER"
    
    if [ "" == "$DOCKER_IMAGE" ]; then
        cmake -DCMAKE_BUILD_TYPE=Release -DWITH_TESTS=Off $CMAKE_OPTIONS \
            -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" "$SRC_DIR"
        
        logInfo "Compiling sources..."
        make $MAKE_OPTIONS
        
        logInfo "Installing to bin dir..."
        make DESTDIR="${OUTPUT_DIR}/bin-release" install/strip
    else
        logInfo "Launching Docker container to compile sources..."
        
        docker run --name "$DOCKER_CONTAINER_NAME" --rm \
            -e "CC=${CC}" -e "CXX=${CXX}" \
            -v "$(realpath "$SRC_DIR"):/keepassxc/src:ro" \
            -v "$(realpath "$OUTPUT_DIR"):/keepassxc/out:rw" \
            "$DOCKER_IMAGE" \
            bash -c "cd /keepassxc/out/build-release && \
                cmake -DCMAKE_BUILD_TYPE=Release -DWITH_TESTS=Off $CMAKE_OPTIONS \
                    -DCMAKE_INSTALL_PREFIX=\"${INSTALL_PREFIX}\" /keepassxc/src && \
                make $MAKE_OPTIONS && make DESTDIR=/keepassxc/out/bin-release install/strip"
        
        logInfo "Build finished, Docker container terminated."
    fi
            
    logInfo "Creating AppImage..."
    ${SRC_DIR}/AppImage-Recipe.sh "$APP_NAME" "$RELEASE_NAME"
    
    cd ..
    logInfo "Signing source tarball..."
    gpg --output "${TARBALL_NAME}.sig" --armor --local-user "$GPG_KEY" --detach-sig "$TARBALL_NAME"
    
    logInfo "Signing AppImage..."
    APPIMAGE_NAME="${APP_NAME}-${RELEASE_NAME}-x86_64.AppImage"
    gpg --output "${APPIMAGE_NAME}.sig" --armor --local-user "$GPG_KEY" --detach-sig "$APPIMAGE_NAME"
    
    logInfo "Creating digests..."
    sha256sum "$TARBALL_NAME"  > "${TARBALL_NAME}.DIGEST"
    sha256sum "$APPIMAGE_NAME" > "${APPIMAGE_NAME}.DIGEST"
fi

logInfo "Leaving source directory..."
cd "$ORIG_CWD"
git checkout "$ORIG_BRANCH" > /dev/null 2>&1

logInfo "All done!"
logInfo "Please merge the release branch back into the develop branch now and then push your changes."
logInfo "Don't forget to also push the tags using \e[1mgit push --tags\e[0m."
