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
git clone -b cross_compile https://github.com/bartslinger/cryptopp
cd cryptopp
export ARM_EMBEDDED_TOOLCHAIN=$HOME/git/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/bin
export ARM_EMBEDDED_SYSROOT=$HOME/git/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/arm-linux-gnueabihf
source ./setenv-rpi.sh
make -f GNUmakefile-cross
```

### OpenSSL

The following commands will install the OpenSSL library files in $HOME/rpi_staging.

```
cd $HOME/git
git clone -b OpenSSL_1_1_1-stable https://github.com/openssl/openssl
cd openssl
export PATH=$HOME/git/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/bin:$PATH
./configure linux-generic32 --prefix=$HOME/rpi_staging
make CC=arm-linux-gnueabihf-gcc
make install
```

### Protobuf

Two different parts are needed: The protobuf compiler which runs on the host machine, and the protobuf library which should be installed to the RPi. The compiler can be installed with the following command:

```
sudo apt install protobuf-compiler
protoc --version
```

If the version is not 3.0.0, uninstall it and download the correct version from here https://github.com/protocolbuffers/protobuf/releases.

To compile the protobuf library for RPi, follow these instructions:

```
cd $HOME/git
git clone --branch v3.9.1 --recurse-submodules https://github.com/protocolbuffers/protobuf.git
cd protobuf
```
Google removed the original file, so open autogen.sh with a text editor and change the gmock url on line 34 to: https://github.com/google/googlemock/archive/release-1.7.0.zip

```
sudo apt-get install autoconf automake libtool curl make g++ unzip
./autogen.sh
./configure --host=arm-linux-gnueabihf CC=$HOME/git/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc \
CXX=$HOME/git/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ \
--with-protoc=/usr/bin/protoc \
--prefix=$HOME/rpi_staging
make && make install
```

### MQTT

The mqtt library for c++ requires the c library first

```
cd $HOME/git
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
git checkout v1.2.1
cmake -Bbuild -H. -DPAHO_WITH_SSL=ON \
-DCMAKE_TOOLCHAIN_FILE=$HOME/git/vutura_utm/toolchains/rpi-toolchain.cmake \
-DOPENSSL_INCLUDE_DIR=$HOME/rpi_staging/include/openssl \
-DOPENSSL_LIB=$HOME/rpi_staging/libssl.a \
-DOPENSSLCRYPTO_LIB=$HOME/rpi_staging/libcrypto.a \
-DCMAKE_INSTALL_PREFIX=$HOME/rpi_staging
cmake --build build/ --target install
```

And compile mqtt for c++

```
cd $HOME/git
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.cpp
git checkout v1.0.1
cmake -Bbuild -H. -DPAHO_WITH_SSL=ON \
-DCMAKE_PREFIX_PATH=$HOME/rpi_staging/ \
-DCMAKE_TOOLCHAIN_FILE=$HOME/git/vutura_utm/toolchains/rpi-toolchain.cmake \
-DOPENSSL_INCLUDE_DIR=$HOME/rpi_staging/include/openssl \
-DOPENSSL_LIB=$HOME/rpi_staging/libssl.a \
-DOPENSSLCRYPTO_LIB=$HOME/rpi_staging/libcrypto.a \
-DCMAKE_INSTALL_PREFIX=$HOME/rpi_staging
cmake --build build/ --target install
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
