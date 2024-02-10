include(CMakeFindDependencyMacro)

macro(find_package_dependency)

  # When loading the *Config.cmake we should
  # call find_dependency which is just a wrapper
  # around find_package to display better
  # messages to the user. When directly dealing
  # with our CMakeLists.txt, we should call
  # find_package directly
  if(FROM_CONFIG_FILE)
     find_dependency(${ARGN})
  else()
     find_package(${ARGN})
  endif()

endmacro()

# ===========================
# == OPTIONAL DEPENDENCIES ==
# ===========================

if(BOFSTD_BUILD_TESTS)
  #find_package(GTest REQUIRED)
endif()

# ===========================
# == REQUIRED DEPENDENCIES ==
# ===========================
#find_package_dependency(date REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(bofstd CONFIG REQUIRED)
find_package(freetype CONFIG REQUIRED)
find_package(sdl2 CONFIG REQUIRED)
#find_package(lunasvg CONFIG REQUIRED)
#find_package(hello-imgui CONFIG REQUIRED)
if (EMSCRIPTEN)
else()
find_package(libwebsockets CONFIG REQUIRED)
find_package(libuv CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
endif()