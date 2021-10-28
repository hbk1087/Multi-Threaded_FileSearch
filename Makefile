all: FileSearch

FileSearch: FileSearch.c
	gcc -g -Wall -Werror -fsanitize=address FileSearch.c -o FileSearch -lm -pthread

clean:
	rm FileSearch

