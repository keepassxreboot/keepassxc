FROM debian:8.7

MAINTAINER Mael Auzias <docker@mael.auzias.net>

LABEL Description="This image is used to run KeePassXC: cross platform community driven port of the windows application “Keepass Password Safe” (GPL) available at https://github.com/keepassxreboot/keepassxc"

RUN apt-get update && apt-get install -y \
      git ca-certificates \
      build-essential cmake libmicrohttpd-dev libxi-dev libxtst-dev qtbase5-dev libqt5x11extras5-dev qttools5-dev qttools5-dev-tools libgcrypt20-dev zlib1g-dev \
      --no-install-recommends \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

ENV HOME /home/keepassxreboot
ENV USER keepassxreboot

RUN adduser -q --disabled-password $USER \
 && chown -R $USER $HOME \
 && cd /home/keepassxreboot \
 && git clone https://github.com/keepassxreboot/keepassxc.git \
 && cd keepassxc \
 && cmake -DWITH_TESTS=OFF \
 && make \
 && make install

WORKDIR $HOME
USER $USER

ENTRYPOINT keepassxc
