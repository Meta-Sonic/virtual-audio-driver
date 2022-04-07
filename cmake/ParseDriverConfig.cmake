macro(ParseConfigFile Filepath prefix)
    file(STRINGS "${Filepath}" LINES)

    foreach (LINE ${LINES})
        string(FIND ${LINE} "=" HAS_EQUAL)

        if (${HAS_EQUAL} GREATER -1)
            # Find variable name.
            string(REGEX MATCH "^[^=]+" VAR_NAME ${LINE})

            # Find the value.
            string(REPLACE "${VAR_NAME}=" "" VALUE ${LINE})
            string(TOUPPER ${VAR_NAME} VAR_NAME)

            # Strip leading and trailing whitespaces.
            string(STRIP ${VAR_NAME} VAR_NAME)
            string(STRIP ${VALUE} VALUE)

            # Set value.
            set(VAR_NAME "${prefix}_${VAR_NAME}")
            set(${VAR_NAME} "${VALUE}")
            message("${VAR_NAME} = ${VALUE}")
        endif()
    endforeach()
endmacro()

macro(ParseDriverConfig Filepath prefix)
    ParseConfigFile(${Filepath} ${prefix})

    set(${prefix}_DEVICE_UID "${${prefix}_DEVICE_UID_PREFIX}Device_UID")
    set(${prefix}_BOX_UID "${${prefix}_DEVICE_UID_PREFIX}Box_UID")
    set(${prefix}_DEVICE_MODEL_UID "${${prefix}_DEVICE_UID_PREFIX}DeviceModel_UID")

    set(${prefix}_CREATE_FUNCTION_NAME "${${prefix}_DEVICE_UID_PREFIX}_Create")

    execute_process(
        COMMAND python -c "import math, sys; sys.stdout.write(str(math.pow(10.0, ${${prefix}_VOLUME_MIN_DB} / 20.0)))"
        OUTPUT_VARIABLE ${prefix}_VOLUME_MIN_AMP)
endmacro()