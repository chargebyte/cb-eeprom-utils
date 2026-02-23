cmake_minimum_required(VERSION 3.10)

project(cb-eeprom-utils
    VERSION 0.1.0
    DESCRIPTION "Command line tools and library for chargebyte's EEPROM contents"
    LANGUAGES C
)

# tags in this repo are prefixed with 'v' which should be stripped;
# we pipe it through two commands: the first might fail, e.g. no git installed
# or not a git checkout; but the second should succeed in any case
# and we don't want to fail here, but prefer an empty variable
execute_process(
    COMMAND git describe --tags --always
    COMMAND cut -c2-
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL LAST
)

if(GIT_VERSION)
    set(GIT_OR_PROJECT_VERSION "${GIT_VERSION}")
else()
    set(GIT_OR_PROJECT_VERSION "${PROJECT_VERSION}")
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

add_compile_options(-Wall -Wextra -Werror=switch)

add_subdirectory(lib)
add_subdirectory(src)
