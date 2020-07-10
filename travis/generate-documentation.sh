#!/bin/sh

# Script modified from scripts by Jeroen de Bruijn, thephez, and Adafruit
# https://gist.github.com/vidavidorra/548ffbcdae99d752da02
# https://github.com/thephez/doxygen-travis-build
# https://learn.adafruit.com/the-well-automated-arduino-library/travis-ci

# Exit with nonzero exit code if anything fails
set -e

cd $TRAVIS_BUILD_DIR/code_docs/Arduino-SDI-12

echo 'Current Doxygen version...'
$TRAVIS_BUILD_DIR/doxygen-src/build/bin/doxygen -v 2>&1

# Redirect both stderr and stdout to the log file AND the console.
# Print out doxygen warnings in red
echo 'Generating Doxygen code documentation...'
$TRAVIS_BUILD_DIR/doxygen-src/build/bin/doxygen $DOXYFILE 2>&1 | tee doxygen.log
# ./doxygen Doxyfile 2>&1 | tee doxygen.log > >(while read line; do echo -e "\e[01;31m$line\e[0m" >&2; done)