#!/bin/bash

sudo add-apt-repository ppa:pch/phd2 -y
sudo apt-add-repository ppa:mutlaqja/ppa -y
sudo apt update -y && sudo apt upgrade -y 
sudo apt install make cmake indi-full phd2 clang-11 libwebsocketpp-dev libasio-dev libopencv-dev libcfitsio-dev libccfits-dev libssl-dev libnova-dev libgphoto2-dev libboost-dev libusb-1.0-0-dev libgsl-dev libcurl4-gnutls-dev libyaml-cpp-dev libglfw3-dev libgles-dev -y

if [ ! -d "/usr/include/opencv2" ]; then
    sudo mv /usr/include/opencv4/opencv2 /usr/include
fi

if [ ! -d "/usr/include/json" ]; then
    echo "Start building Jsoncpp library"
    git clone https://github.com/open-source-parsers/jsoncpp
    cd jsoncpp
    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_CXX_COMPILER=clang++-11 ..
    make -j4
    sudo make install
    cd ../..
    echo "Build newest Jsoncpp library"
else
    echo "Jsoncpp Library has already built"
fi

project_path=$(cd `dirname $0`; pwd)

sudo cp $project_path/config.h /usr/include/libqhy

echo "Start Builing..."
mkdir build && cd build
cmake ..
make -j4
echo "Finished!"
make clean


