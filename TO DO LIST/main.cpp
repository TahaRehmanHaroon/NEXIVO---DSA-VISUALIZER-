#include <raylib.h>
#include <iostream>
#include <cstring>

using namespace std;

enum TDScreen { TD_MENU, TD_ADD_TASK };
TDScreen tdScreen = TD_MENU;

struct Task {
    char name[50];
    Task* prev;
    Task* next;
};

Task* head = nullptr;

void AddTask(const char* taskName) {
    Task* newTask = new Task();
    strcpy(newTask->name, taskName);
    newTask->next = nullptr;
    newTask->prev = nullptr;

    if (!head) head = newTask;
    else {
        Task* temp = head;
        while (temp->next) temp = temp->next;
        temp->next = newTask;
        newTask->prev = temp;
    }
}

void DeleteTask(int index) {
    if (!head) return;
    Task* temp = head;
    int count = 0;
    while (temp && count < index) {
        temp = temp->next;
        count++;
    }
    if (!temp) return;
    if (temp->prev) temp->prev->next = temp->next;
    else head = temp->next;
    if (temp->next) temp->next->prev = temp->prev;
    delete temp;
}

void MoveTaskUp(int index) {
    if (index <= 0) return;
    Task* curr = head;
    for (int i = 0; i < index && curr; ++i) curr = curr->next;
    if (!curr || !curr->prev) return;

    Task* prev = curr->prev;
    if (prev->prev) prev->prev->next = curr;
    curr->prev = prev->prev;
    prev->prev = curr;
    prev->next = curr->next;
    if (curr->next) curr->next->prev = prev;
    curr->next = prev;

    if (head == prev) head = curr;
}

void MoveTaskDown(int index) {
    MoveTaskUp(index + 1);
}

void ClearAllTasks() {
    while (head) {
        Task* next = head->next;
        delete head;
        head = next;
    }
}

void DrawTasks(int startY, int selected) {
    Task* temp = head;
    int y = startY;
    int index = 0;
    while (temp) {
        Color textColor = (index == selected) ? RED : BLACK;
        DrawText(TextFormat("%d. %s", index + 1, temp->name), 60, y, 22, textColor);
        y += 35;
        temp = temp->next;
        index++;
    }
}

int main() {
    InitWindow(800, 600, "Daily Routine Manager");
    SetTargetFPS(60);

    char inputBuffer[50] = "";
    int letterCount = 0;
    int selectedTask = 0;

    Rectangle btnAdd = { 600, 50, 160, 45 };
    Rectangle btnDelete = { 600, 105, 160, 45 };
    Rectangle btnMoveUp = { 600, 160, 160, 45 };
    Rectangle btnMoveDown = { 600, 215, 160, 45 };
    Rectangle btnClear = { 600, 270, 160, 45 };

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(Color{ 250, 245, 240, 255 });

        if (tdScreen == TD_MENU) {
            DrawText("TO-DO LIST", 60, 20, 30, DARKPURPLE);
            DrawTasks(80, selectedTask);

            DrawRectangleRec(btnAdd, Color{ 0, 168, 255, 255 });       DrawText("Add Task", btnAdd.x + 15, btnAdd.y + 10, 20, WHITE);
            DrawRectangleRec(btnDelete, Color{ 255, 94, 87, 255 });    DrawText("Delete Task", btnDelete.x + 15, btnDelete.y + 10, 20, WHITE);
            DrawRectangleRec(btnMoveUp, Color{ 255, 195, 0, 255 });    DrawText("Move Up", btnMoveUp.x + 15, btnMoveUp.y + 10, 20, BLACK);
            DrawRectangleRec(btnMoveDown, Color{ 144, 224, 239, 255 });DrawText("Move Down", btnMoveDown.x + 15, btnMoveDown.y + 10, 20, BLACK);
            DrawRectangleRec(btnClear, Color{ 255, 159, 243, 255 });   DrawText("Clear All", btnClear.x + 15, btnClear.y + 10, 20, BLACK);

            if (IsKeyPressed(KEY_DOWN)) selectedTask++;
            if (IsKeyPressed(KEY_UP) && selectedTask > 0) selectedTask--;

            Vector2 mouse = GetMousePosition();
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                if (CheckCollisionPointRec(mouse, btnAdd)) tdScreen = TD_ADD_TASK;
                else if (CheckCollisionPointRec(mouse, btnDelete)) DeleteTask(selectedTask);
                else if (CheckCollisionPointRec(mouse, btnMoveUp)) MoveTaskUp(selectedTask);
                else if (CheckCollisionPointRec(mouse, btnMoveDown)) MoveTaskDown(selectedTask);
                else if (CheckCollisionPointRec(mouse, btnClear)) ClearAllTasks();
            }

        } else if (tdScreen == TD_ADD_TASK) {
            DrawText("Type task name and press ENTER:", 60, 60, 22, DARKPURPLE);
            DrawRectangle(60, 100, 500, 35, Color{ 255, 255, 255, 255 });
            DrawText(inputBuffer, 70, 107, 22, BLACK);

            int key = GetCharPressed();
            while (key > 0 && letterCount < 49) {
                inputBuffer[letterCount++] = (char)key;
                key = GetCharPressed();
            }
            inputBuffer[letterCount] = '\0';

            if (IsKeyPressed(KEY_BACKSPACE) && letterCount > 0) inputBuffer[--letterCount] = '\0';
            if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
                AddTask(inputBuffer);
                letterCount = 0;
                inputBuffer[0] = '\0';
                tdScreen = TD_MENU;
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                letterCount = 0;
                inputBuffer[0] = '\0';
                tdScreen = TD_MENU;
            }
        }

        EndDrawing();
    }

    ClearAllTasks();
    CloseWindow();
    return 0;
}