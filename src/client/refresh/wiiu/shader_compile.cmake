find_program(WUT_GLSLCOMPILER_EXE NAMES glslcompiler.elf HINTS "${DEVKITPRO}/tools/bin")

function(wut_compile_shaders vertexShader fragmentShader output)
    if (NOT WUT_GLSLCOMPILER_EXE)
        message(FATAL_ERROR "Could not find glslcompiler.elf: try installing wut-tools")
    endif()

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/content/shaders/${output}
        COMMAND ${WUT_GLSLCOMPILER_EXE} -vs ${CMAKE_CURRENT_SOURCE_DIR}/${vertexShader} -ps ${CMAKE_CURRENT_SOURCE_DIR}/${fragmentShader} -o ${CMAKE_CURRENT_BINARY_DIR}/content/shaders/${output}
        DEPENDS ${vertexShader} ${fragmentShader} 
        COMMENT "Compiling GLSL VS: ${vertexShader} FS: ${fragmentShader} to ${output}"
        VERBATIM
    )
    add_custom_target(shader-${output} ALL DEPENDS content/shaders/${output})

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${output} DESTINATION shaders)

endfunction()
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/2d_vert.glsl
    src/client/refresh/wiiu/shaders/2d_frag.glsl
    2d.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/2d_color_vert.glsl
    src/client/refresh/wiiu/shaders/2d_color_frag.glsl
    2d_color.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/2d_vert.glsl
    src/client/refresh/wiiu/shaders/2d_postprocess_frag.glsl
    2d_postprocess.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/2d_vert.glsl
    src/client/refresh/wiiu/shaders/2d_postprocess_water_frag.glsl
    2d_postprocess_water.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/3d_lm_vert.glsl
    src/client/refresh/wiiu/shaders/3d_lm_frag.glsl
    3d_lm.gsh)
# wut_compile_shaders(
# 	src/client/refresh/wiiu/shaders/3d_lm_vert.glsl
# 	src/client/refresh/wiiu/shaders/3d_lm_no_color_frag.glsl
# 	3d_lm_no_color.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/3d_vert.glsl
    src/client/refresh/wiiu/shaders/3d_frag.glsl
    3d_trans.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/3d_vert.glsl
    src/client/refresh/wiiu/shaders/3d_color_frag.glsl
    3d_color.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/3d_water_vert.glsl
    src/client/refresh/wiiu/shaders/3d_water_frag.glsl
    3d_water.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/3d_lm_flow_vert.glsl
    src/client/refresh/wiiu/shaders/3d_lm_frag.glsl
    3d_lm_flow.gsh)
# wut_compile_shaders(
# 	src/client/refresh/wiiu/shaders/3d_lm_flow_vert.glsl
# 	src/client/refresh/wiiu/shaders/3d_lm_no_color_frag.glsl
# 	3d_lm_flow_no_color.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/3d_flow_vert.glsl
    src/client/refresh/wiiu/shaders/3d_frag.glsl
    3d_trans_flow.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/3d_vert.glsl
    src/client/refresh/wiiu/shaders/3d_sky_frag.glsl
    3d_sky.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/3d_vert.glsl
    src/client/refresh/wiiu/shaders/3d_sprite_frag.glsl
    3d_sprite.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/3d_vert.glsl
    src/client/refresh/wiiu/shaders/3d_sprite_alpha_frag.glsl
    3d_sprite_alpha.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/alias_vert.glsl
    src/client/refresh/wiiu/shaders/alias_frag.glsl
    alias.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/alias_vert.glsl
    src/client/refresh/wiiu/shaders/alias_color_frag.glsl
    alias_color.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/particles_vert.glsl
    src/client/refresh/wiiu/shaders/particles_frag.glsl
    particle.gsh)
# wut_compile_shaders(
# 	src/client/refresh/wiiu/shaders/particles_vert.glsl
# 	src/client/refresh/wiiu/shaders/particles_square_frag.glsl
# 	particle_square.gsh)
wut_compile_shaders(
    src/client/refresh/wiiu/shaders/drcCopy_vert.glsl
    src/client/refresh/wiiu/shaders/drcCopy_frag.glsl
    drc_copy.gsh)
