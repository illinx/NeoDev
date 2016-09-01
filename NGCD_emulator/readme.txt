NEOCD/SDL v0.3.1  - the SDL neogeo CD emulator
================
http://www.geocities.com/neocdsdl
http://members.xoom.virgilio.it/neocdsdl

neocdsdl at yahoo dot com

NeoCD/SDL is a port of NeoCD using SDL libraries.
SDL portions written by Fosters(2001,2003-2004).

Original NeoCD(DOS Version) by Martinez Fabrice



About
=====
NeoCD/SDL is a port of the NeoCD neogeo CD emulator to the multiplatform SDL 
library.

It is capable of running many original Neogeo CD games 

It is currently capable of compiling and running on Windows, Linux and BeOS and the Dreamcast

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.



Disclaimer
==========
This program requires a system rom (bios) from the original Neogeo CD console to
function.  This code is copyrighted and therefore you are not entitled to a 
copy unless you own an original Neogeo CD console.

Do not enquire where to find this file.  Suffice to say that it should be called
neocd.bin and should be 512kb in size.

A smaller version of this bios file exists on the internet, however this will no
longer work with NeoCD/SDL(v 0.2.0 onwards)

NEOCD IS FREE, SOURCE CODE IS FREE. SELLING IS NOT ALLOWED.
YOU CANNOT PROVIDE NEOCD AND NEOGEO GAME SOFTWARE ON THE SAME PHYSICAL MEDIUM.
 
YOU CAN REUSE SOURCE CODE AND TECHNICAL KNOWLEDGE AS LONG AS IT IS NOT FOR 
COMMERCIAL PURPOSES.




Usage
=====
Unpack all the files from the archive to a directory.  Ensure the neocd bios is 
called "neocd.bin" and add it to the same directory (it should not be compressed
- it will be 512kb in size).

Insert a neogeo cd into one of your cd drives and ensure that it is mounted.

Run the neocd exectuable.

If all goes well NeoCD/SDL should detect the cd drive that the cd is in and run 
the game.

**** When running on windows all textual output from the program goes into 
*NB* stdout.txt instead of to the console.  You should examine this file when 
**** things go wrong.





Controls
========
Player 1 may be controlled using either the keyboard or the first joystick 
attached to the system.

Player 2 may be controlled using either the keyboard or the second joystick 
attached to the system.

The joypad buttons have been allocated to suit my MS sidewinder gamepads.

The controls roughly follow the default layout for the MAME emulator. 
As such they are as follows:

                     Player 1       Player 2    Joystick(1 and 2)
--------------------------------------------------------------		   
Up                   Cursor-Up      R           Joy-Up
Down                 Cursor-Down    F           Joy-Down
Left                 Cursor-Left    D           Joy-Left
Right                Cursor-Right   G           Joy-Right

A                    Left-Ctrl      A           Button-0
B                    Left-Alt       S           Button-1
C                    Space          Q           Button-2
D                    Left-Shift     W           Button-3

Player Start         1              2           Button-8

Player Select        5              6           Button-9

Toggle Fullscreen    F1
Cycle Video Effect   F2
Increase Frameskip   F3
Toggle Sound         F4

Take Screenshot      F12
(snapxxxx.bmp)

Quit                 Esc 
----------------------------------------------------------------

The player select buttons can often be used to pause the game.

To modify the controls it is necessary to recompile the software.


Credits
=======
Fabrice Martinez                 - The original NeoCD emulator
Fosters                          - SDL port

Karl Stenerud                    - Motorola 68000 32 Bit emulator
Neil Bradley (neil@synthcom.com) - Multi-Z80 CPU emulator
Tatsuyuki Satoh                  - YM2610 Mame core

Derek Liauw Kie Fa.              - 2xSaI engine  

Sam Lantinga (et al)             - Simple DirectMedia Layer



Acknowledgements (in no particular order)
=================
Mathieu Peponas     - The GNGEO source was really useful for information and
                      was really open about licencing files to be compatible 
                      with NeoCD/SDL.

Karl Stenerud       - MUSASHI Motorola M680x0 processor emulation engine

Neil Bradley        - Multi-Z80 CPU emulator
(neil@synthcom.com)

The MAME team       - For an incredible emulator, a source tree full of useful
                      information, and for sucking away more of my time than any
                      other computer program in years 8)

Sam Lantinga et al. - For the amazing libSDL cross-platform C library.

Snes9x              - Where I found the 2xSaI code.

DC Scene            - Thanks for the help translating asm to C and the DC port

Everyone Else       - Thanks for taking an interest.



Enjoy!


Fosters
neocdsdl at yahoo dot com

