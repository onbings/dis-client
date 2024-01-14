include(FetchContent)

#first call this one for each lib AND AFTER cal ONCE fetch_external_library_generate
function(fetch_external_library_add LIB_NAME LIST_OF_LIB GIT_URL GIT_TAG)
    message(STATUS "Start of fetch external library '${LIB_NAME}' from ${GIT_URL} at tag ${GIT_TAG}:")
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_Declare(
        ${LIB_NAME}
        GIT_REPOSITORY ${GIT_URL}
        GIT_TAG ${GIT_TAG}
    )
    list(APPEND LIST_OF_LIB ${LIB_NAME})  # Append library name to the provided list
    set(LIST_OF_LIB "${LIST_OF_LIB}" PARENT_SCOPE)
    message(STATUS "End of of fetch external library add '${LIB_NAME}'")
endfunction()

function(make_external_library_available LIST_OF_LIB)
    message(STATUS "make_external_library_available for the following lib:")
    foreach(_lib ${LIST_OF_LIB})
        message("${_lib}")
    endforeach()
    FetchContent_MakeAvailable("${LIST_OF_LIB}")
endfunction()

# Function to list CMake variables with a given prefix
# list_cmake_vars_with_prefix("CMAKE_")
# list_cmake_vars_with_prefix("CXX_")
function(list_cmake_vars_with_prefix _prefix)
    message(STATUS "Listing CMake variables with prefix '${_prefix}':")
    get_cmake_property(_variable_names VARIABLES)
    foreach(_variable_name IN LISTS _variable_names)
        string(FIND "${_variable_name}" "${_prefix}" _prefix_length)
        if (_prefix_length EQUAL 0)
            message(STATUS "${_variable_name} = ${${_variable_name}}")
        endif()
    endforeach()
    message(STATUS "End of listing")
endfunction()

function(list_cmake_vars_containing _str)
    message(STATUS "Listing CMake variables which contains '${_str}':")
    string(TOLOWER "${_str}" _str_lower)
    get_cmake_property(_variable_names VARIABLES)
    foreach(_variable_name IN LISTS _variable_names)
        string(TOLOWER "${_variable_name}" _variable_lower)
        string(FIND "${_variable_lower}" "$_str_lower}" pos)    
        string(FIND "${_variable_name}" "${_str}" pos)
        if (NOT pos EQUAL -1)
            message(STATUS "${_variable_name} = ${${_variable_name}}")
        endif()
    endforeach()
    message(STATUS "End of listing")
endfunction()


    #get_cmake_property(_variable_names VARIABLES)
    #foreach(_variable_name IN LISTS _variable_names)
    #   message(STATUS "${_variable_name} = ${${_variable_name}}")
    #endforeach()	