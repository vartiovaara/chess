/*
To compile:
cc -Wall --std=c11 -o chess main.c -lncursesw

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

#define DEBUG

#define LENGTH(X) (sizeof X / sizeof X[0])
#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#define USAGE "chess [fen] [argument]\n\nFen: The FEN representation of the board\n\nArguments:\n -h : Shows this help menu. \n\nIf started without arguments, starts with default starting board."

int row, col;

typedef struct {
	// struct for every piece on the board
	// TODO: Have the symbol for drawing the piece stored here
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
	Piece board[8][8]; // [x][y]  top-left is [0][0]
	bool whiteturn;
	bool wqcastle, wkcastle; // whites castling rights
	bool bqcastle, bkcastle; // blacks
	unsigned int enpas_x, enpas_y; // set both to 0 for no en passant
	unsigned int half_c, full_c;
	bool parsingerr; // true if errors happened during parsing
} Board;

Board parsefen(const char* fen) {
	// Function to parse a FEN string and return a board.
	// See: https://www.chessprogramming.org/Forsyth-Edwards_Notation
	// For maximum length of FEN, see: https://chess.stackexchange.com/a/30006

	Board board;
	memset(board.board, NULL, sizeof(Piece)*64);
	board.parsingerr = true; // if error happens we don't change this

	// copy the fen string to a temporary array
	// the +1 is so the \0 gets copied too
	char cp[88];
	memcpy(cp, fen, strlen(fen)+1);

#ifdef DEBUG
	printf("%s\n", cp);
#endif

	// split the FEN to its different fields
	char* p_placement = strtok(cp, " ");
	char* turn = strtok(NULL, " ");
	char* castling = strtok(NULL, " ");
	char* enpassant = strtok(NULL, " ");
	char* half_c = strtok(NULL, " ");
	char* full_c = strtok(NULL, " ");

	// turn
#ifdef DEBUG
	printf("%s\n", turn);
#endif
	if (!strcmp(turn, "w"))
		board.whiteturn = true;
	else if (!strcmp(turn, "b"))
		board.whiteturn = false;
	else
		return board;

	// castling
	board.wqcastle = (strchr(castling, 'Q') ? true : false);
	board.wkcastle = (strchr(castling, 'K') ? true : false);
	board.bqcastle = (strchr(castling, 'q') ? true : false);
	board.bkcastle = (strchr(castling, 'k') ? true : false);
#ifdef DEBUG
	printf("%s\n", castling);
	printf("%d %d %d %d\n", board.wqcastle, board.wkcastle, board.bqcastle, board.bkcastle);
#endif

	// en passant
#ifdef DEBUG
	printf("%s\n", enpassant);
	printf("%c %c\n", enpassant[0], enpassant[1]);
#endif
	if (strcmp(enpassant, "-")) {
		// check that the square is valid
		if (enpassant[0] < 97 || enpassant[0] > 104)
			return board;
		if (enpassant[1] != '3' && enpassant[1] != '6') {
			printf("Other ranks than 3 and 6 are not valid.\n");
			return board;
		}
		board.enpas_x = enpassant[0] - 97;
		board.enpas_y = (enpassant[1] == '3' ? 6 : 3) - 1; // values flipped becouse we count from top-left and -1 cuz counting from 0
	} else {
		board.enpas_x = 0;
		board.enpas_y = 0;
	}
#ifdef DEEBUG
	printf("%i %d\n", board.enpas_x, board.enpas_y);
#endif
	
	board.parsingerr = false; // no errors happened (hopefully)
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
				// TODO: actually set start_fen to the fen user inputted
			}

		}
	} else {
		start_fen = DEFAULT_FEN;
	}

	// parse the fen
	Board board = parsefen(start_fen);
	if (board.parsingerr) {
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