# Terrapin

A project for recording sensor data using an ESP32-S3.

SPDX-FileCopyrightText: Copyright © 2024 Honulanding Software <dev@honulanding.com>  
SPDX-License-Identifier: Apache-2.0

## Licensing

- The ESP ecosystem is licensed under the terms of the ASF Apache 2.0.
- The source files unique to this project adopt the same license.
- linenoise_lite inherits the BSD 2-Clause simplified license from the linenoise library upon which it is based.

## Installing espressif toolchain

A quick guide for getting started with the espressif tools (Mac)
- install prerequisites 
  - `brew install cmake ninja dfu-util`
  - `brew install ccache`
- get the ESP IDF (IoT Development Framework)
  - (follow directions from following link to use VSCode ESP-IDF extension)
  - https://github.com/espressif/vscode-esp-idf-extension/blob/HEAD/docs/tutorial/install.md
  - the tools are downloaded to ~/esp/5.1-rc2/ and ~/.espressif/
  - `idf_tools.py install-python-evn`
  - `cd ~/esp/v5.1-rc2/esp-idf`
  - `./install.sh esp32s3`

## Configuring ESP-IDF

  - `idf.py set-target esp32s3`
  - `./export.sh`

## CLI connection

The ESP32-S3 dev board has a USB connector for terminal access. The datalogger provides a command line interface using this port for debugging, system configuration, and monitoring. The port is configured for 115200 baud.

Installing FTDI drivers (Mac):
- download VCP driver for ARM from FTDI website.
- copy to applications directory and run installer

Installing serial port driver for esp32 connection (Mac):
- download from https://www.wch.cn/downloads/CH34XSER_MAC_ZIP.html
- mount dmg file and copy to applications folder, then run the installer
- device will appear as /dev/tty.wchusbserialxxx

Connecting to device (Mac):
- using terminal
  - open terminal
  - `screen /dev/tty.usbserial-AL036RY8 115200`
  -  or use `ls /dev/tty*` to find the name of the port
  - `<ctrl>+a`, `:`, `quit` to exit.
- using the python based ESP-IDF monitor
  - open a command shell
  - `idf.py monitor`
  - `<ctrl>+]` to exit.

## ESP-IDF Project architecture

Some notes on the structure of an ESP-IDF project
- kconfig is used to configure which elements to include in the project
-   details are stored in sdkconfig
-   idf.py menuconfig provides GUI for updating configuration
- the app has multiple components
-   building a project creates a main executable
-   it also creates a bootloader executable
-   it may create static libraries (.a files)
- ESP-IDF is not in the project heirarchy
-   saved to ~/esp/v5.1-rc2/esp-idf
- The compiler toolchain is not in the project heirarchy
-   saved to ~/.espressif/tools/xtensa-esp32s3-elf/esp-12.2.0_20230208/xtensa-esp32s3-elf/bin

## Building using IDF tools from terminal window

[see this espressif doc for details](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/linux-macos-setup.html)  
- idf.py uses:
  - cmake to configure project
  - ninja to build project
  - esptool.py for flashing project
- change to project directory  
`cd ~/projects/esp32/blink`  
- set environment variables  
  `. ~/esp/v5.1-rc2/esp-idf/export.sh`
- configure project  
  `idf.py set-target esp32s3`  
  `idf.py menuconfig`  
- build project  
  `idf.py build`  
- download to flash  
  `idf.py flash`  
- connect to CLI interface  
  `idf.py -p /dev/tty.wchusbserial<SN> monitor`  
  `idf.py monitor`  
- disconnecting from CLI  
  `<ctrl> + ]`  

Note that the entire framework is built for any project, even if the modules are not part of the project. Only the methods that are used are actually linked into the final binary. This ensures everything successfully builds even if it's not used. That unfortunately makes it harder to locally override certain files in a project. This smallest unit of code that can be overridden is an entire component directory.
