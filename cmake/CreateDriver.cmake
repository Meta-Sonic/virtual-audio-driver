include(ParseDriverConfig)

macro(CreateDriver LIBRARY_NAME CONFIG_FILE_PATH ICON_FILE_PATH)

    ParseDriverConfig("${CONFIG_FILE_PATH}" "MTS_CONFIG")
    add_library(${LIBRARY_NAME} MODULE)

    set(CONFIG_FILE_INPUT_PATH "${VIRTUAL_DRIVER_ROOT_DIRECTORY}/config/config.h.in")
    set(CONFIG_FILE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/config")
    set(CONFIG_FILE_OUTPUT_PATH "${CONFIG_FILE_OUTPUT_DIRECTORY}/config.h")
    configure_file(${CONFIG_FILE_INPUT_PATH} ${CONFIG_FILE_OUTPUT_PATH})

    file(GLOB_RECURSE SOURCES
        "${VIRTUAL_DRIVER_ROOT_DIRECTORY}/src/*.c"
        "${VIRTUAL_DRIVER_ROOT_DIRECTORY}/src/*.cpp"
        "${VIRTUAL_DRIVER_ROOT_DIRECTORY}/src/*.h")

    target_sources(${LIBRARY_NAME} PRIVATE ${SOURCES} ${CONFIG_FILE_OUTPUT_PATH})
    source_group(TREE ${VIRTUAL_DRIVER_ROOT_DIRECTORY} FILES ${SOURCES})

    target_sources(${LIBRARY_NAME} PRIVATE ${ICON_FILE_PATH})
    set_source_files_properties(${ICON_FILE_PATH} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

    source_group("src" FILES "${CONFIG_FILE_OUTPUT_PATH}")
    source_group("Resources" FILES "${ICON_FILE_PATH}")

    target_include_directories(${LIBRARY_NAME} PRIVATE
        ${VIRTUAL_DRIVER_ROOT_DIRECTORY}/src
        ${CONFIG_FILE_OUTPUT_DIRECTORY})

    target_link_libraries(${LIBRARY_NAME} PRIVATE
        "-framework CoreFoundation"
        "-framework CoreAudio"
        "-framework Accelerate")

    target_compile_options(${LIBRARY_NAME} PRIVATE
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -fno-exceptions
        -fno-threadsafe-statics
        -fno-rtti

        -Wno-nonnull
        -Wno-nullability-completeness
        -Wno-unused-parameter
        -Wno-missing-field-initializers
        -Wno-switch
        -Wno-ignored-qualifiers

        -Wall
        -Wpedantic
        -Woverloaded-virtual
        -Wreorder
        -Wuninitialized
        -Wshift-sign-overflow
        -Wswitch-enum
        -Wunused-private-field
        -Wunreachable-code
        -Wcast-align
        -Winconsistent-missing-destructor-override
        -Wnullable-to-nonnull-conversion
        -Wsuggest-override)

    set_target_properties(${LIBRARY_NAME} PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON

        BUNDLE YES
        BUNDLE_EXTENSION "driver"

        XCODE_ATTRIBUTE_WRAPPER_EXTENSION "driver"
        XCODE_ATTRIBUTE_PRODUCT_NAME ${MTS_CONFIG_PRODUCT_NAME}
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER ${MTS_CONFIG_BUNDLE_IDENTIFIER}
        XCODE_ATTRIBUTE_EXECUTABLE_NAME "${MTS_CONFIG_EXECUTABLE_NAME}"

        MACOSX_BUNDLE_INFO_PLIST "${VIRTUAL_DRIVER_ROOT_DIRECTORY}/config/Info.plist.in"
        MACOSX_BUNDLE_INFO_STRING "${MTS_CONFIG_PRODUCT_NAME}"
        MACOSX_BUNDLE_BUNDLE_NAME "${MTS_CONFIG_PRODUCT_NAME}"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${MTS_CONFIG_VERSION_STRING}"
        MACOSX_BUNDLE_BUNDLE_VERSION "1"
        MACOSX_BUNDLE_COPYRIGHT "${MTS_CONFIG_COPYRIGHT}"
    )
endmacro()
