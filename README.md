# chargebyte's EEPROM Utilities

This repository contains some command line tools related for chargebyte's
EEPROM on various chargebyte hardware platforms.

Tools included so far are:

- **cb-dt-eeprom**: This tool allows to create an image for the Device Tree EEPROM
  which is recommended for custom carrier boards and is also present on most
  chargebyte's own carrier boards.

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
