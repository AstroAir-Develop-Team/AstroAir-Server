if [[ ! -d "/usr/include/opencv2" ]]; then
    sudo mv /usr/include/opencv4/opencv2 /usr/include
fi

if [[ ! -d "/usr/include/json" ]]; then
    echo "Start building Jsoncpp library"
    git clone https://github.com/open-source-parsers/jsoncpp
    cd jsoncpp
    mkdir build && cd build
    cmake -DCMAKE_INSTALL_PREFIX=/usr ..
    make -j4
    sudo make install
    cd ../..
    echo "Build newest Jsoncpp library"
fi