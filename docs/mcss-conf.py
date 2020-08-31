DOXYFILE = "mcss-Doxyfile"
THEME_COLOR = "#cb4b16"
# FAVICON = "https://3qzcxr28gq9vutx8scdn91zq-wpengine.netdna-ssl.com/wp-content/uploads/2016/05/cropped-EnviroDIY_LogoMaster_TrueSquare_V5_TwoTree_Trans_notext-192x192.png"
FAVICON = "SDI-12Text-Cropped.png"
LINKS_NAVBAR1 = [
    (
        "Functions",
        "class_s_d_i12",
        [
            ('<a href="class_s_d_i12.html#constructor-destructor-begins-and-setters">Constructor, Destructor, Begins, and Setters</a>', ),
            ('<a href="class_s_d_i12.html#waking-up-and-talking-to-sensors">Waking Up and Talking To Sensors</a>', ),
            ('<a href="class_s_d_i12.html#reading-from-the-sdi-12-buffer">Reading from the SDI-12 Buffer</a>', ),
            ('<a href="class_s_d_i12.html#data-line-states">Data Line States</a>', ),
            ('<a href="class_s_d_i12.html#using-more-than-one-sdi-12-object">Using more than one SDI-12 Object</a>', ),
            ('<a href="class_s_d_i12.html#interrupt-service-routine">Interrupt Service Routine</a>', ),
        ],
    ),
    (
        "Examples",
        "examples_page",
        [
            ('<a href="a_wild_card_8ino-example.html">Getting Sensor Information</a>', ),
            ('<a href="b_address_change_8ino-example.html">Address Change</a>', ),
            ('<a href="c_check_all_addresses_8ino-example.html">Checking all Addresses</a>', ),
            ('<a href="d_simple_logger_8ino-example.html">Logging Data</a>', ),
            ('<a href="e_simple_parsing_8ino-example.html">Parsing Data</a>', ),
            ('<a href="f_basic_data_request_8ino-example.html">Simple Data Request</a>', ),
            ('<a href="g_terminal_window_8ino-example.html">Terminal Emulator 1</a>', ),
            ('<a href="h__s_d_i-12_slave_implementation_8ino-example.html">Slave Implementation</a>',),
            ('<a href="i__s_d_i-12_interface_8ino-example.html">Terminal Emulator 2</a>', ),
            ('<a href="j_external_pcint_library_8ino-example.html">External Interrupts</a>', ),
            ('<a href="k_concurrent_logger_8ino-example.html">Concurrent Measurements</a>', ),
        ],
    ),
    ("Classes", "annotated", [],),
    # ("Files", "files", []),
    (
        "Notes",
        "pages",
        [
            ("The SDI-12 Specification", "specifications"),
            ("Overview of Pin Change Interrupts", "interrupts_page"),
            ("Stepping through the Rx ISR", "rx_page"),
        ],
    ),
]
LINKS_NAVBAR2 = [
]
VERSION_LABELS = True
CLASS_INDEX_EXPAND_LEVELS = 2

STYLESHEETS = [
    "css/m-EnviroDIY+documentation.compiled.css",
]
