A library for providing sensor data storage to an offline device, with cloud upload capability.
could also provide for local retrieval of last value to be used within device software

# Licensing
 - include a blurb about the licensing restrictions for all shared libs, such as FreeRTOS, sqlite, and curl

# Memory
 -  include a blurb about memory usage, static and heap, requirements, etc.

# Code architecture
TSDB data logging using a relational database
 - a table is used for each datastream
 - a datastream contains a single stream of data, ie from a single sensor
 - each data entry has a timestamp
 - data width is fixed to 32/64 bits (maybe, can it be variable and used as a system log..)
 - data includes a 'valid' flag (what's the point of storing invalid data)
 - each datastream includes metadata to define the source of data
  - organization
  - device model number
  - device serial number
  - stream ID
  - stream description (sensor type, datatype, units)

Local table format
 - sensor data table
  - col1 = timestamp
  - col2 = value

 - device info table
  - col1 = sensor data table reference [sensor_1_data_table]
  - col2 = sensor type [left_motor_speed]
  - col3 = datatype [float32]
  - col4 = units [rpm]


local storage
a task manages a local database and queue. The task is blocked on reading commands from the queue. The user
sends write commands to the queue. Most recent values cached for fast access.
 - sqlite
  - if using queue, sqlite db does not need thread safety; presumably more efficient if compliled without threading lib
  - should all db transactions use sql commands for clarity?
 - API
  - start task
  - register a datastream; initialize(*datastream, init_value)
  - write to datastream;  write(&datastream, value)
  - read last published value; get_last(&datastream)
  - upload (row | table | datastream entry) from local db to cloud db
    - removes entry from local db
    - keeps marker of last uploaded datastream entry
  - reporting functions
    - number of entries in [local | cloud] database for datastream; int32_t n_entries(&datastream)
    - 

cloud storage
 - google cloud
 - front end for data visualization via browser

# creating new project
> idf.py set-target esp32s3
> idf.py build

# configuring components
> idf.py menuconfig

# CLI connection
Connecting to device (Mac):
- open terminal
- type 'screen /dev/tty.usbserial-AL036RY8 115200' 
-   or use ls /dev/tty* to find the name of the port
- type '<ctrl>+a', ':', 'quit' to exit.

Installing FTDI drivers (Mac):
- download VCP driver for ARM from FTDI website.
- copy to applications directory and run installer

Installing serial port driver for esp32 connection (Mac):
- download from https://www.wch.cn/downloads/CH34XSER_MAC_ZIP.html
- mount dmg file and copy to applications folder
- run installer
- device will appear as /dev/tty.wchusbserialxxx

# Installing espressif toolchain
- install prerequisites 
  - brew install cmake ninja dfu-util
  - brew install ccache
- get the ESP IDF (IoT Development Framework)
  - (follow directions from following link to use VSCode ESP-IDF extension)
  - https://github.com/espressif/vscode-esp-idf-extension/blob/HEAD/docs/tutorial/install.md
  - tools downloaded to ~/esp/5.1-rc2/ and
  -                     ~/.espressif/
  - idf_tools.py install-python-evn
  - cd ~/esp/v5.1-rc2/esp-idf
  - ./install.sh esp32s3

# Configuring ESP-IDF   
  - idf.py set-target esp32s3
  - ./export.sh

# Project architecture (ESP-IDF)
Details on esp32 project structure
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


# Building using IDF tools from terminal window
(see https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/linux-macos-setup.html)
- idf.py uses:
-    cmake to configure project
-    ninja to build project
-    esptool.py for flashing project
- change to project directory
  - cd ~/projects/esp32/blink
- set environment variables
  - . ~/esp/v5.1-rc2/esp-idf/export.sh
- configure project
  - idf.py set-target esp32s3
  - idf.py menuconfig
- build project
  - idf.py build
- download to flash
  - idf.py -p /dev/tty.wchusbserial<SN> flash
  - or just idf.py flash
- start CLI monitor
  - idf.py -p /dev/tty.wchusbserial<SN> monitor
  - or just idf.py monitor
  - <ctrl> + ] to end monitor

Note that the entire framework is built for any project, even if the modules are not part of the project.
Only the modules/methods that are used are actually linked into the final binary. This ensures everything 
successfully builds even if it's not used. That unfortunately makes it harder to locally override certain 
files in a project. This smallest unit of code that can be overridden is an entire component directory.
