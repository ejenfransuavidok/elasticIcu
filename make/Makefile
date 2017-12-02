CXX   = g++ 
CC    = gcc
LDFLAGS += -pthread -m32 -shared -static -licui18n -licuuc -licudata -licuio
debug: CFLAGS  = --std=c++11 -g3 -MD -Wall -m32 -masm=intel
release: CFLAGS  = -m32
debug: exec
release: exec



OBJS =  v_icu.o
TARGET = v_icu.so



all: exec
exec: $(TARGET)

.cpp.o:
	$(CXX) $(CFLAGS) $(INC) -c $< -o $@ 2> sk
.c.o:
	$(CC) $(CFLAGS) $(INC) -c $< -o $@ 2> sk


$(TARGET):  $(OBJS) 
	$(CXX) -o $@  $(OBJS) $(INC) $(LDFLAGS) 2>sk
