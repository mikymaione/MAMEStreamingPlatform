#!/bin/bash
clear
#Add Fedora free packages repo.
sudo dnf install https://download1.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm
#Add Fedora non-free packages repo.
sudo dnf install https://download1.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm
#Add Fedora develop package
sudo dnf install clang SDL2-devel SDL2_ttf-devel libXi-devel libXinerama-devel qt5-qtbase-devel qt5-qttools expat-devel fontconfig-devel alsa-lib-devel
#Add Fedora develop package 3rdparty
sudo dnf install ffmpeg-devel ffmpeg-libs portaudio-devel portmidi-devel utf8proc-devel pugixml-devel flac-devel sqlite-devel glm-devel rapidjson-devel
