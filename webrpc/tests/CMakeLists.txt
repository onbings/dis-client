include(GoogleTest)

# =============
# == SOURCES ==
# =============
  
set(DATA_FILES
  ./data/dummy_webrpc.txt
)

set(API_FILES
	include/gtestrunner.h
	src/main.cpp
)


set(SOURCES_FILES
	${API_FILES}

)

# For Visual Studio
#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/webrpc PREFIX "public" FILES ${PUBLIC_INCLUDES_FILES})
#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src            PREFIX "src"    FILES ${SOURCES_FILES})

source_group("Api"		 FILES ${API_FILES})


# ===========
# == BUILD ==
# ===========

if (MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/bigobj)
else()
    # lots of warnings and all warnings as errors
#    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

# Create target
add_executable(ut_webrpc
  ${SOURCES_FILES}
)
# Includes
target_include_directories(ut_webrpc
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../lib/src>
)

### VLD ###########################################
set(CMAKE_VLD_TARGET ut_webrpc)
configure_file(
  ${CMAKE_SOURCE_DIR}/vld.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/vld.cmake
  @ONLY
)
include(${CMAKE_CURRENT_BINARY_DIR}/vld.cmake)
### VLD ###########################################

target_link_libraries(ut_webrpc
  PRIVATE
	ONBINGS::webrpc
	GTest::GTest
)
          
# Object library is not a shared library
# void import directive only for linking
target_compile_definitions(ut_webrpc
  PRIVATE
    -DWEBRPC_STATIC_DEFINE=1
)

# Add test and allow them to run in //
gtest_discover_tests(ut_webrpc
  XML_OUTPUT_DIR     ${CMAKE_BINARY_DIR}/GTestReports/
  TEST_PREFIX        webrpc_
   DISCOVERY_TIMEOUT 120
   DISCOVERY_MODE    PRE_TEST
   WORKING_DIRECTORY $<TARGET_FILE_DIR:webrpc>
)

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

# message("ut_webrpc: Create data dir " ${COMPILED_BINARIES_DIR} "/bin/data")
# file(MAKE_DIRECTORY ${COMPILED_BINARIES_DIR}/bin/data)  #Ut under linux from vscode
# foreach(Filename ${DATA_FILES})
#   message("ut_webrpc: Copy " ${Filename} " to " ${COMPILED_BINARIES_DIR} "/bin/data")
#   file(COPY ${Filename} DESTINATION ${COMPILED_BINARIES_DIR}/bin/data)
# endforeach() 

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION  ${COMPILED_BINARIES_DIR}/bin FILES_MATCHING PATTERN "*")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data DESTINATION  ${CMAKE_INSTALL_BINDIR}      COMPONENT webrpc-runtime FILES_MATCHING PATTERN "*")