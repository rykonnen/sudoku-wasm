#define CLAY_IMPLEMENTATION
#include "raylib.h"
#include "clay.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

// --- Sudoku Definitions ---
typedef unsigned char u8;
#define CELLS 81
#define idx(r, c) ((r) * 9 + (c))

// --- Colors (Updated) ---
Clay_Color STATIC_CELL_COLOR   = {236, 255, 255, 255};
Clay_Color EDITABLE_CELL_COLOR = {255, 255, 255, 255};
Clay_Color HOVER_COLOR      = {182, 214, 255, 255};
Clay_Color ACTIVE_COLOR     = {109, 202, 209, 255};
Color GIVEN_NUM_COLOR       = {0, 0, 0, 255};
Color USER_NUM_COLOR        = {44, 53, 54, 255};

// difficulties
#define EASY_HOLES   25
#define MEDIUM_HOLES 40
#define HARD_HOLES   59
int difficultyHoles = MEDIUM_HOLES; // default

// game state
typedef enum { GAME_MENU, GAME_PLAY } GameState;
GameState gameState = GAME_MENU;

// menu hover
int hoveredDifficulty = -1; // 0=Easy,1=Med,2=Hard

// game state vars
int activeCellIndex = -1;
u8 grid[CELLS] = {0};         // The current playable grid state
u8 initial_grid[CELLS] = {0}; // The original puzzle grid (read-only)
u8 solution_grid[CELLS] = {0};
bool flashError = false;
int flashFrames = 0;
const int FLASH_DURATION = 15;
int mistakes = 0;
const int maxMistakes = 3;
bool showSolution = false;

// --- Sudoku functions ---
bool is_valid(const u8 grid[CELLS], int row, int col, u8 val) {
    for (int i = 0; i < 9; i++) {
        if (grid[idx(row, i)] == val) return false;
        if (grid[idx(i, col)] == val) return false;
    }
    int br = (row / 3) * 3;
    int bc = (col / 3) * 3;
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
            if (grid[idx(br + r, bc + c)] == val) return false;
    return true;
}

bool find_empty(const u8 grid[CELLS], int *row, int *col) {
    for (int r = 0; r < 9; r++)
        for (int c = 0; c < 9; c++)
            if (grid[idx(r, c)] == 0) {
                *row = r;
                *col = c;
                return true;
            }
    return false;
}

void shuffle_u8(u8 *arr, int n) {
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        u8 tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

bool generator_recursive(u8 grid[CELLS]) {
    int row, col;
    if (!find_empty(grid, &row, &col)) return true;
    u8 order[9];
    for (int i = 0; i < 9; i++) order[i] = i + 1;
    shuffle_u8(order, 9);
    for (int k = 0; k < 9; k++) {
        u8 v = order[k];
        if (is_valid(grid, row, col, v)) {
            grid[idx(row, col)] = v;
            if (generator_recursive(grid)) return true;
            grid[idx(row, col)] = 0;
        }
    }
    return false;
}

void make_puzzle(u8 current_grid[CELLS], u8 fixed_grid[CELLS], int holes) {
    memset(current_grid, 0, CELLS);
    memset(fixed_grid, 0, CELLS);

    u8 fullGrid[CELLS] = {0};
    generator_recursive(fullGrid);              // generate full solution
    memcpy(solution_grid, fullGrid, CELLS);    // store solution

    memcpy(current_grid, fullGrid, CELLS);     // start puzzle grid as full solution
    memcpy(fixed_grid, fullGrid, CELLS);       // fixed grid for static cells

    int removed = 0;
    while (removed < holes) {
        int pos = rand() % CELLS;
        if (current_grid[pos] != 0) {
            current_grid[pos] = 0;
            fixed_grid[pos] = 0;
            removed++;
        }
    }
}

bool is_complete() {
    for (int i = 0; i < CELLS; i++)
        if (grid[i] == 0) return false;
    return true;
}

// --- Clay Layouts ---
Clay_RenderCommandArray CreateGridLayout(void) {
    Clay_BeginLayout();
    int cellSize = 30; 
    int gap = 1;
    int padding = 10;

    CLAY(CLAY_ID("GridWrapper"), {
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
        }
    }) {
        CLAY(CLAY_ID("GridContainer"), {
            .layout = {
                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                .childGap = gap,
                .padding = CLAY_PADDING_ALL(padding),
            },
            .cornerRadius = CLAY_CORNER_RADIUS(4),
        }) {
            for (int r = 0; r < 9; r++) {
                CLAY_AUTO_ID({
                    .layout = {.layoutDirection = CLAY_LEFT_TO_RIGHT, .childGap = gap}
                }) {
                    for (int c = 0; c < 9; c++) {
                        int cellIndex = idx(r, c);
                        Clay_Color bg = (initial_grid[cellIndex] != 0) ? STATIC_CELL_COLOR : EDITABLE_CELL_COLOR;
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_FIXED(cellSize),
                                           .height = CLAY_SIZING_FIXED(cellSize)}
                            },
                            .backgroundColor = bg,
                        }) {}
                    }
                }
            }
        }
    }
    return Clay_EndLayout();
}

Clay_RenderCommandArray CreateMenuLayout(void) {
    Clay_BeginLayout();
    CLAY(CLAY_ID("MenuWrapper"), {
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_CENTER},
            .childGap = 1,
            .padding = CLAY_PADDING_ALL(0),
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
        }
    }) {
        CLAY(CLAY_ID("Title"), {.layout = {.sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0)}}}) {}
        CLAY(CLAY_ID("EasyButton"), 
        {.layout = 
            {.sizing = 
                {.width = CLAY_SIZING_FIXED(110), .height = CLAY_SIZING_FIXED(50)}
            }, .backgroundColor = (Clay_Color){255,255,255,255}, .cornerRadius = CLAY_CORNER_RADIUS(8)
        }) {}
        CLAY(CLAY_ID("MedButton"),  
        {.layout = 
            {.sizing = 
                {.width = CLAY_SIZING_FIXED(110), .height = CLAY_SIZING_FIXED(50)}
            }, .backgroundColor = (Clay_Color){255,255,255,255}, .cornerRadius = CLAY_CORNER_RADIUS(8)
        }) {}
        CLAY(CLAY_ID("HardButton"), 
        {.layout = 
            {.sizing = 
                {.width = CLAY_SIZING_FIXED(110), .height = CLAY_SIZING_FIXED(50)}
            }, .backgroundColor = (Clay_Color){255,255,255,255}, .cornerRadius = CLAY_CORNER_RADIUS(8)
        }) {}
    }

    return Clay_EndLayout();
}

// Main Program 
int main(void) {
    InitWindow(600, 600, "Sudoku Clay");
    SetTargetFPS(60);
    srand(time(NULL));

    // Clay Memory & Initialization
    uint64_t memorySize = Clay_MinMemorySize();
    void *memory = malloc(memorySize);
    Clay_Arena arena = Clay_CreateArenaWithCapacityAndMemory(memorySize, memory);
    Clay_Initialize(arena, (Clay_Dimensions){600, 600}, (Clay_ErrorHandler){NULL});

    int fontSize = 20;
    int charsCount = 95;
    Font myFont = LoadFontEx("Roboto-Regular.ttf", fontSize, NULL, charsCount);

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();

        // --- MENU ---
        if (gameState == GAME_MENU) {
            hoveredDifficulty = -1;

            Clay_RenderCommandArray menuCommands = CreateMenuLayout();

            Rectangle btnRects[3];
            int btnCount = 0;

            // Collect button rectangles
            for (int i = 0; i < menuCommands.length; i++) {
                Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&menuCommands, i);
                if (cmd->commandType == CLAY_RENDER_COMMAND_TYPE_RECTANGLE &&
                    (int)cmd->boundingBox.width == 110 && (int)cmd->boundingBox.height == 50 &&
                    btnCount < 3) {
                    btnRects[btnCount].x = cmd->boundingBox.x;
                    btnRects[btnCount].y = cmd->boundingBox.y;
                    btnRects[btnCount].width = cmd->boundingBox.width;
                    btnRects[btnCount].height = cmd->boundingBox.height;
                    btnCount++;
                }
            }

            // Mouse hover & click detection
            bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            int clickedIndex = -1;
            for (int b = 0; b < btnCount; b++) {
                if (CheckCollisionPointRec(mouse, btnRects[b])) {
                    hoveredDifficulty = b;
                    if (clicked) clickedIndex = b;
                }
            }

            // Handle button click
            if (clickedIndex != -1) {
                if (clickedIndex == 0) difficultyHoles = EASY_HOLES;
                else if (clickedIndex == 1) difficultyHoles = MEDIUM_HOLES;
                else difficultyHoles = HARD_HOLES;

                make_puzzle(grid, initial_grid, difficultyHoles);
                activeCellIndex = -1;
                flashError = false;
                flashFrames = 0;
                mistakes = 0;
                showSolution = false;
                gameState = GAME_PLAY;
            }

            // --- Draw Menu ---
            BeginDrawing();
            ClearBackground(WHITE);

            // Title
            DrawText("SUDOKU", GetScreenWidth()/2 - 100, btnRects[0].y - 60, 48, MAROON);

            // Buttons & text
            const char *labels[3] = {"EASY", "MEDIUM", "HARD"};
            for (int b = 0; b < btnCount; b++) {
                Color col = (hoveredDifficulty == b) ? (Color){HOVER_COLOR.r, HOVER_COLOR.g, HOVER_COLOR.b, HOVER_COLOR.a} : (Color){255,255,255,255};
                DrawRectangleRec(btnRects[b], col);
                DrawRectangleLinesEx(btnRects[b], 2, BLACK);

                Vector2 textSize = MeasureTextEx(myFont, labels[b], 24, 2);
                DrawTextEx(myFont, labels[b],
                    (Vector2){btnRects[b].x + (btnRects[b].width - textSize.x)/15.0f,
                              btnRects[b].y + (btnRects[b].height - textSize.y)/2-8.0f},
                    24, 2, BLACK);
            }

            EndDrawing();
            continue;
        }

        // --- GAMEPLAY ---
        Clay_RenderCommandArray gridCommands = CreateGridLayout();

        // Active cell selection
        if (!showSolution && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            activeCellIndex = -1;
            flashError = false;
            flashFrames = 0;

            for (int i = 0; i < gridCommands.length; i++) {
                Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&gridCommands, i);
                Rectangle cellRect = {cmd->boundingBox.x, cmd->boundingBox.y, cmd->boundingBox.width, cmd->boundingBox.height};

                if ((int)cellRect.width == 30 && (int)cellRect.height == 30 &&
                    CheckCollisionPointRec(mouse, cellRect)) {
                    activeCellIndex = i;
                    break;
                }
            }
        }

        // Input handling
        if (!showSolution && activeCellIndex != -1 && initial_grid[activeCellIndex] == 0) {
            int key = GetKeyPressed();
            if (key >= KEY_ONE && key <= KEY_NINE) {
                u8 val = (u8)(key - KEY_ZERO);

                if (val == solution_grid[activeCellIndex]) {
                    grid[activeCellIndex] = val;
                    flashError = false;
                } else {
                    flashError = true;
                    flashFrames = FLASH_DURATION;
                    mistakes++;
                    if (mistakes >= 3) showSolution = true;
                }
            } else if (key == KEY_BACKSPACE || key == KEY_DELETE) {
                grid[activeCellIndex] = 0;
                flashError = false;
            }
        }

        if (flashFrames > 0) flashFrames--;
        else flashError = false;

        // Buttons below grid 
        float gridWidth = 30*9;
        float gridX = (GetScreenWidth() - gridWidth)+10;
        float gridY = 30*5;

        // Back & Solution buttons positioned below grid
        Rectangle backBtn = {60, gridY + 20, 80, 30};
        Rectangle resetBtn = {60, gridY + 30*8, 80, 30};
        Rectangle showBtn = {gridX + 120, gridY + 30*8, 100, 30};
        Vector2 mistakesPos = {gridX + 120, gridY + 20};
        Vector2 finishedGame = {gridX + 120, gridY + 30*4};

        if (CheckCollisionPointRec(mouse, backBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            gameState = GAME_MENU;
        }
        if (CheckCollisionPointRec(mouse, showBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            showSolution = true;
        }
        if (CheckCollisionPointRec(mouse, resetBtn) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            make_puzzle(grid, initial_grid, difficultyHoles);
            activeCellIndex = -1;
            flashError = false;
            flashFrames = 0;
            mistakes = 0;
            showSolution = false;
        }

        // --- DRAW GAME ---
        BeginDrawing();
        ClearBackground(WHITE); // background color set to white

        // Grid cells
        for (int i = 0; i < gridCommands.length; i++) {
            Clay_RenderCommand *cmd = Clay_RenderCommandArray_Get(&gridCommands, i);
            if ((int)cmd->boundingBox.width != 30 || (int)cmd->boundingBox.height != 30) continue;

            Rectangle cellRect = {cmd->boundingBox.x, cmd->boundingBox.y, cmd->boundingBox.width, cmd->boundingBox.height};
            bool hovered = CheckCollisionPointRec(mouse, cellRect);
            Color fillColor;

            if (showSolution) { 
                fillColor = (Color){STATIC_CELL_COLOR.r, STATIC_CELL_COLOR.g, STATIC_CELL_COLOR.b, STATIC_CELL_COLOR.a}; 
            } 
            else if (i == activeCellIndex && flashFrames > 0) { 
                fillColor = (Color){255, 100, 100, 255}; 
            } 
            else if (i == activeCellIndex) {
                fillColor = (Color){ACTIVE_COLOR.r, ACTIVE_COLOR.g, ACTIVE_COLOR.b, ACTIVE_COLOR.a}; 
            } 
            else if (hovered) {
                fillColor = (Color){HOVER_COLOR.r, HOVER_COLOR.g, HOVER_COLOR.b, HOVER_COLOR.a}; 
            } 
            else { 
                fillColor = (initial_grid[i] != 0) ? (Color){STATIC_CELL_COLOR.r, STATIC_CELL_COLOR.g, STATIC_CELL_COLOR.b, STATIC_CELL_COLOR.a} : (Color){EDITABLE_CELL_COLOR.r, EDITABLE_CELL_COLOR.g, EDITABLE_CELL_COLOR.b, EDITABLE_CELL_COLOR.a}; 
            }

            DrawRectangleRec(cellRect, fillColor);
            DrawRectangleLinesEx(cellRect, 1, BLACK);

            // Numbers
            if (grid[i] != 0 || showSolution) {
                char label[3];
                snprintf(label, sizeof(label), "%d", showSolution ? solution_grid[i] : grid[i]);
                Vector2 ts = MeasureTextEx(myFont, label, fontSize, 0);
                Color textColor = (initial_grid[i] != 0) ? GIVEN_NUM_COLOR : USER_NUM_COLOR;
                DrawTextEx(myFont, label, 
                    (Vector2){cellRect.x + (cellRect.width - ts.x)/2-5.0f, cellRect.y + (cellRect.height - ts.y)/2-8.0f},
                    fontSize, 0, textColor);
            }
        }

        Color backColor = CheckCollisionPointRec(mouse, backBtn) ? (Color){HOVER_COLOR.r, HOVER_COLOR.g, HOVER_COLOR.b, HOVER_COLOR.a} : (Color){255,255,255,255};
        Color showColor = CheckCollisionPointRec(mouse, showBtn) ? (Color){HOVER_COLOR.r, HOVER_COLOR.g, HOVER_COLOR.b, HOVER_COLOR.a} : (Color){255,255,255,255};
        Color resetColor = CheckCollisionPointRec(mouse, resetBtn) ? (Color){HOVER_COLOR.r, HOVER_COLOR.g, HOVER_COLOR.b, HOVER_COLOR.a} : (Color){255,255,255,255};


        // Back button
        DrawRectangleRec(backBtn, backColor);
        DrawRectangleLinesEx(backBtn, 1, BLACK);
        Vector2 backTextSize = MeasureTextEx(myFont, "BACK", 16, 0);
        DrawTextEx(myFont, "BACK", 
            (Vector2){backBtn.x + (backBtn.width - backTextSize.x)/2-20.0f, backBtn.y + (backBtn.height - backTextSize.y)/2-8.0f},
            16, 0, BLACK);

        // Show Solution button
        DrawRectangleRec(showBtn, showColor);
        DrawRectangleLinesEx(showBtn, 1, BLACK);
        Vector2 showTextSize = MeasureTextEx(myFont, "SOLUTION", 16, 0);
        DrawTextEx(myFont, "SOLUTION", 
            (Vector2){showBtn.x + (showBtn.width - showTextSize.x)/2-35.0f, showBtn.y + (showBtn.height - showTextSize.y)/2-8.0f},
            16, 0, BLACK);

        
        // Reset button
        DrawRectangleRec(resetBtn, resetColor);
        DrawRectangleLinesEx(resetBtn, 1, BLACK);
        Vector2 resetTextSize = MeasureTextEx(myFont, "RESET", 16, 0);
        DrawTextEx(myFont, "RESET", 
            (Vector2){resetBtn.x + (resetBtn.width - resetTextSize.x)/2-25.0f, resetBtn.y + (resetBtn.height - resetTextSize.y)/2-8.0f}, 
            16, 0, BLACK);

        // Mistakes counter
        DrawTextEx(myFont, TextFormat("Mistakes: %d/%d", mistakes, maxMistakes), mistakesPos, 16, 0, RED);

        if (is_complete() && !showSolution) {
            DrawTextEx(myFont, TextFormat("Sudoku Complete! 🎉"), finishedGame, GetScreenHeight()-30, 20, DARKBLUE);
        }

        EndDrawing();
    }

    UnloadFont(myFont);
    free(memory);
    CloseWindow();
    return 0;
}

