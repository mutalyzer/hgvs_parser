INC_DIR = include
SRC_DIR = src

SOURCES  = $(sort $(shell find $(SRC_DIR) -name '*.c'))
OBJECTS  = $(SOURCES:.c=.o)
DEPS     = $(OBJECTS:.o=.d)

TARGET   = a.out

CC       = gcc
CFLAGS   = -std=c99 -march=native -Wall -Wextra -pedantic $(addprefix -D, $(OPTIONS))
CPPFLAGS = $(addprefix -I, $(INC_DIR))

.PHONY: all check clean debug release

debug: CFLAGS += -O0 -ggdb3 -DDEBUG
debug: all

release: CFLAGS += -O3 -fomit-frame-pointer -funroll-loops -DNDEBUG -DRELEASE
release: all

all: $(TARGET)

check: $(TARGET)
	tests/run_tests.sh -m < tests/varnomen.in
	tests/run_tests.sh -fm < tests/error.in

clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $(TARGET) $(OBJECTS)

-include $(DEPS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -o $@ -c $<
