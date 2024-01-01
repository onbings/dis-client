#see README.md
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

cmake_minimum_required(VERSION 3.25)
project(dis-client)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Build hello_imgui
# =================
# 1/  Option 1: if you added hello_imgui as a subfolder, you can add it to your project with:
#     add_subdirectory(external/hello_imgui)

# 2/  Option 2: simply fetch hello_imgui during the build
if (NOT TARGET hello_imgui)
    message(STATUS "Fetching hello_imgui")
    include(FetchContent)
    FetchContent_Declare(hello_imgui GIT_REPOSITORY https://github.com/pthom/hello_imgui.git GIT_TAG master)
    FetchContent_MakeAvailable(hello_imgui)
endif()


# Build your app
# ==============
hello_imgui_add_app(app 
   main.cpp 
   #include/nlohmann_json.hpp
   src/bof_imgui.cpp
   include/bof_imgui.h
   include/bofstd.h
   include/boferr.h
   include/bofstd_export.h
   src/bof_imgui_internal.cpp
   src/bof_imgui_internal.h
)


# hello_imgui_add_app is a helper function, similar to cmake's "add_executable"
# Usage:
# hello_imgui_add_app(app_name file1.cpp file2.cpp ...)
#
# Features: 
# * It will automatically link the target to the required libraries (hello_imgui, OpenGl, glad, etc)
# * It will embed the assets (for desktop, mobile, and emscripten apps)
# * It will perform additional customization (app icon and name on mobile platforms, etc)

# Now you can build your app with
#     mkdir build && cd build && cmake .. && cmake --build .


include(Dependencies.cmake)

# This will build bofstd under build-bofstd/ subdirectory in the project's build tree
####add_subdirectory(C:/pro/github/bofstd build-bofstd)

target_compile_definitions(app
  PRIVATE
    -Dbha=1	
)
target_include_directories(app
  PUBLIC
    ####$<BUILD_INTERFACE:${bofstd_BINARY_DIR}/lib> #for #include <bofstd_export.h> when we do not link with bofstd
    ####$<BUILD_INTERFACE:${bofstd_SOURCE_DIR}/lib/include> #for #include <bofstd/bofstd.h> when we do not link with bofstd
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> 
)

if (EMSCRIPTEN)
#-s ALLOW_MEMORY_GROWTH=1 -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=4 -s MAXIMUM_MEMORY=256MB 
  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread") #-sPTHREAD_POOL_SIZE=4") # -sPROXY_TO_PTHREAD -sASYNCIFY")  # -sUSE_PTHREADS=1) # -sSHARED_MEMORY=1")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sPTHREAD_POOL_SIZE=4") # -sPROXY_TO_PTHREAD -sASYNCIFY") # -sUSE_PTHREADS=1) # -sSHARED_MEMORY=1")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -gsplit-dwarf")
      #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -gsplit-dwarf")
      #message("EMSCRIPTEN Debug Add -g -gsplit-dwarf, flags are now " ${CMAKE_CXX_FLAGS})
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
      message("EMSCRIPTEN Debug Add -g, flags are now " ${CMAKE_CXX_FLAGS})    
  else()    
      message("EMSCRIPTEN Release flags are " ${CMAKE_CXX_FLAGS})
  endif ()
  target_include_directories(app
    PUBLIC
      $<BUILD_INTERFACE:${EMSCRIPTEN_SYSROOT}>
  )
  target_link_libraries(app 
    PRIVATE
      "-lwebsocket.js"
      pthread
  )
endif()


if (MSVC)
  target_link_libraries(app
    PRIVATE
      Shcore
  )
endif()

target_link_libraries(app
  PRIVATE
    nlohmann_json::nlohmann_json
)

# Print all CMake variables
#get_cmake_property(_variableNames VARIABLES)
#foreach(_variableName ${_variableNames})
#    message(STATUS "--------->${_variableName}=${${_variableName}}")
#endforeach()
