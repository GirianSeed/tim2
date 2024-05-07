
SHELL       = /bin/sh

TOP         = /usr/local/sce/ee
LIBDIR      = $(TOP)/lib
INCDIR      = $(TOP)/include

TARGET      = tim2smpl
OBJS        = crt0.o \
              main.o tim2.o

LCFILE      = $(LIBDIR)/app.cmd
LIBS        = $(LIBDIR)/libgraph.a \
              $(LIBDIR)/libdma.a \
              $(LIBDIR)/libdev.a \
              $(LIBDIR)/libpkt.a

PREFIX      = ee
AS          = $(PREFIX)-gcc
CC          = $(PREFIX)-gcc
LD          = $(PREFIX)-gcc
DVPASM      = $(PREFIX)-dvpasm
OBJDUMP     = $(PREFIX)-objdump
RUN         = dsedb -r run
RM          = /bin/rm -f

CFLAGS      = -G0 -g -O2 -Wall -Wa,-al -fno-common
CXXFLAGS    = -O2 -Wall -Werror -Wa,-al -fno-exceptions -fno-common
ASFLAGS     = -c -xassembler-with-cpp -Wa,-al
DVPASMFLAGS = -g 
LDFLAGS     = -Wl,-Map,$(TARGET).map -nostartfiles -L$(LIBDIR) -lm
TMPFLAGS    =

.SUFFIXES: .c .s .cc .dvpasm

all: $(TARGET).elf $(OBJS)


strip:
	ee-strip $(TARGET).elf


main.o: main.c tim2.h

tim2.o:	tim2.c tim2.h


$(TARGET).elf: $(OBJS) $(LIBS)
	$(LD) -o $@ -T $(LCFILE) $(OBJS) $(LIBS) $(LDFLAGS)

crt0.o: $(LIBDIR)/crt0.s
	$(AS) $(ASFLAGS) $(TMPFLAGS) -o $@ $< > $*.lst

.s.o:
	$(AS) $(ASFLAGS) $(TMPFLAGS) -I$(INCDIR) -o $@ $< > $*.lst

.dvpasm.o:
	$(DVPASM) $(DVPASMFLAGS) -I$(INCDIR) -o $@ $< > $*.lst

.c.o:
	$(CC) $(CFLAGS) $(TMPFLAGS) -I$(INCDIR) -c $< -o $*.o > $*.lst

.cc.o:
	$(CC) $(CXXFLAGS) $(TMPFLAGS) -I$(INCDIR) -c $< -o $*.o > $*.lst

run: $(TARGET).elf
	$(RUN) $(TARGET).elf

clean:
	$(RM) *.o *.map *.lst core *.dis *.elf
