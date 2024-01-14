# Description
#   This function updates the rpath
#   of the given file
#
# Parameters
#   file  - The path to the file
#   rpath - The rpath to update
#
function(update_rpath file rpath)

  find_program(PATCHELF patchelf)

  if(PATCHELF STREQUAL "PATCHELF-NOTFOUND")
    return()
  endif()

  # Only process realfiles
  if(NOT IS_SYMLINK ${file})
  
    message(STATUS "Updating rpath of ${file} to ${rpath}")

    # Update it
    execute_process(COMMAND ${PATCHELF} --set-rpath "${rpath}" ${file} 
                    RESULT_VARIABLE result)

  # Display error if failed
  if(NOT ${result} EQUAL 0)
    message(STATUS "Failed to update rpath of ${file} to ${rpath} (error ${result})")
  endif()

endif()

endfunction()

# Description
#   This function reads the rpath
#   of the given file
#
# Parameters
#   file  - The path to the file
#   rpath - The variable where to store the rpath
#
function(read_rpath file rpath)

  find_program(PATCHELF patchelf)

  if(PATCHELF STREQUAL "PATCHELF-NOTFOUND")
    return()
  endif()

  # Only process realfiles
  if(NOT IS_SYMLINK ${file})

    # Update it
    execute_process(COMMAND ${PATCHELF} --print-rpath "${file}"
                    OUTPUT_VARIABLE current_rpath
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    RESULT_VARIABLE result)

    # Display error if failed
    if(NOT ${result} EQUAL 0)
      message(STATUS "Failed to read rpath of ${file}")
      set(current_rpath "")
    endif()
  endif()

  set(${rpath} "${current_rpath}" PARENT_SCOPE)

endfunction()

# Description
#   This function appends the rpath token
#   to the rpath list of the given file
#
# Parameters
#   file    - The path to the file
#   rpath   - The rpath to append
#   prepend - The flag to indicate if we should prepend (append otherwise)
#
function(add_to_rpath file rpath prepend)

  read_rpath("${file}" update_rpath)

  # Check if rpath is already in there
  string(FIND "${update_rpath}" "${rpath}" POS)
  
  # Yes : do nothing
  if(NOT POS EQUAL -1)
    return()
  endif()

  # No append it
  if(update_rpath STREQUAL "")
    set(update_rpath "${rpath}")
  else()
    if(prepend)
      set(update_rpath "${rpath}:${update_rpath}")
    else()
      set(update_rpath "${update_rpath}:${rpath}")
    endif()
  endif()

  update_rpath("${file}" ${update_rpath})

endfunction()

# Description
#   This function updates the rpath
#   for all the so files found in 
#   the given directory
#
# Parameters
#   file  - The path to the file
#   rpath - The rpath to update
#
function(update_rpath_for_dir directory rpath)

  # Get the list of files in the directory
  file(GLOB files LIST_DIRECTORIES false "${directory}/*.so*" )

  # Process them
  foreach(file ${files})
    update_rpath("${file}" "${rpath}")
  endforeach()

endfunction()

# Description
#   This function appends the given
#   rpath to the list of rpath
#   for all the files in the given
#   directory
#
# Parameters
#   directory - The path to the directory
#   rpath     - The rpath to update
#
function(append_rpath_for_dir directory rpath)

  # Get the list of files in the directory
  file(GLOB files LIST_DIRECTORIES false "${directory}/*.so*" )

  # Process them
  foreach(file ${files})
    add_to_rpath("${file}" "${rpath}" OFF)
  endforeach()

endfunction()

# Description
#   This function prepends the given
#   rpath to the list of rpath
#   for all the files in the given
#   directory
#
# Parameters
#   directory - The path to the directory
#   rpath     - The rpath to update
#
function(prepend_rpath_for_dir directory rpath)

  # Get the list of files in the directory
  file(GLOB files LIST_DIRECTORIES false "${directory}/*.so*" )

  # Process them
  foreach(file ${files})
    add_to_rpath("${file}" "${rpath}" ON)
  endforeach()

endfunction()