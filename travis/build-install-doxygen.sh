#!/bin/sh

# Exit with nonzero exit code if anything fails
set -e

cd $TRAVIS_BUILD_DIR

if [ ! -f $TRAVIS_BUILD_DIR/doxygen-src/build/bin/doxygen ]; then

    # Build instructions from: https://www.stack.nl/~dimitri/doxygen/download.html
    echo "Cloning doxygen repository..."
    git clone https://github.com/doxygen/doxygen.git doxygen-src

    cd doxygen-src

    echo "Create build folder..."
    mkdir build
    cd build

    echo "Make..."
    cmake -G "Unix Makefiles" ..
    make
    echo "Done building doxygen."
    echo "doxygen path: " $(pwd)
    ls $TRAVIS_BUILD_DIR/doxygen-src/build/bin/

    echo "Current Doxygen version..."
    $TRAVIS_BUILD_DIR/doxygen-src/build/bin/doxygen -v
fi

cd $TRAVIS_BUILD_DIR/code_docs/Arduino-SDI-12
