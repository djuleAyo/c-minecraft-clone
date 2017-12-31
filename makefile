LFLAGS=-Iinclude -Llib -lGL -lGLU -lglut -lm -lSOIL
SOURCES= $(shell ls src/*.c)

all: debug production

debug:
	gcc -o bin/debug -pg -g $(SOURCES) $(LFLAGS) -Wall -Wextra

production:
	gcc -o bin/production -O3 $(SOURCES) $(LFLAGS)


.PHONY: clean

clean:
	touch bin/dummy build/dummy 
	rm -rf bin/** build/** `find ./ -name "*~"` `find ./ -name "#**#"`
