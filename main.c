#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"

#define EMPTY_COLOR      \
    CLITERAL(Color) {    \
        59, 74, 107, 255 \
    }
#define HEAD_COLOR        \
    CLITERAL(Color) {     \
        34, 178, 218, 255 \
    }
#define TAIL_COLOR       \
    CLITERAL(Color) {    \
        242, 53, 87, 255 \
    }
#define CONDUCTOR_COLOR   \
    CLITERAL(Color) {     \
        240, 212, 58, 255 \
    }

typedef enum { EMPTY, CONDUCTOR, HEAD, TAIL } Cell;

const int screenWidth = 800;
const int screenHeight = 460;

const int cellSize = 20;

const int indicatorSize = cellSize;
const int indicatorPadding = cellSize / 2;
const int indicatorX = screenWidth - indicatorSize * 4 - indicatorPadding;
const int indicatorY = indicatorPadding;

const int playButtonX = cellSize / 2;
const int playButtonY = cellSize / 2;
const int playButtonSize = cellSize;
const int playButtonBarWidth = playButtonSize / 4;
const int playButtonBarGap = playButtonBarWidth;

const int nextButtonX = playButtonX + playButtonSize + cellSize / 2;
const int nextButtonY = cellSize / 2;
const int nextButtonSize = cellSize;
const int nextButtonBarWidth = playButtonSize / 4;

const Rectangle playButtonRect = {playButtonX, playButtonY, playButtonSize,
                                  playButtonSize};
const Rectangle nextButtonRect = {nextButtonX, nextButtonY, nextButtonSize,
                                  nextButtonSize};
const Rectangle indicatorGruopRect = {indicatorX, indicatorY, indicatorSize * 4,
                                      indicatorSize};

const int rows = screenHeight / cellSize;
const int cols = screenWidth / cellSize;

int isPlaying = 0;

const int refreshRate = 5;
const float refreshInterval = 1.0f / refreshRate;

Cell selectCellType = EMPTY;
Cell cellTypes[] = {EMPTY, CONDUCTOR, HEAD, TAIL};

Cell** grid = NULL;

Camera2D camera = {0};

Color GetCellColor(Cell cell) {
    switch (cell) {
        case EMPTY:
            return EMPTY_COLOR;
        case HEAD:
            return HEAD_COLOR;
        case TAIL:
            return TAIL_COLOR;
        case CONDUCTOR:
            return CONDUCTOR_COLOR;
        default:
            return EMPTY_COLOR;
    }
}

int CountHeadNeighbors(int row, int col) {
    int headCount = 0;

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            if (y == 0 && x == 0)
                continue;
            int newRow = row + y;
            int newCol = col + x;
            if (newRow >= 0 && newRow < rows && newCol >= 0 && newCol < cols) {
                if (grid[newRow][newCol] == HEAD) {
                    headCount++;
                }
            }
        }
    }

    return headCount;
}

void ClearGrid(void) {
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            grid[y][x] = EMPTY;
        }
    }
}

void InitGrid(void) {
    grid = malloc(rows * sizeof(Cell*));
    for (int y = 0; y < rows; y++) {
        grid[y] = malloc(cols * sizeof(Cell));
    }
    ClearGrid();
}

void FreeGrid(void) {
    for (int i = 0; i < rows; i++) {
        free(grid[i]);
    }
    free(grid);
}

void UpdateGrid(void) {
    Cell newGrid[rows][cols];

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            switch (grid[y][x]) {
                case EMPTY:
                    newGrid[y][x] = EMPTY;
                    break;
                case HEAD:
                    newGrid[y][x] = TAIL;
                    break;
                case TAIL:
                    newGrid[y][x] = CONDUCTOR;
                    break;
                case CONDUCTOR: {
                    int headNeighbors = CountHeadNeighbors(y, x);
                    if (headNeighbors == 1 || headNeighbors == 2) {
                        newGrid[y][x] = HEAD;
                    } else {
                        newGrid[y][x] = CONDUCTOR;
                    }
                } break;
            }
        }
    }

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            grid[y][x] = newGrid[y][x];
        }
    }
}

void DrawCell(int xGridPos, int yGridPos, Color cellColor) {
    DrawRectangle(xGridPos * cellSize, yGridPos * cellSize, cellSize, cellSize,
                  cellColor);
    DrawRectangleLines(xGridPos * cellSize, yGridPos * cellSize, cellSize,
                       cellSize, BLACK);
}

void DrawCellLines(int xGridPos, int yGridPos, Color cellColor) {
    DrawRectangleLines(xGridPos * cellSize, yGridPos * cellSize, cellSize,
                       cellSize, cellColor);
}

void DrawIndicators(void) {
    for (int i = 0; i < 4; i++) {
        int x = indicatorX + indicatorSize * i;
        DrawRectangle(x, indicatorY, indicatorSize, indicatorSize,
                      GetCellColor(cellTypes[i]));
        DrawRectangleLines(x, indicatorY, indicatorSize, indicatorSize, BLACK);
        if (selectCellType == cellTypes[i]) {
            DrawRectangleLines(x, indicatorY, indicatorSize, indicatorSize,
                               WHITE);
        }
    }
}

void DrawPlayButton(void) {
    if (isPlaying) {
        DrawRectangle(playButtonX, playButtonY, playButtonBarWidth,
                      playButtonSize, WHITE);
        DrawRectangle(playButtonX + playButtonBarWidth + playButtonBarGap,
                      playButtonY, playButtonBarWidth, playButtonSize, WHITE);
    } else {
        Vector2 v1 = (Vector2){playButtonX, playButtonY};
        Vector2 v2 = (Vector2){playButtonX, playButtonY + playButtonSize};
        Vector2 v3 = (Vector2){playButtonX + playButtonSize,
                               (playButtonY + playButtonSize / 2.0)};
        DrawTriangle(v1, v2, v3, WHITE);
    }
}

void DrawNextButton(void) {
    if (!isPlaying) {
        Vector2 v1 = (Vector2){nextButtonX, nextButtonY};
        Vector2 v2 = (Vector2){nextButtonX, nextButtonY + nextButtonSize};
        Vector2 v3 = (Vector2){nextButtonX + nextButtonSize,
                               (nextButtonY + nextButtonSize / 2.0)};
        DrawTriangle(v1, v2, v3, WHITE);
        DrawRectangle(nextButtonX + nextButtonSize - nextButtonBarWidth,
                      nextButtonY, nextButtonBarWidth, nextButtonSize, WHITE);
    }
}

void HandleCellPlacements(void) {
    Vector2 mousePosition = GetMousePosition();
    Vector2 mouseScreenPosition = GetScreenToWorld2D(mousePosition, camera);
    int mouseYGridPos = (int)(mouseScreenPosition.y / cellSize);
    int mouseXGridPos = (int)(mouseScreenPosition.x / cellSize);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(mousePosition, playButtonRect) &&
        !CheckCollisionPointRec(mousePosition, nextButtonRect) &&
        !CheckCollisionPointRec(mousePosition, indicatorGruopRect)) {
        if ((mouseXGridPos >= 0 && mouseXGridPos < cols) &&
            (mouseYGridPos >= 0 && mouseYGridPos < rows)) {
            grid[mouseYGridPos][mouseXGridPos] = selectCellType;
            DrawCell(mouseXGridPos, mouseYGridPos,
                     GetCellColor(selectCellType));
        }
    }

    DrawCellLines(mouseXGridPos, mouseYGridPos, WHITE);
}

void HandleButtonClicks(void) {
    Vector2 mousePosition = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mousePosition, playButtonRect))
            isPlaying = !isPlaying;

        if (CheckCollisionPointRec(mousePosition, nextButtonRect) && !isPlaying)
            UpdateGrid();

        for (int i = 0; i < 4; i++) {
            int x = indicatorX + indicatorSize * i;
            Rectangle indicatorRect = {x, indicatorY, indicatorSize,
                                       indicatorSize};
            if (CheckCollisionPointRec(mousePosition, indicatorRect)) {
                selectCellType = cellTypes[i];
                break;
            }
        }
    }
}

void HandleShortcuts(void) {
    if (IsKeyPressed(KEY_SPACE))
        isPlaying = !isPlaying;

    if (IsKeyPressed(KEY_N) && !isPlaying) {
        UpdateGrid();
    }

    if (IsKeyPressed(KEY_C)) {
        ClearGrid();
    }

    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1))
        selectCellType = EMPTY;
    else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2))
        selectCellType = CONDUCTOR;
    else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3))
        selectCellType = HEAD;
    else if (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4))
        selectCellType = TAIL;
}

void HandleCameraMovement(void) {
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / camera.zoom);
        camera.target = Vector2Add(camera.target, delta);
    }
}

void HandleUserInput(void) {
    HandleCellPlacements();
    HandleButtonClicks();
    HandleShortcuts();
    HandleCameraMovement();
}

int main(void) {
    InitWindow(screenWidth, screenHeight, "Wireworld Simulator");
    SetTargetFPS(60);

    camera.zoom = 1.0f;

    InitGrid();

    float elapsedTime = 0.0f;

    while (!WindowShouldClose()) {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        float frameTime = GetFrameTime();
        elapsedTime += frameTime;

        if (isPlaying && elapsedTime >= refreshInterval) {
            UpdateGrid();
            elapsedTime = 0.0f;
        }

        for (int y = 0; y < rows; y++) {
            for (int x = 0; x < cols; x++) {
                Color cellColor = GetCellColor(grid[y][x]);
                DrawCell(x, y, cellColor);
            }
        }

        HandleUserInput();

        EndMode2D();

        DrawIndicators();

        DrawPlayButton();

        DrawNextButton();

        EndDrawing();
    }

    CloseWindow();

    FreeGrid();

    return 0;
}
