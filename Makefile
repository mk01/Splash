all:

	gcc logger.c splash.c -lm -o splash /usr/lib/arm-linux-gnueabihf/libjpeg.a /usr/lib/arm-linux-gnueabihf/libfreetype.a /usr/lib/arm-linux-gnueabihf/libz.a -I/usr/include/freetype2/