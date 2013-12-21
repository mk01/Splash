GCC = $(CROSS_COMPILE)gcc
SYS := $(shell $(GCC) -dumpmachine)
ifneq (, $(findstring x86_64, $(SYS)))
	OSFLAGS = -Ofast -fPIC -march=native -mtune=native -mfpmath=sse -Wconversion -Wunreachable-code -Wstrict-prototypes 
endif
ifneq (, $(findstring arm, $(SYS)))
	ifneq (, $(findstring gnueabihf, $(SYS)))
		OSFLAGS = -Ofast -mfloat-abi=hard -mfpu=vfp -march=armv6 -Wconversion -Wunreachable-code -Wstrict-prototypes 
	endif
	ifneq (, $(findstring gnueabi, $(SYS)))
		OSFLAGS = -Ofast -mfloat-abi=hard -mfpu=vfp -march=armv6 -Wconversion -Wunreachable-code -Wstrict-prototypes 
	endif	
	ifneq (, $(findstring gnueabisf, $(SYS)))
		OSFLAGS = -Ofast -mfloat-abi=soft -mfpu=vfp -march=armv6 -Wconversion -Wunreachable-code -Wstrict-prototypes 
	endif
endif
ifneq (, $(findstring amd64, $(SYS)))
	OSFLAGS = -O3 -fPIC -march=native -mtune=native -mfpmath=sse -Wno-conversion
endif
CFLAGS =-ffast-math $(OSFLAGS) -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wstrict-overflow=5 -Wwrite-strings -Waggregate-return -Wcast-qual -Wswitch-default -Wswitch-enum -Wformat=2 -g -Wall -isystem. -isystem.. -isystem/usr/include/freetype2/
SRC = $(wildcard *.c)
OBJS = $(SRC:.c=.o)

all: json.o draw.o $(OBJS) libs.o

json.o: json.c json.h
	$(GCC) -ffast-math $(OSFLAGS) -Wno-conversion -g -isystem. -isystem.. -c $(patsubst %.o,%.c,$@)
	
lodepng.o: lodepng.c lodepng.h
	$(GCC) -ffast-math $(OSFLAGS) -Wno-conversion -g -isystem. -isystem.. -c $(patsubst %.o,%.c,$@)

draw.o: draw.c draw.h
	$(GCC) $(CFLAGS) -isystem/usr/include/ -c $(patsubst %.o,%.c,$@)

template.o: template.c template.h
	$(GCC) $(CFLAGS) -isystem/usr/include/ -c $(patsubst %.o,%.c,$@)
	
%.o: %.c %.h
	$(GCC) $(CFLAGS) -c $(patsubst %.o,%.c,$@)

libs.o: $(OBJS)
	$(CROSS_COMPILE)ld -L/usr/local/lib -r $(OBJS) -o libs.o

dist-clean: clean

clean:
	rm *.o >/dev/null 2>&1 || true