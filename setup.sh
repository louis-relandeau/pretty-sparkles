#!/bin/bash

git submodule update --init --recursive

sudo apt update
sudo apt install -y clang-format

sudo apt-get install libglfw3-dev
sudo apt install libxkbcommon-dev

git config core.hooksPath .githooks
chmod +x .githooks/pre-commit

chmod +x compile.sh

mkdir -p fields