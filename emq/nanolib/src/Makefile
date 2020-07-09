
CC = gcc
CFLAGS += -Wall -fno-strict-aliasing -W -std=gnu99
CFLAGS += -Iinclude `pkg-config --cflags json-c`
OFLAGS += -Os
LDFLAGS += $(OFLAGS) -luci -lpolarssl -lrt -lmosquitto -lcurl `pkg-config --libs json-c`

LIBNAME = libevpower.so

# EXTRA_CFLAGS += -DDEBUG_CONSOLE
# EXTRA_CFLAGS += -DDEBUG_FILE
# EXTRA_CFLAGS += -DDEBUG_SYSLOG
# EXTRA_CFLAGS += -DDEBUG_TRACE
# EXTRA_CFLAGS += -DDEBUG_LOCK

ifeq ($(EV_DEBUG), y)
  EXTRA_CFLAGS += -DDEBUG_CONSOLE
  EXTRA_CFLAGS += -DDEBUG_SYSLOG
endif

SRC_C = $(wildcard *.c)
SRC_H = $(wildcard include/*.h)
SRC_O = $(SRC_C:.c=.o)

all: $(LIBNAME)

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(EXTRA_CFLAGS) -fPIC -MD -c $< -o $@

clean:
	rm -f *.o *~ $(LIBNAME)

$(LIBNAME): $(SRC_O)
	$(CC) $(LDFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) -shared -o $@ $^
