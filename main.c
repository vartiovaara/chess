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
#include <math.h>
#include <stdbool.h>

#define _XOPEN_SOURCE_EXTENDED
#include <ncursesw/ncurses.h>

//#define DEBUG

#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
//"rnbqkbnr/pp1ppppp/8/2p5/4P3/5N2/PPPP1PPP/RNBQKB1R b KQkq - 1 2"
//"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
// See: https://chess.stackexchange.com/a/30006
#define MAX_FEN_LEN 88 // includes trailing \0

// moving directions for pieces
#define MOVE_N 8
#define MOVE_E 1
#define MOVE_S -8
#define MOVE_W -1

#define USAGE "chess [fen] [argument]\n\nFen: The FEN representation of the board\n\nArguments:\n -h : Shows this help menu. \n\nIf started without arguments, starts with default starting board.\n"

const wchar_t piece_chars[6] = L"♚♛♝♞♜♙";

typedef struct {
	// struct for every piece on the board
	// 0 = king, 1 = queen, 2 = bishop,
	// 3 = knight, 4 = rook, 5 = pawn
	int type;
	wchar_t ch;
	bool is_white;
} Piece;

Piece makepiece(char piece_c) {
	// Makes a piece from its corresponding FEN symbol
	// TODO: throw an error if char is wrong

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
	return piece;
}

typedef struct {
	Piece pieces[32]; // can have max 32 pieces. The pointers become invalid when out of context when having this array on stack.
	Piece* board[64]; // pointers to pieces NULL if no piece [0] is a1 (a1 b1 ... g1 h1 a2 b2 ... )
	bool whiteturn;
	bool wqcastle, wkcastle; // whites castling rights
	bool bqcastle, bkcastle; // blacks
	unsigned int enpassant; // set to 0 for no en passant
	unsigned int half_c, full_c;
	bool parsingerr; // true if errors happened during parsing
} Board;

Board* parsefen(const char* fen) {
	// Function to parse a FEN string and return a pointer to a board.
	// REMEMBER TO FREE THE BOARD
	// See: https://www.chessprogramming.org/Forsyth-Edwards_Notation
	// For maximum length of FEN, see: https://chess.stackexchange.com/a/30006

	Board* board;
	board = malloc(sizeof(Board)); // allocate the board
	memset(&board->board, 0, sizeof(board->board[0])*64);
	board->parsingerr = true; // if error happens we don't change this
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
	// FIXME: can be more efficient as now it loops trough all
	// 64 squares and all that is needed is looping trough
	// all the characters in the FEN. Thusly not even needing
	// splitting.
	int piece_count = 0;
	for (int i = 0; i < 8; i++) {
		int rc = 0; // stores the index on the current row
		for (int j = 0; j < 8; j++) {
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
				board->pieces[piece_count] = makepiece(rows[i][rc]);
				board->board[(7-i) * MOVE_N + j] = &board->pieces[piece_count];
				piece_count++;
			}
			rc++; // parse the next character
		}
	}

	// turn
	if (!strcmp(turn, "w"))
		board->whiteturn = true;
	else if (!strcmp(turn, "b"))
		board->whiteturn = false;
	else
		return board;

	// castling
	board->wqcastle = (strchr(castling, 'Q') ? true : false);
	board->wkcastle = (strchr(castling, 'K') ? true : false);
	board->bqcastle = (strchr(castling, 'q') ? true : false);
	board->bkcastle = (strchr(castling, 'k') ? true : false);

	// en passant
	if (strcmp(enpassant, "-")) {
		// check that the square is valid
		if (enpassant[0] < 97 || enpassant[0] > 104)
			return board;
		if (enpassant[1] != '3' && enpassant[1] != '6') {
			printf("Other ranks than 3 and 6 for en passant target are not valid.\n");
			return board;
		}
		board->enpassant = (enpassant[0] - 97) * MOVE_E; // x
		board->enpassant += ((enpassant[1] == '3' ? 3 : 6)-1) * MOVE_N; // y (-1 is becouse counting starts from a1 not a0(don't move when 1))
	} else {
		board->enpassant = 0;
	}

	// half-move clock
	errno = 0;
	board->half_c = (unsigned int)strtoul(half_c, NULL, 10);
	if (errno == EINVAL || errno == ERANGE || board->half_c > 999)
		return board;

	// full-move counter
	errno = 0;
	board->full_c = (unsigned int)strtoul(full_c, NULL, 10);
	if (errno == EINVAL || errno == ERANGE || board->full_c > 9999)
		return board;

#ifdef DEBUG
	// debug information
	puts("Fen parsing debug information start: \n");
	for (int i = 63; i >= 0; i--) {
		if (board->board[i] != NULL)
			putc(board->board[i]->type+48+(board->board[i]->is_white ? (97-48) : 0), stdout);
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
	printf("Q: %u K: %u q: %u k: %u\n", board->wqcastle, board->wkcastle, board->bqcastle, board->bkcastle);
	printf("%s\n", enpassant);
	printf("%c %c -> %u\n", enpassant[0], enpassant[1], board->enpassant);
	printf("\"%s\" -> %u\n", half_c, board->half_c);
	printf("\"%s\" -> %u\n", full_c, board->full_c);
	puts("Fen parsing debug information end. \n");
#endif

	board->parsingerr = false; // no errors happened (hopefully)
	return board;
}

void ntostrcoord(int pos, char* str) {
	// Sets str to e.g. a1\0 from pointer table index
	// See: Piece* board[64] from struct Board
	str[0] = (pos % 8) + 97;
	str[1] = floor(pos / 8) + 49;
	str[2] = '\0';
}

int strtoncoord(char* pos) {
	// Returns pointer table index from e.g. a1
	// NOTE: Does NOT throw an error if there are errors in parsing.
	int n = 0;
	n = (pos[0] - 97); // x
	n += (pos[1] - 49)*8;
	return n;
}

bool validstrcoord(char* pos) {
	// Validates a string coord e.g. a1
	// Does not check if a square is occupied
	if (!(isalpha(pos[0]) && isdigit(pos[1]))) {
		return false;
	}
	else if (pos[0] < 97 || pos[0] > 104) {
		return false;
	}
	else if (pos[1] < 49 || pos[1] > 56) {
		return false;
	}
	return true;
}

int nofdigits(int i) {
	// Find out how many digits a number has.
	if (i == 0)
		return 1;
	int n = 0;
	while (i != 0) {
		i = floor(i/10);
		n++;
	}
	return n;
}

void movepiece(int startpos, int endpos, Board* board) {
	// Moves a piece pointer from startpos to endpos.
	board->board[endpos] = board->board[startpos];
	board->board[startpos] = NULL;
}

/*
// actually i'll make this function when the displaying and input is done
void getmoves(Piece piece, int pos, int* moves, int* moves_amount) {
	// Gives all the moves possible by a piece.

	// 0 = king, 1 = queen, 2 = bishop,
	// 3 = knight, 4 = rook, 5 = pawn

	// King
	if (piece.type == 0) {

	}
}
*/

int startprogram(Board* board) {
	/* only 4 input characters is used (e.g "a2a5\0") */
	char fullinput[5] = {'\0', '\0', '\0', '\0', '\0'};
	/* main loop */
	while (true) {
		/* board position on the screen. i don't use windows */
		/* becouse moving them would be cumbersome. this is more elegant */
		unsigned int boardx = ((COLS/2)-4);
		unsigned int boardy = ((LINES/2)-4);

		// clear the screen to prevent ghosting after terminal resize
		for (int y = 0; y < LINES; y++) {
			for (int x = 0; x < COLS; x++) {
				mvaddch(y, x, ' ');
			}
		}

		// input length
		unsigned int inputlen = strlen(fullinput);

		// notate the board according to what's being inputted
		bool notate = validstrcoord(fullinput);
		int notatex, notatey;
		if (notate) {
			notatex = (fullinput[0] - 97); // x
			notatey = (7-(fullinput[1] - 49));
		}

		// render the board
		for (int y = 0; y < 8; y++) {
			for (int x = 0; x < 8; x++) {
				wchar_t ch[2] = L" \0";
				if (board->board[(7-y)*MOVE_N + x] != NULL)
					ch[0] = board->board[(7-y)*MOVE_N + x]->ch;
				
				// determining the colour to be used
				int colour;
				if (notate && (x == notatex && y == notatey)) {
					colour = 5;
					if (board->board[(7-y)*MOVE_N + x] != NULL)
						if (!(board->board[(7-y)*MOVE_N + x] -> is_white))
							colour += 1;
				} else {
					colour = 1;
					if ((x+(7-y)) % 2 == 0)
						colour += 1;
					if (board->board[(7-y)*MOVE_N + x] != NULL)
						if (!(board->board[(7-y)*MOVE_N + x] -> is_white))
							colour += 2;
				}

				attron(COLOR_PAIR(colour));
				mvaddwstr(boardy+y, boardx+x, ch);
				attroff(COLOR_PAIR(colour));
			}
		}
		// print the labels next to the board
		for (int i = 0; i < 8; i++) {
			mvaddch(boardy+(7-i), boardx-1, i+49);
		}
		mvprintw(boardy+8, boardx, "abcdefgh");

		// turn indicator
		mvaddch(boardy+9, boardx+6, (board->whiteturn ? 'W': 'B'));

		// en passant
		if (board->enpassant == 0)
			mvaddch(boardy+9, boardx+8, '-');
		else {
			char enp[3];
			ntostrcoord(board->enpassant, enp);
			mvprintw(boardy+9, boardx+8, enp);
		}

		// halfmove clock
		mvprintw(boardy-1, boardx-2, "h:%u", board->half_c);

		// fullmove counter
		mvprintw(boardy-1, boardx+8-1-nofdigits(board->full_c), "f:%u", board->full_c);

		// inputting area
		mvprintw(boardy+9, boardx-1, fullinput);

		// user input stuff
		unsigned int ch = getch();
		if (isalnum(ch) && inputlen < 4) 
			fullinput[inputlen] = ch;
		
		if (ch == KEY_BACKSPACE && inputlen > 0) 
			fullinput[inputlen-1] = '\0';
		
		if ((ch == KEY_ENTER || ch == '\n') && inputlen == 4) {
			/* the user wants to execute move */
			char startpos[2] = {fullinput[0], fullinput[1]};
			char endpos[2] = {fullinput[2], fullinput[3]};
			if (validstrcoord(startpos) && validstrcoord(endpos)) {
				int startn = strtoncoord(startpos);
				int endn = strtoncoord(endpos);
				/* check if the move is legal */
				/* TODO: Check if is ACTUALLY legal */
				// currently checks if starting square is empty
				// if wrong players piece is moved
				// if try to eat own piece
				if (board->board[startn] == NULL)
					goto END;
				if (board->board[startn]->is_white != board->whiteturn)
					goto END;
				if (board->board[endn] != NULL) {
					if (board->board[endn]->is_white == board->board[startn]->is_white)
						goto END;
				}
				movepiece(startn, endn, board);
				board->whiteturn = !board->whiteturn;
				memset(&fullinput, '\0', sizeof(fullinput[0])*5);

				END: /* move wasn't legal */
					; /* just a placeholder for "do nothing" */
			}
		}

		move(boardy+9, boardx-1+inputlen);

		refresh();

		if (ch == KEY_F(1))
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
	Board* board = parsefen(start_fen);
	if (board->parsingerr) {
		printf("Error while parsing FEN.\n");
		free(board);
		return 1;
	}

	// ncurses init stuff
	setlocale(LC_ALL, "");

	if (!initscr()) {
		puts("Error while starting ncurses.\n");
		free(board);
		return 1;
	}
	raw();
	noecho();
	keypad(stdscr, TRUE);

	if (has_colors() == FALSE) {
		endwin();
		puts("Your terminal doesn't seem to support colours.\n");
		free(board);
		return 1;
	}
	start_color();
	init_color(COLOR_RED, 500, 300, 300); // used for "dark" tile colour
	init_color(COLOR_YELLOW, 400, 400, 200); // used for "light" tile colour
	init_color(COLOR_CYAN, 200, 500, 320); // used for notation
	init_pair(1, COLOR_WHITE, COLOR_YELLOW);
	init_pair(2, COLOR_WHITE, COLOR_RED);
	init_pair(3, COLOR_BLACK, COLOR_YELLOW);
	init_pair(4, COLOR_BLACK, COLOR_RED);
	init_pair(5, COLOR_WHITE, COLOR_CYAN); // for notating
	init_pair(6, COLOR_BLACK, COLOR_CYAN);

	// start program
	int exitcode = startprogram(board);

	// free the pieces
	free(board);

	endwin();
	return exitcode;
}

