# FakeJoystick
Fake joystick module

(C) Chris Bazley, 2002

Version 2.01 (15th March 2003)

-----------------------------------------------------------------------------
Introduction and purpose
========================

  It is difficult for games programmers to implement support for joysticks without access to actual joystick hardware. This module has been written to help (specifically to allow me to test the new joystick support in Star Fighter 3000).

  Machines that don't have built-in joystick hardware (everything except the A3010?) require an interface and software drivers, in the form of a module that implements the standard Joystick SWIs. The cost of such an interface and a decent joystick to go with it is quite expensive.

  If, like me, you are too miserly to pay for the real thing, this module may provide the solution. It allows games to be tested against a 'virtual' (emulated) joystick, which is controlled using the numeric keypad. Not particularly useful for playing games, but for testing purposes certainly far better than nothing.

  The fake Joystick module has been tested with Doom+ (in switched mode) and Star Fighter 3000 (in switched and analogue modes).

-----------------------------------------------------------------------------
Requirements
============

  To run the fake Joystick module you need RISC OS 3.10 or newer, and up-to-date versions of the modules CallASWI (for OS < 3.7) and SharedCLibrary (because the module is written using Acorn C and CMHG).

  If an error of the form "SWI value out of range for module SharedCLibrary" is generated upon loading the fake Joystick module then your SharedCLibrary is out of date - you need at least version 5.17, implementing the new SWIs LibInitAPCS_32 and SharedCLibrary_LibInitModuleAPCS_32. The latest version is available from my website and other sources.

  This Joystick module should be compatible with future 32-bit versions of RISC OS, though this is currently untested.

-----------------------------------------------------------------------------
Keyboard controls:
==================

| Simulated action | Key
|------------------|-------------
| Stick left       | Keypad 4
| Stick right      | Keypad 6
| Stick forward    | Keypad 8
| Stick back       | Keypad 2
| Centre stick     | Keypad 5
| Fire button A    | Keypad enter
| Fire button B    | Keypad +

-----------------------------------------------------------------------------
About the joystick emulation
============================

  Only joystick 0 (the first joystick) is emulated - attempting to read the status of other joysticks will return centred x/y values and buttons clear. The emulated joystick can be configured to be either switched (Atari) or analogue (PC), using the command
```
    *FakeJSType [analogue|switched|damped]
```
With no arguments, *FakeJSType displays the current setting, which defaults to "switched" upon initialisation.

  When in "switched" mode, keypresses cause subsequent calls to Joystick_Read to return discrete values indicating whether the imaginary joystick is left/back (-64), right/forward (+64) or centred (0). Holding a key down has the equivalent effect to holding a switched joystick over in that direction - the x and y values do not return to 0,0 until all keys are released, at which point they snap back instantaneously.

  When in "analogue" mode, keypresses move the imaginary joystick linearly within the full range of values between -127 and +127. Because the imagined stick can be hard to centre in this mode, you can press Keypad 5 to immediately return the stick to the neutral x=0,y=0 position. The full range should be traversable in approximately 2 seconds (in steps of 5).

  "Damped" mode also emulates an analogue joystick, but in a more natural way than the simple linear movement provided by "analogue" mode. Most joysticks nowadays are sprung so that they return to neutral position when released (some early analogue sticks were not, such as the official BBC Microcomputer peripherals). This emulation mode attempts to simulate this effect, in that when a directional key is released the values returned by Joystick_Read decay gradually to the 0,0 position.

-----------------------------------------------------------------------------
Emulation of RISC OS 3.6 extensions
===================================

  In RISC OS 3.6 Acorn's Joystick module was extended with new SWIs and reason codes to provide support for PC-style analogue joysticks, and also to emulate the OS_Byte calls used to access the ADC port on the old I/O podule.

  The I/O podule OS_Byte calls 16,17,128,188,189 and 190 are not implemented by this fake Joystick module.

  If the new Joystick_Read reason code is set to 1 (return 16 bit joystick state) then the new behaviour is emulated by the fake Joystick module, providing that the joystick type is first configured to "analogue" or "damped".

  Note however that the actual accuracy of the 16-bit values returned by Joystick_Read 1 is no better than the 8-bit values, except in "damped" analogue mode. Also, the conversion to 16-bit unsigned values isn't perfect, so the actual range of values returned is only 255-65279 rather than 0-65535.

  The new SWIs Joystick_CalibrateBottomLeft and Joystick_CalibrateTopRight are supported, providing that the fake joystick is first configured to "analogue" or "damped". They do not actually do anything except set a flag that causes Joystick_Read to return a "Calibration incomplete" error until the other of the pair is called - according to volume 5a of the PRMs this is authentic behaviour.

  If the joystick emulation type is set to "switched", then the new calibration SWIs and Joystick_Read 1 will cause the error "Operation not supported by switched joystick" to be returned. I don't actually know whether Acorn's new Joystick module would do this, but the PRMs seem to imply that the new operations are only available for analogue sticks. Old Joystick modules would presumably return some kind of error.

-----------------------------------------------------------------------------
Star Commands
=============
```
*FakeJSType [analogue|switched|damped]
```
Configures the type of the emulated joystick, or with no arguments displays the current setting.

-----------------------------------------------------------------------------
Joystick SWIs
=============
The following is a brief summary of the more detailed documentation in the PRMs.

Joystick_Read (SWI &43F40)
--------------------------
Reads the current state of a joystick.
```
On entry:
  R0 = joystick number and reason code:
       bits 0-7   - joystick number (0 = first, 1 = second, etc)
       bits 8-15  - reason code:
         0 - read 8-bit state of a switched or analogue joystick
         1 - read 16-bit state of an analogue joystick
       bits 16-31 - reserved (0)

On exit:
  Registers depend on reason code (see below)
```
Joystick_Read 0
---------------
Reads the 8-bit state of a switched or analogue joystick.
```
On exit:
  R0 = 8-bit joystick state:
       bits 0-7  - signed 8-bit y value in the range -127 (back) to +127 (forward)
       bits 8-15 - signed 8-bit x value in the range -127 (left) to +127 (right)
       bits 16-23 - fire buttons (bits set reflect buttons pushed)
       bits 24-31 - reserved (0)
```
  When reading only forward/back/left/right state, it is recommended that the 'at rest' state should span a middle range (say from -32 to +32) since analogue joysticks do not reliably produce the value 0 when in a neutral position.

Joystick_Read 1
---------------
Reads the 16-bit state of an analogue joystick.
```
On exit:
  R0 = 16-bit joystick position:
       bits 0-15  - 16-bit y value in the range 0 (back) to 65535 (forward)
       bits 16-31 - 16-bit x value in the range 0 (left) to 65535 (right)
  R1 = fire buttons:
       bits 0-7  - fire buttons (bits set reflect buttons pushed)
       bits 8-31 - reserved (0)
```
  When reading only forward/back/left/right state, it is recommended that the 'at rest' state should span a middle range (say from 24576 to 40960) since analogue joysticks do not reliably produce the value 32767 when in a neutral position.

Joystick_CalibrateTopRight (SWI &43F41)
---------------------------------------
Part of analogue joystick calibration procedure.
```
On entry:
  --

On exit:
  --
```
  To calibrate an analogue joystick, call this SWI (with the stick held in the forward right position) and then Joystick_CalibrateBottomLeft. After calling only one of the pair Joystick_Read will return a error until the calibration process is properly completed.

Joystick_CalibrateBottomLeft (SWI &43F42)
-----------------------------------------
Part of analogue joystick calibration procedure.
```
On entry:
  --

On exit:
  --
```
  To calibrate an analogue joystick, call this SWI (with the stick held in the back left position) and then Joystick_CalibrateTopRight. After calling only one of the pair Joystick_Read will return a error until the calibration process is properly completed.

-----------------------------------------------------------------------------
Errors
======

| Number  | Message                                         | Meaning
|---------|-------------------------------------------------|-----------------------------------------------------
| &81A720 | "Joystick module cannot claim memory"           | Must abort command because memory allocation failed.
| &81A730 | "Joystick_Read reason code not supported"       | Joystick_Read has been called with a reason code (bits 8-15) other than the two recognised values of 0 or 1.
| &81A731 | "Operation not supported for switched joystick" | Attempt to read 16-bit joystick state or calibrate joystick when in "switched" emulation mode.
| &81A732 | "Joystick calibration incomplete"               | Attempt to read joystick status after issuing one of the pair of SWIs Joystick_CalibrateBottomLeft/Joystick_CalibrateTopRight without the other.

-----------------------------------------------------------------------------
Writing joystick code
=====================

  A very simple test program written in BASIC is supplied with the fake Joystick module ("TestJS"). This demonstrates decoding of the packed 8-bit or 16-bit values returned by the module, and the effects of the various joystick emulation modes. You could also use it as a more general test program for joystick hardware, if you removed the *FakeJSType commands.

  If a middle range of values, conventionally from -32 to +32, is treated as the 'at rest' state then the same code can be used to read both analogue and switched joysticks. Conversely, the discrete -64/0/+64 values returned by switched sticks can (at a pinch) be used to control games designed for analogue control.

   In practice however, neither of these situations are terribly satisfactory - switched joystick control of a flight simulator designed for analogue control is likely to be extremely crude, and it can be hard to anticipate the exact activation threshold of an analogue stick when playing an arcade game. You should make clear (in your game documentation) whether analogue or switched sticks are recommended.

  For games with anything more complex than crude left-right-up-down controls, switched joysticks (and keyboard controls) may benefit from the kind of damping implemented by this module. This will require separate support code for analogue and switched joysticks, with user selection of the joystick type. Analogue sticks can then be used as an alternative means of exerting direct control over the game.

  It may be best to steer clear of the RISC OS 3.6 Joystick extensions altogether, or at least allow for the possibility that an old Joystick module is in use. The extra 16-bit accuracy is not particularly necessary for most applications, but joystick calibration is a neat feature. You can check the robustness of your code when it comes to dealing gracefully with old Joystick modules by configuring "*FakeJSType switched".

-----------------------------------------------------------------------------
Technical details
=================

  The module has been given the standard name "Joystick" rather than "FakeJoystick" or anything else distinguishing, so that applications that *RMEnsure Joystick before using the Joystick SWIs will correctly detect its presence.

  The state of the emulated joystick is maintained in real-time, with calls to Joystick_Read just grabbing the current x/y values and buttons status. Therefore there is some processor load (very little, in switched joystick mode) all the time that the module is loaded.

  A routine is installed on the event vector to watch for key transition events (press/release). In switched joystick emulation mode keypresses update the x/y stick values directly (-64/0/+64), whilst in analogue mode the current keys status is stored for later use by a callback routine. The event vector is released upon killing the module.

  When in analogue emulation mode a callback routine is registered with OS_CallEvery. This routine updates the x/y joystick values every 4 centiseconds (25 times a second), according to the current keys pressed status. Note that this timing is approximate, since callbacks only occur when RISC OS is not 'busy'.

  In "analogue" mode the x and y values are updated linearly over the time that a key is held. In "damped" mode, higher resolution fractional values (exponent 10) are updated according to both the keys pressed status and a gradual decay function. This algorithm was taken from Star Fighter 3000's key handling code, which I believe was written by T. D. Parry.

  Upon reverting to "switched" mode or killing the module the callback routine is removed using OS_RemoveTickerEvent.

  To compile and link the module you need the standard library headers and the Shared C Library stubs. Acorn's CMHG (C module header generator) tool is needed to generate the module header and veneers. To generate the error blocks, the makefile invokes Nick Roberts' simple ARM assembler 'ASM', but Acorn's ObjAsm could probably be used instead.

  The makefile is set up to compile APCS-32 code using Castle Technology's new release of the Acorn C/C++ RISC OS development suite. Since APCS-R code is incompatible with 32-bit modes (and hence new ARM CPUs), I consider it obsolescent.

  Before compiling the module for RISC OS, move the files with .a, .cmhg, .c and .h suffixes into subdirectories named 'a', 'cmhg', 'c' and 'h' and remove those suffixes from their names. You probably also need to create an 'o' subdirectory for compiler output.

-----------------------------------------------------------------------------
To do
=====

- Implement mouse control of emulated analogue joystick - should provide a better control method than keys, but might be difficult to implement without control over the screen mode dimensions or current pointer position.

- Allow key redefinition. Using the numeric keypad has the advantage that it is rarely used for keyboard controls by games, but the current layout is rather awkward and it is possible to provoke key clashes.

- Emulation of more than one joystick / joysticks other than 0.

-----------------------------------------------------------------------------
History
=======

2.00 (12th June 2002)
- Initial release.

2.01 (11th November 2002)
- Recompiled using the official Castle release of 32-bit Acorn C/C++.

15th March 2003
- Recompiled without stack-limit checking and embedded function names (both redundant in SVC mode), saving a little time and memory.

(The PRMs say that third party replacements for the Joystick module should have version numbers greater than 2.00 so that Acorn can upgrade their own module.)

-----------------------------------------------------------------------------
Disclaimer
==========

  This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details (in the text file "GNUlicence").

-----------------------------------------------------------------------------
Contact details
===============

  Feel free to contact me with any bug reports, suggestions or anything else.

  Email: mailto:cs99cjb@gmail.com

  WWW:   http://starfighter.acornarcade.com/mysite/
