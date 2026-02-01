add_library(ImGui
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui.cpp
    backends/imgui_impl_dx11.cpp
    backends/imgui_impl_glfw.cpp
)

target_compile_definitions(ImGui PRIVATE GLFW_INCLUDE_NONE=1) 

target_include_directories(ImGui PUBLIC ./)
target_include_directories(ImGui PUBLIC ../DXVK/include/dxvk)
target_include_directories(ImGui PUBLIC ../)
