CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99
TARGET = slosh
SRC = slosh_skeleton.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)