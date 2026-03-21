#!/bin/bash

git submodule update --init --recursive

sudo apt-get install libglfw3-dev
sudo apt install libxkbcommon-dev

chmod +x compile.sh