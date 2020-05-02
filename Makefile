all: map.exe tester.exe

map.exe: blammap.h main.c
	gcc -DBLAMMAP_WINDOWS main.c -o map.exe

tester.exe: blammap.h tester.c
	gcc -DBLAMMAP_WINDOWS tester.c -o tester.exe

.PHONY: clean
clean:
	rm -f map.exe tester.exe
