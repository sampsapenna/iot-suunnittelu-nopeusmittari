# iot-suunnittelu-nopeusmittari
IoT suunnittelu ja perusteet kurssin harjoitustyö, lähdekoodi ja projektitiedostot.

## Getting started
Repository contains `pico sdk` as a submodule. Before starting run
```bash
git submodule init && git submodule update
cd pico-sdk && git submodule init && git submodule update && cd ..
```

## Usage
To build the software run following commands.
```bash
export PICO_SDK_PATH="$PWD/pico-sdk/"
mkdir build
cd build
cmake ..
make nopeusmittari
```
