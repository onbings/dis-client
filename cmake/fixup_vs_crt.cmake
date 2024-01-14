# Make sure MSVC runtime is consistent accross
# all languages (i.e. CXX, CUDA, etc, ...)
# cfr : https://gitlab.kitware.com/cmake/cmake/-/issues/19428
# And
# based on the vcpkg target triplet

# Need the policy to new
cmake_policy(SET CMP0091 NEW)

macro(set_msvc_runtime_library)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_TOOLCHAIN_FILE AND VCPKG_TARGET_TRIPLET)
    get_filename_component(TOOLCHAIN_NAME ${CMAKE_TOOLCHAIN_FILE} NAME)
    if (TOOLCHAIN_NAME STREQUAL "vcpkg.cmake")
      get_filename_component(TOOLCHAIN_DIR ${CMAKE_TOOLCHAIN_FILE} DIRECTORY)
      get_filename_component(VCPKG_ROOT ${TOOLCHAIN_DIR}/../../ ABSOLUTE)
      if (NOT EXISTS "${VCPKG_ROOT}/.vcpkg-root")
        message(FATAL_ERROR "The directory ${VCPKG_ROOT} is not a valid vcpkg root directory.")
      endif()
      # read the CRT linkage from the target file:
      file(GLOB_RECURSE TARGET_FILE "${VCPKG_ROOT}/triplets/${VCPKG_TARGET_TRIPLET}.cmake")
      if (NOT TARGET_FILE)
        message(FATAL_ERROR "Could not find target file for triplet ${VCPKG_TARGET_TRIPLET}.")
      endif()
      include(${TARGET_FILE})

      if (VCPKG_CRT_LINKAGE STREQUAL "static")
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
      else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
      endif()

      message(STATUS "Detected VCPKG_CRT_LINKAGE as ${VCPKG_CRT_LINKAGE}")
      message(STATUS "Setting CMAKE_MSVC_RUNTIME_LIBRARY to ${CMAKE_MSVC_RUNTIME_LIBRARY}")

    endif()
  endif()
 endmacro()
