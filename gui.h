#ifndef GUI_H
#define GUI_H

#include "sudoku.h"
#include "clay.h"
#include "raylib.h"
#define NUM_COUNT 9

typedef enum { GAME_MENU, GAME_PLAY } GameState;
typedef struct {
    int value;
    Rectangle rect;
} NumberButton;

extern GameState gameState;
extern int activeCellIndex;
extern int hoveredDifficulty;
extern bool flashError;
extern int flashFrames;
extern const int flashDuration;
extern Clay_Color STATIC_CELL_COLOR;
extern Clay_Color EDITABLE_CELL_COLOR;
extern Clay_Color HOVER_COLOR;
extern Clay_Color ACTIVE_COLOR;
extern Color GIVEN_NUM_COLOR;
extern Color USER_NUM_COLOR;

extern NumberButton numberButtons[NUM_COUNT];

Clay_RenderCommandArray CreateGridLayout(void);
Clay_RenderCommandArray CreateMenuLayout(void);
void render_game_loop(Clay_Arena arena, int fontSize);

#endif 
