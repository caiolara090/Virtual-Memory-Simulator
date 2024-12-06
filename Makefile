CC = gcc
CFLAGS = -Wall -g
TARGET = tp2virtual
SRC = tp2virtual.c
OBJ = tp2virtual.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean depend
