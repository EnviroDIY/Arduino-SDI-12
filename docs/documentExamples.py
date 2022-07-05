#!/usr/bin/env python
import fileinput
import re
import os
import glob

fileDir = os.path.dirname(os.path.realpath("__file__"))
# print(fileDir)

output_file = "examples.dox"
read_mes = [
    "../examples/ReadMe.md",
    "../examples/a_wild_card/ReadMe.md",
    "../examples/b_address_change/ReadMe.md",
    "../examples/c_check_all_addresses/ReadMe.md",
    "../examples/d_simple_logger/ReadMe.md",
    "../examples/e_continuous_measurement/ReadMe.md",
    "../examples/f_basic_data_request/ReadMe.md",
    "../examples/g_terminal_window/ReadMe.md",
    "../examples/h_SDI-12_slave_implementation/ReadMe.md",
    "../examples/i_SDI-12_interface/ReadMe.md",
    "../examples/j_external_pcint_library/ReadMe.md",
    "../examples/k_concurrent_logger/ReadMe.md",
]

if not os.path.exists(os.path.join(fileDir, "examples")):
    os.makedirs(os.path.join(fileDir, "examples"))

for filename in read_mes:
    out_path = os.path.join(fileDir, "examples")
    out_dir = filename.split("/")[2]
    out_name = out_dir + ".dox"
    if out_name == "ReadMe.md.dox":
        out_name = "examples.dox"
    abs_out = os.path.join(out_path, out_name)
    # print(abs_out)
    # with open(output_file, 'w+') as out_file:
    with open(abs_out, "w+") as out_file:

        abs_file_path = os.path.join(fileDir, filename)
        abs_file_path = os.path.abspath(os.path.realpath(abs_file_path))
        # print(abs_file_path)

        with open(abs_file_path, "r") as in_file:  # open in readonly mode
            out_file.write("/**\n")
            if out_name != "examples.dox":
                # out_file.write(
                #     "@example{{lineno}} {} @m_examplenavigation{{examples_page,{}/}} @m_footernavigation \n\n".format(
                #         filename.replace("..\\examples\\", "").replace(
                #             "\\ReadMe.md", ".ino"
                #         ), out_dir
                #     )
                # )
                out_file.write(
                    "@example{{lineno}} {} @m_examplenavigation{{examples_page,}} @m_footernavigation \n\n".format(
                        filename.replace("../examples/", "").replace(
                            "/ReadMe.md", ".ino"
                        )
                    )
                )
                # out_file.write(
                #     "@example{{lineno}} {} \n\n".format(
                #         filename.replace("..\\examples\\", "").replace(
                #             "\\ReadMe.md", ".ino"
                #         )
                #     )
                # )
            # out_file.write('\n@tableofcontents\n\n')

            print_me = True
            skip_me = False
            i = 1
            lines = in_file.readlines()
            for line in lines:
                # print(i, print_me, skip_me, line)

                # Remove markdown comment tags from doxygen commands within the markdown
                if print_me and not skip_me:
                    new_line = (
                        re.sub(r"\[//\]: # \( @(\w+?.*) \)", r"@\1", line)
                        .replace("```ini", "@code{.ini}")
                        .replace("```cpp", "@code{.cpp}")
                        .replace("```", "@endcode")
                    )
                    if out_name != "examples.dox":
                        new_line = new_line.replace("@page", "@section")
                        #    .replace('@section', '')
                        #    .replace('@subsection', '')
                        #    .replace('@subsubsection', '')
                        #    .replace('@paragraph', '')
                        #    .replace('@par', '')
                    out_file.write(new_line)

                # using skip_me to skip single lines, so unset it after reading a line
                if skip_me:
                    skip_me = False

                # a page, section, subsection, or subsubsection commands followed
                # immediately with by a markdown header leads to that section appearing
                # twice in the doxygen html table of contents.
                # I'm putting the section markers right above the header and then will skip the header.
                if re.match(r"\[//\]: # \( @mainpage", line) is not None:
                    skip_me = True
                if re.match(r"\[//\]: # \( @page", line) is not None:
                    skip_me = True
                if re.match(r"\[//\]: # \( @.*section", line) is not None:
                    skip_me = True
                if re.match(r"\[//\]: # \( @paragraph", line) is not None:
                    skip_me = True

                # I'm using these comments to fence off content that is only intended for
                # github mardown rendering
                if "[//]: # ( Start GitHub Only )" in line:
                    print_me = False

                if "[//]: # ( End GitHub Only )" in line:
                    print_me = True

                i += 1

            out_file.write("\n*/\n\n")
