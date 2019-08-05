# VUTURA: Onboard UTM interface

## Build for desktop (no cross-compiling)

### General dependencies
```
sudo apt-get install nlohmann-json-dev
sudo apt-get install protobuf-compiler
```

Go to a directory where you want to clone the dependencies

```
git clone https://github.com/weidai11/cryptopp.git
cd cryptopp
git checkout CRYPTOPP_8_0_0
make
make test
sudo make install
```

```
git clone https://github.com/nanomsg/nng.git
cd nng
git checkout v1.1.1
mkdir build && cd build
cmake ..
make
make test
sudo make install
```

### Requirements for use with python3
```
sudo apt-get install libnanomsg-dev
sudo pip3 install nanomsg
```

### Build vutura_utm
```
mkdir build && cd build
cmake ..
make
```

## Build for RPi

Get the toolchains from [https://github.com/raspberrypi/tools](https://github.com/raspberrypi/tools). This repository contains multiple toolchains. The one that is known to work is `tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf`.

Clone the RPi tools repository:
```
cd $HOME
mkdir git
cd git
git clone https://github.com/raspberrypi/tools
```

Create a staging directory for installation files, for example `mkdir $HOME/rpi_staging`.

### CryptoPP library

The [cryptopp library](https://github.com/bartslinger/cryptopp) can be compiled for RPi as follows:

```
cd $HOME/git
git clone https://github.com/bartslinger/cryptopp/tree/cross_compile
cd cryptopp
export ARM_EMBEDDED_TOOLCHAIN=$HOME/git/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/bin
export ARM_EMBEDDED_SYSROOT=$HOME/git/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/arm-linux-gnueabihf
source ./setenv-rpi.sh
make -f GNUmakefile-cross
```

### OpenSSL

The following commands will install the openSSL library files in $HOME/rpi_staging.

```
cd $HOME/git
git clone https://github.com/openssl/openssl
cd openssl
export PATH=$HOME/git/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/bin:$PATH
./configure linux-generic32 --prefix=$HOME/rpi_staging
make CC=arm-linux-gnueabihf-gcc
make install
```

## Run instructions

Before running airmap_node, create a configuration file (for example airmap_config.json)
```
{
    "username" : "",
    "password" : "",
    "api_key": "",
    "client_id": "",
    "device_id": ""
}
```

Then, call airmap_node like this:
```
./airmap_node -c /path/to/airmap_config.json
```
