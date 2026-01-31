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

target_include_directories(ShaderBeam PUBLIC /home/user/Projects/ShaderBeam)
target_include_directories(ShaderBeam PUBLIC /home/user/Projects/ShaderBeam/ShaderBeam)
target_include_directories(ShaderBeam PUBLIC /home/user/Projects/ShaderBeam/ImGui)
target_include_directories(ShaderBeam PUBLIC /home/user/Projects/ShaderBeam/GLFW)
target_include_directories(ShaderBeam PUBLIC /home/user/Projects/ShaderBeam/DXVK/include)

target_link_directories(ShaderBeam PUBLIC /home/user/Projects/ShaderBeam/GLFW)
target_link_directories(ShaderBeam PUBLIC /home/user/Projects/ShaderBeam/DXVK/lib)
target_link_libraries(ShaderBeam PRIVATE ImGui)
target_link_libraries(ShaderBeam PRIVATE glfw3)
target_link_libraries(ShaderBeam PRIVATE dxvk_dxgi dxvk_d3d11)
