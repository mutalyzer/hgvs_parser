INC_DIR = include
SRC_DIR = src

SOURCES  = $(shell find $(SRC_DIR)/ -name '*.c')
OBJECTS  = $(SOURCES:.c=.o)
DEPS     = $(OBJECTS:.o=.d)

TARGET   = a.out

CC       = gcc
CFLAGS   = -march=native
CPPFLAGS = $(addprefix -I, $(INC_DIR) $(LIB_INC)) -std=c99 -Wall -Wextra -pedantic

.PHONY: all clean debug release

debug: CFLAGS += -O0 -DDEBUG -ggdb3
debug: all

release: CFLAGS += -O3 -fomit-frame-pointer -funroll-loops -DNDEBUG
release: all

all: $(TARGET)

clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $(TARGET) $(OBJECTS)

-include $(DEPS)

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -o $@ -c $<