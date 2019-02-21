# VUTURA: Onboard UTM interface

## Dependencies

### General dependencies
```
sudo apt-get install nlohmann-json-dev
```

Go to a directory where you want to clone the dependencies
```
git clone https://github.com/protobuf-c/protobuf-c.git
git checkout v1.3.1
cd protobuf-c/build-cmake
cmake .
make
sudo make install
```

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
