#!/usr/bin/env python
# %%
"""
Custom matrix builder for Arduino-SDI-12 step 3 - Build Matrix.

This script builds the Arduino-SDI-12-specific job matrix by:
1. Reading the modem list from Arduino-SDI-12Client.h
2. Loading the extra PlatformIO configuration
3. Applying Arduino-SDI-12-specific matrix exclusions and inclusions
4. Returning the final filtered matrix

This is designed to be called from the CI pipeline as a custom builder.
See: https://github.com/EnviroDIY/workflows
"""

import os
import re
import json

# %%
# Import helper functions from the CI pipeline
from build_utils import (
    dict_product,
    remove_nested_duplicates,
    read_platformio_config,
    build_pio_mappings,
)


# %%
def build_custom_matrix(config: dict) -> list[dict]:
    """
    Build the Arduino-SDI-12-specific job matrix.

    This is the main entry point called by the CI pipeline (3_build_matrix.py).
    """

    # %%
    # Extract config values
    workspace_path = config.get("workspace_path", os.getcwd())

    compiler_list = config.get("compiler_list", ["platformio", "arduino-cli"])
    examples_to_build = config.get("examples_to_build", [])
    build_envs = config.get("build_envs", [])
    build_fqbns = config.get("build_fqbns", [])
    boards = build_envs + build_fqbns
    inline_defines = config.get("inline_defines", [[]])
    # compiler_flags = config.get("compiler_flags", [[]])
    compiler_flags = [["-D SDI12_INTERNAL_PCINT"], []]

    # Read the PlatformIO config and build mapping dictionaries for boards and environments
    print("Reading the PlatformIO config...")
    pio_ini_dir = os.path.dirname(config.get("pio_config_file"))  # type: ignore
    pio_config = read_platformio_config(pio_ini_dir)
    print("Building mapping dictionaries...")
    pio_env_to_board, pio_env_to_platform, board_to_pio_env = build_pio_mappings(
        pio_config
    )

    # %%
    print("=== Arduino-SDI-12 Custom Matrix Builder ===")

    # %%
    # expand the combination of boards, modems, and examples into a job matrix

    p_cart_join = list(
        dict_product(
            {
                "compiler": ["platformio"],
                "example": examples_to_build,
                "board": build_envs,
                "inline_defines": inline_defines,
                "compiler_flags": [[]],
            }
        )
    )

    a_cart_join = list(
        dict_product(
            {
                "compiler": ["arduino-cli"],
                "example": examples_to_build,
                "board": build_fqbns,
                "inline_defines": inline_defines,
                "compiler_flags": [[]],
            }
        )
    )

    a_cart_join_ext = list(
        dict_product(
            {
                "compiler": ["arduino-cli"],
                "example": examples_to_build,
                "board": [fqbn for fqbn in build_fqbns if ":avr:" in fqbn],
                "inline_defines": inline_defines,
                "compiler_flags": [["-D SDI12_INTERNAL_PCINT"]],
            }
        )
    )

    cart_join = p_cart_join + a_cart_join + a_cart_join_ext
    cart_join_list = [json.dumps(e) for e in cart_join]
    print(f"Total possible combinations: {len(cart_join)}")

    # %%

    # a list of known failures to skip in the job matrix

    # exclude the SDI12_INTERNAL_PCINT build on non-AVR boards
    matrix_exclusions = [
        # {
        #     "compiler": compiler_list,
        #     "example": examples_to_build,
        #     "board": [fqbn for fqbn in build_fqbns if ":avr:" not in fqbn],
        #     "inline_defines": [[]],
        #     "compiler_flags": [["-D SDI12_INTERNAL_PCINT"]],
        # },
        {
            "compiler": ["arduino-cli"],
            "example": [os.path.join("extras", "TestCommands")],
            "board": [
                "arduino:avr:leonardo",
                "arduino:avr:uno",
            ],
            "inline_defines": [[]],
            "compiler_flags": [["-D SDI12_INTERNAL_PCINT"], []],
        },
        {
            "compiler": ["platformio"],
            "example": [os.path.join("extras", "TestCommands")],
            "board": [
                "feather328p",
                "feather328p_internal_pcint",
                "feather32u4",
                "feather32u4_internal_pcint",
                "leonardo",
                "leonardo_internal_pcint",
                "uno",
                "uno_internal_pcint",
            ],
            "inline_defines": [[]],
            "compiler_flags": [[]],
        },
    ]

    # expand the matrix exclusions
    expanded_matrix_exclusions = []
    for exclusion in matrix_exclusions:
        exclusion_list = list(dict_product(exclusion))
        expanded_matrix_exclusions.extend(exclusion_list)

    # filter out the known failures from the job matrix
    expanded_matrix_exclusions_set = remove_nested_duplicates(
        expanded_matrix_exclusions
    )
    expanded_matrix_exclusions_list = [
        json.dumps(e) for e in expanded_matrix_exclusions_set
    ]
    expanded_matrix_exclusions_list.sort()
    print(f"Matrix exclusions: {len(expanded_matrix_exclusions_list)}")

    # %%
    # decide on the filtered matrix to use for the job matrix
    expanded_matrix_exclusions_lookup = set(expanded_matrix_exclusions_list)
    assembled_matrix = [
        json.loads(e)
        for e in cart_join_list
        if e not in expanded_matrix_exclusions_lookup
    ]

    assembled_matrix = sorted(
        assembled_matrix,
        key=lambda x: (
            x["compiler"],
            x["board"],
            x["example"],
            x["compiler_flags"],
        ),
    )

    final_matrix = remove_nested_duplicates(assembled_matrix)
    print(f"Final filtered matrix: {len(final_matrix)}")

    # %%
    return final_matrix


# %%
# cSpell:ignore fqbn fqbns rensas
