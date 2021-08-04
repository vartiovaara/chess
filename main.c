/*
To compile:
	make

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
#include <ncursesw/ncurses.h>

#define DEBUG

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
//"rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2"
// See: https://chess.stackexchange.com/a/30006
#define MAX_FEN_LEN 88 // includes trailing \0

// moving directions for pieces
#define MOVE_N 8
#define MOVE_E 1
#define MOVE_S -8
#define MOVE_W -1 

#define USAGE "chess [fen] [argument]\n\nFen: The FEN representation of the board\n\nArguments:\n -h : Shows this help menu. \n\nIf started without arguments, starts with default starting board.\n"

int row, col;
//const wchar_t white_chars[6] = L"♚♛♝♞♜♟";
//const wchar_t black_chars[6] = L"♔♕♗♘♖♙";
const wchar_t piece_chars[6] = L"♚♛♝♞♜♙";

typedef struct {
	// struct for every piece on the board
	// TODO: Have the symbol for drawing the piece stored here
	// 0 = king, 1 = queen, 2 = bishop,
	// 3 = knight, 4 = rook, 5 = pawn
	int type;
	wchar_t ch;
	bool is_white;
} Piece;

Piece makepiece(char piece_c) {
	// Makes a piece from its corresponding FEN symbol

	Piece piece;

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
	piece.ch = piece_chars[piece.type];
	/*
	if (!piece.is_white)
		piece.ch = white_chars[piece.type];
	else
		piece.ch = black_chars[piece.type];
	*/
	return piece;
}

typedef struct {
	Piece* pieces; // can have max 32 pieces. For some reason, the pointers become invalid when out of context when having this array on stack.
	Piece* board[64]; // pointers to pieces NULL if no piece [0] is a1 (a1 b1 ... g1 h1 a2 b2 ... )
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
	board.pieces = malloc(sizeof(Piece)*32);
	memset(&board.board, 0, sizeof(board.board[0])*64);
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

	// piece placement
	// split the rows
	char* rows[8];
	rows[0] = strtok(p_placement, "/");
	for (int i = 1; i < 8; i++) {
		rows[i] = strtok(NULL, "/");
	}
	// go through the rows and make the pieces if necessary
	int piece_count = 0;
	for (int i = 0; i < 8; i++) {
		int rc = 0;
		for (int j = 0; j < 8; j++) {
			printf("%d %d: %c\n", j, i, rows[i][j]);
			if (rows[i][rc] == '\0')
				break;
			// skip the amount of colons needed
			if (isdigit(rows[i][rc])) {
				char num[2];
				num[0] = rows[i][rc];
				num[1] = '\0';
				errno = 0;
				unsigned int n = (unsigned int)strtoul(num, NULL, 10);
				if (errno == EINVAL || errno == ERANGE || n > 8)
					return board;
				j += n-1;
			} else {
				board.pieces[piece_count] = makepiece(rows[i][rc]);
				board.board[(7-i) * MOVE_N + j] = &board.pieces[piece_count];
				piece_count++;
			}
			rc++;
		}
	}

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
	errno = 0;
	board.half_c = (unsigned int)strtoul(half_c, NULL, 10);
	if (errno == EINVAL || errno == ERANGE || board.half_c > 999)
		return board;

	// full-move counter
	errno = 0;
	board.full_c = (unsigned int)strtoul(full_c, NULL, 10);
	if (errno == EINVAL || errno == ERANGE || board.full_c > 9999)
		return board;

#ifdef DEBUG
	// debug information
	puts("Fen parsing debug information start: \n");
	for (int i = 63; i >= 0; i--) {
		if (board.board[i] != NULL)
			putc(board.board[i]->type+48+(board.board[i]->is_white ? (97-48) : 0), stdout);
		else
			putc('-', stdout);
		if (i % 8 == 0)
			putc('\n', stdout);
	}
	putc('\n', stdout);
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
		// clear the screen to prevent ghosting after terminal resize
		for (int y = 0; y < row; y++) {
			for (int x = 0; x < col; x++) {
				mvaddch(y, x, ' ');
			}
		}

		// render the board
		for (int y = 0; y < 8; y++) {
			for (int x = 0; x < 8; x++) {
				// in this loop, the actual position on the
				// chess board is ((x), (7-y)) counting from a1
				wchar_t ch[2] = L" \0";
				if (board.board[(7-y)*MOVE_N + x] != NULL) {
					ch[0] = board.board[(7-y)*MOVE_N + x]->ch;
				}
				int colour = 1;
				if ((x+(7-y)) % 2 == 0)
					colour += 1;
				if (board.board[(7-y)*MOVE_N + x] != NULL)
					if (!(board.board[(7-y)*MOVE_N + x] -> is_white))
						colour += 2;

				attron(COLOR_PAIR(colour));
				move((row/2)-4+y, (col/2)-4+x);
				addwstr(ch);

				attroff(COLOR_PAIR(colour));
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
	init_color(COLOR_RED, 500, 300, 300); // used for "dark" tile colour
	init_color(COLOR_YELLOW, 400, 400, 200); // used for "light" tile colour
	init_pair(1, COLOR_WHITE, COLOR_YELLOW);
	init_pair(2, COLOR_WHITE, COLOR_RED);
	init_pair(3, COLOR_BLACK, COLOR_YELLOW);
	init_pair(4, COLOR_BLACK, COLOR_RED);

	// TODO: change to signal based thing
	//		 as right now resizing isn't recognized
	getmaxyx(stdscr, row, col);

	// start program
	int exitcode = startprogram(board);

	free(board.pieces);

	endwin();
	return exitcode;
}