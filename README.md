# VUTURA: Onboard UTM interface

## Dependencies

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

## Build instructions
```
mkdir build && cd build
cmake ..
make
```

## Build for RPi

Use the compiler repository from `https://github.com/raspberrypi/tools`

For cryptopp, use `https://github.com/bartslinger/cryptopp/tree/crosscompile_rpi`
```
git clone https://github.com/bartslinger/cryptopp
cd cryptopp
git checkout crosscompile_rpi
export ARM_EMBEDDED_SYSROOT=<rpi/tools>/arm-bcm2708/arm-linux-gnueabihf/arm-linux-gnueabihf
export ARM_EMBEDDED_TOOLCHAIN=<rpi/tools>/arm-bcm2708/arm-linux-gnueabihf/bin
source ./setenv-rpi.sh
make -f GNUmakefile-cross -j8
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
