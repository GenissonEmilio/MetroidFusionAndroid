# Stage small presentation-only adaptations without modifying the pinned core.
# Exact anchors fail closed when a future core update requires review.
function(minish_replace_once variable before after)
    string(LENGTH "${${variable}}" original_size)
    string(REPLACE "${before}" "" removed "${${variable}}")
    string(LENGTH "${removed}" remaining_size)
    string(LENGTH "${before}" anchor_size)
    math(EXPR removed_size "${original_size} - ${remaining_size}")
    if(NOT removed_size EQUAL anchor_size)
        message(FATAL_ERROR "Minish Android renderer anchor changed; review the gbarecomp pin")
    endif()
    string(REPLACE "${before}" "${after}" replaced "${${variable}}")
    set(${variable} "${replaced}" PARENT_SCOPE)
endfunction()

set(minish_window "${GBARECOMP_ROOT}/src/runtime/host_window.cpp")
set(minish_runtime "${GBARECOMP_ROOT}/src/runtime/runtime.cpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${minish_window}" "${minish_runtime}")
file(READ "${minish_window}" window_source)
minish_replace_once(window_source "#include \"host_window.h\""
    "#include \"host_window.h\"\n#include \"pixel_presenter.h\"")
minish_replace_once(window_source "    SDL_Texture*  texture  = nullptr;"
    "    SDL_Texture*  texture  = nullptr;\n    minish::PixelPresenter pixel_presenter;")
minish_replace_once(window_source
    "    if (b->resize_driven_view || b->expanded_view || b->touch_policy) {"
    [=[    if (b->touch_policy && !b->resize_driven_view && !b->expanded_view) {
        const auto pixels = minish::pixel_layout(drawable_w, drawable_h,
            b->base_w, b->base_h, minish::pixel_mode());
        layout = {pixels.x, pixels.y, pixels.width, pixels.height, pixels.integer_scale};
    } else if (b->resize_driven_view || b->expanded_view || b->touch_policy) {]=])
minish_replace_once(window_source
    "    SDL_RenderCopy(b->renderer, b->texture, nullptr, &destination);\n#else"
    "    b->pixel_presenter.present(b->renderer, b->texture, rgb888, texture_w, texture_h, destination);\n#else")
minish_replace_once(window_source "    if (b->texture)   SDL_DestroyTexture(b->texture);"
    "    b->pixel_presenter.close();\n    if (b->texture)   SDL_DestroyTexture(b->texture);")

# Replace the generic blur switch with the game's live, persistent pixel modes.
file(READ "${minish_runtime}" runtime_source)
minish_replace_once(runtime_source "            RECOMP_RUNTIME_UI_STANDARD_LINEAR_FILTER |\n" "")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/minish_renderer")
foreach(name window runtime)
    set(staged "${CMAKE_CURRENT_BINARY_DIR}/minish_renderer/${name}.cpp")
    # configure_file(COPYONLY) preserves mtime when content is unchanged.
    file(WRITE "${staged}.tmp" "${${name}_source}")
    configure_file("${staged}.tmp" "${staged}" COPYONLY)
endforeach()
get_target_property(minish_runtime_sources gbarecomp_runtime SOURCES)
list(REMOVE_ITEM minish_runtime_sources src/runtime/host_window.cpp src/runtime/runtime.cpp
    "${minish_window}" "${minish_runtime}")
set_property(TARGET gbarecomp_runtime PROPERTY SOURCES "${minish_runtime_sources}")
target_sources(gbarecomp_runtime PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/minish_renderer/window.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/minish_renderer/runtime.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/pixel_presenter.cpp")
target_include_directories(gbarecomp_runtime PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
