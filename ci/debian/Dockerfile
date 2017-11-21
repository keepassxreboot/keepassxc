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

# TIP: check this Dockerfile using this online tool: https://www.fromlatest.io

FROM debian:stretch-slim

RUN set -x \
    && apt-get -y update \
    && apt-get -y --no-install-recommends install \
      git cmake make \
      curl ca-certificates gnupg2 \
      libgcrypt20-dev libz-dev libyubikey-dev libykpers-1-dev \
      libkf5archive-dev \
      qttools5-dev \
      qttools5-dev-tools \
      qtbase5-dev \
      libqt5x11extras5-dev \
      libxi-dev \
      libxtst-dev \
    && apt-get -y install xvfb \
    && echo "deb http://apt.llvm.org/stretch/ llvm-toolchain-stretch-5.0 main" > /etc/apt/sources.list.d/llvm.list \
    && echo "deb-src http://apt.llvm.org/stretch/ llvm-toolchain-stretch-5.0 main" >> /etc/apt/sources.list.d/llvm.list \
    && curl -L https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - \
    && apt-get -y update \
    && apt-get -y --no-install-recommends install \
      clang-5.0 g++ \
      clang-format-5.0 clang-tidy-5.0 \
    && apt-get autoremove --purge \
    && rm -rf /var/lib/apt/lists/*

VOLUME ["/keepassxc"]
WORKDIR /keepassxc
