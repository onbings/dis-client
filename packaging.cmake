# ===========
# == CPACK ==
# ===========

# DISCLAIMER :
#   The intent of this usage of CPack is 
#   not to create fancy or complicated
#   package. It's simply to create an 
#   archive (.e.g. tar.gz) that will contain
#   the relocatable runtime to deploy on a 
#   machine and executes
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR "EVS Broadcast Equipment")
set(CPACK_PACKAGE_DESCRIPTION "The installer package of ${PROJECT_NAME}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${CPACK_PACKAGE_DESCRIPTION})

# if(UNIX)
#   set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/evs/AppFs/${CPACK_PACKAGE_NAME}")
# endif()

# Add an indication about the nature of binaries
if(BUILD_SHARED_LIBS)
  set(LINK_TYPE "dynamic")
else()
  set(LINK_TYPE "static")
endif()

if(DEFINED OS_DISTRO AND DEFINED OS_VERSION)
  set(OS_FLAVOUR "-${OS_DISTRO}.${OS_VERSION}")
endif()

if(DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE STREQUAL "")
  set(BUILD_TYPE "-${CMAKE_BUILD_TYPE}")
endif()

# Create output file name in the given directory
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${LINK_TYPE}${BUILD_TYPE}${OS_FLAVOUR}")
if (${CMAKE_INSTALL_PREFIX} STREQUAL "")
  set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_BINARY_DIR}/package)
else()
  set(CPACK_OUTPUT_FILE_PREFIX ${CMAKE_INSTALL_PREFIX}/package)
endif()

# Only support tar.gz

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

if(UNIX)
  set(CPACK_GENERATOR "TGZ")
else()
  set(CPACK_GENERATOR "ZIP")
endif()

# Allow archive to do component based install
set(CPACK_ARCHIVE_COMPONENT_INSTALL 1)

# Install only those components in this archive
set(CPACK_COMPONENTS_ALL
  evs-gbio-runtime
  AppBofStd-app
  bofstd-runtime
  dis-runtime
  webrpc-runtime
)

set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)


# Allow compression in //
set(CPACK_THREADS 0)

# Try to build relocatable packages
set(CPACK_PACKAGE_RELOCATABLE TRUE)

if(DEFINED OS_DISTRO AND OS_DISTRO STREQUAL "centos")
  list(APPEND CPACK_GENERATOR "RPM")

  include(rpm.cmake)
endif()

# Always the last directive
include(CPack)
