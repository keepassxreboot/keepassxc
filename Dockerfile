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
    && add-apt-repository ppa:beineri/opt-qt${QT5_PPA_VERSION}-trusty

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
        mesa-common-dev

ENV CMAKE_PREFIX_PATH=/opt/qt${QT5_VERSION}/lib/cmake
ENV LD_LIBRARY_PATH=/opt/qt${QT5_VERSION}/lib
RUN set -x \
    && echo /opt/qt${QT_VERSION}/lib > /etc/ld.so.conf.d/qt${QT5_VERSION}.conf

# AppImage dependencies
RUN set -x \
    && apt-get install -y \
        wget \
        libfuse2

# build libyubikey
ENV YUBIKEY_VERSION=1.13
RUN set -x \
    && wget "https://developers.yubico.com/yubico-c/Releases/libyubikey-${YUBIKEY_VERSION}.tar.gz" \
    && tar xf libyubikey-${YUBIKEY_VERSION}.tar.gz \
    && cd libyubikey-${YUBIKEY_VERSION} \
    && ./configure --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu \
    && make \
    && make install \
    && cd .. \
    && rm -Rf libyubikey-${YUBIKEY_VERSION}*

# build libykpers-1
ENV YKPERS_VERSION=1.18.0
RUN set -x \
    && apt-get install -y libusb-dev
RUN set -x \
    && wget "https://developers.yubico.com/yubikey-personalization/Releases/ykpers-${YKPERS_VERSION}.tar.gz" \
    && tar xf ykpers-${YKPERS_VERSION}.tar.gz \
    && cd ykpers-${YKPERS_VERSION} \
    && ./configure --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu \
    && make \
    && make install \
    && cd .. \
    && rm -Rf ykpers-${YKPERS_VERSION}*

VOLUME /keepassxc/src
VOLUME /keepassxc/out
WORKDIR /keepassxc
