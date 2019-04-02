all:
	gcc -o buildworld buildworld.c
	gcc -o wyeast wyeast.c -lpthread

debug:
	gcc -g -o buildworld buildworld.c
	gcc -g -o wyeast wyeast.c -lpthread

zip:
	zip program2-wyeast.zip \
		wyeast     \
		buildworld

clean:
	rm -f buildworld
	rm -f wyeast
