# - Try to find openmpt
# Once done this will define
#
# OPENMPT_FOUND - system has openmpt
# OPENMPT_INCLUDE_DIRS - the openmpt include directory
# OPENMPT_LIBRARIES - The openmpt libraries

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules (OPENMPT libopenmpt)
  list(APPEND OPENMPT_INCLUDE_DIRS ${OPENMPT_INCLUDEDIR})
endif()

if(NOT OPENMPT_FOUND)
  find_path(OPENMPT_INCLUDE_DIRS libopenmpt/libopenmpt.h)
  find_library(OPENMPT_LIBRARIES NAMES openmpt)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Openmpt DEFAULT_MSG OPENMPT_INCLUDE_DIRS OPENMPT_LIBRARIES)

mark_as_advanced(OPENMPT_INCLUDE_DIRS OPENMPT_LIBRARIES)
