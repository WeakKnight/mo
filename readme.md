# Mo  an interactive rendering engine

It is still at an early development stage!

![Image](https://github.com/WeakKnight/mo/blob/master/screenshots/enginescreenshot01.png?raw=true)
![Image](https://github.com/WeakKnight/mo/blob/master/screenshots/enginescreenshot02.png?raw=true)

## Features
1. Deferred-Forward Mixed Shading Pipeline
2. Use Command Buffer To Manage Rendering Process
3. Screen Space Reflection(Early Stage, WIP)
4. Post Processing(Tonemapping And Vignette Only, Early Stage, WIP) 
5. Data Driven Design
6. Shadow Mapping
7. Editor(WIP)

## Build Instruction
### macOS
1. mkdir build
2. cd build
3. mkdir macos
4. cd macos
5. cmake -G  "Xcode" ../../

### Windows
1. mkdir build
2. cd build
3. mkdir windows
4. cd windows
5. cmake -G "Visual Studio 16 2019" ../../

## Thirdparty Library

Library                                     | Functionality         
------------------------------------------  | -------------
[assimp](https://github.com/assimp/assimp)  | Mesh Loading And Pre Processing
[glfw](https://github.com/glfw/glfw)        | Windowing And Input Handling
[glm](https://github.com/g-truc/glm)        | Mathematics
[imgui](https://github.com/ocornut/imgui)    | GUI
[ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)    | Guizmo
[spdlog](https://github.com/gabime/spdlog)   | Debug Logging
[glad](https://github.com/Dav1dde/glad)   | OpenGL Loader



