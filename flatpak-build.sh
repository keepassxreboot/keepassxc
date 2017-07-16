#!/usr/bin/env bash
set -e

# Prerequisites: Install flatpak + flatpak-builder
#
# Add the freedesktop runtime platform and sdk:
# $ flatpak remote-add --from gnome https://sdk.gnome.org/gnome.flatpakrepo
# $ flatpak install gnome org.freedesktop.Platform//1.6
# $ flatpak install gnome org.freedesktop.Sdk//1.6
#
# Build flatpak in local repo (".flatpak-test-repo"), add the remote, install
# $ *run this script*
# $ flatpak remote-add --user --no-gpg-verify keepassxc .flatpak-test-repo
# $ flatpak install --user keepassxc org.keepassxc.App
#
# To rebuild and update the installed flatpak from the local repo
# $ *run this script*
# $ flatpak --user update org.keepassxc.App
#
# To produce the single file flatpak package
# $ flatpak build-bundle .flatpak-test-repo org.keepassxc.App.flatpak org.keepassxc.App
# 
# To install flatpak package from single file package
# $ flatpak --user org.keepassxc.App.flatpak

flatpak-builder \
    --force-clean \
    --ccache \
    --require-changes \
    --repo=.flatpak-test-repo \
    --arch=$(flatpak --default-arch) \
    --subject="build of org.keepassxc.App, $(date)" \
    build \
    org.keepassxc.App.json

