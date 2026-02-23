add_executable(cb-dt-eeprom
    cb-dt-eeprom.c
)

target_include_directories(cb-dt-eeprom
    PRIVATE
        ${LIBCBEEPROMUTILS_INCLUDE_DIR}
)

target_link_libraries(cb-dt-eeprom
    PRIVATE
        cb-eeprom-utils
)

install(TARGETS cb-dt-eeprom DESTINATION bin)
