# Config file for the RedisClient package
#
# Usage from an external project:
#  In your CMakeLists.txt, add these lines:
#
#  find_package(RedisClient REQUIRED)
#  include_directories(${RedisClient_INCLUDE_DIRS})
#  link_directories(${RedisClient_LIBRARY_DIRS})
#  target_link_libraries(MY_TARGET_NAME ${RedisClient_LIBRARIES})
#
# This file defines the following variables
#  -RedisClient_INCLUDE_DIRS  :Include directories for RedisClient
#  -RedisClient_LIBRARIES     :Libraries to link against
#  -RedisClient_LIBRARY_DIRS  :Directories to add for libraries
#  -RedisClient_FOUND	       :Found status

set(RedisClientInstall ON)
include(${CMAKE_CURRENT_LIST_DIR}/RedisClientDependencies.cmake)

# Our library dependencies (contains definitions for IMPORTED targets)
include(${CMAKE_CURRENT_LIST_DIR}/RedisClientTargets.cmake)


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was Config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

set_and_check(RedisClient_INCLUDE_DIRS "${PACKAGE_PREFIX_DIR}/include")
set_and_check(RedisClient_LIB_DIR "${PACKAGE_PREFIX_DIR}/")
check_required_components(RedisClient)

# Set found status
set(RedisClient_FOUND TRUE) 
set(RedisClient_INCLUDE_DIRS ${RedisClient_INCLUDE_DIRS} ${INCLUDE_DIRS})
set(RedisClient_LIBRARY_DIRS ${RedisClient_LIB_DIR} ${LIBRARY_DIRS})
set(RedisClient_LIBRARIES ${RedisClient_LIBS_LIST})
