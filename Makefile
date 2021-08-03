chess-cli: main.c
	$(CC) main.c -o chess-cli -Wall -Wextra -pedantic -g -std=c11 -lncursesw