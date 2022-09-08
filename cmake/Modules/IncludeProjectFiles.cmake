# Will not be built, but it will list all project files in the IDE.
# - target: Name of a ${target}_unbuilt that will be created.
# - regexes: Patterns of files to find.  May even be paths.
macro( INCLUDE_PROJECT_FILES target path_globs )
  foreach(path_glob ${path_globs})
    file(GLOB_RECURSE _hdrs "${path_glob}")
    list(APPEND _INCLUDE_PROJECT_HEADERS "${_hdrs}")
  endforeach()
  add_custom_target("${target}_unbuilt" SOURCES ${_INCLUDE_PROJECT_HEADERS})
endmacro()
