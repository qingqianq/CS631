CC       = gcc
CFLAGS   = -Wall -Werror -Wextra sws.c -lmagic -lpthread
OBJFILES = sws.c
TARGET   = sws

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES)

clean:
	rm -f $(TARGET)
