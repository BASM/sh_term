include makefiles/pre.mk

DEVICE = atmega328p
TYPEPROG=stk500v2 -P avrdoper
AVRDUDE = avprog

OBJECTS = main.o twi_sw.o terms.o

DEPLIBS+=libstwi
AVRLIBS+=uart
AVRLIBS+=twi
AVRLIBS+=adc
AVRLIBS+=stdioconf

include makefiles/avr.mk
