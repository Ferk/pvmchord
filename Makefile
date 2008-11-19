
PVM_ARCH= LINUX64
PVMDIR= ~/pvm3/bin/$(PVM_ARCH)
XDIR= ..
CFLOPTS= -g
CFLAGS= $(CFLOPTS) -I/usr/include/ $(ARCHCFLAGS)  -ggdb
PVMLIB= -lpvm3 -lgpvm3

CC= gcc


all:
	for file in *.c; do
	make $(echo $file | sed s/".c"//)
	done

%: %.c
	$(CC) $(CFLAGS) -o $@ $@.c $(PVMLIB)
	mv $@ $(PVMDIR)


# Flymake syntax checking
check-syntax: 
	$(CC) -o nul -Wall -S $(CHK_SOURCES)



