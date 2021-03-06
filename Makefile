TARGET=ringmaster player
CC=g++
CPPFLAGS=-std=gnu++98 -pedantic -Wall -Werror -ggdb3

all: $(TARGET)
ringmaster: ringmaster.cpp potato.h
	$(CC) $(CPPFLAGS) -o $@ ringmaster.cpp
player: player.cpp potato.h
	$(CC) $(CPPFLAGS) -o $@ player.cpp
.PHONY: clean depend
clean:
	rm -f $(TARGET) *.o *.cpp~ *.hpp~ *.sh~
