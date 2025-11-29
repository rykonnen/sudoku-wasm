#include "gui.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

GameState gameState = GAME_MENU;
int activeCellIndex = -1;
int hoveredDifficulty = -1;

Clay_Color STATIC_CELL_COLOR   = {236, 255, 255, 255};
Clay_Color EDITABLE_CELL_COLOR = {255, 255, 255, 255};
Clay_Color HOVER_COLOR         = {182, 214, 255, 255};
Clay_Color ACTIVE_COLOR        = {109, 202, 209, 255};
Color GIVEN_NUM_COLOR          = {0, 0, 0, 255};
Color USER_NUM_COLOR           = {44, 53, 54, 255};

NumberButton numberButtons[NUM_COUNT];

bool flashError = false;
int flashFrames = 0;
const int flashDuration = 15;



//start clay layouts
Clay_RenderCommandArray CreateGridLayout(void) {
    Clay_BeginLayout();
    int cellSize = 30;
    int gap = 0;
    int padding = 10;

    CLAY(CLAY_ID("GridWrapper"), {
        .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER},
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




void render_game_loop(Clay_Arena arena, int fontSize) {


    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();

        //MENU
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

            //Draw Menu
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

                int textSize = MeasureText("EASY", 24);
                DrawText(labels[b],
                    btnRects[b].x + (btnRects[b].width - textSize)/2-15.0f,
                    btnRects[b].y + (btnRects[b].height - textSize)/2+22.0f,
                    24, BLACK);
            }

            EndDrawing();
            continue;
        }

        //GAMEPLAY
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
        if (!showSolution && activeCellIndex != -1 && initial_grid[activeCellIndex] == 0 && !gameComplete) {
            int key = GetKeyPressed();
            if (key >= KEY_ONE && key <= KEY_NINE) {
                u8 val = (u8)(key - KEY_ZERO);

                if (val == solution_grid[activeCellIndex]) {
                    grid[activeCellIndex] = val;
                    flashError = false;
                } else {
                    flashError = true;
                    flashFrames = flashDuration;
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

        //DRAW GAME
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
                int ts = MeasureText(label, fontSize);
                Color textColor = (initial_grid[i] != 0) ? GIVEN_NUM_COLOR : USER_NUM_COLOR;
                DrawText(label, 
                    cellRect.x + (cellRect.width - ts)/2, cellRect.y + (cellRect.height - ts)/2-3.0f,
                    fontSize, textColor);
            }
        }

        Color backColor = CheckCollisionPointRec(mouse, backBtn) ? (Color){HOVER_COLOR.r, HOVER_COLOR.g, HOVER_COLOR.b, HOVER_COLOR.a} : (Color){255,255,255,255};
        Color showColor = CheckCollisionPointRec(mouse, showBtn) ? (Color){HOVER_COLOR.r, HOVER_COLOR.g, HOVER_COLOR.b, HOVER_COLOR.a} : (Color){255,255,255,255};
        Color resetColor = CheckCollisionPointRec(mouse, resetBtn) ? (Color){HOVER_COLOR.r, HOVER_COLOR.g, HOVER_COLOR.b, HOVER_COLOR.a} : (Color){255,255,255,255};


        // Back button
        DrawRectangleRec(backBtn, backColor);
        DrawRectangleLinesEx(backBtn, 1, BLACK);
        int backTextSize = MeasureText("BACK", 16);
        DrawText("BACK", 
            backBtn.x + (backBtn.width - backTextSize)/2, backBtn.y + (backBtn.height - backTextSize)/2+14.0f,
            16, BLACK);

        // Show Solution button
        DrawRectangleRec(showBtn, showColor);
        DrawRectangleLinesEx(showBtn, 1, BLACK);
        int showTextSize = MeasureText("SOLUTION", 16);
        DrawText("SOLUTION", 
            showBtn.x + (showBtn.width - showTextSize)/2, showBtn.y + (showBtn.height - showTextSize)/2+32.0f,
            16, BLACK);

        
        // Reset button
        DrawRectangleRec(resetBtn, resetColor);
        DrawRectangleLinesEx(resetBtn, 1, BLACK);
        int resetTextSize = MeasureText("RESET", 16);
        DrawText("RESET", 
            resetBtn.x + (resetBtn.width - resetTextSize)/2, resetBtn.y + (resetBtn.height - resetTextSize)/2+20.0f, 
            16, BLACK);

        // Mistakes counter
        DrawText(
            TextFormat("Mistakes: %d/%d", mistakes, maxMistakes),
            mistakesPos.x,
            mistakesPos.y,
            16,
            RED
        );

        if (is_complete() && !gameComplete) {
            gameComplete = true;
            showSolution = true; 

        }

        if (gameComplete) {

            DrawText(
                "Sudoku \n Complete! 🎉",
                finishedGame.x,
                finishedGame.y,
                20,
                DARKBLUE
            );

            
        }
        

        EndDrawing();
    }
}

