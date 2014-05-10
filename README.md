Embedded software (firmware) for the laser system and components, including configuration files.

==Build setup, using MBED library sources (No GCC4MBED requried):
* Get your compiler here: https://launchpad.net/gcc-arm-embedded (or use the one that comes with your
    distribution: apt-get install gcc-arm-none-eabi )
* Download LaosLaser source:
```
git clone --recursive https://github.com/LaosLaser/Firmware.git
```
```
* Patch mbed libraries:
```
cd mbed/
patch -p1 < ../laser/mbed.patch
```
* Set your GCC path in private_settings.py, for example using:
```
echo 'GCC_ARM_PATH = "/usr/bin/"' > workspace_tools/private_settings.py
```
* Build MBED libraries:
```
python workspace_tools/build.py -m LPC1768 -t GCC_ARM -r -e -u -c
```
* Link LaOSlaser as an mbed example project
```
cd Firmware/mbed/libraries/tests/net/protocols/
ln -s  ../../../../../laser .
```
* Build LaosLaser:
```
cd Firmware/mbed
python workspace_tools/make.py -m LPC1768 -t GCC_ARM -n laser
```
* Read http://mbed.org/handbook/mbed-tools for more info
