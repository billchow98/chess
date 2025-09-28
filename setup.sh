#!/usr/bin/env bash

set -euxo pipefail

GIT_ROOT_DIR=$(dirname "$0")
# Install packages
curl -fsSL https://pixi.sh/install.sh | sh
sudo apt update
sudo apt install -y byobu unzip neovim build-essential
byobu-enable
# Set editor
tee "$HOME/.bashrc" << EOF
export PATH=$HOME/.pixi/bin:$PATH
export EDITOR=nvim
EOF
. "$HOME/.bashrc"
mkdir -p "$HOME/.config/nvim"
tee "$HOME/.config/nvim/init.vim" << EOF
set shiftwidth=4
set expandtab
set tabstop=8
set softtabstop=0
EOF
# Git settings
cd "$GIT_ROOT_DIR"
git config user.name "Bill Chow"
git config user.email "19355340+billchow98@users.noreply.github.com"
# Build executable
pixi run build
cd .build
cp tuna all_tests
# Download opening book
wget https://github.com/official-stockfish/books/raw/refs/heads/master/8moves_v3.pgn.zip
unzip 8moves_v3.pgn.zip
rm 8moves_v3.pgn.zip
# Get runner
git clone https://github.com/Disservin/fastchess.git
cd fastchess
make -j
cd ..
# Symlinks
ln -s fastchess/fastchess runner
ln -s ../anchors.txt .
cd ..
ln -s .build/compile_commands.json .
exec byobu
