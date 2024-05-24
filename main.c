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

int isPlaying = 0;

const int refreshRate = 5;
const float refreshInterval = 1.0f / refreshRate;

Cell selectCellType = EMPTY;
Cell cellTypes[] = {EMPTY, CONDUCTOR, HEAD, TAIL};

typedef enum { UP, DOWN, LEFT, RIGHT } Direction;

typedef struct {
    Vector2 position;
    int rows;
    int cols;
    Cell** cells;
} Grid;

Grid grid = {{0.0f, 0.0f},
             screenHeight / cellSize,
             screenWidth / cellSize,
             NULL};

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
            if (newRow >= 0 && newRow < grid.rows && newCol >= 0 &&
                newCol < grid.cols) {
                if (grid.cells[newRow][newCol] == HEAD) {
                    headCount++;
                }
            }
        }
    }

    return headCount;
}

void ClearGrid(void) {
    for (int y = 0; y < grid.rows; y++) {
        for (int x = 0; x < grid.cols; x++) {
            grid.cells[y][x] = EMPTY;
        }
    }
}

void InitGrid(void) {
    grid.cells = RL_CALLOC(grid.rows, sizeof(Cell*));
    for (int y = 0; y < grid.rows; y++) {
        grid.cells[y] = RL_CALLOC(grid.cols, sizeof(Cell));
    }
}

void FreeGrid(void) {
    for (int i = 0; i < grid.rows; i++) {
        RL_FREE(grid.cells[i]);
    }
    RL_FREE(grid.cells);
}

void UpdateGrid(void) {
    Cell newCells[grid.rows][grid.cols];

    for (int y = 0; y < grid.rows; y++) {
        for (int x = 0; x < grid.cols; x++) {
            switch (grid.cells[y][x]) {
                case EMPTY:
                    newCells[y][x] = EMPTY;
                    break;
                case HEAD:
                    newCells[y][x] = TAIL;
                    break;
                case TAIL:
                    newCells[y][x] = CONDUCTOR;
                    break;
                case CONDUCTOR: {
                    int headNeighbors = CountHeadNeighbors(y, x);
                    if (headNeighbors == 1 || headNeighbors == 2) {
                        newCells[y][x] = HEAD;
                    } else {
                        newCells[y][x] = CONDUCTOR;
                    }
                } break;
            }
        }
    }

    for (int y = 0; y < grid.rows; y++) {
        for (int x = 0; x < grid.cols; x++) {
            grid.cells[y][x] = newCells[y][x];
        }
    }
}

void ExpandGrid(Direction direction, int expandSize) {
    int newRows =
        grid.rows + (direction == UP || direction == DOWN ? expandSize : 0);
    int newCols =
        grid.cols + (direction == LEFT || direction == RIGHT ? expandSize : 0);
    int xOffset = (direction == LEFT ? expandSize : 0);
    int yOffset = (direction == UP ? expandSize : 0);

    Cell** newCells = RL_CALLOC(newRows, sizeof(Cell*));
    for (int y = 0; y < newRows; y++) {
        newCells[y] = RL_CALLOC(newCols, sizeof(Cell));
    }

    for (int y = 0; y < grid.rows; y++) {
        for (int x = 0; x < grid.cols; x++) {
            newCells[y + yOffset][x + xOffset] = grid.cells[y][x];
        }
    }

    FreeGrid();

    grid.cells = newCells;
    grid.rows = newRows;
    grid.cols = newCols;

    grid.position.x -= cellSize * xOffset;
    grid.position.y -= cellSize * yOffset;
}

void DrawCell(int xGridPos, int yGridPos, Color cellColor) {
    DrawRectangle(grid.position.x + xGridPos * cellSize,
                  grid.position.y + yGridPos * cellSize, cellSize, cellSize,
                  cellColor);
    DrawRectangleLines(grid.position.x + xGridPos * cellSize,
                       grid.position.y + yGridPos * cellSize, cellSize,
                       cellSize, BLACK);
}

void DrawCellLines(int xGridPos, int yGridPos, Color cellColor) {
    DrawRectangleLines(grid.position.x + xGridPos * cellSize,
                       grid.position.y + yGridPos * cellSize, cellSize,
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
    mouseScreenPosition = Vector2Subtract(mouseScreenPosition, grid.position);
    int mouseYGridPos = (int)(mouseScreenPosition.y / cellSize);
    int mouseXGridPos = (int)(mouseScreenPosition.x / cellSize);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(mousePosition, playButtonRect) &&
        !CheckCollisionPointRec(mousePosition, nextButtonRect) &&
        !CheckCollisionPointRec(mousePosition, indicatorGruopRect)) {
        if ((mouseXGridPos >= 0 && mouseXGridPos < grid.cols) &&
            (mouseYGridPos >= 0 && mouseYGridPos < grid.rows)) {
            grid.cells[mouseYGridPos][mouseXGridPos] = selectCellType;
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

        Vector2 cameraOffset = Vector2Subtract(camera.target, grid.position);
        bool isOutsideX = cameraOffset.x + screenWidth >
                          grid.position.x + grid.cols * cellSize;
        bool isOutsideY = cameraOffset.y + screenHeight >
                          grid.position.y + grid.rows * cellSize;
        if (cameraOffset.x < 0)
            ExpandGrid(LEFT, 10);
        if (cameraOffset.y < 0)
            ExpandGrid(UP, 10);
        if (isOutsideX)
            ExpandGrid(RIGHT, 10);
        if (isOutsideY)
            ExpandGrid(DOWN, 10);
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

    float elapsedTime = 0.0f;

    camera.zoom = 1.0f;

    InitGrid();

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

        for (int y = 0; y < grid.rows; y++) {
            for (int x = 0; x < grid.cols; x++) {
                Color cellColor = GetCellColor(grid.cells[y][x]);
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
