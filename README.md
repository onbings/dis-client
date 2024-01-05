#COMPIL EMSCRIPTEN VCPKG WASM
#md dis-client-web
#cd dis-client-web
#emcmake cmake -DCMAKE_BUILD_TYPE=Debug -DHELLOIMGUI_WITH_SDL=ON -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=C:/pro/emsdk/upstream/emscripten/cmake/Modules/Platform/emscripten.cmake -DCMAKE_TOOLCHAIN_FILE=C:/pro/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=wasm32-emscripten -DBUILD_SHARED_LIBS=OFF -B C:/bld/dis-client-web -S C:/pro/github/dis-client
#cmake --build . --target help
#cmake --build . --target app
#emrun --browser chrome .
#
#COMPIL WINDOWS VCPKG 
#md dis-client
#cd dis-client
#cmake -DHELLOIMGUI_WITH_SDL=ON -DCMAKE_TOOLCHAIN_FILE=C:/pro/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DBUILD_SHARED_LIBS=OFF -B C:/bld/dis-client -S C:/pro/github/dis-client
bofstd:
compile with emscripten: https://thatonegamedev.com/cpp/programming-a-c-game-for-the-web-emscripten/
#emcmake cmake -S C:/pro/github/bofstd -D__EMSCRIPTEN__=1 -B C:/bld/bofstd-web -DCMAKE_TOOLCHAIN_FILE=C:/pro/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=wasm32-emscripten



https://floooh.github.io/2023/11/11/emscripten-ide.html
install ninja in path: https://github.com/ninja-build/ninja/releases
install emsdk in c:/pro/emsdk: : https://emscripten.org/docs/getting_started/downloads.html
add C:\pro\emsdk\upstream\bin and C:\pro\emsdk\upstream\emscripten to path
vscode kit should be Clang 18.0.0 git x86_64-pc-windows-msvc
cmake 3.25.1 at least but prefer 3.28.1
install ms-vscode.wasm-dwarf-debugging in vscode
install C/C++ DevTools Support (DWARF) in chrome
cmake configure for emscripten is configure: cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=C:/pro/emsdk/upstream/emscripten/cmake/Modules/Platform/emscripten.cmake -DCMAKE_TOOLCHAIN_FILE=C:/pro/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=wasm32-emscripten -DBUILD_SHARED_LIBS=OFF -DCMAKE_GENERATOR=Ninja -DCMAKE_BUILD_TYPE=Debug -SC:/pro/github/dis-client -BC:/bld/dis-client-web -DCMAKE_CROSSCOMPILING_EMULATOR=C:/pro/emsdk/node/16.20.0_64bit/bin/node.exe -G Ninja   

PS C:\bld\imgui_bundle\bin> emrun --list
emrun has automatically found the following browsers in the default install locations on the system:

  - firefox: Mozilla Firefox
  - chrome: Google Chrome
  - iexplore: Microsoft Internet Explorer

You can pass the --browser <id> option to launch with the given browser above.
Even if your browser was not detected, you can use --browser /path/to/browser/executable to launch with that browser.
PS C:\bld\imgui_bundle\bin> emrun --browser chrome .
Now listening at http://0.0.0.0:6931/

=>Connect on 127.0.0.1:6931


# Step 5: run a web server
python3 -m http.server

# Step 6: Open a browser and open the url
# http://localhost:8000/hello.html


DOC:
https://www.youtube.com/watch?v=dArP4lBnOr8&ab_channel=CodeBallads
https://github.com/pthom/hello_imgui_my_app
https://github.com/pthom/hello_imgui_template
https://thatonegamedev.com/cpp/debugging-c-compiled-to-webassembly/  in chrome
https://developer.chrome.com/blog/wasm-debugging-2020  in chrome
https://floooh.github.io/2023/11/11/emscripten-ide.html in vscode
https://code.visualstudio.com/blogs/2023/06/05/vscode-wasm-wasi
https://decovar.dev/blog/2023/11/20/webassembly-with-pthreads/ pthread
https://emscripten.org/docs/porting/pthreads.html#blocking-on-the-main-browser-thread 

How to quickly create a truly multi-platform application with Dear ImGui and Hello ImGui (for Windows, Linux, macOS, iOS, and as a web application), based on an example where we develop a simple RPN Calculator.

Links:
=====

- RPN Calculator repository: https://github.com/pthom/rpn_calculator

- Dear ImGui: https://github.com/ocornut/imgui
  (bloat-free Graphical User interface for C++ with minimal dependencies )
- ImGui Manual: https://pthom.github.io/imgui_manual/ ...
  (an interactive manual for Dear ImGui, where the source code for any of the demo is displayed whenever you look at it)
- Emcripten: https://emscripten.org/
  (a complete compiler toolchain to WebAssembly)

- Hello ImGui: https://github.com/pthom/hello_imgui
  (Cross-platform apps for Windows, Mac, Linux, iOS, Emscripten with the simplicity of a "Hello World" app)
- Hello ImGui template repository: https://github.com/pthom/hello_imgui_...

- Dear ImGui Bundle: https://pthom.github.io/imgui_bundle/
  (easily create ImGui applications in Python and C++. Batteries included!)
- Dear ImGui template repository: https://github.com/pthom/imgui_bundle...


00:00 Intro
00:54 About Hello ImGui
01:35 Using Hello ImGui template
02:13 Add assets and change the app icon
04:00 Try it on your side 
04:07 Build and deploy on Windows
04:41 Build under Linux and macOS
04:54 Build as a Web Application
05:54 Build for iOS
06:43 Some info about the source code

Note: the interface of the RPN calculator tries to be a tribute to the ancient "HP 28S" calculator ( https://en.wikipedia.org/wiki/HP-28_s... ), and its descendants.


C:\Users\User>cd \bld
C:\bld>md rpn_calculator
C:\bld>cd rpn_calculator

--- SDL VERSION JUST BELOW --- (C:\bld\rpn_calculator>cmake -DHELLOIMGUI_WITH_SDL=ON -B . -S C:/pro/github/rpn_calculator)

C:\bld\rpn_calculator>cmake -B . -S C:/pro/github/rpn_calculator
-- Building for: Visual Studio 17 2022
-- Selecting Windows SDK version 10.0.22000.0 to target Windows 10.0.22621.
-- The C compiler identification is MSVC 19.35.32215.0
-- The CXX compiler identification is MSVC 19.35.32215.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.35.32215/bin/Hostx64/x64/cl.exe - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.35.32215/bin/Hostx64/x64/cl.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Populating hello_imgui
-- Selecting Windows SDK version 10.0.22000.0 to target Windows 10.0.22621.
-- Configuring done
-- Generating done
-- Build files have been written to: C:/bld/rpn_calculator/_deps/hello_imgui-subbuild
MSBuild version 17.5.0+6f08c67f3 for .NET Framework

  Checking Build System
  Creating directories for 'hello_imgui-populate'
  Building Custom Rule C:/bld/rpn_calculator/_deps/hello_imgui-subbuild/CMakeLists.txt
  Performing download step (git clone) for 'hello_imgui-populate'
  Cloning into 'hello_imgui-src'...
  Already on 'master'
  Your branch is up to date with 'origin/master'.
  Submodule '_example_integration' (https://github.com/pthom/hello_imgui_template.git) registered for path '_example_integration'
  Submodule 'external/imgui' (https://github.com/ocornut/imgui.git) registered for path 'external/imgui'
  Submodule 'external/ios-cmake' (https://github.com/leetal/ios-cmake.git) registered for path 'hello_imgui_cmake/ios-cmake'
  Cloning into 'C:/bld/rpn_calculator/_deps/hello_imgui-src/_example_integration'...
  Cloning into 'C:/bld/rpn_calculator/_deps/hello_imgui-src/external/imgui'...
  Cloning into 'C:/bld/rpn_calculator/_deps/hello_imgui-src/hello_imgui_cmake/ios-cmake'...
  Submodule path '_example_integration': checked out '004e7adb1bc8a2710857430cb13a34ec34f8af35'
  Submodule path 'external/imgui': checked out 'ce0d0ac8298ce164b5d862577e8b087d92f6e90e'
  Submodule path 'hello_imgui_cmake/ios-cmake': checked out '06465b27698424cf4a04a5ca4904d50a3c966c45'
  Performing update step for 'hello_imgui-populate'
  No patch step for 'hello_imgui-populate'
  No configure step for 'hello_imgui-populate'
  No build step for 'hello_imgui-populate'
  No install step for 'hello_imgui-populate'
  No test step for 'hello_imgui-populate'
  Completed 'hello_imgui-populate'
  Building Custom Rule C:/bld/rpn_calculator/_deps/hello_imgui-subbuild/CMakeLists.txt
-- HELLOIMGUI_WITH_TEST_ENGINE=OFF
--
                HelloImGui: using Glfw as default default backend (it will be downloaded and built inside {CMAKE_CURRENT_BINARY_DIR}/_deps/glfw-*)

                    In order to select your own backend, use one of the cmake options below:
                    -DHELLOIMGUI_WITH_GLFW=ON             # To download and build glfw automatically
                    -DHELLOIMGUI_WITH_SDL=ON              # To download and build SDL automatically
                    -DHELLOIMGUI_USE_GLFW_OPENGL3=ON      # To use your own version of GLFW (it should be findable via find_package(glfw3))
                    -DHELLOIMGUI_USE_SDL_OPENGL3=ON       # To use your own version of SDL (it should be findable via find_package(SDL2))

                    (Note: under Linux, it is advised to use system-wide libraries, and not to use
                    -DHELLOIMGUI_WITH_GLFW=ON or -DHELLOIMGUI_WITH_SDL=ON)

-- HelloImGui: downloading and building glfw
-- Library hello_imgui
-- Populating glfw
-- Selecting Windows SDK version 10.0.22000.0 to target Windows 10.0.22621.
-- Configuring done
-- Generating done
-- Build files have been written to: C:/bld/rpn_calculator/_deps/glfw-subbuild
MSBuild version 17.5.0+6f08c67f3 for .NET Framework

  Checking Build System
  Creating directories for 'glfw-populate'
  Building Custom Rule C:/bld/rpn_calculator/_deps/glfw-subbuild/CMakeLists.txt
  Performing download step (git clone) for 'glfw-populate'
  Cloning into 'glfw-src'...
  remote: Enumerating objects: 31288, done.
  remote: Counting objects:   0% (1/401)
  remote: Counting objects:   1% (5/401)
  remote: Counting objects:   2% (9/401)
...
  remote: Counting objects: 100% (401/401), done.
  remote: Compressing objects:   0% (1/166)
  remote: Compressing objects:   1% (2/166)
  remote: Compressing objects:   2% (4/166)
...
  remote: Compressing objects: 100% (166/166), done.
  Receiving objects:   0% (1/31288)
  Receiving objects:   1% (313/31288)
  Receiving objects:   2% (626/31288)
...
  Receiving objects: 100% (31288/31288), 16.02 MiB | 9.75 MiB/s, done.
  Resolving deltas:   0% (0/22088)
  Resolving deltas:   1% (221/22088)
  Resolving deltas:   2% (446/22088)
...
  Resolving deltas: 100% (22088/22088), done.
  HEAD is now at 7482de60 Documentation work
  Performing update step for 'glfw-populate'
  No patch step for 'glfw-populate'
  No configure step for 'glfw-populate'
  No build step for 'glfw-populate'
  No install step for 'glfw-populate'
  No test step for 'glfw-populate'
  Completed 'glfw-populate'
  Building Custom Rule C:/bld/rpn_calculator/_deps/glfw-subbuild/CMakeLists.txt
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD
-- Performing Test CMAKE_HAVE_LIBC_PTHREAD - Failed
-- Looking for pthread_create in pthreads
-- Looking for pthread_create in pthreads - not found
-- Looking for pthread_create in pthread
-- Looking for pthread_create in pthread - not found
-- Found Threads: TRUE
-- Using Win32 for window creation
-- Configuring done
-- Generating done
-- Build files have been written to: C:/bld/rpn_calculator

C:\bld\rpn_calculator>

--- SDL VERSION ------------------------------------


C:\bld\rpn_calculator>cmake -DHELLOIMGUI_WITH_SDL=ON -B . -S C:/pro/github/rpn_calculator
-- Building for: Visual Studio 17 2022
-- Selecting Windows SDK version 10.0.22000.0 to target Windows 10.0.22621.
-- The C compiler identification is MSVC 19.35.32215.0
-- The CXX compiler identification is MSVC 19.35.32215.0
-- Detecting C compiler ABI info
-- Detecting C compiler ABI info - done
-- Check for working C compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.35.32215/bin/Hostx64/x64/cl.exe - skipped
-- Detecting C compile features
-- Detecting C compile features - done
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.35.32215/bin/Hostx64/x64/cl.exe - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Populating hello_imgui
-- Selecting Windows SDK version 10.0.22000.0 to target Windows 10.0.22621.
-- Configuring done
-- Generating done
-- Build files have been written to: C:/bld/rpn_calculator/_deps/hello_imgui-subbuild
MSBuild version 17.5.0+6f08c67f3 for .NET Framework

  Checking Build System
  Creating directories for 'hello_imgui-populate'
  Building Custom Rule C:/bld/rpn_calculator/_deps/hello_imgui-subbuild/CMakeLists.txt
  Performing download step (git clone) for 'hello_imgui-populate'
  Cloning into 'hello_imgui-src'...
  Already on 'master'
  Your branch is up to date with 'origin/master'.
  Submodule '_example_integration' (https://github.com/pthom/hello_imgui_template.git) registered for path '_example_integration'
  Submodule 'external/imgui' (https://github.com/ocornut/imgui.git) registered for path 'external/imgui'
  Submodule 'external/ios-cmake' (https://github.com/leetal/ios-cmake.git) registered for path 'hello_imgui_cmake/ios-cmake'
  Cloning into 'C:/bld/rpn_calculator/_deps/hello_imgui-src/_example_integration'...
  Cloning into 'C:/bld/rpn_calculator/_deps/hello_imgui-src/external/imgui'...
  Cloning into 'C:/bld/rpn_calculator/_deps/hello_imgui-src/hello_imgui_cmake/ios-cmake'...
  Submodule path '_example_integration': checked out '004e7adb1bc8a2710857430cb13a34ec34f8af35'
  Submodule path 'external/imgui': checked out 'ce0d0ac8298ce164b5d862577e8b087d92f6e90e'
  Submodule path 'hello_imgui_cmake/ios-cmake': checked out '06465b27698424cf4a04a5ca4904d50a3c966c45'
  Performing update step for 'hello_imgui-populate'
  No patch step for 'hello_imgui-populate'
  No configure step for 'hello_imgui-populate'
  No build step for 'hello_imgui-populate'
  No install step for 'hello_imgui-populate'
  No test step for 'hello_imgui-populate'
  Completed 'hello_imgui-populate'
  Building Custom Rule C:/bld/rpn_calculator/_deps/hello_imgui-subbuild/CMakeLists.txt
-- HELLOIMGUI_WITH_TEST_ENGINE=OFF
-- HelloImGui: downloading and building SDL
-- Library hello_imgui
-- Populating sdl
-- Selecting Windows SDK version 10.0.22000.0 to target Windows 10.0.22621.
-- Configuring done
-- Generating done
-- Build files have been written to: C:/bld/rpn_calculator/_deps/sdl-subbuild
MSBuild version 17.5.0+6f08c67f3 for .NET Framework

  Checking Build System
  Creating directories for 'sdl-populate'
  Building Custom Rule C:/bld/rpn_calculator/_deps/sdl-subbuild/CMakeLists.txt
  Performing download step (git clone) for 'sdl-populate'
  Cloning into 'sdl-src'...
  remote: Enumerating objects: 137428, done.
  remote: Counting objects:   0% (1/368)
  remote: Counting objects:   1% (4/368)
  remote: Counting objects:   2% (8/368)
...
  remote: Counting objects: 100% (368/368), done.
  remote: Compressing objects:   0% (1/201)
  remote: Compressing objects:   1% (3/201)
  remote: Compressing objects:   2% (5/201)
...
  remote: Compressing objects: 100% (201/201), done.
  Receiving objects:   0% (1/137428)
  Receiving objects:   1% (1375/137428)
  Receiving objects:   2% (2749/137428)
...
  Receiving objects: 100% (137428/137428), 109.55 MiB | 10.00 MiB/s, done.
  Resolving deltas:   0% (0/105913)
  Resolving deltas:   1% (1060/105913)
  Resolving deltas:   2% (2120/105913)
...
  Resolving deltas: 100% (105913/105913), done.
  HEAD is now at 55b03c749 Updated to version 2.24.2 for point release
  Performing update step for 'sdl-populate'
  No patch step for 'sdl-populate'
  No configure step for 'sdl-populate'
  No build step for 'sdl-populate'
  No install step for 'sdl-populate'
  No test step for 'sdl-populate'
  Completed 'sdl-populate'
  Building Custom Rule C:/bld/rpn_calculator/_deps/sdl-subbuild/CMakeLists.txt
-- Could NOT find PkgConfig (missing: PKG_CONFIG_EXECUTABLE)
-- Looking for __GLIBC__
-- Looking for __GLIBC__ - not found
-- Looking for immintrin.h
-- Looking for immintrin.h - found
-- Performing Test HAVE_WIN32_CC
-- Performing Test HAVE_WIN32_CC - Success
-- Looking for d3d9.h
-- Looking for d3d9.h - found
-- Looking for d3d11_1.h
-- Looking for d3d11_1.h - found
-- Performing Test HAVE_D3D12_H
-- Performing Test HAVE_D3D12_H - Success
-- Looking for ddraw.h
-- Looking for ddraw.h - found
-- Looking for dsound.h
-- Looking for dsound.h - found
-- Looking for dinput.h
-- Looking for dinput.h - found
-- Looking for dxgi.h
-- Looking for dxgi.h - found
-- Performing Test HAVE_XINPUT_H
-- Performing Test HAVE_XINPUT_H - Success
-- Performing Test HAVE_XINPUT_GAMEPAD_EX
-- Performing Test HAVE_XINPUT_GAMEPAD_EX - Failed
-- Performing Test HAVE_XINPUT_STATE_EX
-- Performing Test HAVE_XINPUT_STATE_EX - Failed
-- Performing Test HAVE_WINDOWS_GAMING_INPUT_H
-- Performing Test HAVE_WINDOWS_GAMING_INPUT_H - Success
-- Looking for tpcshrd.h
-- Looking for tpcshrd.h - found
-- Looking for roapi.h
-- Looking for roapi.h - found
-- Looking for mmdeviceapi.h
-- Looking for mmdeviceapi.h - found
-- Looking for audioclient.h
-- Looking for audioclient.h - found
-- Looking for sensorsapi.h
-- Looking for sensorsapi.h - found
-- Looking for shellscalingapi.h
-- Looking for shellscalingapi.h - found
-- Found Git: C:/Program Files/Git/cmd/git.exe (found version "2.39.1.windows.1")
--
-- SDL2 was configured with the following options:
--
-- Platform: Windows-10.0.22621
-- 64-bit:   TRUE
-- Compiler: C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.35.32215/bin/Hostx64/x64/cl.exe
-- Revision: https://github.com/pthom/rpn_calculator@21011551987b42cb8910c1a0a54fd4de43f7cd34
--
-- Subsystems:
--   Atomic:    ON
--   Audio:     ON
--   Video:     ON
--   Render:    ON
--   Events:    ON
--   Joystick:  ON
--   Haptic:    ON
--   Hidapi:    ON
--   Power:     ON
--   Threads:   ON
--   Timers:    ON
--   File:      ON
--   Loadso:    ON
--   CPUinfo:   ON
--   Filesystem:        ON
--   Sensor:    ON
--   Locale:    ON
--   Misc:      ON
--
-- Options:
--   SDL2_DISABLE_INSTALL        (Wanted: ON): OFF
--   SDL2_DISABLE_SDL2MAIN       (Wanted: OFF): OFF
--   SDL2_DISABLE_UNINSTALL      (Wanted: OFF): OFF
--   SDL_3DNOW                   (Wanted: ON): OFF
--   SDL_ALSA                    (Wanted: OFF): OFF
--   SDL_ALSA_SHARED             (Wanted: OFF): OFF
--   SDL_ALTIVEC                 (Wanted: ON): OFF
--   SDL_ARMNEON                 (Wanted: OFF): OFF
--   SDL_ARMSIMD                 (Wanted: OFF): OFF
--   SDL_ARTS                    (Wanted: OFF): OFF
--   SDL_ARTS_SHARED             (Wanted: OFF): OFF
--   SDL_ASAN                    (Wanted: OFF): OFF
--   SDL_ASSEMBLY                (Wanted: ON): OFF
--   SDL_ASSERTIONS              (Wanted: auto): auto
--   SDL_BACKGROUNDING_SIGNAL    (Wanted: OFF): OFF
--   SDL_CLOCK_GETTIME           (Wanted: OFF): OFF
--   SDL_COCOA                   (Wanted: OFF): OFF
--   SDL_DIRECTFB                (Wanted: OFF): OFF
--   SDL_DIRECTFB_SHARED         (Wanted: OFF): OFF
--   SDL_DIRECTX                 (Wanted: ON): ON
--   SDL_DISKAUDIO               (Wanted: ON): ON
--   SDL_DUMMYAUDIO              (Wanted: ON): ON
--   SDL_DUMMYVIDEO              (Wanted: ON): ON
--   SDL_ESD                     (Wanted: OFF): OFF
--   SDL_ESD_SHARED              (Wanted: OFF): OFF
--   SDL_FOREGROUNDING_SIGNAL    (Wanted: OFF): OFF
--   SDL_FUSIONSOUND             (Wanted: OFF): OFF
--   SDL_FUSIONSOUND_SHARED      (Wanted: OFF): OFF
--   SDL_GCC_ATOMICS             (Wanted: OFF): OFF
--   SDL_HIDAPI                  (Wanted: ON): ON
--   SDL_HIDAPI_JOYSTICK         (Wanted: ON): ON
--   SDL_HIDAPI_LIBUSB           (Wanted: OFF): OFF
--   SDL_INSTALL_TESTS           (Wanted: OFF): OFF
--   SDL_JACK                    (Wanted: OFF): OFF
--   SDL_JACK_SHARED             (Wanted: OFF): OFF
--   SDL_KMSDRM                  (Wanted: OFF): OFF
--   SDL_KMSDRM_SHARED           (Wanted: OFF): OFF
--   SDL_LIBC                    (Wanted: OFF): OFF
--   SDL_LIBSAMPLERATE           (Wanted: OFF): OFF
--   SDL_LIBSAMPLERATE_SHARED    (Wanted: OFF): OFF
--   SDL_METAL                   (Wanted: OFF): OFF
--   SDL_MMX                     (Wanted: ON): OFF
--   SDL_NAS                     (Wanted: OFF): OFF
--   SDL_NAS_SHARED              (Wanted: OFF): OFF
--   SDL_OFFSCREEN               (Wanted: OFF): OFF
--   SDL_OPENGL                  (Wanted: ON): ON
--   SDL_OPENGLES                (Wanted: ON): ON
--   SDL_OSS                     (Wanted: OFF): OFF
--   SDL_PIPEWIRE                (Wanted: OFF): OFF
--   SDL_PIPEWIRE_SHARED         (Wanted: OFF): OFF
--   SDL_PTHREADS                (Wanted: OFF): OFF
--   SDL_PTHREADS_SEM            (Wanted: OFF): OFF
--   SDL_PULSEAUDIO              (Wanted: OFF): OFF
--   SDL_PULSEAUDIO_SHARED       (Wanted: OFF): OFF
--   SDL_RENDER_D3D              (Wanted: ON): ON
--   SDL_RENDER_METAL            (Wanted: OFF): OFF
--   SDL_RPATH                   (Wanted: OFF): OFF
--   SDL_RPI                     (Wanted: OFF): OFF
--   SDL_SNDIO                   (Wanted: OFF): OFF
--   SDL_SNDIO_SHARED            (Wanted: OFF): OFF
--   SDL_SSE                     (Wanted: ON): ON
--   SDL_SSE2                    (Wanted: ON): ON
--   SDL_SSE3                    (Wanted: ON): ON
--   SDL_SSEMATH                 (Wanted: ON): OFF
--   SDL_STATIC_PIC              (Wanted: OFF): OFF
--   SDL_TESTS                   (Wanted: OFF): OFF
--   SDL_VIRTUAL_JOYSTICK        (Wanted: ON): ON
--   SDL_VIVANTE                 (Wanted: OFF): OFF
--   SDL_VULKAN                  (Wanted: ON): ON
--   SDL_WASAPI                  (Wanted: ON): ON
--   SDL_WAYLAND                 (Wanted: OFF): OFF
--   SDL_WAYLAND_LIBDECOR        (Wanted: OFF): OFF
--   SDL_WAYLAND_LIBDECOR_SHARED (Wanted: OFF): OFF
--   SDL_WAYLAND_QT_TOUCH        (Wanted: OFF): OFF
--   SDL_WAYLAND_SHARED          (Wanted: OFF): OFF
--   SDL_X11                     (Wanted: OFF): OFF
--   SDL_X11_SHARED              (Wanted: OFF): OFF
--   SDL_X11_XCURSOR             (Wanted: OFF): OFF
--   SDL_X11_XDBE                (Wanted: OFF): OFF
--   SDL_X11_XFIXES              (Wanted: OFF): OFF
--   SDL_X11_XINPUT              (Wanted: OFF): OFF
--   SDL_X11_XRANDR              (Wanted: OFF): OFF
--   SDL_X11_XSCRNSAVER          (Wanted: OFF): OFF
--   SDL_X11_XSHAPE              (Wanted: OFF): OFF
--   SDL_XINPUT                  (Wanted: ON): ON
--
--  CFLAGS:        /DWIN32 /D_WINDOWS /W3
--  EXTRA_CFLAGS:
--  EXTRA_LDFLAGS:
--  EXTRA_LIBS:    user32;gdi32;winmm;imm32;ole32;oleaut32;version;uuid;advapi32;setupapi;shell32;dinput8
--
--  Build Shared Library: ON
--  Build Static Library: ON
--  Build Static Library with Position Independent Code: OFF
--
-- Configuring done
-- Generating done
-- Build files have been written to: C:/bld/rpn_calculator

C:\bld\rpn_calculator>cmake --build .
C:\bld\rpn_calculator>Debug\rpc_calculator.exe




--- SDL VERSION + OPENGL3 ------------------------------------


C:\bld\rpn_calculator>cmake -DHELLOIMGUI_USE_SDL_OPENGL3=ON -B . -S C:/pro/github/rpn_calculator
SDL2 must be found by find_package





--- EMSCRIPTEN ------------------------------------

emcmake cmake -DHELLOIMGUI_WITH_SDL=ON -B . -S C:/pro/github/rpn_calculator

C:\bld>md rpn_calculator_web
C:\bld>cd rpn_calculator_web
C:\bld\rpn_calculator_web>emcmake cmake -DHELLOIMGUI_WITH_SDL=ON -B . -S C:/pro/github/rpn_calculator
configure: cmake -DHELLOIMGUI_WITH_SDL=ON -B . -S C:/pro/github/rpn_calculator -DCMAKE_TOOLCHAIN_FILE=C:\pro\emsdk\upstream\emscripten\cmake\Modules\Platform\Emscripten.cmake -DCMAKE_CROSSCOMPILING_EMULATOR=C:/pro/emsdk/node/16.20.0_64bit/bin/node.exe -G Ninja
-- Populating hello_imgui
-- Configuring done
-- Generating done
-- Build files have been written to: C:/bld/rpn_calculator_web/_deps/hello_imgui-subbuild
Cloning into 'hello_imgui-src'...
Your branch is up to date with 'origin/master'.
Already on 'master'
Submodule '_example_integration' (https://github.com/pthom/hello_imgui_template.git) registered for path '_example_integration'
Submodule 'external/imgui' (https://github.com/ocornut/imgui.git) registered for path 'external/imgui'
Submodule 'external/ios-cmake' (https://github.com/leetal/ios-cmake.git) registered for path 'hello_imgui_cmake/ios-cmake'
Cloning into 'C:/bld/rpn_calculator_web/_deps/hello_imgui-src/_example_integration'...
Cloning into 'C:/bld/rpn_calculator_web/_deps/hello_imgui-src/external/imgui'...
Cloning into 'C:/bld/rpn_calculator_web/_deps/hello_imgui-src/hello_imgui_cmake/ios-cmake'...
Submodule path '_example_integration': checked out '004e7adb1bc8a2710857430cb13a34ec34f8af35'
Submodule path 'external/imgui': checked out 'ce0d0ac8298ce164b5d862577e8b087d92f6e90e'
Submodule path 'hello_imgui_cmake/ios-cmake': checked out '06465b27698424cf4a04a5ca4904d50a3c966c45'
[1/9] Creating directories for 'hello_imgui-populate'
[1/9] Performing download step (git clone) for 'hello_imgui-populate'
[2/9] Performing update step for 'hello_imgui-populate'
[3/9] No patch step for 'hello_imgui-populate'
[5/9] No configure step for 'hello_imgui-populate'
[6/9] No build step for 'hello_imgui-populate'
[7/9] No install step for 'hello_imgui-populate'
[8/9] No test step for 'hello_imgui-populate'
[9/9] Completed 'hello_imgui-populate'
-- HELLOIMGUI_WITH_TEST_ENGINE=OFF
-- Setting build type to 'RelWithDebInfo' as none was specified.
-- Library hello_imgui
-- Configuring done
-- Generating done
-- Build files have been written to: C:/bld/rpn_calculator_web

C:\bld\rpn_calculator_web>cmake --build .
[49/53] Building CXX object CMakeFiles/rpn_calculator.dir/rpn_calculator.cpp.o
cache:INFO: generating port: sysroot\lib\wasm32-emscripten\libSDL2.a... (this will be cached in "C:\pro\emsdk\upstream\emscripten\cache\sysroot\lib\wasm32-emscripten\libSDL2.a" for subsequent builds)
system_libs:INFO: compiled 115 inputs in 3.60s
cache:INFO:  - ok
[53/53] Linking CXX executable rpn_calculator.html
cache:INFO: generating system asset: symbol_lists/132641f9461739e1b09c47ba7bccf375e0d3d240.json... (this will be cached in "C:\pro\emsdk\upstream\emscripten\cache\symbol_lists\132641f9461739e1b09c47ba7bccf375e0d3d240.json" for subsequent builds)
cache:INFO:  - ok
em++: warning: running limited binaryen optimizations because DWARF info requested (or indirectly required) [-Wlimited-postlink-optimizations]

C:\bld\rpn_calculator_web>dir
23/12/2023  12:49           213,032 .ninja_deps
23/12/2023  12:49             7,570 .ninja_log
23/12/2023  12:48            84,365 build.ninja
23/12/2023  12:48            17,045 CMakeCache.txt
23/12/2023  12:48    <DIR>          CMakeFiles
23/12/2023  12:48             1,794 cmake_install.cmake
23/12/2023  12:48            38,004 compile_commands.json
23/12/2023  12:49         1,134,432 rpn_calculator.data
23/12/2023  12:49             2,863 rpn_calculator.html
23/12/2023  12:49           430,863 rpn_calculator.js
23/12/2023  12:49        14,264,105 rpn_calculator.wasm
23/12/2023  12:48            97,347 rpn_calculator_favicon.png

C:\bld\rpn_calculator_web>emrun --browser chrome .
Now listening at http://0.0.0.0:6931/


hello_imgui_template multiplatform

# hello_imgui_template: get started with HelloImGui in 5 minutes 

This template demonstrates how to easily integrate HelloImGui to your own project. 

You can use it as a starting point for your own project.

Template repository: https://github.com/pthom/hello_imgui_template

## Explanations

### CMake

The [CMakeLists.txt](CMakeLists.txt) file will download and build hello_imgui at configure time, and make the "hello_imgui_add_app" cmake function available. 

By default, you do not need to add HelloImGui as a dependency to your project, it will be downloaded and built automatically during CMake configure time.
If you wish to use a local copy of HelloImGui, edit CMakeLists.txt and uncomment the `add_subdirectory` line.

*Note: `hello_imgui_add_app` will automatically link your app to hello_imgui, embed the assets folder (for desktop, mobile, and emscripten apps), and the application icon.*

### Assets
 

Anything in the assets/ folder located beside the app's CMakeLists will be bundled together with the app (for macOS, iOS, Android, Emscripten).
The files in assets/app_settings/ will be used to generate the app icon, and the app settings.

```
assets/
├── world.jpg                   # A custom asset
│
├── app_settings/               # Application settings
│    ├── icon.png               # This will be the app icon, it should be square
│    │                          # and at least 256x256. It will  be converted
│    │                          # to the right format, for each platform (except Android)
│    ├── apple/
│    │    │── Info.plist         # macOS and iOS app settings
│    │    │                      # (or Info.ios.plist + Info.macos.plist)
│    │    └── Resources/
│    │      └── ios/             # iOS specific settings: storyboard
│    │        └── LaunchScreen.storyboard
│    │
│    ├── android/                # Android app settings: any file placed here will be deployed 
│    │   │── AndroidManifest.xml # (Optional manifest, HelloImGui will generate one if missing)
│    │   └── res/                
│    │       └── mipmap-xxxhdpi/ # Optional icons for different resolutions
│    │           └── ...         # Use Android Studio to generate them: 
│    │                           # right click on res/ => New > Image Asset
│    └── emscripten/
│      ├── shell.emscripten.html # Emscripten shell file
│      │                         #   (this file will be cmake "configured"
│      │                         #    to add the name and favicon) 
│      └── custom.js             # Any custom file here will be deployed
│                                #   in the emscripten build folder
│
├── fonts/
│    ├── DroidSans.ttf           # Default fonts
│    └── fontawesome-webfont.ttf #   used by HelloImGui
```

## Build instructions

### Build for Linux and macOS

#### 1. Optional: clone hello_imgui

_Note: This step is optional, since the CMakeLists.txt file will by default download and build hello_imgui at configure time._

In this example, we clone hello_imgui inside `external/hello_imgui`

Note: `external/` is mentioned in `.gitignore`

```bash
mkdir -p external && cd external
git clone https://github.com/pthom/hello_imgui.git
cd ..
```

Add this line at the top of your CMakeLists.txt

```cmake
add_subdirectory(external/hello_imgui)
```

#### 2. Create the build directory, run cmake and make

```bash
mkdir build && cd build
cmake ..
make -j 4
```

### Build for Windows

#### 1. Optional: clone hello_imgui
Follow step 1 from the Linux/macOS section above.

#### 2. Create the build directory, run cmake

```bash
mkdir build && cd build
cmake ..
```

#### 3. Open the Visual Studio solution
It should be located in build/helloworld_with_helloimgui.sln


### Build for Android

#### 1. Clone hello_imgui:
You will need to clone hello_imgui. In this example, we clone hello_imgui inside hello_imgui_template/external/hello_imgui

Note: external/ is mentioned in .gitignore

```bash
mkdir -p external && cd external
git clone https://github.com/pthom/hello_imgui.git
cd ..
```

Add this line at the top of your CMakeLists.txt

```cmake
add_subdirectory(external/hello_imgui)
```

#### 2. Create the Android Studio project
```bash
# Set the ANDROID_HOME and ANDROID_NDK_HOME environment variables
# For example:
export ANDROID_HOME=/Users/YourName/Library/Android/sdk
export ANDROID_NDK_HOME=/Users/YourName/Library/Android/sdk/ndk/26.1.10909125

mkdir build_android && cd build_android
../external/hello_imgui/tools/android/cmake_arm-android.sh ../
```

#### 3. Open the project in Android Studio
It should be located in build_android/hello_world_AndroidStudio.


### Build for iOS

#### 1. Clone hello_imgui: follow steps 1 from the Android section above.

#### 2. Create the Xcode project
```bash
mkdir build_ios && cd build_ios
```

Run CMake with the following command, where you replace XXXXXXXXX with your Apple Developer Team ID,
and com.your_website with your website (e.g. com.mycompany).

```bash
cmake .. \
-GXcode \
-DCMAKE_TOOLCHAIN_FILE=../external/hello_imgui/hello_imgui_cmake/ios-cmake/ios.toolchain.cmake \
-DPLATFORM=OS64COMBINED \
-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=XXXXXXXXX \
-DHELLO_IMGUI_BUNDLE_IDENTIFIER_URL_PART=com.your_website \
-DHELLOIMGUI_USE_SDL_OPENGL3=ON
```

Then, open the XCode project in build_ios/helloworld_with_helloimgui.xcodeproj


### Build for emscripten

#### Install emscripten
You can either install emsdk following [the instruction on the emscripten website](https://emscripten.org/docs/getting_started/downloads.html) or you can use the script [../tools/emscripten/install_emscripten.sh](../tools/emscripten/install_emscripten.sh).

#### Compile with emscripten

```bash
# Add emscripten tools to your path
source ~/emsdk/emsdk_env.sh

# cmake and build
mkdir build_emscripten
cd build_emscripten
emcmake cmake ..
make -j 4

# launch a webserver
python3 -m http.server
```

Open a browser, and navigate to [http://localhost:8000](http://localhost:8000).

HOWTO:
git clone https://github.com/pthom/hello_imgui_template.git
copy C:\pro\github\hello_imgui_template dir in C:\pro\github\dis-client WITHOUT .git and readme.md
edit C:\pro\github\dis-client\CMakeLists.txt and adapt:
- project(dis-client)
- hello_imgui_add_app(app main.cpp)


COMPIL EMSCRIPTEN VCPKG WASM
if (EMSCRIPTEN)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif ()
target_include_directories(app
  PUBLIC
    $<BUILD_INTERFACE:${EMSCRIPTEN_SYSROOT}>
)
target_link_libraries(app
  PRIVATE
     "-lwebsocket.js"
)
endif()
endif()
md dis-client-web
cd dis-client-web
emcmake cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug -DHELLOIMGUI_WITH_SDL=ON -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=C:/pro/emsdk/upstream/emscripten/cmake/Modules/Platform/emscripten.cmake -DCMAKE_TOOLCHAIN_FILE=C:/pro/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=wasm32-emscripten -DBUILD_SHARED_LIBS=OFF -B C:/bld/dis-client-web -S C:/pro/github/dis-client
cmake --build . --target help
cmake --build . --target app
emrun --browser chrome .

create build-web under c:/pro/github/dis-client
emcmake cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug -DHELLOIMGUI_WITH_SDL=ON -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=C:/pro/emsdk/upstream/emscripten/cmake/Modules/Platform/emscripten.cmake -DCMAKE_TOOLCHAIN_FILE=C:/pro/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=wasm32-emscripten -DBUILD_SHARED_LIBS=OFF -B C:/pro/github/dis-client/build-web -S C:/pro/github/dis-client



COMPIL WINDOWS VCPKG 
md dis-client
cd dis-client
cmake -DHELLOIMGUI_WITH_SDL=ON -DCMAKE_TOOLCHAIN_FILE=C:/pro/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DBUILD_SHARED_LIBS=OFF -B C:/bld/dis-client -S C:/pro/github/dis-client
