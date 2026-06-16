#include "raylib.h"
#include <string>
#include <vector>
#include <sstream>

using namespace std;

const int screenWidth = 800;
const int screenHeight = 800;

enum FibScreen {
    FIB_MENU,
    FIB_INPUT,
    FIB_CALCULATE,
    FIB_RESULT
};

struct CallInfo {
    string text;
};

vector<CallInfo> callStack;

int factorial(int n) {
    callStack.push_back({"factorial(" + to_string(n) + ")"});
    if (n <= 1) {
        callStack.push_back({"Return 1"});
        return 1;
    }
    int result = n * factorial(n - 1);
    callStack.push_back({"Return " + to_string(result)});
    return result;
}

int fibonacci(int n) {
    callStack.push_back({"fibonacci(" + to_string(n) + ")"});
    if (n <= 0) {
        callStack.push_back({"Return 0"});
        return 0;
    }
    if (n == 1) {
        callStack.push_back({"Return 1"});
        return 1;
    }
    int result = fibonacci(n - 1) + fibonacci(n - 2);
    callStack.push_back({"Return " + to_string(result)});
    return result;
}

int main() {
    InitWindow(screenWidth, screenHeight, "Recursive Factorial and Fibonacci Visualizer");
    SetTargetFPS(60);

    FibScreen fibScreen = FIB_MENU;
    bool isFactorial = true;
    string inputStr = "";
    int inputNum = 0;
    int result = 0;
    int scrollOffset = 0;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (fibScreen == FIB_MENU) {
            DrawText("Select Function to Visualize", screenWidth / 2 - MeasureText("Select Function to Visualize", 30) / 2, 150, 30, DARKBLUE);
            DrawText("1. Factorial", screenWidth / 2 - 70, 300, 30, BLACK);
            DrawText("2. Fibonacci", screenWidth / 2 - 70, 350, 30, BLACK);
            DrawText("Press 1 or 2 to select", screenWidth / 2 - MeasureText("Press 1 or 2 to select", 20) / 2, 450, 20, DARKGRAY);

            if (IsKeyPressed(KEY_ONE)) {
                isFactorial = true;
                inputStr = "";
                fibScreen = FIB_INPUT;
            }
            if (IsKeyPressed(KEY_TWO)) {
                isFactorial = false;
                inputStr = "";
                fibScreen = FIB_INPUT;
            }
        }
        else if (fibScreen == FIB_INPUT) {
            DrawText("Enter a number (0-15):", screenWidth / 2 - MeasureText("Enter a number (0-15):", 30) / 2, 250, 30, BLACK);
            DrawText(inputStr.c_str(), screenWidth / 2 - MeasureText(inputStr.c_str(), 50) / 2, 320, 50, DARKBLUE);
            DrawText("Press ENTER to calculate, BACKSPACE to delete, M to go back", 100, 700, 20, DARKGRAY);

            int key = GetCharPressed();
            while (key > 0) {
                if (key >= '0' && key <= '9' && inputStr.size() < 2) {
                    inputStr += (char)key;
                }
                key = GetCharPressed();
            }

            if (IsKeyPressed(KEY_BACKSPACE) && !inputStr.empty()) {
                inputStr.pop_back();
            }
            if (IsKeyPressed(KEY_M)) {
                fibScreen = FIB_MENU;
            }
            if (IsKeyPressed(KEY_ENTER) && !inputStr.empty()) {
                inputNum = stoi(inputStr);
                if (inputNum >= 0 && inputNum <= 15) {
                    callStack.clear();
                    if (isFactorial) {
                        result = factorial(inputNum);
                    } else {
                        result = fibonacci(inputNum);
                    }
                    scrollOffset = 0;
                    fibScreen = FIB_RESULT;
                }
            }
        }
        else if (fibScreen == FIB_RESULT) {
            string title = isFactorial ? "Factorial" : "Fibonacci";
            DrawText((title + " of " + to_string(inputNum) + " = " + to_string(result)).c_str(), 20, 20, 30, DARKBLUE);
            DrawText("Call Stack Visualization (scroll with UP/DOWN keys)", 20, 60, 20, DARKGRAY);
            DrawText("Press M to go back", 20, 780, 20, DARKGRAY);

            int startY = 100 - scrollOffset;
            int lineHeight = 25;

            for (size_t i = 0; i < callStack.size(); i++) {
                DrawText(callStack[i].text.c_str(), 20, startY + i * lineHeight, 20, BLACK);
            }

            if (IsKeyDown(KEY_DOWN)) {
                scrollOffset += 5;
                if (scrollOffset > (int)(callStack.size() * lineHeight) - (screenHeight - 120)) {
                    scrollOffset = (int)(callStack.size() * lineHeight) - (screenHeight - 120);
                }
                if (scrollOffset < 0) scrollOffset = 0;
            }
            if (IsKeyDown(KEY_UP)) {
                scrollOffset -= 5;
                if (scrollOffset < 0) scrollOffset = 0;
            }

            if (IsKeyPressed(KEY_M)) {
                fibScreen = FIB_MENU;
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
