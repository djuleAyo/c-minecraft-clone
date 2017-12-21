all: debug production

debug:
	gcc -Iinclude -Llib -o bin/debug -pg -g src/lodepng.c src/blockcraft.c src/keyStore.c src/perlin.c -lGL -lGLU -lglut -lm -lSOIL 

production:
	echo "hello production"


.PHONY: clean

clean:
	touch bin/dummy build/dummy 
	rm -rf bin/** build/** `find ./ -name "*~"` `find ./ -name "#**#"`
