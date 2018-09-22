# KeePassXC Linux Release Build Dockerfile
# Copyright (C) 2017-2018 KeePassXC team <https://keepassxc.org/>
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

FROM ubuntu:14.04

ENV REBUILD_COUNTER=8

ENV QT5_VERSION=59
ENV QT5_PPA_VERSION=${QT5_VERSION}4
ENV TERM=xterm-256color

RUN set -x \
    && apt-get update -y \
    && apt-get -y install software-properties-common

RUN set -x \
    && add-apt-repository ppa:beineri/opt-qt${QT5_PPA_VERSION}-trusty \
    && add-apt-repository ppa:phoerious/keepassxc

RUN set -x \
    && apt-get update -y \
    && apt-get upgrade -y

# build and runtime dependencies
RUN set -x \
    && apt-get install -y \
        cmake3 \
        g++ \
        git \
        libgcrypt20-18-dev \
        libargon2-0-dev \
        libsodium-dev \
        libcurl-no-gcrypt-dev \
        qt${QT5_VERSION}base \
        qt${QT5_VERSION}tools \
        qt${QT5_VERSION}x11extras \
        qt${QT5_VERSION}translations \
        qt${QT5_VERSION}imageformats \
        zlib1g-dev \
        libxi-dev \
        libxtst-dev \
        mesa-common-dev \
        libyubikey-dev \
        libykpers-1-dev

ENV CMAKE_PREFIX_PATH="/opt/qt${QT5_VERSION}/lib/cmake"
ENV CMAKE_INCLUDE_PATH="/opt/keepassxc-libs/include"
ENV CMAKE_LIBRARY_PATH="/opt/keepassxc-libs/lib/x86_64-linux-gnu"
ENV CPATH="${CMAKE_INCLUDE_PATH}"
ENV LD_LIBRARY_PATH="${CMAKE_LIBRARY_PATH}:/opt/qt${QT5_VERSION}/lib"

RUN set -x \
    && echo "/opt/qt${QT5_VERSION}/lib" > /etc/ld.so.conf.d/qt${QT5_VERSION}.conf \
    && echo "/opt/keepassxc-libs/lib/x86_64-linux-gnu" > /etc/ld.so.conf.d/keepassxc.conf

# AppImage dependencies
RUN set -x \
    && apt-get install -y \
        libfuse2 \
        wget

RUN set -x \
    && apt-get autoremove --purge \
    && rm -rf /var/lib/apt/lists/*

VOLUME /keepassxc/src
VOLUME /keepassxc/out
WORKDIR /keepassxc
