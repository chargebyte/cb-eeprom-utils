add_executable(cb-dt-eeprom
    cb-dt-eeprom.c
)

add_executable(cb-eeprom
    cb-eeprom.c
)

target_include_directories(cb-dt-eeprom
    PRIVATE
        ${LIBCBEEPROMUTILS_INCLUDE_DIR}
        ${LIBCBEEPROMUTILS_INCLUDE_BINARY_DIR}
)

target_link_libraries(cb-dt-eeprom
    PRIVATE
        cb-eeprom-utils
)

target_include_directories(cb-eeprom
    PRIVATE
        ${LIBCBEEPROMUTILS_INCLUDE_DIR}
        ${LIBCBEEPROMUTILS_INCLUDE_BINARY_DIR}
)

target_link_libraries(cb-eeprom
    PRIVATE
        cb-eeprom-utils
)

install(TARGETS cb-dt-eeprom cb-eeprom DESTINATION bin)
