#!/bin/bash
set -e

EXT_DIR="$(cd "$(dirname "$0")" && pwd)/ext"

mkdir -p "$EXT_DIR/fft2d"
mkdir -p "$EXT_DIR/neon2sse"
mkdir -p "$EXT_DIR/pthreadpool"
mkdir -p "$EXT_DIR/fp16"
mkdir -p "$EXT_DIR/fxdiv"

echo "Downloading fft2d..."
cd "$EXT_DIR/fft2d"
wget -nc https://github.com/petewarden/OouraFFT/archive/v1.0.tar.gz
tar xzf v1.0.tar.gz

echo "Downloading neon2sse..."
cd "$EXT_DIR/neon2sse"
wget -nc https://github.com/intel/ARM_NEON_2_x86_SSE/archive/a15b489e1222b2087007546b4912e21293ea86ff.tar.gz
tar xzf a15b489e1222b2087007546b4912e21293ea86ff.tar.gz

echo "Downloading pthreadpool..."
cd "$EXT_DIR/pthreadpool"
wget -nc https://github.com/google/pthreadpool/archive/c2ba5c50bb58d1397b693740cf75fad836a0d1bf.zip
unzip -n c2ba5c50bb58d1397b693740cf75fad836a0d1bf.zip

echo "Downloading FP16..."
cd "$EXT_DIR/fp16"
wget -nc https://github.com/Maratyszcza/FP16/archive/0a92994d729ff76a58f692d3028ca1b64b145d91.zip
unzip -n 0a92994d729ff76a58f692d3028ca1b64b145d91.zip

echo "Downloading FXdiv..."
cd "$EXT_DIR/fxdiv"
wget -nc https://github.com/Maratyszcza/FXdiv/archive/b408327ac2a15ec3e43352421954f5b1967701d1.zip
unzip -n b408327ac2a15ec3e43352421954f5b1967701d1.zip

echo "Downloading Kleidiai"
mkdir -p "$EXT_DIR/kleidiai"
cd "$EXT_DIR/kleidiai"
wget -nc https://github.com/ARM-software/kleidiai/archive/dc69e899945c412a8ce39ccafd25139f743c60b1.zip
unzip -n dc69e899945c412a8ce39ccafd25139f743c60b1.zip

echo "All dependencies downloaded."

