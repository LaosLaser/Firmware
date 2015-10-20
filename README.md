Embedded software (firmware) for the laser system and components, including configuration files.

## Build setup, using MBED library sources 
(No GCC4MBED required)
### Get your compiler 
Download from: `https://launchpad.net/gcc-arm-embedded` 
(or use the one that comes with your distribution: apt-get install gcc-arm-none-eabi ). On Ubuntu 14.04 64 bit, you need to install libc6:i386.
### Download LaosLaser source:
```
git clone --recursive https://github.com/LaosLaser/Firmware.git
```
### Patch mbed libraries:
```
cd Firmware/mbed/
patch -p1 < ../laser/mbed.patch
```
### Set your GCC path 
Set the path to the gcc compiler in a `workspace_tools/private_settings.py` file. Make sure you end with the bin folder. 
For example using:
```
echo 'GCC_ARM_PATH = "home/usr/gcc-arm-none-eabi-4_8-2014q1/bin/"' > workspace_tools/private_settings.py
```
### Build MBED libraries:
```
python workspace_tools/build.py -m LPC1768 -t GCC_ARM -r -e -u -c
```
You might need to install the python-colorama package for this to work.
### Link LaOSlaser as an mbed example project
```
cd libraries/tests/net/protocols/
ln -s  ../../../../../laser .
cd ../../../..
```
### Build LaosLaser:
```
python workspace_tools/make.py -m LPC1768 -t GCC_ARM -n laser
```
### Link IOtest as an mbed exmaple project
```
cd libraries/tests/net/protocols/
ln -s ../../../../../iotest_board/ iotest
cd ../../../..
```

### Build IOtest:
```
python workspace_tools/make.py -m LPC1768 -t GCC_ARM -n iotest
```

### Attach debugger for step-by-step debugging
```
arm-none-eabi-gdb build/test/LPC1768/GCC_ARM/laser/laser.elf --eval-command \
    'target remote | openocd -c "gdb_port pipe" --file /usr/share/openocd/scripts/interface/cmsis-dap.cfg --file /usr/share/openocd/scripts/target/lpc1768.cfg -cinit -chalt'
```
This starts gdb and lets gdb start openocd to connect to the board. It
seems you can only attach after startup. Normally, you can also use `-c
'reset halt'` to reset the board and start debugging from the start, but
since the startup procedure uses "semihost" calls to read config.txt
from the flash, this generates dozens of `SIGTRAP` interrupts, making
debugging effectively useless.

### Read http://mbed.org/handbook/mbed-tools for more info
