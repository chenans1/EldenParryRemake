My Personal SKSE plugin template that i created by cloning my original msco template and then deleting all the stuff i dont use
# SKSE Plugin Template

A minimal CMake + CommonLibVR-ng + SKSE template for Skyrim SE plugins.

## Quick start

```bash
git clone https://github.com/<you>/SKSE-Plugin-Template
cd SKSE-Plugin-Template
git submodule add -b ng https://github.com/alandtse/CommonLibVR.git extern/CommonLibVR-ng
git submodule update --init --recursive
cmake -B build -S .
cmake --build build --config Release
