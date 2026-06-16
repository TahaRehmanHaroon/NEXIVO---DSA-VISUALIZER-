
#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800

#define GRID_SIZE 40


#define GRID_WIDTH  (SCREEN_WIDTH / GRID_SIZE)
#define GRID_HEIGHT (SCREEN_HEIGHT / GRID_SIZE)

typedef enum { SNAKE_MENU, SNAKE_GAME, SNAKE_CONTROLS, SNAKE_GAME_OVER, SNAKE_EXIT } SnakeScreen;

typedef enum { SNAKE_UP, SNAKE_DOWN, SNAKE_LEFT, SNAKE_RIGHT } SnakeDirection;

typedef struct SnakeNode {
    int x;
    int y;
    struct SnakeNode* next;
} SnakeNode;

typedef struct {
    SnakeNode* head;   
    SnakeNode* tail;  
    SnakeDirection dir;
    SnakeDirection nextDir;
    int length;
} Snake;

typedef struct {
    int x;
    int y;
} Food;

void InitGame(void);

void DrawMenu(void);
void UpdateMenu(void);

void DrawControls(void);
void UpdateControls(void);

void UpdateGame(void);
void DrawGame(void);
void InitSnake(void);
void FreeSnake(void);
void MoveSnake(void);
bool CheckCollision(int x, int y);
bool CheckCollisionExceptTail(int x, int y);
void AddSnakeHead(int x, int y);
void RemoveSnakeTail(void);
void SpawnFood(void);

void DrawGameOver(void);
void UpdateGameOver(void);

void DrawTextCentered(const char* text, int y, int fontSize, Color color);

SnakeScreen currentsnakeScreen = SNAKE_MENU;

Snake snake = {0};
Food food = {0};

int score = 0;
int highScore = 0;

int framesCounter = 0;
int gameSpeed = 8; 

const char* menuItems[] = { "Start Game", "Controls", "Exit" };
int menuItemCount = 3;
int menuSelected = 0;

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snake Game - raylib C");
    SetTargetFPS(45);

    InitGame();

    while (!WindowShouldClose() && currentsnakeScreen!= SNAKE_EXIT) {
        switch (currentsnakeScreen) {
            case SNAKE_MENU: UpdateMenu(); break;
            case SNAKE_CONTROLS: UpdateControls(); break;
            case SNAKE_GAME: UpdateGame(); break;
            case SNAKE_GAME_OVER: UpdateGameOver(); break;
            default: break;
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);
        switch (currentsnakeScreen) {
            case SNAKE_MENU: DrawMenu(); break;
            case SNAKE_CONTROLS: DrawControls(); break;
            case SNAKE_GAME: DrawGame(); break;
            case SNAKE_GAME_OVER: DrawGameOver(); break;
            default: break;
        }
        EndDrawing();
    }

    FreeSnake();
    CloseWindow();
    return 0;
}

void InitGame(void) {
    FreeSnake();
    InitSnake();
    SpawnFood();
    score = 0;
    framesCounter = 0;
    gameSpeed = 14;
}

void InitSnake(void) {
    snake.head = NULL;
    snake.tail = NULL;
    snake.length = 0;
    snake.dir = SNAKE_RIGHT;
    snake.nextDir = SNAKE_RIGHT;

    int startX = GRID_WIDTH / 2;
    int startY = GRID_HEIGHT / 2;

    AddSnakeHead(startX - 2, startY);
    AddSnakeHead(startX - 1, startY);
    AddSnakeHead(startX, startY);
}

void FreeSnake(void) {
    SnakeNode* current = snake.head;
    while (current != NULL) {
        SnakeNode* temp = current;
        current = current->next;
        free(temp);
    }
    snake.head = NULL;
    snake.tail = NULL;
    snake.length = 0;
}

void AddSnakeHead(int x, int y) {
    SnakeNode* node = (SnakeNode*)malloc(sizeof(SnakeNode));
    node->x = x;
    node->y = y;
    node->next = snake.head;
    snake.head = node;
    if (snake.tail == NULL) snake.tail = node;
    snake.length++;
}

void RemoveSnakeTail(void) {
    if (!snake.head) return;

    if (snake.head == snake.tail) {
        free(snake.head);
        snake.head = NULL;
        snake.tail = NULL;
        snake.length = 0;
        return;
    }

    SnakeNode* current = snake.head;
    while (current->next != snake.tail) {
        current = current->next;
    }
    free(snake.tail);
    snake.tail = current;
    snake.tail->next = NULL;
    snake.length--;
}

void SpawnFood(void) {
    int x, y;
    do {
        x = GetRandomValue(0, GRID_WIDTH - 1);
        y = GetRandomValue(0, GRID_HEIGHT - 1);
    } while (CheckCollision(x, y));
    food.x = x;
    food.y = y;
}

bool CheckCollision(int x, int y) {
    SnakeNode* current = snake.head;
    while (current != NULL) {
        if (current->x == x && current->y == y) return true;
        current = current->next;
    }
    return false;
}

bool CheckCollisionExceptTail(int x, int y) {
    if (!snake.head) return false;
    SnakeNode* current = snake.head;
    while (current != NULL && current != snake.tail) {
        if (current->x == x && current->y == y) return true;
        current = current->next;
    }
    return false;
}

void MoveSnake(void) {
    
    if ((snake.nextDir == SNAKE_UP && snake.dir != SNAKE_DOWN) ||
        (snake.nextDir == SNAKE_DOWN && snake.dir != SNAKE_UP) ||
        (snake.nextDir == SNAKE_LEFT && snake.dir != SNAKE_RIGHT) ||
        (snake.nextDir == SNAKE_RIGHT && snake.dir != SNAKE_LEFT)) {
        snake.dir = snake.nextDir;
    }

    int newX = snake.head->x;
    int newY = snake.head->y;

    switch (snake.dir) {
        case SNAKE_UP: newY--; break;
        case SNAKE_DOWN: newY++; break;
        case SNAKE_LEFT: newX--; break;
        case SNAKE_RIGHT: newX++; break;
    }

   
    if (newX < 0) newX = GRID_WIDTH - 1;
    if (newX >= GRID_WIDTH) newX = 0;
    if (newY < 0) newY = GRID_HEIGHT - 1;
    if (newY >= GRID_HEIGHT) newY = 0;

    
    if (CheckCollisionExceptTail(newX, newY)) {
        currentsnakeScreen = SNAKE_GAME_OVER;
        if (score > highScore) highScore = score;
        return;
    }

   
    AddSnakeHead(newX, newY);

  
    if (newX == food.x && newY == food.y) {
        score += 10;
        SpawnFood();
        
        if (score % 50 == 0 && gameSpeed < 15) gameSpeed++;
    } else {
        
        RemoveSnakeTail();
    }
}

void UpdateGame(void) {
    
    if (IsKeyPressed(KEY_UP) && snake.dir != SNAKE_DOWN) snake.nextDir = SNAKE_UP;
    else if (IsKeyPressed(KEY_DOWN) && snake.dir != SNAKE_UP) snake.nextDir = SNAKE_DOWN;
    else if (IsKeyPressed(KEY_LEFT) && snake.dir != SNAKE_RIGHT) snake.nextDir = SNAKE_LEFT;
    else if (IsKeyPressed(KEY_RIGHT) && snake.dir != SNAKE_LEFT) snake.nextDir = SNAKE_RIGHT;

    if (IsKeyPressed(KEY_ESCAPE)) {
        currentsnakeScreen = SNAKE_MENU;
        InitGame();
    }

    framesCounter++;
    if (framesCounter >= 60 / gameSpeed) {
        MoveSnake();
        framesCounter = 0;
    }
}

void DrawGame(void) {
    ClearBackground(RAYWHITE);

    for (int x = 0; x <= SCREEN_WIDTH; x += GRID_SIZE)
        DrawLine(x, 0, x, SCREEN_HEIGHT, LIGHTGRAY);
    for (int y = 0; y <= SCREEN_HEIGHT; y += GRID_SIZE)
        DrawLine(0, y, SCREEN_WIDTH, y, LIGHTGRAY);

 
    SnakeNode* current = snake.head;
    bool first = true;
    while (current) {
        Rectangle rect = {(float)(current->x * GRID_SIZE), (float)(current->y * GRID_SIZE), (float)GRID_SIZE, (float)GRID_SIZE};
        DrawRectangleRec(rect, first ? ColorFromHSV(120, 0.8f, 0.7f) : ColorFromHSV(120, 0.8f, 0.4f));
        first = false;
        current = current->next;
    }
  
    Rectangle foodRec = {(float)(food.x * GRID_SIZE), (float)(food.y * GRID_SIZE), (float)GRID_SIZE, (float)GRID_SIZE};
    DrawRectangleRec(foodRec, RED);

    DrawText(TextFormat("Score: %d", score), 10, 10, 20, BLACK);
    DrawText(TextFormat("High Score: %d", highScore), 10, 40, 20, BLACK);
    DrawText("Use arrow keys to move. ESC to menu.", 10, SCREEN_HEIGHT - 30, 15, BLACK);
}

void DrawMenu(void) {
    DrawTextCentered("SNAKE GAME", SCREEN_HEIGHT / 4, 40, BLACK);
    Vector2 mousePoint = GetMousePosition();

    for (int i = 0; i < menuItemCount; i++) {
        int textWidth = MeasureText(menuItems[i], 30);
        int posX = SCREEN_WIDTH / 2 - textWidth / 2;
        int posY = SCREEN_HEIGHT / 2 + i * 40;
        Rectangle btnRec = {(float)(posX - 10), (float)(posY - 5), (float)(textWidth + 20), 40};

        bool mouseOver = CheckCollisionPointRec(mousePoint, btnRec);
        Color color = (i == menuSelected || mouseOver) ? DARKGRAY : BLACK;
        DrawText(menuItems[i], posX, posY, 30, color);
        if (mouseOver) DrawRectangleLinesEx(btnRec, 2, GRAY);
    }

    DrawText("Use UP/DOWN keys to navigate, ENTER to select or click with mouse.", SCREEN_WIDTH / 2 - 320, SCREEN_HEIGHT - 50, 15, BLACK);
}

void UpdateMenu(void) {
    if (IsKeyPressed(KEY_UP)) {
        menuSelected--;
        if (menuSelected < 0) menuSelected = menuItemCount - 1;
    }
    if (IsKeyPressed(KEY_DOWN)) {
        menuSelected++;
        if (menuSelected >= menuItemCount) menuSelected = 0;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
        switch (menuSelected) {
            case 0: currentsnakeScreen = SNAKE_GAME; InitGame(); break;
            case 1: currentsnakeScreen = SNAKE_CONTROLS; break;
            case 2: currentsnakeScreen = SNAKE_EXIT; break;
        }
    }
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mousePoint = GetMousePosition();
        for (int i = 0; i < menuItemCount; i++) {
            int textWidth = MeasureText(menuItems[i], 30);
            int posX = SCREEN_WIDTH / 2 - textWidth / 2;
            int posY = SCREEN_HEIGHT / 2 + i * 40;
            Rectangle btnRec = {(float)(posX - 10), (float)(posY - 5), (float)(textWidth + 20), 40};
            if (CheckCollisionPointRec(mousePoint, btnRec)) {
                menuSelected = i;
                switch (menuSelected) {
                    case 0: currentsnakeScreen = SNAKE_GAME; InitGame(); break;
                    case 1: currentsnakeScreen = SNAKE_CONTROLS; break;
                    case 2: currentsnakeScreen = SNAKE_EXIT; break;
                }
                break;
            }
        }
    }
}

void DrawControls(void) {
    DrawTextCentered("CONTROLS", SCREEN_HEIGHT / 6, 40, BLACK);
    DrawText("- Use Arrow Keys to move Snake", SCREEN_WIDTH / 2 - 160, SCREEN_HEIGHT / 3, 25, BLACK);
    DrawText("- Eat food (red blocks) to grow and score", SCREEN_WIDTH / 2 - 230, SCREEN_HEIGHT / 3 + 40, 25, BLACK);
    DrawText("- Don't collide with yourself", SCREEN_WIDTH / 2 - 150, SCREEN_HEIGHT / 3 + 80, 25, BLACK);
    DrawText("- Press ESC, M, or click button below to return to Main Menu", SCREEN_WIDTH / 2 - 280, SCREEN_HEIGHT / 3 + 120, 25, BLACK);

    const char* returnText = "Return to Main Menu";
    int textWidth = MeasureText(returnText, 25);
    int posX = SCREEN_WIDTH / 2 - textWidth / 2;
    int posY = SCREEN_HEIGHT - 100;
    Rectangle btnRec = {(float)(posX - 10), (float)(posY - 10), (float)(textWidth + 20), 40};
    Vector2 mousePoint = GetMousePosition();
    bool mouseOver = CheckCollisionPointRec(mousePoint, btnRec);
    DrawRectangleRec(btnRec, mouseOver ? LIGHTGRAY : GRAY);
    DrawText(returnText, posX, posY, 25, BLACK);
}

void UpdateControls(void) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_M)) {
        currentsnakeScreen = SNAKE_MENU;
    }
    Vector2 mousePoint = GetMousePosition();
    const char* returnText = "Return to Main Menu";
    int textWidth = MeasureText(returnText, 25);
    int posX = SCREEN_WIDTH / 2 - textWidth / 2;
    int posY = SCREEN_HEIGHT - 100;
    Rectangle btnRec = {(float)(posX - 10), (float)(posY - 10), (float)(textWidth + 20), 40};
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePoint, btnRec)) {
        currentsnakeScreen = SNAKE_MENU;
    }
}

void DrawGameOver(void) {
    DrawTextCentered("GAME OVER", SCREEN_HEIGHT / 3, 60, BLACK);
    DrawTextCentered(TextFormat("Your Score: %d", score), SCREEN_HEIGHT / 3 + 80, 30, BLACK);
    DrawTextCentered("Press ENTER to go to Menu", SCREEN_HEIGHT / 3 + 140, 25, BLACK);
    DrawTextCentered("Press ESC, M, or click button below to return to Main Menu", SCREEN_HEIGHT / 3 + 180, 20, BLACK);

    const char* returnText = "Return to Main Menu";
    int textWidth = MeasureText(returnText, 25);
    int posX = SCREEN_WIDTH / 2 - textWidth / 2;
    int posY = SCREEN_HEIGHT / 2 + 150;
    Rectangle btnRec = {(float)(posX - 10), (float)(posY - 10), (float)(textWidth + 20), 40};
    Vector2 mousePoint = GetMousePosition();
    bool mouseOver = CheckCollisionPointRec(mousePoint, btnRec);
    DrawRectangleRec(btnRec, mouseOver ? LIGHTGRAY : GRAY);
    DrawText(returnText, posX, posY, 25, BLACK);
}

void UpdateGameOver(void) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_M)) {
        currentsnakeScreen = SNAKE_MENU;
        InitGame();
    }

    Vector2 mousePoint = GetMousePosition();
    const char* returnText = "Return to Main Menu";
    int textWidth = MeasureText(returnText, 25);
    int posX = SCREEN_WIDTH / 2 - textWidth / 2;
    int posY = SCREEN_HEIGHT / 2 + 150;
    Rectangle btnRec = {(float)(posX - 10), (float)(posY - 10), (float)(textWidth + 20), 40};
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mousePoint, btnRec)) {
        currentsnakeScreen = SNAKE_MENU;
        InitGame();
    }
}

void DrawTextCentered(const char* text, int y, int fontSize, Color color) {
    int textWidth = MeasureText(text, fontSize);
    DrawText(text, SCREEN_WIDTH / 2 - textWidth / 2, y, fontSize, color);
}

