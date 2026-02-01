add_executable(ShaderBeam
    CaptureBase.cpp
    CaptureFile.cpp
    Charts.cpp
    Common.cpp
    Helpers.cpp
    Renderer.cpp
    RenderThread.cpp
    ShaderBeam.cpp
    ShaderManager.cpp
    ShaderProfile.cpp
    SinglePassShaderProfile.cpp
    Watcher.cpp
    UI.cpp
    Window.cpp
)

target_compile_definitions(ShaderBeam PRIVATE GLFW_INCLUDE_NONE=1)

target_include_directories(ShaderBeam PUBLIC ../)
target_include_directories(ShaderBeam PUBLIC ./)
target_include_directories(ShaderBeam PUBLIC ../ImGui)
target_include_directories(ShaderBeam PUBLIC ../GLFW)
target_include_directories(ShaderBeam PUBLIC ../DXVK/include/dxvk)

target_link_directories(ShaderBeam PUBLIC ../GLFW)
target_link_directories(ShaderBeam PUBLIC ../DXVK/lib)
target_link_libraries(ShaderBeam PRIVATE ImGui)
target_link_libraries(ShaderBeam PRIVATE glfw)
target_link_libraries(ShaderBeam PRIVATE dxvk_dxgi dxvk_d3d11)
