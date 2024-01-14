include(${CMAKE_CURRENT_LIST_DIR}/update_rpath.cmake)

# Description
#  This function returns the list of subdirectories
#  of the specified ones in relative format
# 
#  Ex : If directory /some/path/ contains A, B and C
#       it will return "A B C"
#
# Parameters
#   result - The name of the variable to return the result
#   curdir - The directory to scan
#
# Returns
#   The list of subdirectories in variable result
#
function(get_subdirs result curdir)
  # Get the children of this directory
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()

  # Return the result
  set(${result} ${dirlist} PARENT_SCOPE)

endfunction()

# Description
#  This function returns the list of directories
#  where dependencies fetched by vcpkg are located
# 
#  e.g. ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/
#
# Parameters
#   result - The name of the variable to return the result
#
# Returns
#   The list of directories containing libraries installed by vcpkg
#
function(get_vcpkg_installed_directories result)

  set(vcpkg_lib_dirs "")
  set(vcpkg_installed_dir ${CMAKE_BINARY_DIR}/vcpkg_installed)

  message(STATUS "Checking existence of ${CMAKE_BINARY_DIR}/vcpkg_installed")

  # Is there a vcpkg_installed directory ?
  if(IS_DIRECTORY ${vcpkg_installed_dir})

    # List triplets
    get_subdirs(triplets ${vcpkg_installed_dir})

    foreach(triplet ${triplets})

      # Check for lib and debug/lib directories
      set(subdirs lib debug/lib bin debug/bin)

      foreach(subdir ${subdirs})
        if(IS_DIRECTORY ${vcpkg_installed_dir}/${triplet}/${subdir})
          list(APPEND vcpkg_lib_dirs ${vcpkg_installed_dir}/${triplet}/${subdir}/)
        endif()
      endforeach()
    endforeach()
  endif()

  # Return result
  set(${result} ${vcpkg_lib_dirs} PARENT_SCOPE)

endfunction()

# Description
#  This function detects the path in the binary directory
#  where vcpkg is installing the dependencies and patch
#  the rpath of every shared library found there to make
#  sure the first path to look for is those.
# 
#
# Parameters
#   None
#
# Returns
#   Nothing
#
function(patch_vcpkg_installed_rpath)

  # Retrieve directories where vcpkg install libraries
  get_vcpkg_installed_directories(VCPKG_DIRS)

  # Append $ORIGIN to libraries in vpckg directories
  foreach(vcpkg_dir ${VCPKG_DIRS})
    prepend_rpath_for_dir("${vcpkg_dir}" $ORIGIN)
  endforeach()

endfunction()

# Description
#  This function creates an install component
#  to install the shared libraries that might
#  be created by vcpkg
#
# Parameters
#   None
#
# Returns
#   Nothing
#
function(install_vcpkg_dependencies)

  # Retrieve directories where vcpkg install libraries
  get_vcpkg_installed_directories(VCPKG_DIRS)

  set(VPCKG_COMPONENT ${PROJECT_NAME}-vcpkg)

  # Create a script that will be runned at installation time that performs the following :
  #
  # 1°) It checks if there is a directory called vcpkg_installed in ${CMAKE_BINARY_DIR}
  # 2°) If it exists, it lists the different triplets found (i.e. subdirs of vcpkg_installed except vcpkg)
  # 3°) For each triplets, it collects the .so* and .dll in lib and debug/lib
  # 4°) It installs all those collected files
  #
  install(CODE "set(CMAKE_INSTALL_LIBDIR            \"${CMAKE_INSTALL_LIBDIR}\")"             COMPONENT ${VPCKG_COMPONENT})
  install(CODE "set(CMAKE_INSTALL_BINDIR            \"${CMAKE_INSTALL_BINDIR}\")"             COMPONENT ${VPCKG_COMPONENT})
  install(CODE "set(CMAKE_BINARY_DIR                \"${CMAKE_BINARY_DIR}\")"                 COMPONENT ${VPCKG_COMPONENT})
  install(CODE "set(CMAKE_CURRENT_FUNCTION_LIST_DIR \"${CMAKE_CURRENT_FUNCTION_LIST_DIR}\")"  COMPONENT ${VPCKG_COMPONENT})
  install(CODE "set(VCPKG_DIRS                      \"${VCPKG_DIRS}\")"                       COMPONENT ${VPCKG_COMPONENT})
  install(CODE [[

        include(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/update_rpath.cmake)

        # Search inside those directories
        foreach(dep_dir ${VCPKG_DIRS})

          file(GLOB so_files  "${dep_dir}/*.so*")
          file(GLOB dll_files "${dep_dir}/*.dll")

          # Install them all
          if(NOT "x${so_files}" STREQUAL "x")
            file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" FILES ${so_files})
          endif()

          if(NOT "x${dll_files}" STREQUAL "x")
            file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}" FILES ${dll_files})
          endif()

          # Update rpath for libraries
          update_rpath_for_dir("${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" $ORIGIN)

        endforeach()
    ]]
    COMPONENT ${VPCKG_COMPONENT}
  )
    
endfunction()