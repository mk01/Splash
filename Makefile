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
CFLAGS = -ffast-math $(OSFLAGS) -Wfloat-equal -Wshadow -Wpointer-arith -Wcast-align -Wstrict-overflow=5 -Wwrite-strings -Waggregate-return -Wcast-qual -Wswitch-default -Wswitch-enum -Wformat=2 -g -Wall -isystem. -isystem.. -Ilibs/ -isystem/usr/include/ -isystem/usr/include/freetype2/
SUBDIRS = libs
SRC = $(wildcard *.c)
INCLUDES = $(wildcard libs/*.h) $(wildcard libs/*.o)
PROGAMS = $(patsubst %.c,splash-%,$(SRC))
LIBS = libs/libs.o /usr/lib/arm-linux-gnueabihf/libjpeg.a /usr/lib/arm-linux-gnueabihf/libfreetype.a /usr/lib/arm-linux-gnueabihf/libz.a -lpthread

.PHONY: subdirs $(SUBDIRS)

subdirs: $(SUBDIRS) all

$(SUBDIRS):
	$(MAKE) -C $@

all: $(LIBS) $(PROGAMS) 

splash-daemon: daemon.c $(INCLUDES) $(LIBS)
	$(GCC) $(CFLAGS) -lm -o $@ $(patsubst splash-%,%.c,$@) $(LIBS)
	
splash-send: send.c $(INCLUDES) $(LIBS)
	$(GCC) $(CFLAGS) -lm -o $@ $(patsubst splash-%,%.c,$@) $(LIBS)	

clean:
	rm splash-* >/dev/null 2>&1 || true
	rm *.so* || true
	rm *.a* || true
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done