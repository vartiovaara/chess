/*
To compile with gcc:
gcc --std=c11 -o chess main.c -lncursesw

♔♕♗♘♙♖
♚♛♝♞♟♜
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <locale.h>

#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>

#define LENGTH(X) (sizeof X / sizeof X[0])
#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

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

	piece.bitboard = 0; // TODO: make bitboard work or change to xy or just remove this shit
	
	piece.is_white = isupper(piece_c);

	switch(tolower(piece_c)) {
		case 'k':
			piece.type = 0;
			break;
		case 'q':
			piece.type = 1;
			break;
		case 'b':
			piece.type = 2;
			break;
		case 'n':
			piece.type = 3;
			break;
		case 'r':
			piece.type = 4;
			break;
		case 'p':
			piece.type = 5;
			break;
	}

	return piece;
}

typedef struct {
	Piece** board; // [x][y]
	bool whiteturn;
	bool wqcastle, wkcastle;
	bool bqcastle, bkcastle;
	int half_c, full_c;
	int enpas_x, enpas_y;
} Board;

Board parsefen(char* fen) {
	Board board;
	board.board = malloc(sizeof(Piece)*64);
	memset(board.board, NULL, sizeof(Piece)*64);

	char* cp = strdupa(fen);
	char* p_placement = strtok(cp, ' ');
	char* turn = strtok(NULL, ' ');
	char* castling = strtok(NULL, ' ');
	char* enpassant = strtok(NULL, ' ');
	char* half_c = strtok(NULL, ' ');
	char* full_c = strtok(NULL, ' ');

	// turn
	if (turn == "w" || turn == "b")
		board.whiteturn = (turn == "w" ? true : false);
	else
		return null;

	if (castling == "-"){
		board.wqcastle = false;
		board.wkcastle = false;
		board.bqcastle = false;
		board.bkcastle = false;
	} else {
		
	}

	return board;
}

int startprogram(Board board) {
	// main loop
	while (true) {
		// render the board
		// rendering happens from top left (a8) square
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

	// parse the fen
	Board board = parsefen(start_fen);
	if (!board) {
		printf("Error while parsing FEN.\n");
		return 1;
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
	int exitcode = startprogram(board);

	endwin();
	return exitcode;
}