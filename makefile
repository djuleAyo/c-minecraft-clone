all: debug production

debug:
	gcc -o bin/debug -pg -g src/blockcraft.c -lGL -lGLU -lglut

production:
	echo "hello production"


.PHONY: clean

clean:
	touch bin/dummy build/dummy 
	rm -rf bin/** build/** `find ./ -name "*~"` `find ./ -name "#**#"`
