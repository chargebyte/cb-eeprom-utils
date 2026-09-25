configure_file(version.h.in version.h)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/version.h
    DESTINATION "include/cb-eeprom-utils"
)

set(LIBCBEEPROMUTILS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)
set(LIBCBEEPROMUTILS_INCLUDE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)

add_library(cb-eeprom-utils SHARED)

target_sources(cb-eeprom-utils
    PRIVATE
        "cb-eeprom.c"
        "crc32.c"
        "tools.c"
)

install(
    FILES
        "cb-eeprom-utils.h"
        "cb-eeprom.h"
        "cb-dt-eeprom.h"
        "crc32.h"
        "tools.h"
    DESTINATION
        "include/cb-eeprom-utils"
)

set_target_properties(cb-eeprom-utils
    PROPERTIES
       VERSION ${PROJECT_VERSION}
       SOVERSION 1
)

set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR})

install(
    TARGETS cb-eeprom-utils
    EXPORT cb-eeprom-utilsTargets
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT cb-eeprom-utilsTargets
    FILE cb-eeprom-utilsTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cb-eeprom-utils
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cb-eeprom-utilsConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cb-eeprom-utilsConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cb-eeprom-utils
    PATH_VARS INCLUDE_INSTALL_DIR
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/cb-eeprom-utilsConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cb-eeprom-utils
)

# pkg-config file generation
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cb-eeprom-utils.pc.in cb-eeprom-utils.pc @ONLY)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/cb-eeprom-utils.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
