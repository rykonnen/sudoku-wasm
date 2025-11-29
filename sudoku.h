#ifndef SUDOKU_H
#define SUDOKU_H

#include <stdbool.h>

typedef unsigned char u8;
#define CELLS 81
#define idx(r, c) ((r) * 9 + (c))

// Sudoku grids
extern u8 grid[CELLS];         // Current playable grid
extern u8 initial_grid[CELLS]; // Original puzzle
extern u8 solution_grid[CELLS];

// Difficulty holes
extern int difficultyHoles;
#define EASY_HOLES   25
#define MEDIUM_HOLES 40
#define HARD_HOLES   59

// Game state
extern int mistakes;
extern const int maxMistakes;
extern bool showSolution;
extern bool gameComplete;

bool is_valid(const u8 grid[CELLS], int row, int col, u8 val);
bool find_empty(const u8 grid[CELLS], int *row, int *col);
void shuffle_u8(u8 *arr, int n);
bool generator_recursive(u8 grid[CELLS]);
int count_solutions(u8 grid[CELLS]);
void make_puzzle(u8 current_grid[CELLS], u8 fixed_grid[CELLS], int holes);
bool is_complete(void);

#endif // SUDOKU_H
