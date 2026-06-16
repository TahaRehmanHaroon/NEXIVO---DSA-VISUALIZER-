#include <iostream>
#include <raylib.h>

using namespace std;

class Node
{
public:
    int data;
    Node *next;

    Node(int val)
    {
        data = val;
        next = NULL;
    }
};

class List
{
    Node *head;
    Node *tail;

public:
    List()
    {
        head = tail = NULL;
    }

    string toString() {
    Node* temp = head;
    string result = "List: ";
    while (temp != nullptr) {
        result += to_string(temp->data) + " ";
        temp = temp->next;
    }
    return result.empty() ? "List is empty!" : result;
}

    void push_back(int val)
    {
        Node *newNode = new Node(val);

        if (head == NULL)
        {
            head = tail = newNode;
        }
        else
        {
            tail->next = newNode;
            tail = newNode;
        }
    }

    void pop_front()
    {

        if (head == NULL)
        {
            cout << "List is empty!" << endl;
            return;
        }

        Node *temp = head;
        head = head->next;

        temp->next = NULL;
        delete temp;
    }

    int search(int key)
    {
        Node *temp = head;
        int index = 0;

        while (temp != NULL)
        {
            if (temp->data == key)
            {
                return index;
            }

            temp = temp->next;
            index++;
        }

        return -1;
    }

    void print()
    {
        Node *temp = head;

        while (temp != NULL)
        {
            cout << temp->data << " ";
            temp = temp->next;
        }
        cout << endl;
    }
};

struct Button
{
    Rectangle rect;
    const char *label;
};

enum LinkedListScreen {
    LL_MAIN_MENU,
    LL_MESSAGE,
    LL_INPUT,
    LL_SEARCH_INPUT
};

LinkedListScreen llScreen = LL_MAIN_MENU;
string messageText = "";

string inputText = "";
// bool inputActive = false;

int main()
{

    List l1;

    InitWindow(800, 800, "Linked List Simulator");
    SetTargetFPS(60);

    Button buttons[] = {
        {{200, 100, 400, 80}, "Push Back"},
        {{200, 200, 400, 80}, "Pop Front"},
        {{200, 300, 400, 80}, "Print"},
        {{200, 400, 400, 80}, "Search"}};

    while (WindowShouldClose() == false)
    {

        BeginDrawing();

        if (llScreen == LL_MAIN_MENU)
        {
            // Your existing button drawing and click detection here
            Vector2 mousePoint = GetMousePosition();
            for (int i = 0; i < 4; i++)
            {
                ClearBackground(BLACK);
                DrawRectangleRec(buttons[i].rect, LIGHTGRAY);
                DrawText(buttons[i].label, buttons[i].rect.x + 20, buttons[i].rect.y + 25, 30, BLACK);

                if (CheckCollisionPointRec(mousePoint, buttons[i].rect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                {
                    switch (i)
                    {
                    case 0:
                        inputText = "";
                        // inputActive = true;
                        llScreen = LL_INPUT;

                        break;
                    case 1:
                        l1.pop_front();
                        messageText = "First Node Removed!";
                        llScreen = LL_MESSAGE;
                        break;
                    case 2: 
                    messageText = l1.toString();
                    llScreen = LL_MESSAGE;
                        break;
                    case 3: 
                    inputText = "";
                    llScreen = LL_SEARCH_INPUT;
                        break;
                    }
                }
            }
        }
        else if (llScreen == LL_MESSAGE)
        {
            ClearBackground(RAYWHITE);
            DrawText(messageText.c_str(), 200, 350, 30, DARKGREEN);
            DrawText("Press ENTER to return to main menu!", 250, 400, 20, GRAY);

            if (IsKeyPressed(KEY_ENTER))
            {
                llScreen = LL_MAIN_MENU;
            }
        }

        else if (llScreen == LL_INPUT)
        {
            ClearBackground(RAYWHITE);
            DrawText("Enter number to push back:", 150, 300, 30, DARKGRAY);
            DrawText(inputText.c_str(), 150, 350, 40, BLACK);
            DrawText("Press Enter to confirm", 150, 420, 20, GRAY);
            DrawText("Press Backspace to delete", 150, 450, 20, GRAY);

            // Handle typing input
            int key = GetKeyPressed();

            while (key > 0)
            {
                if ((key >= '0' && key <= '9') && inputText.size() < 10)
                { // max 10 digits
                    inputText.push_back((char)key);
                }
                key = GetKeyPressed();
            }

            // Handle backspace
            if (IsKeyPressed(KEY_BACKSPACE) && !inputText.empty())
            {
                inputText.pop_back(); // these pop and push backs are string functions
            }

            // Confirm input
            if (IsKeyPressed(KEY_ENTER) && !inputText.empty())
            {
                int val = stoi(inputText);
                l1.push_back(val);

                messageText = "Added " + inputText + " to list!";
                llScreen = LL_MESSAGE;
                // inputActive = false;
            }
        }
        else if (llScreen == LL_SEARCH_INPUT) {
    ClearBackground(RAYWHITE);
    DrawText("Enter number to search:", 150, 300, 30, DARKGRAY);
    DrawText(inputText.c_str(), 150, 350, 40, BLACK);
    DrawText("Press Enter to search", 150, 420, 20, GRAY);
    DrawText("Press Backspace to delete", 150, 450, 20, GRAY);

    int key = GetKeyPressed();
    while (key > 0) {
        if ((key >= '0' && key <= '9') && inputText.size() < 10) {
            inputText.push_back((char)key);
        }
        key = GetKeyPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !inputText.empty()) {
        inputText.pop_back();
    }

    if (IsKeyPressed(KEY_ENTER) && !inputText.empty()) {
        int val = stoi(inputText);
        int idx = l1.search(val);
        if (idx == -1) {
            messageText = "Value not found!";
        } else {
            messageText = "Value found at index " + to_string(idx);
        }
        llScreen = LL_MESSAGE;
        inputText = "";
    }
}

        EndDrawing();
    }

    CloseWindow();

    return 0;
}