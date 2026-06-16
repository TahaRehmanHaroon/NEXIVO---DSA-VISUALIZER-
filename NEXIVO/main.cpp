#include <iostream>
#include <raylib.h>
#include <cstring> 
#include <cstdlib> 
#include <cstdio>  
#include <string>
#include <vector>
#include <map>
#include <algorithm> 
#include <ctime>     
#include <cmath>     
#include <sstream>  

using namespace std;

const int screenWidth = 800;
const int screenHeight = 800;

enum NexivoScreen
{
    HOME,
    TD_MODULE,
    STUDY_MODULE,
    SNAKE_MODULE,
    DSA_MODULE, 
    LL_MODULE,  
    FIB_MODULE  
};
NexivoScreen mcurrentScreen = HOME;

// -------------------- TO DO LIST DECLARATIONS-------------------
enum TDScreen
{
    TD_MENU,
    TD_ADD_TASK
};
TDScreen currentTdScreen = TD_MENU; 

struct Task
{
    char name[50];
    Task *prev;
    Task *next;
};

Task *tdHead = nullptr;

void AddTask(const char *taskName)
{
    Task *newTask = new Task();
    strncpy(newTask->name, taskName, 49); 
    newTask->name[49] = '\0';             
    newTask->next = nullptr;
    newTask->prev = nullptr;

    if (!tdHead)
        tdHead = newTask;
    else
    {
        Task *temp = tdHead;
        while (temp->next)
            temp = temp->next;
        temp->next = newTask;
        newTask->prev = temp;
    }
}

void DeleteTask(int index)
{
    if (!tdHead)
        return;
    Task *temp = tdHead;
    int count = 0;
    while (temp && count < index)
    {
        temp = temp->next;
        count++;
    }
    if (!temp)
        return; 
    if (temp->prev)
        temp->prev->next = temp->next;
    else
        tdHead = temp->next; 
    if (temp->next)
        temp->next->prev = temp->prev;
    delete temp;
}

void MoveTaskUp(int index)
{
    if (index <= 0)
        return; 
    Task *curr = tdHead;
    for (int i = 0; i < index && curr; ++i)
        curr = curr->next;

    if (!curr || !curr->prev)
        return; 

    Task *prevNode = curr->prev;

    if (curr->next)
        curr->next->prev = prevNode;
    prevNode->next = curr->next;

    if (prevNode->prev)
        prevNode->prev->next = curr;
    else
        tdHead = curr; 

    curr->prev = prevNode->prev;
    curr->next = prevNode;
    prevNode->prev = curr;
}

void MoveTaskDown(int index)
{

    Task *curr = tdHead;
    int count = 0;
    while (curr != nullptr && count < index)
    {
        curr = curr->next;
        count++;
    }

    if (curr == nullptr || curr->next == nullptr)
        return; 

    MoveTaskUp(index + 1);
}

void ClearAllTasks()
{
    while (tdHead)
    {
        Task *next = tdHead->next;
        delete tdHead;
        tdHead = next;
    }
}

void DrawTasks(int startX, int startY, int selectedIndex)
{ 
    Task *temp = tdHead;
    int y = startY;
    int index = 0;
    while (temp)
    {
        Color textColor = (index == selectedIndex) ? RED : BLACK;
        DrawText(TextFormat("%d. %s", index + 1, temp->name), startX, y, 22, textColor);
        y += 35;
        temp = temp->next;
        index++;
    }
    if (index == 0)
    { 
        DrawText("No tasks yet. Add some!", startX, y, 20, GRAY);
    }
}

int CountTasks()
{ 
    int count = 0;
    Task *temp = tdHead;
    while (temp)
    {
        count++;
        temp = temp->next;
    }
    return count;
}
// ---------------------------------------------------------------

// -------------------- STUDY PROGRAM DECLARATIONS-------------------
const float QUIZ_TIME_SECONDS_STUDY = 60.0f;
const Color DARK_BLUE_BACKGROUND = {28, 28, 40, 255};
const Color LIGHT_LAVENDER_TEXT = {220, 220, 255, 255};
const Color GOLD_ACCENT = {255, 223, 100, 255};
const Color MEDIUM_DARK_BUTTON = {70, 70, 90, 255};
const Color LIGHT_DARK_BUTTON_HOVER = {100, 100, 120, 255};
const Color VERY_DARK_TEXT = {10, 10, 10, 255};

enum StudyScreen
{
    STUDY_MAIN_MENU,
    STUDY_QUIZ_TYPE_SELECTION,
    STUDY_QUIZ_IN_PROGRESS,
    STUDY_QUIZ_SUMMARY
};
StudyScreen currentStudyScreen = STUDY_MAIN_MENU; 

enum StudyCategory
{
    STUDY_ARITHMETIC,
    STUDY_ALGEBRA,
    STUDY_FORMULAS,
    STUDY_NONE_SELECTED
};

enum StudyOperation
{
    STUDY_ADD,
    STUDY_SUBTRACT,
    STUDY_MULTIPLY,
    STUDY_DIVIDE
};

struct ArithmeticQuestion
{
    long long firstNumber;
    long long secondNumber;
    StudyOperation operation;
    double correctAnswer;
    string questionText;
};

struct AlgebraQuestion
{
    int coeffA;
    int constB;
    int resultC;
    double correctXValue;
    string questionText;
};

struct FormulaQuestion
{
    string questionPrompt;
    string fullFormulaText; 
    string answerPattern;   
};

struct QuizSession
{
    float timeLeft;
    int score;
    int questionsAttempted;
    bool showFeedback;
    bool wasLastAnswerCorrect;
    float feedbackTimer;
    string currentQuestionText;
    string userInput;
    int inputLength; 
    Rectangle inputBoxRect;
    StudyCategory currentCategory;

    ArithmeticQuestion currentArithmeticQ;
    AlgebraQuestion currentAlgebraQ;
    FormulaQuestion currentFormulaQ;

    vector<string> suggestions;

    QuizSession() : timeLeft(QUIZ_TIME_SECONDS_STUDY), score(0), questionsAttempted(0),
                    showFeedback(false), wasLastAnswerCorrect(false), feedbackTimer(0.0f),
                    inputLength(0), currentCategory(STUDY_NONE_SELECTED) {}
};
QuizSession activeQuizSession;                                
StudyCategory chosenQuizCategory = STUDY_NONE_SELECTED;       
StudyCategory lastCategoryPlayed_Study = STUDY_NONE_SELECTED; 

namespace FormulaSuggester
{
    const int ALPHABET_SIZE = 128; 

    struct TrieNode
    {
        TrieNode *children[ALPHABET_SIZE];
        bool isEndOfFormulaPattern;
        string formulaPattern; 

        TrieNode() : isEndOfFormulaPattern(false), formulaPattern("")
        {
            for (int i = 0; i < ALPHABET_SIZE; ++i)
                children[i] = nullptr;
        }
        ~TrieNode()
        {
            for (int i = 0; i < ALPHABET_SIZE; ++i)
                delete children[i]; 
        }
    };

    TrieNode *rootNode = nullptr;

    int getCharIndex(char c)
    { 
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc < ALPHABET_SIZE)
            return uc;
        return -1; 
    }

    void addFormulaPattern(const std::string &pattern)
    { 
        if (!rootNode)
            rootNode = new TrieNode();
        TrieNode *currentNode = rootNode;

        for (char ch : pattern)
        {
            int index = getCharIndex(std::toupper(ch)); 
            if (index == -1)
                continue; 

            if (!currentNode->children[index])
            {
                currentNode->children[index] = new TrieNode();
            }
            currentNode = currentNode->children[index];
        }
        currentNode->isEndOfFormulaPattern = true;
        currentNode->formulaPattern = pattern; 
    }

    void collectPatternsFromNode(TrieNode *startingNode, std::vector<std::string> &foundPatterns)
    {
        if (!startingNode)
            return;
        if (startingNode->isEndOfFormulaPattern && !startingNode->formulaPattern.empty())
        {
            foundPatterns.push_back(startingNode->formulaPattern);
        }
        for (int i = 0; i < ALPHABET_SIZE; ++i)
        {
            if (startingNode->children[i])
            {
                collectPatternsFromNode(startingNode->children[i], foundPatterns);
            }
        }
    }

    vector<string> findMatchingPatterns(const string &prefix)
    { 
        vector<string> foundSuggestions;
        if (!rootNode || prefix.empty())
            return foundSuggestions;

        TrieNode *currentNode = rootNode;
        for (char ch : prefix)
        {
            int index = getCharIndex(std::toupper(ch)); 
            if (index == -1 || !currentNode->children[index])
            {
                return foundSuggestions;
            }
            currentNode = currentNode->children[index];
        }
        collectPatternsFromNode(currentNode, foundSuggestions);
        sort(foundSuggestions.begin(), foundSuggestions.end());
        return foundSuggestions;
    }

    void loadFormulaPatternsFromList(const vector<FormulaQuestion> &questions)
    {
        if (rootNode)
        {
            delete rootNode;
            rootNode = nullptr;
        } 
        for (const auto &q : questions)
        {
            addFormulaPattern(q.answerPattern);
        }
    }

    void clearSuggesterData()
    {
        delete rootNode;
        rootNode = nullptr;
    }
}

namespace PerformanceTracker
{
    map<string, pair<int, int>> categoryPerformance; 

    void recordAttempt(const string &categoryName, bool wasCorrect)
    {
        if (categoryPerformance.find(categoryName) == categoryPerformance.end())
        {
            categoryPerformance[categoryName] = {0, 0};
        }
        categoryPerformance[categoryName].second++; 
        if (wasCorrect)
        {
            categoryPerformance[categoryName].first++; 
        }
    }

    void clearPerformanceData()
    {
        categoryPerformance.clear();
    }
}

namespace QuestionGenerator
{
    vector<FormulaQuestion> formulaList;

    void loadFormulasToList()
    { 
        formulaList.clear();
        formulaList.push_back({"Area of a Circle = ?", "Area of a Circle = PI*radius*radius", "PI*r^2"});
        formulaList.push_back({"Perimeter of a Square = ?", "Perimeter of a Square = 4*side", "4*s"});
        formulaList.push_back({"Volume of a Cube = ?", "Volume of a Cube = side*side*side", "s^3"});
        formulaList.push_back({"Pythagorean Theorem (c^2) = ?", "Hypotenuse of a Right Triangle (c^2) = a^2+b^2", "a^2+b^2"});
        formulaList.push_back({"Area of a Rectangle = ?", "Area of a Rectangle = length*width", "l*w"});
        formulaList.push_back({"Circumference of a Circle = ?", "Circumference of a Circle = 2*PI*radius", "2*PI*r"});
        formulaList.push_back({"Area of Triangle = ?", "Area of Triangle = 0.5*base*height", "0.5*b*h"});
        formulaList.push_back({"Simple Interest = ?", "Simple Interest = Principal*Rate*Time", "P*R*T"});
        formulaList.push_back({"Speed = ?", "Speed = Distance/Time", "D/T"});
        FormulaSuggester::loadFormulaPatternsFromList(formulaList);
    }

    ArithmeticQuestion createArithmeticQuestion()
    { 
        ArithmeticQuestion q;
        q.firstNumber = GetRandomValue(1, 25);
        q.secondNumber = GetRandomValue(1, 25);
        int operationType = GetRandomValue(0, 3);

        switch (operationType)
        {
        case 0: 
            q.operation = STUDY_ADD;
            q.correctAnswer = static_cast<double>(q.firstNumber + q.secondNumber);
            q.questionText = to_string(q.firstNumber) + " + " + to_string(q.secondNumber) + " = ?";
            break;
        case 1: 
            q.operation = STUDY_SUBTRACT;
            if (q.firstNumber < q.secondNumber)
                std::swap(q.firstNumber, q.secondNumber);
            q.correctAnswer = static_cast<double>(q.firstNumber - q.secondNumber);
            q.questionText = to_string(q.firstNumber) + " - " + to_string(q.secondNumber) + " = ?";
            break;
        case 2: 
            q.operation = STUDY_MULTIPLY;
            q.firstNumber = GetRandomValue(1, 12);
            q.secondNumber = GetRandomValue(1, 12);
            q.correctAnswer = static_cast<double>(q.firstNumber * q.secondNumber);
            q.questionText = to_string(q.firstNumber) + " * " + to_string(q.secondNumber) + " = ?";
            break;
        case 3: 
            q.operation = STUDY_DIVIDE;
            q.secondNumber = GetRandomValue(1, 10);
            q.firstNumber = q.secondNumber * GetRandomValue(1, 10);
            q.correctAnswer = static_cast<double>(q.firstNumber) / q.secondNumber;
            q.questionText = to_string(q.firstNumber) + " / " + to_string(q.secondNumber) + " = ?";
            break;
        }
        return q;
    }

    AlgebraQuestion createAlgebraQuestion()
    { 
        AlgebraQuestion q;
        int desiredX = GetRandomValue(-9, 9);
        while (desiredX == 0)
            desiredX = GetRandomValue(-9, 9);

        int puzzleType = GetRandomValue(0, 2);

        if (puzzleType == 0)
        {
            q.coeffA = GetRandomValue(1, 5) * (GetRandomValue(0, 1) ? 1 : -1);
            q.constB = GetRandomValue(-10, 10);
        }
        else if (puzzleType == 1)
        {
            q.coeffA = GetRandomValue(1, 5) * (GetRandomValue(0, 1) ? 1 : -1);
            q.constB = 0;
        }
        else
        {
            q.coeffA = 1;
            q.constB = GetRandomValue(-10, 10);
        }
        if (q.coeffA == 0 && puzzleType != 2)
            q.coeffA = 1;

        q.resultC = q.coeffA * desiredX + q.constB;
        q.correctXValue = static_cast<double>(desiredX);

        std::string statement;
        if (q.coeffA != 0)
        {
            if (q.coeffA == 1)
                statement += "X";
            else if (q.coeffA == -1)
                statement += "-X";
            else
                statement += std::to_string(q.coeffA) + "X";
        }

        if (q.constB != 0)
        {
            if (!statement.empty())
            {
                if (q.constB > 0)
                    statement += " + ";
                else
                    statement += " - ";
            }
            else if (q.constB < 0)
            {
                statement += "-";
            }
            statement += std::to_string(std::abs(q.constB));
        }
        else if (statement.empty())
        {
            q.coeffA = 1;
            q.constB = 0;
            q.resultC = desiredX;
            statement = "X";
        }

        statement += " = " + std::to_string(q.resultC);
        statement += "  (Solve for X)";
        q.questionText = statement;
        return q;
    }

    FormulaQuestion getRandomFormulaQuestion()
    {
        if (formulaList.empty())
        {
            return {"List Empty: No questions found!", "", ""};
        }
        return formulaList[GetRandomValue(0, formulaList.size() - 1)];
    }

    std::string normalizeAnswer(const std::string &str)
    { // From your new code
        std::string result = str;
        result.erase(std::remove_if(result.begin(), result.end(), ::isspace), result.end());
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }

    bool checkArithmeticAnswer(const ArithmeticQuestion &q, const std::string &userAnswerStr)
    {
        try
        {
            double userAnswer = std::stod(userAnswerStr);
            return std::abs(userAnswer - q.correctAnswer) < 0.001;
        }
        catch (const std::exception &)
        {
            return false;
        }
    }

    bool checkAlgebraAnswer(const AlgebraQuestion &q, const std::string &userAnswerStr)
    {
        try
        {
            double userAnswer = std::stod(userAnswerStr);
            return std::abs(userAnswer - q.correctXValue) < 0.001;
        }
        catch (const std::exception &)
        {
            return false;
        }
    }

    bool checkFormulaAnswer(const FormulaQuestion &q, const std::string &userAnswerStr)
    {
        return normalizeAnswer(userAnswerStr) == normalizeAnswer(q.answerPattern);
    }

    std::string getCategoryNameForStats(StudyCategory category, const ArithmeticQuestion *arithQ = nullptr)
    {
        switch (category)
        {
        case STUDY_ARITHMETIC:
            if (arithQ)
            {
                switch (arithQ->operation)
                {
                case STUDY_ADD:
                    return "Arithmetic-Add";
                case STUDY_SUBTRACT:
                    return "Arithmetic-Subtract";
                case STUDY_MULTIPLY:
                    return "Arithmetic-Multiply";
                case STUDY_DIVIDE:
                    return "Arithmetic-Divide";
                }
            }
            return "Arithmetic-General";
        case STUDY_ALGEBRA:
            return "Algebra-Equations";
        case STUDY_FORMULAS:
            return "Formulas-Recall";
        default:
            return "Other";
        }
    }

    void generateNewQuestion(QuizSession &session)
    {
        session.userInput = "";
        session.inputLength = 0;
        session.suggestions.clear();

        switch (session.currentCategory)
        {
        case STUDY_ARITHMETIC:
            session.currentArithmeticQ = createArithmeticQuestion();
            session.currentQuestionText = session.currentArithmeticQ.questionText;
            break;
        case STUDY_ALGEBRA:
            session.currentAlgebraQ = createAlgebraQuestion();
            session.currentQuestionText = session.currentAlgebraQ.questionText;
            break;
        case STUDY_FORMULAS:
            session.currentFormulaQ = getRandomFormulaQuestion();
            session.currentQuestionText = session.currentFormulaQ.questionPrompt;
            break;
        case STUDY_NONE_SELECTED:
            session.currentQuestionText = "Error: No quiz type chosen.";
            break;
        }
    }
}

namespace UIRenderer
{
    const int MAX_INPUT_CHARS_SHORT = 20;
    const int MAX_INPUT_CHARS_LONG = 60;

    bool DrawQuizButton(Rectangle buttonRect, const char *buttonText, Color baseColor, Color hoverColor, Color textColor)
    {
        bool isClicked = false;
        Vector2 mousePos = GetMousePosition();
        Color currentButtonColor = baseColor;

        if (CheckCollisionPointRec(mousePos, buttonRect))
        {
            currentButtonColor = hoverColor;
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                isClicked = true;
        }
        DrawRectangleRec(buttonRect, currentButtonColor);
        DrawRectangleLinesEx(buttonRect, 2, Fade(textColor, 0.5f));

        int fontSize = 20;
        int textWidth = MeasureText(buttonText, fontSize);
        while (textWidth > buttonRect.width - 20 && fontSize > 10)
        {
            fontSize -= 2;
            textWidth = MeasureText(buttonText, fontSize);
        }
        DrawText(buttonText, buttonRect.x + (buttonRect.width - textWidth) / 2, buttonRect.y + (buttonRect.height - fontSize) / 2, fontSize, textColor);
        return isClicked;
    }

    Rectangle CalculateButtonRect(const char *text, float centerX, float y, float height, int fontSize, float minWidth = 150.0f, float paddingX = 30.0f)
    {
        int textWidth = MeasureText(text, fontSize);
        float buttonWidth = (textWidth + paddingX > minWidth) ? (textWidth + paddingX) : minWidth;
        return {centerX - buttonWidth / 2, y, buttonWidth, height};
    }

    void HandleTextInput(std::string &inputText, int &currentLength, int maxChars)
    { 
        int keyCode = GetCharPressed();
        while (keyCode > 0)
        {
            if ((keyCode >= 32) && (keyCode <= 125) && (currentLength < maxChars))
            {
                inputText += (char)keyCode;
                currentLength++;
            }
            keyCode = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE))
        {
            if (currentLength > 0)
            {
                inputText.pop_back();
                currentLength--;
            }
        }
    }

    void DrawInputBox(Rectangle boxRect, const std::string &currentText, bool isActive)
    {
        DrawRectangleRec(boxRect, Fade(LIGHTGRAY, 0.8f));
        int fontSize = 20;
        if (isActive)
        {
            DrawRectangleLinesEx(boxRect, 2, GOLD_ACCENT);
            if (((int)(GetTime() * 2.5f) % 2) == 0)
            { 
                DrawText("_", boxRect.x + 8 + MeasureText(currentText.c_str(), fontSize), boxRect.y + (boxRect.height - fontSize) / 2 + 2, fontSize, VERY_DARK_TEXT);
            }
        }
        else
            DrawRectangleLinesEx(boxRect, 1, DARKGRAY);
        DrawText(currentText.c_str(), boxRect.x + 10, boxRect.y + (boxRect.height - fontSize) / 2, fontSize, VERY_DARK_TEXT);
    }

    void ShowAnswerFeedback(const string &message, Color messageColor)
    {
        int fontSize = 30;
        int feedbackWidth = MeasureText(message.c_str(), fontSize);
        DrawText(message.c_str(), (screenWidth - feedbackWidth) / 2, screenHeight / 2 + 80, fontSize, messageColor);
    }

    void DrawStudyMainMenuScreen(StudyScreen &currentLocalScreen, StudyCategory &selectedCategory)
    { 
        const char *title = "Math Challenge";
        const char *subtitle = "Choose a topic to practice your math skills!";
        DrawText(title, screenWidth / 2 - MeasureText(title, 36) / 2, 60, 36, GOLD_ACCENT);
        DrawText(subtitle, screenWidth / 2 - MeasureText(subtitle, 20) / 2, 130, 20, LIGHT_LAVENDER_TEXT);

        DrawText("Press 'M' to return to NEXIVO Main Menu", screenWidth / 2 - MeasureText("Press 'M' to return to NEXIVO Main Menu", 20) / 2, screenHeight - 50, 20, LIGHT_LAVENDER_TEXT);
        if (IsKeyPressed(KEY_M))
        {
            mcurrentScreen = HOME;
            currentLocalScreen = STUDY_MAIN_MENU;
            return;
        }

        float btnW = 300, btnH = 55, btnX = (float)screenWidth / 2 - btnW / 2;
        if (DrawQuizButton({btnX, 200, btnW, btnH}, "Arithmetic Practice", MEDIUM_DARK_BUTTON, LIGHT_DARK_BUTTON_HOVER, LIGHT_LAVENDER_TEXT))
        {
            selectedCategory = STUDY_ARITHMETIC;
            currentLocalScreen = STUDY_QUIZ_TYPE_SELECTION;
        }
        if (DrawQuizButton({btnX, 275, btnW, btnH}, "Algebra Practice", MEDIUM_DARK_BUTTON, LIGHT_DARK_BUTTON_HOVER, LIGHT_LAVENDER_TEXT))
        {
            selectedCategory = STUDY_ALGEBRA;
            currentLocalScreen = STUDY_QUIZ_TYPE_SELECTION;
        }
        if (DrawQuizButton({btnX, 350, btnW, btnH}, "Formula Practice", MEDIUM_DARK_BUTTON, LIGHT_DARK_BUTTON_HOVER, LIGHT_LAVENDER_TEXT))
        {
            selectedCategory = STUDY_FORMULAS;
            currentLocalScreen = STUDY_QUIZ_TYPE_SELECTION;
        }
        DrawText("v1.0 - SimpleFun Edition", 10, screenHeight - 20, 10, Fade(LIGHT_LAVENDER_TEXT, 0.5f));
    }

    void DrawQuizModeScreen(StudyScreen &currentLocalScreen, QuizSession &quizSession, StudyCategory chosenCat, StudyCategory &categoryForRetry)
    {
        if (IsKeyPressed(KEY_M))
        {
            mcurrentScreen = HOME;
            currentLocalScreen = STUDY_MAIN_MENU;
            return;
        }

        std::string modeName;
        switch (chosenCat)
        {
        case STUDY_ARITHMETIC:
            modeName = "Arithmetic";
            break;
        case STUDY_ALGEBRA:
            modeName = "Algebra";
            break;
        case STUDY_FORMULAS:
            modeName = "Formulas";
            break;
        default:
            modeName = "Unknown Mode";
            break;
        }
        const char *titleText = TextFormat("Selected Mode: %s", modeName.c_str());
        DrawText(titleText, screenWidth / 2 - MeasureText(titleText, 28) / 2, 180, 28, GOLD_ACCENT);
        DrawText("A 60-second timed quiz. Get ready!", screenWidth / 2 - MeasureText("A 60-second timed quiz. Get ready!", 20) / 2, 240, 20, LIGHT_LAVENDER_TEXT);

        if (DrawQuizButton(CalculateButtonRect("START QUIZ", (float)screenWidth / 2, 300, 50, 20, 220), "START QUIZ", LIME, DARKGREEN, VERY_DARK_TEXT))
        {
            quizSession = QuizSession();
            quizSession.currentCategory = chosenCat;
            QuestionGenerator::generateNewQuestion(quizSession);
            PerformanceTracker::clearPerformanceData();
            currentLocalScreen = STUDY_QUIZ_IN_PROGRESS;
            categoryForRetry = chosenCat;
        }
        if (DrawQuizButton(CalculateButtonRect("Back to Category Menu", (float)screenWidth / 2, 370, 50, 20, 220), "Back to Category Menu", DARKGRAY, GRAY, LIGHT_LAVENDER_TEXT))
        {
            currentLocalScreen = STUDY_MAIN_MENU;
        }
        DrawText("Press 'M' for NEXIVO Menu", screenWidth / 2 - MeasureText("Press 'M' for NEXIVO Menu", 18) / 2, screenHeight - 30, 18, LIGHT_LAVENDER_TEXT);
    }

    void DrawQuizGameplayScreen(StudyScreen &currentLocalScreen, QuizSession &session)
    {
        if (IsKeyPressed(KEY_M))
        {
            mcurrentScreen = HOME;
            currentLocalScreen = STUDY_MAIN_MENU;
            return;
        }

        DrawText(TextFormat("Time Left: %02.0fs", session.timeLeft), screenWidth - 200, 30, 22, RED);
        DrawText(TextFormat("Score: %d", session.score), 30, 30, 22, LIME);
        DrawText("Solve this problem:", 50, 100, 22, LIGHT_LAVENDER_TEXT);
        DrawText(session.currentQuestionText.c_str(), 50, 140, 26, GOLD_ACCENT);

        int maxChars = (session.currentCategory == STUDY_FORMULAS) ? MAX_INPUT_CHARS_LONG : MAX_INPUT_CHARS_SHORT;
        float inputW = (session.currentCategory == STUDY_FORMULAS) ? 450.0f : 300.0f;
        session.inputBoxRect = {(float)screenWidth / 2 - inputW / 2, 220, inputW, 45};

        HandleTextInput(session.userInput, session.inputLength, maxChars);
        DrawInputBox(session.inputBoxRect, session.userInput, true);

        if (session.currentCategory == STUDY_FORMULAS && session.inputLength > 0)
        {
            if (session.suggestions.empty() || IsKeyPressed(KEY_BACKSPACE) || GetCharPressed() != 0)
            {
                session.suggestions = FormulaSuggester::findMatchingPatterns(session.userInput);
            }
            int sugY = session.inputBoxRect.y + session.inputBoxRect.height + 10;
            for (size_t i = 0; i < session.suggestions.size() && i < 4; ++i)
            {
                Rectangle sugBox = {session.inputBoxRect.x, (float)sugY + i * 30, session.inputBoxRect.width, 25.0f};
                bool hov = CheckCollisionPointRec(GetMousePosition(), sugBox);
                DrawRectangleRec(sugBox, hov ? LIGHT_DARK_BUTTON_HOVER : MEDIUM_DARK_BUTTON);
                DrawText(session.suggestions[i].c_str(), sugBox.x + 10, sugBox.y + 4, 18, LIGHT_LAVENDER_TEXT);
                if (hov && IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
                {
                    session.userInput = session.suggestions[i];
                    session.inputLength = session.userInput.length();
                    session.suggestions.clear();
                }
            }
        }

        bool submitted = DrawQuizButton(CalculateButtonRect("Submit Answer", (float)screenWidth / 2, 320, 45, 20, 180), "Submit Answer", BLUE, SKYBLUE, WHITE) ||
                         (IsKeyPressed(KEY_ENTER) && session.inputLength > 0);

        if (submitted && session.inputLength > 0)
        {
            bool isCorrect = false;
            std::string catKey;
            switch (session.currentCategory)
            {
            case STUDY_ARITHMETIC:
                isCorrect = QuestionGenerator::checkArithmeticAnswer(session.currentArithmeticQ, session.userInput);
                catKey = QuestionGenerator::getCategoryNameForStats(session.currentCategory, &session.currentArithmeticQ);
                break;
            case STUDY_ALGEBRA:
                isCorrect = QuestionGenerator::checkAlgebraAnswer(session.currentAlgebraQ, session.userInput);
                catKey = QuestionGenerator::getCategoryNameForStats(session.currentCategory);
                break;
            case STUDY_FORMULAS:
                isCorrect = QuestionGenerator::checkFormulaAnswer(session.currentFormulaQ, session.userInput);
                catKey = QuestionGenerator::getCategoryNameForStats(session.currentCategory);
                break;
            default:
                break;
            }
            PerformanceTracker::recordAttempt(catKey, isCorrect);
            session.questionsAttempted++;
            if (isCorrect)
                session.score++;
            session.wasLastAnswerCorrect = isCorrect;
            session.showFeedback = true;
            session.feedbackTimer = 1.2f;
            QuestionGenerator::generateNewQuestion(session);
        }

        if (session.showFeedback)
        {
            ShowAnswerFeedback(session.wasLastAnswerCorrect ? "CORRECT!" : "INCORRECT!", session.wasLastAnswerCorrect ? LIME : ORANGE);
            session.feedbackTimer -= GetFrameTime();
            if (session.feedbackTimer <= 0)
                session.showFeedback = false;
        }

        if (DrawQuizButton(CalculateButtonRect("End Quiz Now", screenWidth - 120, screenHeight - 55, 45, 20, 180), "End Quiz Now", MAROON, RED, WHITE))
        {
            currentLocalScreen = STUDY_QUIZ_SUMMARY;
        }
        DrawText("Press 'M' for NEXIVO Menu", screenWidth / 2 - MeasureText("Press 'M' for NEXIVO Menu", 18) / 2, screenHeight - 30, 18, LIGHT_LAVENDER_TEXT);

        session.timeLeft -= GetFrameTime();
        if (session.timeLeft <= 0)
        {
            session.timeLeft = 0;
            currentLocalScreen = STUDY_QUIZ_SUMMARY;
        }
    }

    void DrawSummaryScreen(StudyScreen &currentLocalScreen, StudyCategory &categoryForRetryOption, const QuizSession &session, const StudyCategory lastPlayedCat)
    {
        if (IsKeyPressed(KEY_M))
        {
            mcurrentScreen = HOME;
            currentLocalScreen = STUDY_MAIN_MENU;
            return;
        }

        DrawText("Quiz Summary", screenWidth / 2 - MeasureText("Quiz Summary", 30) / 2, 50, 30, GOLD_ACCENT);
        DrawText(TextFormat("Total Questions Attempted: %d", session.questionsAttempted), 100, 120, 22, LIGHT_LAVENDER_TEXT);
        DrawText(TextFormat("Correct Answers: %d", session.score), 100, 155, 22, LIME);
        float acc = (session.questionsAttempted > 0) ? ((float)session.score / session.questionsAttempted) * 100.0f : 0.0f;
        DrawText(TextFormat("Accuracy: %.1f%%", acc), 100, 190, 22, (acc >= 60.0f ? SKYBLUE : ORANGE));

        DrawText("Performance by Sub-Category:", 100, 250, 24, GOLD_ACCENT);
        int yPos = 290;
        if (PerformanceTracker::categoryPerformance.empty())
        {
            DrawText("No specific category data recorded.", 120, yPos, 20, Fade(LIGHT_LAVENDER_TEXT, 0.7f));
            yPos += 30;
        }
        for (const auto &rec : PerformanceTracker::categoryPerformance)
        {
            DrawText(TextFormat("  %s: %d / %d correct", rec.first.c_str(), rec.second.first, rec.second.second), 120, yPos, 20, LIGHT_LAVENDER_TEXT);
            yPos += 30;
        }

        float btnY = (float)screenHeight - 120, btnH = 50;
        if (DrawQuizButton(CalculateButtonRect("Retry Same Mode", screenWidth * 0.3f, btnY, btnH, 20, 220), "Retry Same Mode", GREEN, LIME, VERY_DARK_TEXT))
        {
            categoryForRetryOption = lastPlayedCat;
            currentLocalScreen = STUDY_QUIZ_TYPE_SELECTION;
        }
        if (DrawQuizButton(CalculateButtonRect("Back to Study Menu", screenWidth * 0.7f, btnY, btnH, 20, 220), "Back to Study Menu", MEDIUM_DARK_BUTTON, LIGHT_DARK_BUTTON_HOVER, LIGHT_LAVENDER_TEXT))
        {
            currentLocalScreen = STUDY_MAIN_MENU;
        }
        DrawText("Press 'M' for NEXIVO Menu", screenWidth / 2 - MeasureText("Press 'M' for NEXIVO Menu", 18) / 2, screenHeight - 50, 18, LIGHT_LAVENDER_TEXT);
    }
}
// ------------------------------------------------------------------

// -------------------- SNAKE GAME DECLARATIONS----------------------
#define GRID_SIZE 40
#define GRID_WIDTH (screenWidth / GRID_SIZE)
#define GRID_HEIGHT (screenHeight / GRID_SIZE)

typedef enum
{
    SNAKE_MENU,
    SNAKE_GAME,
    SNAKE_CONTROLS,
    SNAKE_GAME_OVER
} SnakeGameScreen;

typedef enum
{
    SNAKE_UP,
    SNAKE_DOWN,
    SNAKE_LEFT,
    SNAKE_RIGHT
} SnakeDirection;

typedef struct SnakeNode
{
    int x;
    int y;
    struct SnakeNode *next;
} SnakeNode;

typedef struct
{
    SnakeNode *head;
    SnakeNode *tail;
    SnakeDirection dir;
    SnakeDirection nextDir; 
    int length;
} Snake;

typedef struct
{
    int x;
    int y;
} Food;

// Function declarations for Snake Game
void InitSnakeGame(void);
void DrawSnakeMenu(void);
void UpdateSnakeMenu(void);
void DrawSnakeControls(void);
void UpdateSnakeControls(void);
void UpdateSnakeGameplay(void);
void DrawSnakeGameplay(void);
void InitActualSnake(void);            
void FreeActualSnake(void);             
void MoveActualSnake(void);             
bool CheckSnakeCollision(int x, int y); 
void AddSnakeHead(int x, int y);
void RemoveSnakeTail(void);
void SpawnSnakeFood(void);
void DrawSnakeGameOver(void);
void UpdateSnakeGameOver(void);
void DrawTextCentered(const char *text, int y, int fontSize, Color color); 

SnakeGameScreen currentSnakeScreen = SNAKE_MENU;
Snake snake = {0}; 
Food food = {0};
int snakeScore = 0;
int snakeHighScore = 0;
int snakeFramesCounter = 0;
int snakeGameSpeed = 10; 

const char *snakeMenuItems[] = {"Start Game", "Controls", "Exit to NEXIVO Menu"};
int snakeMenuItemCount = 3;
int snakeMenuSelected = 0;
// ------------------------------------------------------------------

// -------------------- LINKED LIST DECLARATIONS---------------------
class Node_LL
{ 
public:
    int data;
    Node_LL *next;
    Node_LL(int val) : data(val), next(nullptr) {}
};

class List_LL
{ 
    Node_LL *head;
    Node_LL *tail;

public:
    List_LL() : head(nullptr), tail(nullptr) {}
    ~List_LL() { clear(); }

    void clear()
    {
        while (head != nullptr)
            pop_front();
    }

    string toString()
    {
        if (head == nullptr)
            return "List is empty!";
        Node_LL *temp = head;
        string result = "List: ";
        while (temp != nullptr)
        {
            result += to_string(temp->data) + " ";
            temp = temp->next;
        }
        return result;
    }
    void push_back(int val)
    {
        Node_LL *newNode = new Node_LL(val);
        if (head == nullptr)
            head = tail = newNode;
        else
        {
            tail->next = newNode;
            tail = newNode;
        }
    }
    void pop_front()
    {
        if (head == nullptr)
            return;
        Node_LL *temp = head;
        head = head->next;
        if (head == nullptr)
            tail = nullptr;
        delete temp;
    }
    int search(int key)
    {
        Node_LL *temp = head;
        int index = 0;
        while (temp != nullptr)
        {
            if (temp->data == key)
                return index;
            temp = temp->next;
            index++;
        }
        return -1; 
    }
};

struct Button_LL
{
    Rectangle rect;
    const char *label;
};

enum LinkedListScreen
{
    LL_MAIN_MENU,
    LL_MESSAGE,
    LL_INPUT,
    LL_SEARCH_INPUT
};
LinkedListScreen currentLLScreen = LL_MAIN_MENU;
string llMessageText = "";
string llInputText = "";
// ------------------------------------------------------------------

// -------------------- FIB / FACTORIAL (DSA) DECLARATIONS-----------------
enum FibFactScreen
{
    FIBFACT_MENU,
    FIBFACT_INPUT,
    FIBFACT_RESULT
};
FibFactScreen currentFibFactScreen = FIBFACT_MENU;

struct CallInfo
{
    string text; 
};
vector<CallInfo> fibFactCallStack;

int calculateFactorial(int n, vector<CallInfo> &stack);
int calculateFibonacci(int n, vector<CallInfo> &stack);

string fibFactInputStr = "";
bool isFactorialMode = true; 
int fibFactInputNum = 0;
int fibFactResult = 0;
int fibFactScrollOffset = 0;
// ------------------------------------------------------------------

int main()
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "NEXIVO ");
    SetTargetFPS(60);

    InitAudioDevice();

    Music bgMusic = LoadMusicStream("bgmusic.mp3");
    // Music gamebg = LoadMusicStream("GAMEBG.mp3");
    PlayMusicStream(bgMusic);
    UpdateMusicStream(bgMusic);

    QuestionGenerator::loadFormulasToList(); 
    SetRandomSeed(time(0));                  

    //------------------------------------------
    // HOME Screen Variables
    Rectangle btnToDo = {(float)screenWidth / 2 - 250, 220, 500, 90};
    Rectangle btnStudy = {(float)screenWidth / 2 - 250, 320, 500, 90};
    Rectangle btnSnake = {(float)screenWidth / 2 - 250, 420, 500, 90};
    Rectangle btnDSA = {(float)screenWidth / 2 - 250, 520, 500, 90};
    //------------------------------------------

    //------------------------------------------
    // TD_MODULE Variables
    char tdInputBuffer[50] = "";
    int tdLetterCount = 0;
    int tdSelectedTask = 0;
    Rectangle btnTDAdd = {(float)screenWidth - 200, 50, 180, 45};
    Rectangle btnTDDelete = {(float)screenWidth - 200, 105, 180, 45};
    Rectangle btnTDMoveUp = {(float)screenWidth - 200, 160, 180, 45};
    Rectangle btnTDMoveDown = {(float)screenWidth - 200, 215, 180, 45};
    Rectangle btnTDClear = {(float)screenWidth - 200, 270, 180, 45};
    //------------------------------------------

    //------------------------------------------
    // STUDY_MODULE Variables are global
    //------------------------------------------

    //------------------------------------------
    // SNAKE_MODULE Variables are mostly global 
    InitSnakeGame(); 
    //------------------------------------------

    // DSA_MODULE Screen Variables 
    Rectangle btnDSALinkedList = {(float)screenWidth / 2 - 250, 250, 500, 90};
    Rectangle btnDSAFib = {(float)screenWidth / 2 - 250, 350, 500, 90};
    //------------------------------------------

    //------------------------------------------
    // LL_MODULE (DSA Linked List) Variables
    List_LL dsaLinkedList; 
    Button_LL llButtons[] = {
        {{(float)screenWidth / 2 - 200, 100, 400, 80}, "Push Back (Add to End)"},
        {{(float)screenWidth / 2 - 200, 200, 400, 80}, "Pop Front (Remove First)"},
        {{(float)screenWidth / 2 - 200, 300, 400, 80}, "Print List"},
        {{(float)screenWidth / 2 - 200, 400, 400, 80}, "Search for Value"}};
    //------------------------------------------

    //------------------------------------------
    // FIB_MODULE (DSA Fibonacci/Factorial Visualizer) Variables are global for the module
    //------------------------------------------

    while (WindowShouldClose() == false)
    {
        Vector2 mousePos = GetMousePosition();

        switch (mcurrentScreen)
        {
        case HOME:
        {
            UpdateMusicStream(bgMusic);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mousePos, btnToDo))
                    mcurrentScreen = TD_MODULE;
                else if (CheckCollisionPointRec(mousePos, btnStudy))
                {
                    mcurrentScreen = STUDY_MODULE;
                    currentStudyScreen = STUDY_MAIN_MENU;
                }
                else if (CheckCollisionPointRec(mousePos, btnSnake))
                {
                    mcurrentScreen = SNAKE_MODULE;
                    currentSnakeScreen = SNAKE_MENU;
                }
                else if (CheckCollisionPointRec(mousePos, btnDSA))
                    mcurrentScreen = DSA_MODULE;
            }
        }
        break;
        case TD_MODULE:
        {
            UpdateMusicStream(bgMusic);
            if (IsKeyPressed(KEY_TAB))
            {
                mcurrentScreen = HOME;
                currentTdScreen = TD_MENU;
                tdSelectedTask = 0;
            }

            if (currentTdScreen == TD_MENU)
            {
                int taskCount = CountTasks();
                if (IsKeyPressed(KEY_DOWN))
                {
                    if (taskCount > 0)
                        tdSelectedTask = (tdSelectedTask + 1) % taskCount;
                }
                if (IsKeyPressed(KEY_UP))
                {
                    if (taskCount > 0)
                        tdSelectedTask = (tdSelectedTask - 1 + taskCount) % taskCount;
                }

                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                {
                    if (CheckCollisionPointRec(mousePos, btnTDAdd))
                    {
                        currentTdScreen = TD_ADD_TASK;
                        tdInputBuffer[0] = '\0';
                        tdLetterCount = 0;
                    }
                    else if (CheckCollisionPointRec(mousePos, btnTDDelete) && taskCount > 0)
                    {
                        DeleteTask(tdSelectedTask);
                        if (tdSelectedTask >= CountTasks() && CountTasks() > 0)
                            tdSelectedTask = CountTasks() - 1;
                        else if (CountTasks() == 0)
                            tdSelectedTask = 0;
                    }
                    else if (CheckCollisionPointRec(mousePos, btnTDMoveUp) && taskCount > 0)
                    {
                        MoveTaskUp(tdSelectedTask);
                        if (tdSelectedTask > 0)
                            tdSelectedTask--;
                    }
                    else if (CheckCollisionPointRec(mousePos, btnTDMoveDown) && taskCount > 0)
                    {
                        MoveTaskDown(tdSelectedTask);
                        if (tdSelectedTask < CountTasks() - 1)
                            tdSelectedTask++;
                    }
                    else if (CheckCollisionPointRec(mousePos, btnTDClear))
                    {
                        ClearAllTasks();
                        tdSelectedTask = 0;
                    }
                }
            }
            else if (currentTdScreen == TD_ADD_TASK)
            {
                int key = GetCharPressed();
                while (key > 0)
                {
                    if ((key >= 32 && key <= 125) && (tdLetterCount < 49))
                    {
                        tdInputBuffer[tdLetterCount++] = (char)key;
                        tdInputBuffer[tdLetterCount] = '\0';
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && tdLetterCount > 0)
                    tdInputBuffer[--tdLetterCount] = '\0';
                if (IsKeyPressed(KEY_ENTER) && tdLetterCount > 0)
                {
                    AddTask(tdInputBuffer);
                    currentTdScreen = TD_MENU;
                }
                if (IsKeyPressed(KEY_ESCAPE))
                {
                    currentTdScreen = TD_MENU; 
                }
            }
        }
        break;
        case STUDY_MODULE:
        { 
        }
        break;
        case SNAKE_MODULE:
        {

            switch (currentSnakeScreen)
            {
            case SNAKE_MENU:
                UpdateSnakeMenu();
                break;
            case SNAKE_GAME:
                UpdateSnakeGameplay();
                break;
            case SNAKE_CONTROLS:
                UpdateSnakeControls();
                break;
            case SNAKE_GAME_OVER:
                UpdateSnakeGameOver();
                break;
            default:
                break;
            }
        }
        break;
        case DSA_MODULE:
        { 
            UpdateMusicStream(bgMusic);
            if (IsKeyPressed(KEY_M))
                mcurrentScreen = HOME;
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                if (CheckCollisionPointRec(mousePos, btnDSALinkedList))
                {
                    mcurrentScreen = LL_MODULE;
                    currentLLScreen = LL_MAIN_MENU;
                    llInputText = "";
                    llMessageText = ""; 
                }
                else if (CheckCollisionPointRec(mousePos, btnDSAFib))
                {
                    mcurrentScreen = FIB_MODULE;
                    currentFibFactScreen = FIBFACT_MENU;
                    fibFactInputStr = "";
                    fibFactCallStack.clear(); 
                }
            }
        }
        break;
        case LL_MODULE:
        { 
            UpdateMusicStream(bgMusic);
            if (IsKeyPressed(KEY_E))
            {
                mcurrentScreen = DSA_MODULE;
                currentLLScreen = LL_MAIN_MENU;
            }

            if (currentLLScreen == LL_INPUT || currentLLScreen == LL_SEARCH_INPUT)
            {
                int key = GetCharPressed();
                while (key > 0)
                {
                    if (((key >= '0' && key <= '9') || (key == '-' && llInputText.empty())) && llInputText.length() < 10)
                    {
                        llInputText += (char)key;
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && !llInputText.empty())
                    llInputText.pop_back();
            }

            if (currentLLScreen == LL_MAIN_MENU)
            {
                for (int i = 0; i < 4; i++)
                { 
                    if (CheckCollisionPointRec(mousePos, llButtons[i].rect) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    {
                        switch (i)
                        {
                        case 0:
                            llInputText = "";
                            currentLLScreen = LL_INPUT;
                            break; 
                        case 1:   
                            if (dsaLinkedList.toString() == "List is empty!")
                                llMessageText = "List is already empty!";
                            else
                            {
                                dsaLinkedList.pop_front();
                                llMessageText = "First Node Removed!";
                                if (dsaLinkedList.toString() == "List is empty!")
                                    llMessageText = "List is now empty!";
                            }
                            currentLLScreen = LL_MESSAGE;
                            break;
                        case 2:
                            llMessageText = dsaLinkedList.toString();
                            currentLLScreen = LL_MESSAGE;
                            break; 
                        case 3:
                            llInputText = "";
                            currentLLScreen = LL_SEARCH_INPUT;
                            break; 
                        }
                    }
                }
            }
            else if (currentLLScreen == LL_MESSAGE)
            {
                if (IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    currentLLScreen = LL_MAIN_MENU;
            }
            else if (currentLLScreen == LL_INPUT)
            {
                if (IsKeyPressed(KEY_ENTER) && !llInputText.empty())
                {
                    try
                    {
                        dsaLinkedList.push_back(std::stoi(llInputText));
                        llMessageText = "Added " + llInputText + "!";
                    }
                    catch (const std::exception &)
                    {
                        llMessageText = "Invalid input!";
                    }
                    currentLLScreen = LL_MESSAGE;
                    llInputText = "";
                }
                else if (IsKeyPressed(KEY_ESCAPE))
                {
                    currentLLScreen = LL_MAIN_MENU;
                    llInputText = "";
                }
            }
            else if (currentLLScreen == LL_SEARCH_INPUT)
            {
                if (IsKeyPressed(KEY_ENTER) && !llInputText.empty())
                {
                    try
                    {
                        int idx = dsaLinkedList.search(std::stoi(llInputText));
                        if (idx == -1)
                            llMessageText = llInputText + " not found!";
                        else
                            llMessageText = llInputText + " found at index " + std::to_string(idx);
                    }
                    catch (const std::exception &)
                    {
                        llMessageText = "Invalid input!";
                    }
                    currentLLScreen = LL_MESSAGE;
                    llInputText = "";
                }
                else if (IsKeyPressed(KEY_ESCAPE))
                {
                    currentLLScreen = LL_MAIN_MENU;
                    llInputText = "";
                }
            }
        }
        break;
        case FIB_MODULE:
        { 
            UpdateMusicStream(bgMusic);
            if (IsKeyPressed(KEY_E))
            {
                mcurrentScreen = DSA_MODULE;
                currentFibFactScreen = FIBFACT_MENU;
            }

            if (currentFibFactScreen == FIBFACT_MENU)
            {
                if (IsKeyPressed(KEY_F))
                {
                    isFactorialMode = true;
                    currentFibFactScreen = FIBFACT_INPUT;
                    fibFactInputStr = "";
                    fibFactCallStack.clear();
                }
                if (IsKeyPressed(KEY_B))
                {
                    isFactorialMode = false;
                    currentFibFactScreen = FIBFACT_INPUT;
                    fibFactInputStr = "";
                    fibFactCallStack.clear();
                }
            }
            else if (currentFibFactScreen == FIBFACT_INPUT)
            {
                int key = GetCharPressed();
                while (key > 0)
                {
                    if ((key >= '0' && key <= '9') && (fibFactInputStr.length() < 3))
                        fibFactInputStr += (char)key;
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && !fibFactInputStr.empty())
                    fibFactInputStr.pop_back();
                if (IsKeyPressed(KEY_ENTER) && !fibFactInputStr.empty())
                {
                    try
                    {
                        fibFactInputNum = std::stoi(fibFactInputStr);
                        int maxN = isFactorialMode ? 12 : 15;
                        if (fibFactInputNum >= 0 && fibFactInputNum <= maxN)
                        {
                            fibFactCallStack.clear();
                            if (isFactorialMode)
                                fibFactResult = calculateFactorial(fibFactInputNum, fibFactCallStack);
                            else
                                fibFactResult = calculateFibonacci(fibFactInputNum, fibFactCallStack);
                            fibFactScrollOffset = 0;
                            currentFibFactScreen = FIBFACT_RESULT;
                        }
                        else
                            fibFactInputStr += " (0-" + std::to_string(maxN) + ")";
                    }
                    catch (const std::exception &)
                    {
                        fibFactInputStr = "ERR";
                    }
                }
                else if (IsKeyPressed(KEY_ESCAPE))
                {
                    currentFibFactScreen = FIBFACT_MENU;
                    fibFactInputStr = "";
                }
            }
            else if (currentFibFactScreen == FIBFACT_RESULT)
            {
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    currentFibFactScreen = FIBFACT_MENU;
                if (IsKeyPressed(KEY_DOWN))
                    fibFactScrollOffset += 20;
                if (IsKeyPressed(KEY_UP))
                    fibFactScrollOffset -= 20;
                if (fibFactScrollOffset < 0)
                    fibFactScrollOffset = 0;

            }
        }
        break;
        }

        BeginDrawing();
        ClearBackground(Color{10, 15, 60, 255}); 

        switch (mcurrentScreen)
        {
        case HOME:
        {

            DrawText("NEXIVO", screenWidth / 2 - MeasureText("NEXIVO", 50) / 2, 80, 50, Color{0, 200, 255, 255});

            DrawText("All-in-One", screenWidth / 2 - MeasureText("All-in-One", 25) / 2, 135, 25, WHITE);
            DrawRectangleRec(btnToDo, Color{0, 180, 255, 255});
            DrawText("TO-DO LIST", btnToDo.x + (btnToDo.width - MeasureText("TO-DO LIST", 35)) / 2, btnToDo.y + 27, 35, WHITE);
            DrawRectangleRec(btnStudy, Color{0, 150, 220, 255});
            DrawText("STUDY PRACTICE", btnStudy.x + (btnStudy.width - MeasureText("STUDY PRACTICE", 35)) / 2, btnStudy.y + 27, 35, WHITE);
            DrawRectangleRec(btnSnake, Color{0, 120, 200, 255});
            DrawText("SNAKE GAME", btnSnake.x + (btnSnake.width - MeasureText("SNAKE GAME", 35)) / 2, btnSnake.y + 27, 35, WHITE);
            DrawRectangleRec(btnDSA, Color{0, 90, 170, 255});
            DrawText("DSA VISUALIZATION", btnDSA.x + (btnDSA.width - MeasureText("DSA VISUALIZATION", 35)) / 2, btnDSA.y + 27, 35, WHITE);
        }
        break;
        case TD_MODULE:
        {
            ClearBackground(Color{250, 245, 240, 255});
            DrawText("TO-DO LIST", 60, 20, 30, DARKPURPLE);
            DrawText("Press 'TAB' for Main Menu", screenWidth / 2 - MeasureText("Press 'TAB' for Main Menu", 20) / 2, screenHeight - 40, 20, DARKPURPLE);

            if (currentTdScreen == TD_MENU)
            {
                DrawTasks(60, 80, tdSelectedTask);
                bool hovA = CheckCollisionPointRec(mousePos, btnTDAdd);
                DrawRectangleRec(btnTDAdd, hovA ? SKYBLUE : Color{0, 168, 255, 255});
                DrawText("Add Task", btnTDAdd.x + 20, btnTDAdd.y + 12, 20, WHITE);
                bool hovD = CheckCollisionPointRec(mousePos, btnTDDelete);
                DrawRectangleRec(btnTDDelete, hovD ? RED : DARKGRAY);
                DrawText("Delete Sel.", btnTDDelete.x + 20, btnTDDelete.y + 12, 20, WHITE);
                bool hovU = CheckCollisionPointRec(mousePos, btnTDMoveUp);
                DrawRectangleRec(btnTDMoveUp, hovU ? LIME : DARKGREEN);
                DrawText("Move Up Sel.", btnTDMoveUp.x + 20, btnTDMoveUp.y + 12, 20, WHITE);
                bool hovDn = CheckCollisionPointRec(mousePos, btnTDMoveDown);
                DrawRectangleRec(btnTDMoveDown, hovDn ? GOLD : ORANGE);
                DrawText("Move Down Sel.", btnTDMoveDown.x + 20, btnTDMoveDown.y + 12, 20, WHITE);
                bool hovC = CheckCollisionPointRec(mousePos, btnTDClear);
                DrawRectangleRec(btnTDClear, hovC ? MAROON : PURPLE);
                DrawText("Clear All", btnTDClear.x + 20, btnTDClear.y + 12, 20, WHITE);
                DrawText("Use UP/DOWN keys to select.", 60, screenHeight - 70, 18, GRAY);
            }
            else if (currentTdScreen == TD_ADD_TASK)
            {
                DrawText("Type task name and press ENTER:", 60, 60, 22, DARKPURPLE);
                // DrawText("Press ESC to cancel.", 60, screenHeight - 70, 18, GRAY);
                Rectangle inputBox = {60, 100, (float)screenWidth - 120, 40};
                DrawRectangleRec(inputBox, WHITE);
                DrawRectangleLinesEx(inputBox, 1, DARKGRAY);
                DrawText(tdInputBuffer, 70, 107, 22, BLACK);
                if (((int)(GetTime() * 2.0f) % 2) == 0)
                    DrawText("_", 70 + MeasureText(tdInputBuffer, 22), 107, 22, BLACK);
            }
        }
        break;
        case STUDY_MODULE:
        {
            ClearBackground(DARK_BLUE_BACKGROUND); 
            switch (currentStudyScreen)
            {
            case STUDY_MAIN_MENU:
                UIRenderer::DrawStudyMainMenuScreen(currentStudyScreen, chosenQuizCategory);
                break;
            case STUDY_QUIZ_TYPE_SELECTION:
                UIRenderer::DrawQuizModeScreen(currentStudyScreen, activeQuizSession, chosenQuizCategory, lastCategoryPlayed_Study);
                break;
            case STUDY_QUIZ_IN_PROGRESS:
                UIRenderer::DrawQuizGameplayScreen(currentStudyScreen, activeQuizSession);
                break;
            case STUDY_QUIZ_SUMMARY:
                UIRenderer::DrawSummaryScreen(currentStudyScreen, chosenQuizCategory, activeQuizSession, lastCategoryPlayed_Study);
                break;
            }
        }
        break;
        case SNAKE_MODULE:
        {
            switch (currentSnakeScreen)
            { 
            case SNAKE_MENU:
                DrawSnakeMenu();
                break;
            case SNAKE_GAME:
                DrawSnakeGameplay();
                break;
            case SNAKE_CONTROLS:
                DrawSnakeControls();
                break;
            case SNAKE_GAME_OVER:
                DrawSnakeGameOver();
                break;
            default:
                ClearBackground(RAYWHITE);
                DrawText("SNAKE: UNKNOWN STATE", 50, 50, 30, RED);
                break;
            }
        }
        break;
        case DSA_MODULE:
        {                                            
            ClearBackground(Color{20, 30, 70, 255}); 
            DrawText("DSA VISUALIZATION", screenWidth / 2 - MeasureText("DSA VISUALIZATION", 40) / 2, 100, 40, SKYBLUE);
            bool hLL = CheckCollisionPointRec(mousePos, btnDSALinkedList);
            DrawRectangleRec(btnDSALinkedList, hLL ? Color{0, 200, 255, 255} : Color{0, 180, 255, 255});
            DrawText("LINKED LIST", btnDSALinkedList.x + (btnDSALinkedList.width - MeasureText("LINKED LIST", 30)) / 2, btnDSALinkedList.y + 30, 30, WHITE);
            bool hFib = CheckCollisionPointRec(mousePos, btnDSAFib);
            DrawRectangleRec(btnDSAFib, hFib ? Color{0, 170, 220, 255} : Color{0, 150, 220, 255});
            DrawText("FIBONACCI / FACTORIAL", btnDSAFib.x + (btnDSAFib.width - MeasureText("FIBONACCI / FACTORIAL", 30)) / 2, btnDSAFib.y + 30, 30, WHITE);
            DrawText("Press 'M' for NEXIVO Menu", screenWidth / 2 - MeasureText("Press 'M' for NEXIVO Menu", 20) / 2, screenHeight - 70, 20, LIGHTGRAY);
        }
        break;
        case LL_MODULE:
        { 
            ClearBackground(BLACK);
            DrawText("Press 'E' for DSA Menu", screenWidth / 2 - MeasureText("Press 'E' for DSA Menu", 20) / 2, screenHeight - 40, 20, GRAY);
            if (currentLLScreen == LL_MAIN_MENU)
            {
                DrawText("LINKED LIST OPERATIONS", screenWidth / 2 - MeasureText("LINKED LIST OPERATIONS", 30) / 2, 40, 30, RAYWHITE);
                for (int i = 0; i < 4; i++)
                {
                    bool hov = CheckCollisionPointRec(mousePos, llButtons[i].rect);
                    DrawRectangleRec(llButtons[i].rect, hov ? DARKGRAY : LIGHTGRAY);
                    DrawText(llButtons[i].label, llButtons[i].rect.x + 20, llButtons[i].rect.y + 25, 25, hov ? WHITE : BLACK);
                }
            }
            else if (currentLLScreen == LL_MESSAGE)
            {
                DrawText(llMessageText.c_str(), screenWidth / 2 - MeasureText(llMessageText.c_str(), 30) / 2, screenHeight / 2 - 40, 30, LIME);
                DrawText("Press ENTER or Click to return", screenWidth / 2 - MeasureText("Press ENTER or Click to return", 20) / 2, screenHeight / 2 + 20, 20, GRAY);
            }
            else if (currentLLScreen == LL_INPUT || currentLLScreen == LL_SEARCH_INPUT)
            {
                const char *p = (currentLLScreen == LL_INPUT) ? "Enter number to push back:" : "Enter number to search:";
                DrawText(p, screenWidth / 2 - MeasureText(p, 30) / 2, screenHeight / 2 - 80, 30, LIGHTGRAY);
                Rectangle ib = {(float)screenWidth / 2 - 150, (float)screenHeight / 2 - 25, 300, 50};
                DrawRectangleRec(ib, RAYWHITE);
                DrawRectangleLinesEx(ib, 1, BLACK);
                DrawText(llInputText.c_str(), ib.x + 10, ib.y + 10, 30, BLACK);
                if (((int)(GetTime() * 2.0f) % 2) == 0)
                    DrawText("_", ib.x + 10 + MeasureText(llInputText.c_str(), 30), ib.y + 10, 30, BLACK);
                DrawText("ENTER to submit, ESC to cancel", screenWidth / 2 - MeasureText("ENTER to submit, ESC to cancel", 20) / 2, ib.y + 70, 20, GRAY);
            }
        }
        break;
        case FIB_MODULE:
        { 
            ClearBackground(Color{20, 20, 50, 255});
            DrawText("Press 'E' for DSA Menu", screenWidth / 2 - MeasureText("Press 'E' for DSA Menu", 20) / 2, screenHeight - 40, 20, LIGHTGRAY);
            if (currentFibFactScreen == FIBFACT_MENU)
            {
                DrawText("Fibonacci / Factorial Visualizer", screenWidth / 2 - MeasureText("Fibonacci / Factorial Visualizer", 30) / 2, 100, 30, SKYBLUE);
                DrawText("Press 'F' for Factorial", screenWidth / 2 - MeasureText("Press 'F' for Factorial", 25) / 2, 200, 25, GOLD);
                DrawText("Press 'B' for Fibonacci", screenWidth / 2 - MeasureText("Press 'B' for Fibonacci", 25) / 2, 250, 25, GOLD);
            }
            else if (currentFibFactScreen == FIBFACT_INPUT)
            {
                const char *mT = isFactorialMode ? "Factorial" : "Fibonacci";
                DrawText(TextFormat("Enter N for %s (0-%d):", mT, isFactorialMode ? 12 : 15), screenWidth / 2 - MeasureText(TextFormat("Enter N for %s (0-12):", mT), 25) / 2, 150, 25, RAYWHITE);
                Rectangle ib = {(float)screenWidth / 2 - 100, 200, 200, 50};
                DrawRectangleRec(ib, RAYWHITE);
                DrawRectangleLinesEx(ib, 1, BLACK);
                DrawText(fibFactInputStr.c_str(), ib.x + 10, ib.y + 10, 30, BLACK);
                if (((int)(GetTime() * 2.0f) % 2) == 0)
                    DrawText("_", ib.x + 10 + MeasureText(fibFactInputStr.c_str(), 30), ib.y + 10, 30, BLACK);
                DrawText("ENTER to calculate, ESC back", screenWidth / 2 - MeasureText("ENTER to calculate, ESC back", 20) / 2, 270, 20, GRAY);
            }
            else if (currentFibFactScreen == FIBFACT_RESULT)
            {
                const char *mT = isFactorialMode ? "Factorial" : "Fibonacci";
                DrawText(TextFormat("%s(%d) = %d", mT, fibFactInputNum, fibFactResult), screenWidth / 2 - MeasureText(TextFormat("%s(%d) = %d", mT, fibFactInputNum, fibFactResult), 30) / 2, 50, 30, LIME);
                DrawText("Call Stack (Scroll UP/DOWN):", 50, 100, 20, GOLD);
                Rectangle sV = {50, 130, (float)screenWidth - 100, (float)screenHeight - 220};
                DrawRectangleLinesEx(sV, 1, GRAY);
                BeginScissorMode(sV.x, sV.y, sV.width, sV.height);
                for (size_t i = 0; i < fibFactCallStack.size(); ++i)
                    DrawText(fibFactCallStack[i].text.c_str(), sV.x + 10, sV.y + 10 + i * 22 - fibFactScrollOffset, 20, RAYWHITE);
                EndScissorMode();
                DrawText("Press ENTER, ESC or Click to return", screenWidth / 2 - MeasureText("Press ENTER, ESC or Click to return", 20) / 2, screenHeight - 70, 20, GRAY);
            }
        }
        break;
        }
        EndDrawing();
    }
    
    UnloadMusicStream(bgMusic);
    FormulaSuggester::clearSuggesterData();
    FreeActualSnake();
    ClearAllTasks();   
    CloseWindow();
    return 0;
}

// -------------------- SNAKE GAME IMPLEMENTATIONS----------------------
void InitSnakeGame(void)
{
    FreeActualSnake(); 
    InitActualSnake(); 
    SpawnSnakeFood();  
    snakeScore = 0;
    snakeHighScore = snakeHighScore; 
    snakeFramesCounter = 0;
    snakeGameSpeed = 10; 
}

void InitActualSnake(void)
{
    snake.head = NULL;
    snake.tail = NULL;
    snake.length = 0;
    snake.dir = SNAKE_RIGHT;
    snake.nextDir = SNAKE_RIGHT;
    int startX = GRID_WIDTH / 2, startY = GRID_HEIGHT / 2;
    AddSnakeHead(startX - 2, startY);
    AddSnakeHead(startX - 1, startY);
    AddSnakeHead(startX, startY); 
}

void FreeActualSnake(void)
{
    SnakeNode *current = snake.head;
    while (current != NULL)
    {
        SnakeNode *temp = current;
        current = current->next;
        free(temp);
    }
    snake.head = NULL;
    snake.tail = NULL;
    snake.length = 0;
}

void AddSnakeHead(int x, int y)
{
    SnakeNode *node = (SnakeNode *)malloc(sizeof(SnakeNode));
    if (!node)
        return; 
    node->x = x;
    node->y = y;
    node->next = snake.head;
    snake.head = node;
    if (snake.tail == NULL)
        snake.tail = node;
    snake.length++;
}

void RemoveSnakeTail(void)
{
    if (!snake.head)
        return;
    if (snake.head == snake.tail)
    { 
        free(snake.head);
        snake.head = NULL;
        snake.tail = NULL;
        snake.length = 0;
        return;
    }
    SnakeNode *current = snake.head;
    while (current->next != snake.tail)
        current = current->next;
    free(snake.tail);
    snake.tail = current;
    snake.tail->next = NULL;
    snake.length--;
}

void SpawnSnakeFood(void)
{
    do
    {
        food.x = GetRandomValue(0, GRID_WIDTH - 1);
        food.y = GetRandomValue(0, GRID_HEIGHT - 1);
    } while (CheckSnakeCollision(food.x, food.y)); 
}

bool CheckSnakeCollision(int x, int y)
{ 
    SnakeNode *current = snake.head;
    while (current != NULL)
    {
        if (current->x == x && current->y == y)
            return true;
        current = current->next;
    }
    return false;
}

void MoveActualSnake(void)
{
    if ((snake.nextDir == SNAKE_UP && snake.dir != SNAKE_DOWN) ||
        (snake.nextDir == SNAKE_DOWN && snake.dir != SNAKE_UP) ||
        (snake.nextDir == SNAKE_LEFT && snake.dir != SNAKE_RIGHT) ||
        (snake.nextDir == SNAKE_RIGHT && snake.dir != SNAKE_LEFT))
    {
        snake.dir = snake.nextDir;
    }

    int newX = snake.head->x, newY = snake.head->y;
    switch (snake.dir)
    {
    case SNAKE_UP:
        newY--;
        break;
    case SNAKE_DOWN:
        newY++;
        break;
    case SNAKE_LEFT:
        newX--;
        break;
    case SNAKE_RIGHT:
        newX++;
        break;
    }

    if (newX < 0)
        newX = GRID_WIDTH - 1;
    if (newX >= GRID_WIDTH)
        newX = 0;
    if (newY < 0)
        newY = GRID_HEIGHT - 1;
    if (newY >= GRID_HEIGHT)
        newY = 0;

    SnakeNode *currentSegment = snake.head;
    while (currentSegment)
    {
        if (currentSegment->x == newX && currentSegment->y == newY)
        {

            if (currentSegment == snake.tail && !(newX == food.x && newY == food.y))
            {
                
            }
            else
            {
                currentSnakeScreen = SNAKE_GAME_OVER;
                if (snakeScore > snakeHighScore)
                    snakeHighScore = snakeScore;
                return; 
            }
        }
        currentSegment = currentSegment->next;
    }

    AddSnakeHead(newX, newY); 

    if (newX == food.x && newY == food.y)
    { 
        snakeScore += 10;
        SpawnSnakeFood();
        if (snakeScore > 0 && snakeScore % 50 == 0 && snakeGameSpeed < 20)
            snakeGameSpeed++;
    }
    else
    {
        RemoveSnakeTail();
    } 
}

void UpdateSnakeGameplay(void)
{
    if (IsKeyPressed(KEY_M))
    {
        mcurrentScreen = HOME;
        return;
    }
    if (IsKeyPressed(KEY_ESCAPE))
    {
        currentSnakeScreen = SNAKE_MENU;
        return;
    }

    if (IsKeyPressed(KEY_UP))
        snake.nextDir = SNAKE_UP;
    else if (IsKeyPressed(KEY_DOWN))
        snake.nextDir = SNAKE_DOWN;
    else if (IsKeyPressed(KEY_LEFT))
        snake.nextDir = SNAKE_LEFT;
    else if (IsKeyPressed(KEY_RIGHT))
        snake.nextDir = SNAKE_RIGHT;

    snakeFramesCounter++;
    if (snakeFramesCounter >= (60 / snakeGameSpeed))
    {
        MoveActualSnake();
        snakeFramesCounter = 0;
    }
}

void DrawSnakeGameplay(void)
{
    ClearBackground(RAYWHITE);
    for (int x = 0; x <= screenWidth; x += GRID_SIZE)
        DrawLine(x, 0, x, screenHeight, LIGHTGRAY);
    for (int y = 0; y <= screenHeight; y += GRID_SIZE)
        DrawLine(0, y, screenWidth, y, LIGHTGRAY);

    SnakeNode *current = snake.head;
    bool first = true;
    while (current)
    {
        Rectangle r = {(float)(current->x * GRID_SIZE), (float)(current->y * GRID_SIZE), GRID_SIZE, GRID_SIZE};
        DrawRectangleRec(r, first ? DARKGREEN : GREEN);
        DrawRectangleLinesEx(r, 1, BLACK);
        first = false;
        current = current->next;
    }
    DrawRectangleRec({(float)(food.x * GRID_SIZE), (float)(food.y * GRID_SIZE), GRID_SIZE, GRID_SIZE}, RED);
    DrawText(TextFormat("Score: %d", snakeScore), 10, 10, 20, BLACK);
    DrawText(TextFormat("High Score: %d", snakeHighScore), 10, 40, 20, BLACK);
    DrawText("ARROWS move. ESC for Snake Menu. 'M' for NEXIVO Menu.", 10, screenHeight - 30, 15, BLACK);
}

void DrawSnakeMenu(void)
{
    ClearBackground(RAYWHITE);
    DrawTextCentered("SNAKE GAME", screenHeight / 4, 50, BLACK);
    Vector2 mouse = GetMousePosition();
    for (int i = 0; i < snakeMenuItemCount; i++)
    {
        int tw = MeasureText(snakeMenuItems[i], 30);
        int px = screenWidth / 2 - tw / 2, py = screenHeight / 2 + i * 50 - 20;
        Rectangle r = {(float)(px - 20), (float)(py - 10), (float)(tw + 40), 50};
        bool hov = CheckCollisionPointRec(mouse, r);
        DrawRectangleRec(r, hov ? LIGHTGRAY : Color{200, 200, 200, 255});
        DrawRectangleLinesEx(r, 2, DARKGRAY);
        DrawText(snakeMenuItems[i], px, py, 30, (i == snakeMenuSelected || hov) ? BLACK : DARKGRAY);
    }
    DrawTextCentered("UP/DOWN to navigate, ENTER or Click to select.", screenHeight - 50, 15, BLACK);
    DrawTextCentered("'M' always returns to NEXIVO Main Menu.", screenHeight - 25, 15, BLACK);
}

void UpdateSnakeMenu(void)
{
    if (IsKeyPressed(KEY_M))
    {
        mcurrentScreen = HOME;
        return;
    }
    if (IsKeyPressed(KEY_UP))
    {
        snakeMenuSelected = (snakeMenuSelected - 1 + snakeMenuItemCount) % snakeMenuItemCount;
    }
    if (IsKeyPressed(KEY_DOWN))
    {
        snakeMenuSelected = (snakeMenuSelected + 1) % snakeMenuItemCount;
    }

    bool clickedByMouse = false;
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    {
        Vector2 mouse = GetMousePosition();
        for (int i = 0; i < snakeMenuItemCount; i++)
        {
            int tw = MeasureText(snakeMenuItems[i], 30);
            int px = screenWidth / 2 - tw / 2, py = screenHeight / 2 + i * 50 - 20;
            Rectangle r = {(float)(px - 20), (float)(py - 10), (float)(tw + 40), 50};
            if (CheckCollisionPointRec(mouse, r))
            {
                snakeMenuSelected = i;
                clickedByMouse = true;
                break;
            }
        }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || clickedByMouse)
    {
        switch (snakeMenuSelected)
        {
        case 0:
            currentSnakeScreen = SNAKE_GAME;
            InitSnakeGame();
            break; 
        case 1:
            currentSnakeScreen = SNAKE_CONTROLS;
            break; 
        case 2:
            mcurrentScreen = HOME;
            break; 
        }
    }
}

void DrawSnakeControls(void)
{
    ClearBackground(RAYWHITE);
    DrawTextCentered("CONTROLS", screenHeight / 6, 40, BLACK);
    int y = screenHeight / 3;
    DrawTextCentered("- Arrow Keys to move Snake", y, 25, BLACK);
    y += 40;
    DrawTextCentered("- Eat food (red blocks) to grow and score", y, 25, BLACK);
    y += 40;
    DrawTextCentered("- Don't collide with yourself (walls wrap around)", y, 25, BLACK);
    y += 40;
    DrawTextCentered("- ESC returns to Snake Game Menu", y, 25, BLACK);
    y += 40;
    DrawTextCentered("- 'M' always returns to NEXIVO Main Menu", y, 25, BLACK);
    y += 60;

    const char *rt = "Back to Snake Menu";
    int tw = MeasureText(rt, 25);
    Rectangle r = {(float)(screenWidth / 2 - (tw + 40) / 2), (float)y, (float)(tw + 40), 50};
    bool hov = CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRec(r, hov ? LIGHTGRAY : Color{200, 200, 200, 255});
    DrawRectangleLinesEx(r, 2, DARKGRAY);
    DrawText(rt, r.x + 20, r.y + 12, 25, BLACK);
}

void UpdateSnakeControls(void)
{
    if (IsKeyPressed(KEY_M))
    {
        mcurrentScreen = HOME;
        return;
    }
    if (IsKeyPressed(KEY_ESCAPE))
    {
        currentSnakeScreen = SNAKE_MENU;
        return;
    }

    const char *rt = "Back to Snake Menu";
    int tw = MeasureText(rt, 25);
    int yBtn = screenHeight / 3 + 40 * 4 + 60;
    Rectangle r = {(float)(screenWidth / 2 - (tw + 40) / 2), (float)yBtn, (float)(tw + 40), 50};
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), r))
    {
        currentSnakeScreen = SNAKE_MENU;
    }
}

void DrawSnakeGameOver(void)
{
    ClearBackground(RAYWHITE);
    DrawTextCentered("GAME OVER", screenHeight / 3, 60, RED);
    DrawTextCentered(TextFormat("Your Score: %d", snakeScore), screenHeight / 3 + 80, 30, BLACK);
    DrawTextCentered(TextFormat("High Score: %d", snakeHighScore), screenHeight / 3 + 120, 30, BLACK);
    int y = screenHeight / 3 + 180;
    DrawTextCentered("ENTER or Click button for Snake Menu", y, 25, BLACK);
    y += 30;
    DrawTextCentered("'M' for NEXIVO Main Menu", y, 25, BLACK);
    y += 50;

    const char *rt = "Try Again (Snake Menu)";
    int tw = MeasureText(rt, 25);
    Rectangle r = {(float)(screenWidth / 2 - (tw + 40) / 2), (float)y, (float)(tw + 40), 50};
    bool hov = CheckCollisionPointRec(GetMousePosition(), r);
    DrawRectangleRec(r, hov ? LIGHTGRAY : Color{200, 200, 200, 255});
    DrawRectangleLinesEx(r, 2, DARKGRAY);
    DrawText(rt, r.x + 20, r.y + 12, 25, BLACK);
}

void UpdateSnakeGameOver(void)
{
    if (IsKeyPressed(KEY_M))
    {
        mcurrentScreen = HOME;
        InitSnakeGame();
        return;
    } 
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_ESCAPE))
    {
        currentSnakeScreen = SNAKE_MENU;
        InitSnakeGame();
        return; 
    }
    const char *rt = "Try Again (Snake Menu)";
    int tw = MeasureText(rt, 25);
    int yBtn = screenHeight / 3 + 180 + 30 + 50; 
    Rectangle r = {(float)(screenWidth / 2 - (tw + 40) / 2), (float)yBtn, (float)(tw + 40), 50};
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), r))
    {
        currentSnakeScreen = SNAKE_MENU;
        InitSnakeGame(); 
    }
}

void DrawTextCentered(const char *text, int y, int fontSize, Color color)
{
    DrawText(text, screenWidth / 2 - MeasureText(text, fontSize) / 2, y, fontSize, color);
}
// -----------------------------------------------------

// -------------------- FIB / FACTORIAL IMPLEMENTATIONS-----------------
int calculateFactorial(int n, vector<CallInfo> &stack)
{
    stringstream ss;
    ss << "factorial(" << n << ")";
    stack.push_back({ss.str()});
    if (n < 0)
    {
        stack.push_back({"Error: Negative input"});
        return -1;
    }
    if (n == 0 || n == 1)
    {
        ss.str("");
        ss.clear();
        ss << "  factorial(" << n << ") returns 1";
        stack.push_back({ss.str()});
        return 1;
    }
    int recRes = calculateFactorial(n - 1, stack);
    int res = n * recRes;
    ss.str("");
    ss.clear();
    ss << "  calc: " << n << " * " << recRes << " = " << res;
    stack.push_back({ss.str()});
    ss.str("");
    ss.clear();
    ss << "  factorial(" << n << ") returns " << res;
    stack.push_back({ss.str()});
    return res;
}

int calculateFibonacci(int n, vector<CallInfo> &stack)
{
    stringstream ss;
    ss << "fibonacci(" << n << ")";
    stack.push_back({ss.str()});
    if (n < 0)
    {
        stack.push_back({"Error: Negative input"});
        return -1;
    }
    if (n == 0)
    {
        ss.str("");
        ss.clear();
        ss << "  fibonacci(0) returns 0";
        stack.push_back({ss.str()});
        return 0;
    }
    if (n == 1)
    {
        ss.str("");
        ss.clear();
        ss << "  fibonacci(1) returns 1";
        stack.push_back({ss.str()});
        return 1;
    }
    int fib1 = calculateFibonacci(n - 1, stack);
    int fib2 = calculateFibonacci(n - 2, stack);
    int res = fib1 + fib2;
    ss.str("");
    ss.clear();
    ss << "  calc: " << fib1 << " + " << fib2 << " = " << res;
    stack.push_back({ss.str()});
    ss.str("");
    ss.clear();
    ss << "  fibonacci(" << n << ") returns " << res;
    stack.push_back({ss.str()});
    return res;
}
// -----------------------------------------------------