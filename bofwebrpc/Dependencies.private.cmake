# ==========================
# == PRIVATE DEPENDENCIES ==
# ==========================

if(WEBRPC_BUILD_TESTS)

  if(NOT TARGET GTest::GTest)
    find_package(GTest REQUIRED)
  endif()
endif()
#  find_package(OpenSSL REQUIRED)
#  find_package(ffmpeg REQUIRED)
#  find_package(Boost REQUIRED COMPONENTS thread)
