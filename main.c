/*
To compile with gcc:
gcc --std=c11 -o chess main.c -lncursesw

♔♕♗♘♙♖
♚♛♝♞♟♜
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>

#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>

#define LENGTH(X) (sizeof X / sizeof X[0])
#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR"

#define USAGE "chess [fen] [argument]\n\nFen: The FEN representation of the board\n\nArguments:\n -h : Shows this help menu. \n\nIf started without arguments, starts with default starting board."

int row, col;

// struct for every piece on the board
typedef struct {
	// 0 = king, 1 = queen, 2 = bishop,
	// 3 = knight, 4 = rook, 5 = pawn
	int type;
	bool is_white;
	uint64_t bitboard;
} Piece;

Piece makepiece(char piece_c, int x, int y) {
	Piece piece;

	piece.bitboard = 0; // TODO: make bitboard work
	
	piece.is_white = isupper(piece_c);

	// TODO: finish writing all of these
	switch(tolower(piece_c)) {
		case 'q':
			break;
		case 'b':
			break;
	}

	return piece;
}

int startprogram() {
	// main loop
	while (true) {
		// render the board
		for (int y = 0; y < 8; y++) {
			for (int x = 0; x < 8; x++) {
				move((row/2)-4+y, (col/2)-4+x);
				printw("♚"); // | ((y+x+2 % 2)==0 ? COLOR_PAIR(1) : COLOR_PAIR(2)));
				//mvaddch((row/2)-4+y, (col/2)-4+x, "♚");
			}
		}
		refresh();

		// input 
		if (getch() == KEY_F(1))
			return 0;
	}
}

int main(int argc, char** argv) {
	// arg parsing
	char* start_fen;
	if (argc > 1) {
		for (int i = 1; i < argc; i++) {
			if (!strcmp(argv[i], "-h")) {
				puts(USAGE);
				return 0;
			} else {
				// TODO: actually parse the fen
			}

		}
	} else {
		start_fen = DEFAULT_FEN;
	}

	// ncurses init stuff
	setlocale(LC_ALL, "");

	initscr();
	raw();
	noecho();
	keypad(stdscr, TRUE);

	if (has_colors() == FALSE) {
		endwin();
		puts("Your terminal doesn't seem to support colours.");
		return 1;
	}
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_WHITE, COLOR_WHITE);
	init_pair(3, COLOR_BLACK, COLOR_BLACK);
	init_pair(4, COLOR_BLACK, COLOR_WHITE);

	// TODO: change to signal based thing
	//		 as right now resizing isn't recognized
	getmaxyx(stdscr, row, col);

	// start program
	int exitcode = startprogram();

	endwin();
	return exitcode;
}