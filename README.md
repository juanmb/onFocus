# MoonliteFocuser

An Arduino-based telescope focuser implementing the Moonlite protocol.

Inspired by [Orly Andico's project](http://orlygoingthirty.blogspot.co.nz/2014/04/arduino-based-motor-focuser-controller.html).

The Arduino requires an [EasyDriver](https://www.sparkfun.com/products/12779)
module to control a small stepper motor.


## Required libraries

The [AccelStepper](http://www.airspayce.com/mikem/arduino/AccelStepper) is
required to compile this project.


## Makefile

The code can be compiled and uploaded to the Arduino using `make`.
The provided Makefile requires
[Arduino-Makefile](https://github.com/sudar/Arduino-Makefile) to work.
In Debian/Ubuntu/Mint, you can install it with

    sudo apt-get install arduino-mk
