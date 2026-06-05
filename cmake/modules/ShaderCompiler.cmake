function(compile_slang_shaders TARGET SHADERS_SOURCE_DIR BUILD_DIR)
    find_program(SLANGC slangc REQUIRED)

    set(SLANGC_FLAGS -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name)
    set(OUTPUT_DIR ${BUILD_DIR}/spirv)

    set(INPUT_SHADER "${SHADERS_SOURCE_DIR}/helloTriangle.slang")
    set(OUTPUT_SPV "${OUTPUT_DIR}/helloTriangle.spv")

    add_custom_command(
        OUTPUT  ${OUTPUT_SPV}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND ${SLANGC} ${INPUT_SHADER} ${SLANGC_FLAGS} -o ${OUTPUT_SPV}
        DEPENDS ${INPUT_SHADER}
        VERBATIM
    )

    add_custom_target(${TARGET} DEPENDS ${OUTPUT_SPV})
endfunction()

