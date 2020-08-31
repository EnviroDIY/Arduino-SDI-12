#!/usr/bin/env python
import fileinput, re

print_me = True
skip_me = False
i = 1
# for line in fileinput.input(openhook=fileinput.hook_encoded("utf-8", "surrogateescape")):
for line in fileinput.input():
    # print(i, print_me, skip_me, line)

    # Remove markdown comment tags from doxygen commands within the markdown
    if print_me and not skip_me:
        print(re.sub(r'\[//\]: # \( @(\w+?.*) \)', r'@\1', line), end="")

    # using skip_me to skip single lines, so unset it after reading a line
    if skip_me:
        skip_me = False;

    # a page, section, subsection, or subsubsection commands followed
    # immediately with by a markdown header leads to that section appearing
    # twice in the doxygen html table of contents.
    # I'm putting the section markers right above the header and then will skip the header.
    if re.match(r'\[//\]: # \( @mainpage', line) is not None:
        skip_me = True;
    if re.match(r'\[//\]: # \( @page', line) is not None:
        skip_me = True;
    if re.match(r'\[//\]: # \( @.*section', line) is not None:
        skip_me = True;
    if re.match(r'\[//\]: # \( @paragraph', line) is not None:
        skip_me = True;

    # I'm using these comments to fence off content that is only intended for
    # github mardown rendering
    if "[//]: # ( Start GitHub Only )" in line:
        print_me = False

    if "[//]: # ( End GitHub Only )" in line:
        print_me = True

    i += 1
