all: map.exe

map.exe: blammap.h main.c
	gcc -DBLAMMAP_WINDOWS main.c -o map.exe

.PHONY: clean
clean:
	rm -f map.exe
