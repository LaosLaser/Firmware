Embedded software (firmware) for the laser system and components, including configuration files.

## Build setup, using MBED library sources 
(No GCC4MBED required)
### Get your compiler 
Download from: `https://launchpad.net/gcc-arm-embedded` 
(or use the one that comes with your distribution: apt-get install gcc-arm-none-eabi )
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
### Link LaOSlaser as an mbed example project
```
ln -s  ../../../../../laser libraries/tests/net/protocols/
```
### Build LaosLaser:
```
python workspace_tools/make.py -m LPC1768 -t GCC_ARM -n laser
```
### Read http://mbed.org/handbook/mbed-tools for more info
