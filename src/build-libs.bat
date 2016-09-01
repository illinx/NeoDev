@set NEODEV=c:\NeoDev

@set path=%NEODEV%\m68k\bin;%path%

cd libc
make clean
make
cd ..\libcpp
make clean
make
cd ..\libinput
make clean
make
cd ..\libmath
make clean
make
cd ..\libprocess
make clean
make
cd ..\libvideo
make clean
make
cd ..
