#!/bin/sh

# Script modified from scripts by Jeroen de Bruijn, thephez, and Adafruit
# https://gist.github.com/vidavidorra/548ffbcdae99d752da02
# https://github.com/thephez/doxygen-travis-build
# https://learn.adafruit.com/the-well-automated-arduino-library/travis-ci

# Exit with nonzero exit code if anything fails
set -e

cd $TRAVIS_BUILD_DIR/code_docs/Arduino-SDI-12

wget -q http://doxygen.nl/files/doxygen-1.8.18.linux.bin.tar.gz

echo "Decompressing..."
tar -xzf doxygen-1.8.18.linux.bin.tar.gz
ls

echo "Moving directory..."
mv doxygen-1.8.18/bin/doxygen .
chmod +x doxygen
echo "Current path: " $(pwd)
ls

echo 'Installed Doxygen version...'
./doxygen -v 2>&1
