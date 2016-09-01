
makeroms.bat (in the m68k\bin directory)
------------
This batch file converts your compiled binaries to rom images
suitable for testing in MAME. It currently creates a Puzzle De Pon
romset (202-??.bin) and copies them to a puzzledp subdirectory
under your MAME installation. You will need to edit this batch
file to change the 'copy' statement to copy to your  
ROMS\PUZZLEDP directory under MAME.

------------

Finally, once you have done the above you can compile the 
libraries and samples. Under the 'src' directory there are two
batch files to make this easier. 'Build-Libs' and 'Build-Samples'
you must run 'Build-Libs' first as the samples demonstrate the
use of the library.

