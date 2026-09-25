# chargebyte's EEPROM Utilities

This repository contains some command line tools related for chargebyte's
EEPROM on various chargebyte hardware platforms.

Tools included so far are:

- **cb-dt-eeprom**: This tool allows to create an image for the Device Tree EEPROM
  which is recommended for custom carrier boards and is also present on most
  chargebyte's own carrier boards.
- **cb-eeprom**: This tool reads and prints the EEPROM contents of chargebyte
  hardware in a shell-compatible format.

`cb-eeprom` checks the SOM EEPROM first and then the Mint/Doublemint EEPROM.
The EEPROM paths can be overridden with `--som-eeprom`, `--cb-eeprom`, and
`--dt-eeprom`; without these options the platform default paths are used.
When a carrier-board EEPROM contains the default hardware revision, a valid
hardware revision from the DT EEPROM is preferred.
For example:

    cb-eeprom
    cb-eeprom som_pcb_dmc
    cb-eeprom -n cb_serial

Without `-n`, output has the form `variable=value`. With `-n`, only the value
is printed. Numeric DMCs are printed as 16 digits when their final four bytes
are zero, otherwise as 24 digits. DMCs beginning with `PT` are interpreted as
ASCII strings and may either be NUL-terminated or occupy all twelve bytes.
Serial numbers are printed without leading zeroes.

## Building and Installation on the Target

Since this project is quite small and has no dependencies, it is possible
to compile it on the target itself. Here is an example transcript:

    git clone https://github.com/chargebyte/cb-eeprom-utils.git

    mkdir cb-eeprom-utils/build

    cd cb-eeprom-utils/build

    CMAKE_INSTALL_PATH_DEFINES=" \
          -DCMAKE_INSTALL_PREFIX:PATH=/usr \
          -DCMAKE_INSTALL_BINDIR:PATH=/usr/bin \
          -DCMAKE_INSTALL_SBINDIR:PATH=/usr/sbin \
          -DCMAKE_INSTALL_LIBEXECDIR:PATH=/usr/libexec  \
          -DCMAKE_INSTALL_SYSCONFDIR:PATH=/etc \
          -DCMAKE_INSTALL_SHAREDSTATEDIR:PATH=/var/share \
          -DCMAKE_INSTALL_LOCALSTATEDIR:PATH=/var \
          -DCMAKE_INSTALL_LIBDIR:PATH=/usr/lib \
    "
    export CMAKE_INSTALL_PATH_DEFINES

    cmake \
          $CMAKE_INSTALL_PATH_DEFINES \
          ..

    make -j$(nproc)

    make install

Remember, that the tool is already pre-installed on chargebyte's distributions.
The very same procedure can be used on a host system, too.
