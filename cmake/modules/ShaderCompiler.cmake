function(compile_slang_shaders TARGET SHADERS_SOURCE_DIR BUILD_DIR)
    find_program(SLANGC slangc REQUIRED)

    file(GLOB_RECURSE ALL_SHADERS ${SHADERS_SOURCE_DIR}/*.slang)

    if(NOT ALL_SHADERS)
        message(WARNING "No shader files found in ${SHADERS_SOURCE_DIR}")
        add_custom_target(${TARGET} SOURCES ${ALL_SHADER})
        return()
    endif()

    set(SLANGC_FLAGS -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name)
    set(OUTPUT_DIR ${BUILD_DIR}/spirv)

    set(INPUT_SHADER "${SHADERS_SOURCE_DIR}/helloTriangle.slang")
    set(OUTPUT_SPV "${OUTPUT_DIR}/helloTriangle.spv")
    set(OUTPUT_SPVS)

    foreach(SHADER IN LISTS ALL_SHADERS)
        get_filename_component(SHADER_NAME "${SHADER}" NAME_WE)

        set(OUTPUT_SPV "${OUTPUT_DIR}/${SHADER_NAME}.spv")

        add_custom_command(
            OUTPUT  ${OUTPUT_SPV}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
            COMMAND ${SLANGC} ${SHADER} ${SLANGC_FLAGS} -o ${OUTPUT_SPV}
            DEPENDS ${SHADER}
            VERBATIM
        )

        list(APPEND OUTPUT_SPVS "${OUTPUT_SPV}")
    endforeach()

    add_custom_target(${TARGET} DEPENDS ${OUTPUT_SPVS})
endfunction()

