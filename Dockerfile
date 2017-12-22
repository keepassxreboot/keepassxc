# KeePassXC Linux Release Build Dockerfile
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

FROM ubuntu:14.04

ENV QT5_VERSION=59
ENV QT5_PPA_VERSION=${QT5_VERSION}2

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
        libgcrypt20-dev \
        qt${QT5_VERSION}base \
        qt${QT5_VERSION}tools \
        qt${QT5_VERSION}x11extras \
        zlib1g-dev \
        libxi-dev \
        libxtst-dev \
        mesa-common-dev \
        libyubikey-dev \
        libykpers-1-dev

ENV CMAKE_PREFIX_PATH=/opt/qt${QT5_VERSION}/lib/cmake
ENV LD_LIBRARY_PATH=/opt/qt${QT5_VERSION}/lib
RUN set -x \
    && echo /opt/qt${QT_VERSION}/lib > /etc/ld.so.conf.d/qt${QT5_VERSION}.conf

# AppImage dependencies
RUN set -x \
    && apt-get install -y \
        libfuse2 \
        wget

VOLUME /keepassxc/src
VOLUME /keepassxc/out
WORKDIR /keepassxc
