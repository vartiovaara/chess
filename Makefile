chess-cli: main.c
	$(CC) main.c -o chess-cli -Wall -Wextra -pedantic -O3 -g -std=c11 -lncursesw

clean:
	rm chess-cli