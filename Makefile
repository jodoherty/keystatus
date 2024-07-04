CFLAGS += -O2 -Wall -std=gnu17
LDLIBS += -lX11

all: keystatus
.PHONY: all

clean:
	$(RM) *.o keystatus
.PHONY: clean

keystatus: keystatus.o
