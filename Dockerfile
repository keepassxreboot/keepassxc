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

RUN set -x \
    && apt-get update \
    && apt-get install --yes software-properties-common

RUN set -x \
    && add-apt-repository --yes ppa:beineri/opt-qt58-trusty

RUN set -x \
    && apt-get update \
    && apt-get install --yes \
        g++ \
        cmake \
        libgcrypt20-dev \
        qt58base \
        qt58tools \
        qt58x11extras \
        libmicrohttpd-dev \
        libxi-dev \
        libxtst-dev \
        zlib1g-dev \
        wget \
        file \
        fuse \
        python

RUN set -x \
    && apt-get install --yes mesa-common-dev
        
VOLUME /keepassxc/src
VOLUME /keepassxc/out
WORKDIR /keepassxc

ENV CMAKE_PREFIX_PATH=/opt/qt58/lib/cmake
ENV LD_LIBRARY_PATH=/opt/qt58/lib
RUN set -x \
    && echo /opt/qt58/lib > /etc/ld.so.conf.d/qt58.conf
