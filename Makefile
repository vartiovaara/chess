chess-cli: main.c
	$(CC) main.c -o chess-cli -Wall -Wextra -pedantic -O3 -g -std=c17 -lncursesw

clean:
	rm chess-cli