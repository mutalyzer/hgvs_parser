INC_DIR = include
SRC_DIR = src

SOURCES  = $(shell find $(SRC_DIR)/ -name '*.c')
OBJECTS  = $(SOURCES:.c=.o)
DEPS     = $(OBJECTS:.o=.d)

TARGET   = a.out

CC       = gcc
CFLAGS   = -march=native
CPPFLAGS = $(addprefix -I, $(INC_DIR)) -std=c99 -Wall -Wextra -pedantic -DANSI

.PHONY: all check clean debug release

debug: CFLAGS += -O0 -DDEBUG -ggdb3
debug: all

release: CFLAGS += -O3 -fomit-frame-pointer -funroll-loops -DNDEBUG
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
