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

FROM ubuntu:trusty

RUN set -x \
    && apt-get -y update \
    && apt-get -y --no-install-recommends install \
      git build-essential clang-3.6 libclang-common-3.6-dev clang-format-3.6 cmake3 make \
      curl ca-certificates gnupg2 \
      libgcrypt20-dev zlib1g-dev libyubikey-dev libykpers-1-dev \
      qttools5-dev \
      qttools5-dev-tools \
      qtbase5-dev \
      libqt5x11extras5-dev \
      libxi-dev \
      libxtst-dev \
      xvfb \
    && apt-get autoremove --purge \
    && rm -rf /var/lib/apt/lists/*

VOLUME ["/keepassxc"]
WORKDIR /keepassxc
