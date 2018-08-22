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

FROM snapcore/snapcraft

ENV REBUILD_COUNTER=1

ENV QT5_VERSION=510
ENV QT5_PPA_VERSION=5.10.1

RUN set -x \
    && apt update -y \
    && apt -y install software-properties-common

RUN set -x \
    && add-apt-repository ppa:phoerious/keepassxc

RUN set -x \
    && apt update -y \
    && apt-get -y --no-install-recommends install \
      build-essential \
      cmake \
      libgcrypt20-18-dev \
      libargon2-0-dev \
      libsodium-dev \
      qtbase5-dev \
      qttools5-dev \
      qttools5-dev-tools \
      zlib1g-dev \
      libyubikey-dev \
      libykpers-1-dev \
      libxi-dev \
      libxtst-dev \
      xvfb

RUN set -x \
    && apt-get autoremove --purge

