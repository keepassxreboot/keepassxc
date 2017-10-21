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

FROM centos:7

RUN set -x \
    && curl "https://copr.fedorainfracloud.org/coprs/bugzy/keepassxc/repo/epel-7/bugzy-keepassxc-epel-7.repo" \
        > /etc/yum.repos.d/bugzy-keepassxc-epel-7.repo

RUN set -x \
    && curl "https://copr.fedorainfracloud.org/coprs/sic/backports/repo/epel-7/sic-backports-epel-7.repo" \
        > /etc/yum.repos.d/sic-backports-epel-7.repo

RUN set -x \
    && yum clean -y all \
    && yum upgrade -y

# build and runtime dependencies
RUN set -x \
    && yum install -y \
        make \
        automake \
        gcc-c++ \
        cmake \
        libgcrypt16-devel \
        qt5-qtbase-devel \
        qt5-linguist \
        qt5-qttools \
        zlib-devel \
        qt5-qtx11extras \
        qt5-qtx11extras-devel \
        libXi-devel \
        libXtst-devel

# AppImage dependencies
RUN set -x \
    && yum install -y \
        wget \
        fuse-libs

# build libyubikey
ENV YUBIKEY_VERSION=1.13
RUN set -x && yum install -y libusb-devel
RUN set -x \
    && wget "https://developers.yubico.com/yubico-c/Releases/libyubikey-${YUBIKEY_VERSION}.tar.gz" \
    && tar xf libyubikey-${YUBIKEY_VERSION}.tar.gz \
    && cd libyubikey-${YUBIKEY_VERSION} \
    && ./configure --prefix=/usr --libdir=/usr/lib64 \
    && make \
    && make install \
    && cd .. \
    && rm -Rf libyubikey-${YUBIKEY_VERSION}*

# build libykpers-1
ENV YKPERS_VERSION=1.18.0
RUN set -x \
    && wget "https://developers.yubico.com/yubikey-personalization/Releases/ykpers-${YKPERS_VERSION}.tar.gz" \
    && tar xf ykpers-${YKPERS_VERSION}.tar.gz \
    && cd ykpers-${YKPERS_VERSION} \
    && ./configure --prefix=/usr --libdir=/usr/lib64 \
    && make \
    && make install \
    && cd .. \
    && rm -Rf ykpers-${YKPERS_VERSION}*

VOLUME /keepassxc/src
VOLUME /keepassxc/out
WORKDIR /keepassxc
