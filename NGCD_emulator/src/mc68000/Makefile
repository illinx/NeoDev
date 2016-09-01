##########################
# Makefile for 68k Core  #
##########################

#setup C Flags
CFLAGS= -I. -I./obj -O3 -fomit-frame-pointer -fno-ident -funsigned-char 

# 68k instructions file
#INFILE = m68k_in.c
INFILE = neocd68k.c


all: obj mc68000.a

clean:
	rm -rf obj
	rm -f mc68000.a

obj:
	mkdir obj


mc68000.a: obj/m68kcpu.o obj/m68kops.o obj/m68kopac.o obj/m68kopdm.o obj/m68kopnz.o
	$(AR) -r $@ $^

obj/m68kcpu.o: obj/m68kops.h m68k.h m68kconf.h
	$(CC) $(CFLAGS) -c m68kcpu.c -o obj/m68kcpu.o

obj/m68kops.o: obj/m68kmake.exe obj/m68kops.h obj/m68kops.c m68k.h m68kconf.h
	$(CC) $(CFLAGS) -c obj/m68kops.c -o obj/m68kops.o

obj/m68kopac.o: obj/m68kmake.exe obj/m68kops.h obj/m68kopac.c m68k.h m68kconf.h
	$(CC) $(CFLAGS) -c obj/m68kopac.c -o obj/m68kopac.o

obj/m68kopdm.o: obj/m68kmake.exe obj/m68kops.h obj/m68kopdm.c m68k.h m68kconf.h
	$(CC) $(CFLAGS) -c obj/m68kopdm.c -o obj/m68kopdm.o

obj/m68kopnz.o: obj/m68kmake.exe obj/m68kops.h obj/m68kopnz.c m68k.h m68kconf.h
	$(CC) $(CFLAGS) -c obj/m68kopnz.c -o obj/m68kopnz.o

obj/m68kops.h: obj/m68kmake.exe
	obj/m68kmake.exe obj $(INFILE)

obj/m68kmake.exe: m68kmake.c $(INFILE)
	$(CC) $(WARNINGS) m68kmake.c -o obj/m68kmake.exe
