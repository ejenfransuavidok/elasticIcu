CXX   = g++ 
CC    = gcc
LDFLAGS += -pthread -m32 -shared
debug: CFLAGS  = --std=c++11 -g3 -MD -Wall -m32 -masm=intel
release: CFLAGS  = -m32
debug: exec
release: exec

LIBSINC = -L/home/vidok/Downloads/icu/32-bit/icu/source/lib/

OBJS =  v_icu.o
TARGET = v_icu.so


all: exec
exec: $(TARGET)

.cpp.o:
	$(CXX) $(CFLAGS) $(INC) -c $< -o $@ 2> sk
.c.o:
	$(CC) $(CFLAGS) $(INC) -c $< -o $@ 2> sk


$(TARGET):  $(OBJS) 
	$(CXX) -o $@  $(OBJS) $(LDFLAGS) /home/vidok/Downloads/icu/32-bit/icu/source/lib/libicui18n.a /home/vidok/Downloads/icu/32-bit/icu/source/lib/libicuuc.a /home/vidok/Downloads/icu/32-bit/icu/source/lib/libicudata.a /home/vidok/Downloads/icu/32-bit/icu/source/lib/libicuio.a 2>sk
