#!/bin/sh

# Script modified from scripts by Jeroen de Bruijn, thephez, and Adafruit
# https://gist.github.com/vidavidorra/548ffbcdae99d752da02
# https://github.com/thephez/doxygen-travis-build
# https://learn.adafruit.com/the-well-automated-arduino-library/travis-ci

# Exit with nonzero exit code if anything fails
set -e
cd $TRAVIS_BUILD_DIR/code_docs
echo "Current path: " $(pwd)
echo "Contents (ls):"
ls
cd $TRAVIS_BUILD_DIR/code_docs/Arduino-SDI-12
echo "Current path: " $(pwd)
echo "Contents (ls):"
ls
cd $TRAVIS_BUILD_DIR/code_docs/Arduino-SDI-12Doxygen
echo "Current path: " $(pwd)
echo "Contents (ls):"
ls

################################################################################
##### Upload the documentation to the gh-pages branch of the repository.   #####
# Only upload if Doxygen successfully created the documentation.
# Check this by verifying that the html directory and the file html/index.html
# both exist. This is a good indication that Doxygen did it's work.
if [ -d $TRAVIS_BUILD_DIR/code_docs/Arduino-SDI-12Doxygen/m.css ] &&
   [ -f $TRAVIS_BUILD_DIR/code_docs/Arduino-SDI-12Doxygen/m.css/index.html ]; then

    echo 'Uploading documentation to the gh-pages branch...'
    # Add everything in this directory (the Doxygen code documentation) to the
    # gh-pages branch.
    # GitHub is smart enough to know which files have changed and which files have
    # stayed the same and will only update the changed files.
    git add --all

    # Commit the added files with a title and description containing the Travis CI
    # build number and the GitHub commit reference that issued this build.
    git commit -m "Deploy code docs to GitHub Pages Travis build: ${TRAVIS_BUILD_NUMBER}" -m "Commit: ${TRAVIS_COMMIT}"

    # Force push to the remote gh-pages branch.
    # The ouput is redirected to /dev/null to hide any sensitive credential data
    # that might otherwise be exposed.
    git push --force "https://${GH_REPO_TOKEN}@${GH_REPO_REF}" > /dev/null 2>&1
else
    echo '' >&2
    echo 'Warning: No documentation (html) files have been found!' >&2
    echo 'Warning: Not going to push the documentation to GitHub!' >&2
    exit 1
fi