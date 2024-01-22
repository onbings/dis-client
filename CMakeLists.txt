#see README.md
#cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=C:/pro/emsdk/upstream/emscripten/cmake/Modules/Platform/emscripten.cmake -DCMAKE_TOOLCHAIN_FILE=C:/pro/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=wasm32-emscripten -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Debug -SC:/pro/github/dis-client -BC:/bld/dis-client-web -DCMAKE_CROSSCOMPILING_EMULATOR=C:/pro/emsdk/node/16.20.0_64bit/bin/node.exe -G Ninja
#cmake -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=C:/pro/emsdk/upstream/emscripten/cmake/Modules/Platform/emscripten.cmake -DCMAKE_TOOLCHAIN_FILE=C:/pro/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=wasm32-emscripten -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Debug -SC:/pro/github/dis-client -BC:/bld/dis-client-web -DCMAKE_CROSSCOMPILING_EMULATOR=C:/pro/emsdk/node/16.20.0_64bit/bin/node.exe
#cmake -DCMAKE_TOOLCHAIN_FILE=C:/pro/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows-static -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Debug  -DHELLOIMGUI_USE_SDL_OPENGL3=ON -SC:/pro/github/dis-client -BC:/bld/dis-client-win
#https://github.com/microsoft/vcpkg/discussions/36049
#https://github.com/microsoft/vcpkg/discussions/30108#top
#/opt/evs/evs-gbio/bin/dis_service --DisServer=ws://10.129.171.112:8080
cmake_minimum_required(VERSION 3.28)

# Paranoid check : in-source generation is forbidden
if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "You cannot generate your binary solution into your source directory !")
endif()
 
include(cmake/fixup_vs_crt.cmake)
include(cmake/vcpkg_install.cmake)
include(cmake/fetch_content.cmake)
# Programs used for integration testing should not be included
# in code coverage computation as they are test programs themselves.
set(EXTRA_COVERAGE_EXCLUSION "\'${CMAKE_CURRENT_SOURCE_DIR}/integration/*\'")

project(dis-client VERSION 1.2.3.4)
# Some naming conventions either requires lower or upper case.
# And some don't like underscore.
# So already prepare all those variables
string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)
string(REPLACE "-" "_" PROJECT_NAME_UNDERSCORE       ${PROJECT_NAME})
string(REPLACE "-" "_" PROJECT_NAME_LOWER_UNDERSCORE ${PROJECT_NAME_LOWER})
string(REPLACE "-" "_" PROJECT_NAME_UPPER_UNDERSCORE ${PROJECT_NAME_UPPER})
message("==============PROJECT_NAME=============>" ${PROJECT_NAME} )
option(DISCLIENT_BUILD_EXAMPLES    "Build example of dis-client project"                                      OFF)
option(DISCLIENT_BUILD_TESTS       "Build units tests of dis-client project"                                  OFF)
option(DISCLIENT_BUILD_TOOLS       "Build tools of dis-client project"                                        OFF)
option(DISCLIENT_ENABLE_VERSIONING "Enable library versioning naming scheme for dis-client libraries"         ON)
option(DISCLIENT_FORCE_EXPORT_ALL  "Force exporting all symbols of dis-client libraries (for shared library)" OFF)
#EMSCRIPTEN
option(EMSCRIPTEN_GEN_HTML "EMSCRIPTEN generateq .wasm and .js file. If you set this option to ON we also generate an html container" OFF)
set(DISCLIENT_EMSCRIPTEN_MEMORY 256MB CACHE STRING "Specify emscripten default memory size")
message("emscripten default memory size: ${DISCLIENT_EMSCRIPTEN_MEMORY}")
set(DISCLIENT_EMSCRIPTEN_STACK 4MB CACHE STRING "Specify emscripten default stack size")
message("emscripten default stack size: ${DISCLIENT_EMSCRIPTEN_STACK}")
set(DISCLIENT_EMSCRIPTEN_THREAD_POOL 4 CACHE STRING "Specify emscripten default thread pool size")
message("emscripten default thread pool size: ${DISCLIENT_EMSCRIPTEN_THREAD_POOL}")
#set(DISCLIENT_EMSCRIPTEN_FS_SRC_ROOT_DIR ${CMAKE_BINARY_DIR}/binaries/bin/data CACHE STRING "Specify emscripten fs source dir")
set(DISCLIENT_EMSCRIPTEN_FS_SRC_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/assets CACHE STRING "Specify emscripten preload file dir")
message("emscripten fs source file directory: ${DISCLIENT_EMSCRIPTEN_FS_SRC_ROOT_DIR}")
set(DISCLIENT_EMSCRIPTEN_FS_DST_ROOT_DIR assets CACHE STRING "Specify emscripten fs destination dir")
message("emscripten fs destination file directory: ${DISCLIENT_EMSCRIPTEN_FS_DST_ROOT_DIR}") 
#em++: error: 'C:\pro\emsdk\upstream\emscripten\tools\file_packager.bat binaries\bin\dis-client.data --from-emcc --preload C:/bld/dis-client-web/binaries/bin/data' failed (returned 1)
# ===============================
# == NON TRANSITIVE PROPERTIES ==
# ===============================

# Non transitive properties are properties that are not
# usage requirements. It means properties that only need
# to be set for building this target only. As such they
# don't need to propagated outside.
# For convenience, use global initializer for those

# Disable compiler extension to maximize portability
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 
SET(CMAKE_COLOR_MAKEFILE ON)
SET(CMAKE_VERBOSE_MAKEFILE ON)
# set(CMAKE_DEBUG_POSTFIX _d)

if (EMSCRIPTEN)

#sometime it cannot find pthread. If i force it in C:\Program Files\CMake\share\cmake-3.28\Modules\FindThreads.cmake it works...
#macro(_threads_check_libc)
#      set(CMAKE_THREAD_LIBS_INIT "")
#      set(Threads_FOUND TRUE)

#https://decovar.dev/blog/2023/11/20/webassembly-with-pthreads/
  set(HELLOIMGUI_EMSCRIPTEN_PTHREAD ON)
  set(HELLOIMGUI_EMSCRIPTEN_PTHREAD_ALLOW_MEMORY_GROWTH OFF)

  if (EMSCRIPTEN_GEN_HTML)
    message("===EMSCRIPTEN=== Add .html to .wasm and .js files")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")   #to generate .html in addition to .js and .wasm
  else()
    message("===EMSCRIPTEN=== Generate .wasm and .js files")
  endif()

  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -pthread")  #hang with imgui: -s NO_DISABLE_EXCEPTION_CATCHING
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")  #hang with imgui: -s NO_DISABLE_EXCEPTION_CATCHING
#ici ca bloque qq part dans imgui car je pense qu'on est dans un autre thread entre imgui et le main...
#  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -pthread -sPROXY_TO_PTHREAD") #-sPTHREAD_POOL_SIZE=4") # -sPROXY_TO_PTHREAD -sASYNCIFY")  # -sUSE_PTHREADS=1) # -sSHARED_MEMORY=1")
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -sPROXY_TO_PTHREAD") #-sUSE_PTHREADS=1") #-sPTHREAD_POOL_SIZE=4") # -sPROXY_TO_PTHREAD -sASYNCIFY") # -sUSE_PTHREADS=1) # -sSHARED_MEMORY=1")

  #The @ symbol is needed because sometimes it is useful to package files that are not nested below the compile-time directory, and for which #there is therefore no default mapping to a location in the virtual file system.
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s TOTAL_STACK=${DISCLIENT_EMSCRIPTEN_STACK} -s TOTAL_MEMORY=${DISCLIENT_EMSCRIPTEN_MEMORY} -s PTHREAD_POOL_SIZE=${DISCLIENT_EMSCRIPTEN_THREAD_POOL} --embed-file ${DISCLIENT_EMSCRIPTEN_FS_SRC_ROOT_DIR}@${DISCLIENT_EMSCRIPTEN_FS_DST_ROOT_DIR}")    
  #-sPROXY_TO_PTHREAD  not good for  imgui car je pense qu'on est dans un autre thread entre imgui et le main...
  #-s ASYNCIFY (very long and sometime forever)
  message("===EMSCRIPTEN=== CMAKE_EXE_LINKER_FLAGS is ${CMAKE_EXE_LINKER_FLAGS}")
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
  endif ()
  message("===EMSCRIPTEN=== CMAKE_CXX_FLAGS is ${CMAKE_CXX_FLAGS}")
endif()

# For shared libraries we need to compile with "-fPIC"
# But for static libraries user can choose
if(NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
  set(CMAKE_POSITION_INDEPENDENT_CODE  ${BUILD_SHARED_LIBS})
endif()

# API visibility control
if(DISCLIENT_FORCE_EXPORT_ALL)

  if(MSVC AND ${BUILD_SHARED_LIBS})
    message(WARNING "\nYou requested to force exporting ALL symbols "
                    "in a Windows DLL through CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS\n"
                    "This is a hack that has limitations. Use it as a temporary workaround")
  endif()

  # By "default",
  #  - GCC and Clang export everything (i.e. nothing to do)
  #  - MSVC hides everything and requires explicit listing
  #
  # This command tells CMake to generate such listing
  # for all the symbols found in the binary
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS   ON)

else()

  # We only want the API symbols to be visible
  # Tell CMake to pass the proper flags to hide
  # everything that is not explicitly marked
  set(CMAKE_CXX_VISIBILITY_PRESET         hidden)
  set(CMAKE_CXX_VISIBILITY_INLINES_HIDDEN ON)

endif()

enable_testing()

# Make sure MSVC runtime is consistent accross
# all languages (i.e. CXX, CUDA, etc, ...)
# cfr : https://gitlab.kitware.com/cmake/cmake/-/issues/19428
set_msvc_runtime_library()

# Fix rpath from installed vpckg dependencies
patch_vcpkg_installed_rpath()

# ==============================
# == PRESET INSTALL VARIABLES ==
# ==============================

# This include default installation directories
# based on detection performed by CMake. They
# are just default that can be overriden by the user.
include(GNUInstallDirs)

# It creates for example the following variables
#  ${CMAKE_INSTALL_BINDIR}      --> bin
#  ${CMAKE_INSTALL_LIBDIR}      --> lib, lib64, etc, ...
#  ${CMAKE_INSTALL_INCLUDEDIR}  --> include
#
# All those paths are relative to ${CMAKE_INSTALL_PREFIX} that is not
# known at configuration time and can be overriden by the user when installing.
#
# For platforms that support RPATH and ORIGIN, we can then compute the
# relative path from ${CMAKE_INSTALL_BINDIR} to ${CMAKE_INSTALL_PREFIX}
# example :
#   bin           --> some/weird/path
#   relative path --> ../../../
#
# and override RPATH with $ORIGIN/RELATIVE_PATH/${CMAKE_INSTALL_LIBDIR} at installation
#
file(RELATIVE_PATH BIN_TO_BASE_INSTALL_PATH ${CMAKE_CURRENT_LIST_DIR}/${CMAKE_INSTALL_BINDIR} ${CMAKE_CURRENT_LIST_DIR})
set (RELATIVE_PATH_BIN_TO_LIB ${BIN_TO_BASE_INSTALL_PATH}${CMAKE_INSTALL_LIBDIR})
#see also update_rpath function in update_rpath.cmake

# =============================
# == SETUP BUILD DIRECTORIES ==
# =============================

# Pre-stage binaries like if they were already installed
#  - CMAKE_LIBRARY_OUTPUT_DIRECTORY is for shared libraries
#  - CMAKE_ARCHIVE_OUTPUT_DIRECTORY is for static libraries
#  - CMAKE_RUNTIME_OUTPUT_DIRECTORY is for applications
set(COMPILED_BINARIES_DIR ${CMAKE_BINARY_DIR}/binaries)
set(BASE_RESOURCES_DIR    ${CMAKE_BINARY_DIR}/resources)
 
if(WIN32)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${COMPILED_BINARIES_DIR}/bin)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${COMPILED_BINARIES_DIR}/bin)
else()
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${COMPILED_BINARIES_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${COMPILED_BINARIES_DIR}/bin)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${COMPILED_BINARIES_DIR}/lib)

# ===============
# == PACKAGING ==
# ===============

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  # make package only if dis-client is the main project (not a submodule)
  include(packaging.cmake)
endif()

# =======================
# == POPULATE PROJECTS ==
# =======================

string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)

set(VCPKG_DEBUG_LIB_DIR   "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/lib")
set(VCPKG_RELEASE_LIB_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib")

# Fix RPATH depending on build type
if("x${CMAKE_BUILD_TYPE_LOWER}x" STREQUAL "xdebugx")
  set(CMAKE_BUILD_RPATH "${VCPKG_DEBUG_LIB_DIR};${VCPKG_RELEASE_LIB_DIR}")
else()
  set(CMAKE_BUILD_RPATH "${VCPKG_RELEASE_LIB_DIR};${VCPKG_DEBUG_LIB_DIR}")
endif()

# To avoid problem with EMSCRIPTEN compilation we only get third party libs which dont 
# depend on pthread lib. These one are get using the fetch_vcpkg_library function as
# they must be built with a strict control of options which is not possible with vcpkg 
# build in manifest mode 
message("==============================================start fetch=========================")
#fetch_external_library_add(googletest "${LIST_OF_LIB}" https://github.com/google/googletest release-1.12.1)
#fetch_external_library_add(spdlog "${LIST_OF_LIB}" https://github.com/gabime/spdlog v1.12.0)
#make_external_library_available("${LIST_OF_LIB}")
message("==============================================END fetch=========================")
include(Dependencies.cmake)

find_package(Doxygen)
message("=====================> Doxygen compiler: " ${DOXYGEN_EXECUTABLE})
if(${DOXYGEN_EXECUTABLE} STREQUAL "doxygen-NOTFOUND")
  message("WARNING: Could not find doxygen executable. Is doxygen installed ?")
endif()

#Done by helloimgui: result in C:\bld\dis-client-win\assets, must contain something for --embed-file ${DISCLIENT_EMSCRIPTEN_FS_SRC_ROOT_DIR}@${DISCLIENT_EMSCRIPTEN_FS_DST_ROOT_DIR}
#set(DATA_FILES
#  ./assets/font/cour.ttf
#)


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
if (EMSCRIPTEN)
hello_imgui_add_app(dis-client 
   main.cpp 

   src/bof_imgui.cpp
   include/bof_imgui.h
   src/bof_imgui_internal.cpp
   src/bof_imgui_internal.h

   include/dis_client.h
   src/dis_client.cpp
)
else()
hello_imgui_add_app(dis-client 
   main.cpp 

   src/bof_imgui.cpp
   include/bof_imgui.h
   src/bof_imgui_internal.cpp
   src/bof_imgui_internal.h

   include/dis_client.h
   src/dis_client.cpp
   include/dis_client_websocket.h
   src/dis_client_websocket.cpp

   webrpc/lib/include/webrpc/grpc.h
   webrpc/lib/src/grpc.cpp
   webrpc/lib/include/webrpc/webrpc.h
   webrpc/lib/include/webrpc/websocket.h
   webrpc/lib/src/websocket.cpp

   webrpc/lib/include/webrpc/webrpcversioninfo.h.in
)
endif()

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


# This will build disclient under build-disclient/ subdirectory in the project's build tree
####add_subdirectory(C:/pro/github/disclient build-disclient)

if (EMSCRIPTEN)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -gsplit-dwarf")
      #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -gsplit-dwarf")
      #message("EMSCRIPTEN Debug Add -g -gsplit-dwarf, flags are now " ${CMAKE_CXX_FLAGS})
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
  endif ()

  target_include_directories(dis-client 
    PUBLIC
      $<BUILD_INTERFACE:${EMSCRIPTEN_SYSROOT}>
  )
  target_link_libraries(dis-client 
    PRIVATE
      "-lwebsocket.js"
  )
else()
  target_link_libraries(dis-client 
    PRIVATE
	websockets
    $<IF:$<TARGET_EXISTS:uv_a>,uv_a,uv>
    protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite
    gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts	
  )
endif()

if (MSVC)
  target_link_libraries(dis-client
    PRIVATE
      Shcore
  )
endif()

target_compile_definitions(dis-client
  PRIVATE
    -Dbha=1	
)
target_include_directories(dis-client
  PUBLIC
    ####$<BUILD_INTERFACE:${dis-client_BINARY_DIR}/lib> #for #include <dis-client_export.h> when we do not link with dis-client
    ####$<BUILD_INTERFACE:${dis-client_SOURCE_DIR}/lib/include> #for #include <dis-client/dis-client.h> when we do not link with dis-client
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> 
    $<BUILD_INTERFACE:${HELLOIMGUI_IMGUI_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/webrpc/lib/include>  #TDO TBD
)
target_link_libraries(dis-client
  PRIVATE
    nlohmann_json::nlohmann_json
    ONBINGS::bofstd

)
if(DISCLIENT_BUILD_TESTS)
  add_subdirectory(tests)
endif()

if(DISCLIENT_BUILD_TOOLS)
  add_subdirectory(tools)
endif()

# If you plan on running your unit tests with Valgrind
# it might happen that you need to add 'suppressions'
# to silence known issues. valgrind accepts suppressions
# by taking a file (--log-file=...). The variable
# MEMORYCHECK_SUPPRESSIONS_FILE is not a CMake variable
# but can be used as a convenience to accumulate suppressions
# from all the unit tests. Consequently, if you configure
# valgrind to use that file and define this variable, you will
# have the expected result.
if(MEMORYCHECK_SUPPRESSIONS_FILE)
  file(READ   valgrind_suppression.txt           CONTENT)
  file(APPEND ${MEMORYCHECK_SUPPRESSIONS_FILE} ${CONTENT})
endif()

#Done by helloimgui: result in C:\bld\dis-client-win\assets, must contain something for --embed-file ${DISCLIENT_EMSCRIPTEN_FS_SRC_ROOT_DIR}@${DISCLIENT_EMSCRIPTEN_FS_DST_ROOT_DIR}
#message("dis-client: Create data dir " ${COMPILED_BINARIES_DIR} "/bin/data")
#file(MAKE_DIRECTORY ${COMPILED_BINARIES_DIR}/bin/data)  #Ut under linux from vscode
#foreach(Filename ${DATA_FILES})
#  message("dis-client: Copy " ${Filename} " to " ${COMPILED_BINARIES_DIR} "/bin/data")
#  file(COPY ${Filename} DESTINATION ${COMPILED_BINARIES_DIR}/bin/data)
#endforeach() 

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION  ${COMPILED_BINARIES_DIR}/bin FILES_MATCHING PATTERN "*")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION  ${CMAKE_INSTALL_BINDIR}      COMPONENT bofstd-runtime FILES_MATCHING PATTERN "*")

# ==================
# == INSTALLATION ==
# ==================

if(NOT APPLE)

  # Fix RPATH upon installation by replacing
  # the absolute RPATH generated by the compiler
  # by a keyword that basically says 'look for my
  # dependencies in the directory I'm currently
  # installed'.
  set_target_properties(dis-client PROPERTIES
    INSTALL_RPATH $ORIGIN
  )

endif()

# Install the target runtime
	   
install(
  TARGETS  dis-client
  EXPORT   dis-client
  RUNTIME  DESTINATION        ${CMAKE_INSTALL_BINDIR}     # For binaries and dll
           COMPONENT          dis-client-runtime
  LIBRARY  DESTINATION        ${CMAKE_INSTALL_LIBDIR}     # For shared libraries
           COMPONENT          dis-client-runtime
           NAMELINK_COMPONENT dis-client-devel
  ARCHIVE  DESTINATION        ${CMAKE_INSTALL_LIBDIR}     # For static libraries
           COMPONENT          dis-client-devel
)

# Install the public headers (don't forget the generated export directives)
install(DIRECTORY   ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT   dis-client-devel
)
install(FILES       ${CMAKE_CURRENT_BINARY_DIR}/bofstd_export.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT   dis-client-devel
)
# install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third/fmt-5.3.0/include/fmt/.		
# 		 DESTINATION include/fmt-5.3.0
#          COMPONENT   dis-client-devel
# )
# install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third/spdlog-1.3.1/include/.			
#          DESTINATION include/spdlog-1.3.1		
#          COMPONENT   dis-client-devel
# )
#install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third/vld/.	
#         DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dis-client/vld
#         COMPONENT   dis-client-devel
#) 
install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third/AsyncMulticastDelegate/include/asyncmulticastdelegate/.	
         DESTINATION include/asyncmulticastdelegate
         COMPONENT   dis-client-devel
)
# install (DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/third/libyuv/include/.	
#          DESTINATION include/libyuv
#          COMPONENT   dis-client-devel
# )

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/help/html/. DESTINATION doc COMPONENT doc OPTIONAL)


# ==========================
# == PACKAGE INSTALLATION ==
# ==========================

# Export our targets
install(
  EXPORT      dis-client
  FILE        dis-client-targets.cmake
  NAMESPACE   ONBINGS::
  DESTINATION ${CMAKE_INSTALL_DATADIR}
  COMPONENT   dis-client-devel
)

# Install dependencies
set(INCLUDE_REGEXES "^${CMAKE_BINARY_DIR}/vcpkg_installed/")
set(EXCLUDE_REGEXES 
  ".*[Aa][Pp][Ii]-[Mm][Ss].*"                                                         # *api-ms*
  ".*[Ee][Xx][Tt]-[Mm][Ss]-[Ww][Ii][Nn].*"                                            # *ext-ms-win*
  ".*[/\\\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\].*"   # *\Windows\System32\*
  "^/lib"                                                                             # /lib (unix system)
  "^/usr/lib"                                                                         # /usr/lib (unix system)
) 

install(RUNTIME_DEPENDENCY_SET dis-client-imported-runtime-set
  PRE_INCLUDE_REGEXES    ${INCLUDE_REGEXES}
  PRE_EXCLUDE_REGEXES    ${EXCLUDE_REGEXES}
  POST_INCLUDE_REGEXES   ${INCLUDE_REGEXES}
  POST_EXCLUDE_REGEXES   ${EXCLUDE_REGEXES}
  COMPONENT              dis-client-runtime
)

# Create configuration file so that find_package() from
# our dependees can find us
include(CMakePackageConfigHelpers)

set(INTERNAL_NAMESPACE ONBINGS)

# Create and install the cmake configuration file based on our template
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/dis-client-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}
)

unset(INTERNAL_NAMESPACE)

# Give the file a nice naming convention
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Dependencies.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/dis-client-dependencies.cmake
  COPYONLY
)

# Generate the version file and indicates compatibility strategy
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/dis-client-version.cmake
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)

# Install them
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/dis-client-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/dis-client-version.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/dis-client-dependencies.cmake
  DESTINATION ${CMAKE_INSTALL_DATADIR}
  COMPONENT   dis-client-devel
)
