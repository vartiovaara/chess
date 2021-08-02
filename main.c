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
#include <errno.h>

#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>

#define DEBUG

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
// See: https://chess.stackexchange.com/a/30006
#define MAX_FEN_LEN 88 // includes trailing \0

// moving directions for pieces
#define MOVE_N 8
#define MOVE_E 1
#define MOVE_S -8
#define MOVE_W -1 

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
	Piece board[64]; // [0] is a1 (a1 b1 ... g1 h1 a2 b2 ... )
	bool whiteturn;
	bool wqcastle, wkcastle; // whites castling rights
	bool bqcastle, bkcastle; // blacks
	unsigned int enpassant; // set to 0 for no en passant
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

	// the +1 is so the \0 gets copied too
	unsigned int fen_len = strlen(fen) + 1;

	// copy the fen string to a temporary array
	if (fen_len > MAX_FEN_LEN) {
		puts("The FEN is too long, this probably means it is not valid.\n");
		puts("If the FEN is valid, please make a bug report and the issue will be investigated.\n");
		return board;
	}
	char cp[MAX_FEN_LEN]; // this is the copy
	memcpy(cp, fen, fen_len);

	// split the FEN to its different fields
	char* p_placement = strtok(cp, " ");
	char* turn = strtok(NULL, " ");
	char* castling = strtok(NULL, " ");
	char* enpassant = strtok(NULL, " ");
	char* half_c = strtok(NULL, " ");
	char* full_c = strtok(NULL, " ");

	// turn
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

	// en passant
	if (strcmp(enpassant, "-")) {
		// check that the square is valid
		if (enpassant[0] < 97 || enpassant[0] > 104)
			return board;
		if (enpassant[1] != '3' && enpassant[1] != '6') {
			printf("Other ranks than 3 and 6 for en passant target are not valid.\n");
			return board;
		}
		board.enpassant = (enpassant[0] - 97) * MOVE_E; // x
		board.enpassant += ((enpassant[1] == '3' ? 3 : 6)-1) * MOVE_N; // y (-1 is becouse counting starts from a1 not a0(don't move when 1))
	} else {
		board.enpassant = 0;
	}

	// half-move clock
	board.half_c = (unsigned int)strtoul(half_c, NULL, 10);
	if (errno == EINVAL || errno == ERANGE || board.half_c > 999)
		return board;

	// full-move counter
	board.full_c = (unsigned int)strtoul(full_c, NULL, 10);
	if (errno == EINVAL || errno == ERANGE || board.full_c > 9999)
		return board;

#ifdef DEBUG
	// debug information
	puts("Fen parsing debug information start: \n");
	printf("%s\n", fen);
	printf("%s\n", p_placement);
	printf("%s\n", turn);
	printf("%s\n", castling);
	printf("Q: %u K: %u q: %u k: %u\n", board.wqcastle, board.wkcastle, board.bqcastle, board.bkcastle);
	printf("%s\n", enpassant);
	printf("%c %c -> %u\n", enpassant[0], enpassant[1], board.enpassant);
	printf("\"%s\" -> %u\n", half_c, board.half_c);
	printf("\"%s\" -> %u\n", full_c, board.full_c);
	puts("Fen parsing debug information end. \n");
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