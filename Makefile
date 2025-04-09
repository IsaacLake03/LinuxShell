CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = slosh
SRC = SLOsh.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)