#include "raylib.h"
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <ctime>
#include <cmath>
#include <iostream>

using namespace std;

const int SCREEN_WIDTH = 900;
const int SCREEN_HEIGHT = 700;
const float QUIZ_TIME_SECONDS = 60.0f;
const Color DARK_BLUE_BACKGROUND = {28, 28, 40, 255};
const Color LIGHT_LAVENDER_TEXT = {220, 220, 255, 255};
const Color GOLD_ACCENT = {255, 223, 100, 255};
const Color MEDIUM_DARK_BUTTON = {70, 70, 90, 255};
const Color LIGHT_DARK_BUTTON_HOVER = {100, 100, 120, 255};
const Color VERY_DARK_TEXT = {10, 10, 10, 255};

enum StudyScreen {
    STUDY_MAIN_MENU,
    STUDY_QUIZ_TYPE_SELECTION,
    STUDY_QUIZ_IN_PROGRESS,
    STUDY_QUIZ_SUMMARY
};

enum StudyCategory {
    STUDY_ARITHMETIC,
    STUDY_ALGEBRA,
    STUDY_FORMULAS,
    STUDY_NONE_SELECTED
};

enum StudyOperation {
    STUDY_ADD,
    STUDY_SUBTRACT,
    STUDY_MULTIPLY,
    STUDY_DIVIDE
};

struct ArithmeticQuestion {
    long long firstNumber;
    long long secondNumber;
    StudyOperation operation;
    double correctAnswer;
    string questionText;
};

struct AlgebraQuestion {
    int coeffA;
    int constB;
    int resultC;
    double correctXValue;
    string questionText;
};

struct FormulaQuestion {
    string questionPrompt;
    string fullFormulaText;
    string answerPattern;
};

struct QuizSession {
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

    QuizSession() : timeLeft(QUIZ_TIME_SECONDS), score(0), questionsAttempted(0),
                    showFeedback(false), wasLastAnswerCorrect(false), feedbackTimer(0.0f),
                    inputLength(0), currentCategory(STUDY_NONE_SELECTED) {}
};

namespace FormulaSuggester {
    const int ALPHABET_SIZE = 128;

    struct TrieNode {
        TrieNode* children[ALPHABET_SIZE];
        bool isEndOfFormulaPattern;
        string formulaPattern;

        TrieNode() : isEndOfFormulaPattern(false), formulaPattern("") {
            for (int i = 0; i < ALPHABET_SIZE; ++i) children[i] = nullptr;
        }
        ~TrieNode() {
            for (int i = 0; i < ALPHABET_SIZE; ++i) delete children[i];
        }
    };

    TrieNode* rootNode = nullptr;

    int getCharIndex(char c) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc < ALPHABET_SIZE) return uc;
        return -1;
    }

    void addFormulaPattern(const std::string& pattern) {
        if (!rootNode) rootNode = new TrieNode();
        TrieNode* currentNode = rootNode;
        string normalizedPattern = pattern;
        transform(normalizedPattern.begin(), normalizedPattern.end(), normalizedPattern.begin(), ::toupper);

        for (char ch : normalizedPattern) {
            int index = getCharIndex(ch);
            if (index == -1) continue;

            if (!currentNode->children[index]) {
                currentNode->children[index] = new TrieNode();
            }
            currentNode = currentNode->children[index];
        }
        currentNode->isEndOfFormulaPattern = true;
        currentNode->formulaPattern = pattern;
    }

    void collectPatternsFromNode(TrieNode* startingNode, std::vector<std::string>& foundPatterns) {
        if (!startingNode) return;
        if (startingNode->isEndOfFormulaPattern && !startingNode->formulaPattern.empty()) {
            foundPatterns.push_back(startingNode->formulaPattern);
        }
        for (int i = 0; i < ALPHABET_SIZE; ++i) {
            if (startingNode->children[i]) {
                collectPatternsFromNode(startingNode->children[i], foundPatterns);
            }
        }
    }

    vector<string> findMatchingPatterns(const string& prefix) {
        vector<string> foundSuggestions;
        if (!rootNode || prefix.empty()) return foundSuggestions;

        TrieNode* currentNode = rootNode;
        string normalizedPrefix = prefix;
        transform(normalizedPrefix.begin(), normalizedPrefix.end(), normalizedPrefix.begin(), ::toupper);

        for (char ch : normalizedPrefix) {
            int index = getCharIndex(ch);
            if (index == -1 || !currentNode->children[index]) {
                return foundSuggestions;
            }
            currentNode = currentNode->children[index];
        }
        collectPatternsFromNode(currentNode, foundSuggestions);
        sort(foundSuggestions.begin(), foundSuggestions.end());
        return foundSuggestions;
    }

    void loadFormulaPatternsFromList(const vector<FormulaQuestion>& questions) {
        for (const auto& q : questions) {
            addFormulaPattern(q.answerPattern);
        }
    }

    void clearSuggesterData() {
        delete rootNode;
        rootNode = nullptr;
    }
}

namespace PerformanceTracker {
    map<string, pair<int, int>> categoryPerformance;

    void recordAttempt(const string& categoryName, bool wasCorrect) {
        if (categoryPerformance.find(categoryName) == categoryPerformance.end()) {
            categoryPerformance[categoryName] = {0, 0};
        }
        categoryPerformance[categoryName].second++;
        if (wasCorrect) {
            categoryPerformance[categoryName].first++;
        }
    }

    void clearPerformanceData() {
        categoryPerformance.clear();
    }
}

namespace QuestionGenerator {
    vector<FormulaQuestion> formulaList;

    void loadFormulasToList() {
        formulaList.push_back({"Area of a Circle = ?", "Area of a Circle=PI*radius*radius", "PI*radius*radius"});
        formulaList.push_back({"Perimeter of a Square = ?", "Perimeter of a Square=4*side", "4*side"});
        formulaList.push_back({"Volume of a Cube = ?", "Volume of a Cube=side*side*side", "side*side*side"});
        formulaList.push_back({"Pythagorean Theorem (c*c) = ?", "Hypotenuse of a Right Triangle (c*c)=a*a+b*b", "a*a+b*b"});
        formulaList.push_back({"Area of a Rectangle = ?", "Area of a Rectangle=length*width", "length*width"});
        formulaList.push_back({"Circumference of a Circle = ?", "Circumference of a Circle=2*PI*radius", "2*PI*radius"});
        FormulaSuggester::loadFormulaPatternsFromList(formulaList);
    }

    ArithmeticQuestion createArithmeticQuestion() {
        ArithmeticQuestion q;
        q.firstNumber = GetRandomValue(1, 25);
        q.secondNumber = GetRandomValue(1, 25);
        int operationType = GetRandomValue(0, 3);

        switch (operationType) {
            case 0:
                q.operation = STUDY_ADD;
                q.correctAnswer = static_cast<double>(q.firstNumber + q.secondNumber);
                q.questionText = to_string(q.firstNumber) + " + " + to_string(q.secondNumber) + " = ?";
                break;
            case 1:
                q.operation = STUDY_SUBTRACT;
                if (q.firstNumber < q.secondNumber) std::swap(q.firstNumber, q.secondNumber);
                q.correctAnswer = static_cast<double>(q.firstNumber - q.secondNumber);
                q.questionText = to_string(q.firstNumber) + " - " + to_string(q.secondNumber) + " = ?";
                break;
            case 2:
                q.operation = STUDY_MULTIPLY;
                q.firstNumber = GetRandomValue(1, 12); q.secondNumber = GetRandomValue(1, 12);
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

    AlgebraQuestion createAlgebraQuestion() {
        AlgebraQuestion q;
        int desiredX = GetRandomValue(-9, 9);
        while(desiredX == 0) desiredX = GetRandomValue(-9,9);

        int puzzleType = GetRandomValue(0, 2);

        if (puzzleType == 0) {
            q.coeffA = GetRandomValue(1, 5) * (GetRandomValue(0,1) ? 1 : -1);
            q.constB = GetRandomValue(-10, 10);
        } else if (puzzleType == 1) {
            q.coeffA = GetRandomValue(1, 5) * (GetRandomValue(0,1) ? 1 : -1);
            q.constB = 0;
        } else {
            q.coeffA = 1;
            q.constB = GetRandomValue(-10, 10);
        }
        if (q.coeffA == 0 && puzzleType !=2) q.coeffA = 1;

        q.resultC = q.coeffA * desiredX + q.constB;
        q.correctXValue = static_cast<double>(desiredX);

        std::string statement;
        if (q.coeffA != 0) {
            if (q.coeffA == 1) statement += "X";
            else if (q.coeffA == -1) statement += "-X";
            else statement += std::to_string(q.coeffA) + "X";
        }

        if (q.constB != 0) {
            if (!statement.empty()) {
                 if (q.constB > 0) statement += " + ";
                 else statement += " - ";
            } else if (q.constB < 0) {
                statement += "-";
            }
            statement += std::to_string(std::abs(q.constB));
        } else if (statement.empty()) {
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

    FormulaQuestion getRandomFormulaQuestion() {
        if (formulaList.empty()) {
            return {"List Empty: No questions found!", "", ""};
        }
        return formulaList[GetRandomValue(0, formulaList.size() - 1)];
    }

    std::string normalizeAnswer(const std::string& str) {
        std::string result = str;
        result.erase(std::remove_if(result.begin(), result.end(), ::isspace), result.end());
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }

    bool checkArithmeticAnswer(const ArithmeticQuestion& q, const std::string& userAnswerStr) {
        try {
            double userAnswer = std::stod(userAnswerStr);
            return std::abs(userAnswer - q.correctAnswer) < 0.001;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool checkAlgebraAnswer(const AlgebraQuestion& q, const std::string& userAnswerStr) {
        try {
            double userAnswer = std::stod(userAnswerStr);
            return std::abs(userAnswer - q.correctXValue) < 0.001;
        } catch (const std::exception&) {
            return false;
        }
    }

    bool checkFormulaAnswer(const FormulaQuestion& q, const std::string& userAnswerStr) {
        return normalizeAnswer(userAnswerStr) == normalizeAnswer(q.answerPattern);
    }

    std::string getCategoryNameForStats(StudyCategory category, const ArithmeticQuestion* arithQ = nullptr) {
        switch (category) {
            case STUDY_ARITHMETIC:
                if (arithQ) {
                    switch(arithQ->operation) {
                        case STUDY_ADD: return "Arithmetic-Add";
                        case STUDY_SUBTRACT: return "Arithmetic-Subtract";
                        case STUDY_MULTIPLY: return "Arithmetic-Multiply";
                        case STUDY_DIVIDE: return "Arithmetic-Divide";
                    }
                }
                return "Arithmetic-General";
            case STUDY_ALGEBRA: return "Algebra-Equations";
            case STUDY_FORMULAS: return "Formulas-Recall";
            default: return "Other";
        }
    }

    void generateNewQuestion(QuizSession& session) {
        session.userInput = "";
        session.inputLength = 0;
        session.suggestions.clear();

        switch (session.currentCategory) {
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

namespace UIRenderer {
    const int MAX_INPUT_CHARS_SHORT = 20;
    const int MAX_INPUT_CHARS_LONG = 60;

    bool DrawQuizButton(Rectangle buttonRect, const char* buttonText, Color baseColor, Color hoverColor, Color textColor) {
        bool isClicked = false;
        Vector2 mousePos = GetMousePosition();
        Color currentButtonColor = baseColor;

        if (CheckCollisionPointRec(mousePos, buttonRect)) {
            currentButtonColor = hoverColor;
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                isClicked = true;
            }
        }
        DrawRectangleRec(buttonRect, currentButtonColor);
        DrawRectangleLinesEx(buttonRect, 2, Fade(textColor,0.5f));
        
        int fontSize = 20;
        int textWidth = MeasureText(buttonText, fontSize);
        while (textWidth > buttonRect.width - 20 && fontSize > 10) {
            fontSize -= 2;
            textWidth = MeasureText(buttonText, fontSize);
        }

        DrawText(buttonText, buttonRect.x + (buttonRect.width - textWidth) / 2, buttonRect.y + (buttonRect.height - fontSize) / 2, fontSize, textColor);
        return isClicked;
    }
    
    Rectangle CalculateButtonRect(const char* text, float centerX, float y, float height, int fontSize, float minWidth = 150.0f, float paddingX = 30.0f) {
        int textWidth = MeasureText(text, fontSize);
        float buttonWidth = (textWidth + paddingX > minWidth) ? (textWidth + paddingX) : minWidth;
        return {centerX - buttonWidth / 2, y, buttonWidth, height};
    }

    void HandleTextInput(std::string& inputText, int& currentLength, int maxChars) {
        int keyCode = GetCharPressed();
        while (keyCode > 0) {
            if ((keyCode >= 32) && (keyCode <= 125) && (currentLength < maxChars)) {
                inputText += (char)keyCode;
                currentLength++;
            }
            keyCode = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (currentLength > 0) {
                inputText.pop_back();
                currentLength--;
            }
        }
    }
    
    void DrawInputBox(Rectangle boxRect, const std::string& currentText, bool isActive) {
        DrawRectangleRec(boxRect, Fade(LIGHTGRAY, 0.8f));
        int fontSize = 20;
        if (isActive) {
            DrawRectangleLinesEx(boxRect, 2, GOLD_ACCENT);
            if (((int)(GetTime() * 2.5f) % 2) == 0) {
                DrawText("_", boxRect.x + 8 + MeasureText(currentText.c_str(), fontSize), boxRect.y + (boxRect.height - fontSize)/2 + 2, fontSize, VERY_DARK_TEXT);
            }
        } else {
            DrawRectangleLinesEx(boxRect, 1, DARKGRAY);
        }
        DrawText(currentText.c_str(), boxRect.x + 10, boxRect.y + (boxRect.height - fontSize)/2, fontSize, VERY_DARK_TEXT);
    }
    
    void ShowAnswerFeedback(const string& message, Color messageColor) {
        int fontSize = 30;
        int feedbackWidth = MeasureText(message.c_str(), fontSize);
        DrawText(message.c_str(), (SCREEN_WIDTH - feedbackWidth) / 2, SCREEN_HEIGHT / 2 + 80, fontSize, messageColor);
    }

    void DrawMainMenuScreen(StudyScreen& currentScreen, StudyCategory& selectedCategory) {
        const char* title = "Math Challenge";
        const char* subtitle = "Choose a topic to practice your math skills!";
        int titleFontSize = 36;
        int subtitleFontSize = 20;

        DrawText(title, SCREEN_WIDTH / 2 - MeasureText(title, titleFontSize) / 2, 60, titleFontSize, GOLD_ACCENT);
        DrawText(subtitle, SCREEN_WIDTH / 2 - MeasureText(subtitle, subtitleFontSize) / 2, 130, subtitleFontSize, LIGHT_LAVENDER_TEXT);
        
        DrawText("Enter M to return to main menu", 160, 500, titleFontSize, GOLD_ACCENT);

        // if (IsKeyDown(KEY_M)){
        //     mcurrentScreen = HOME;
        // }


        float buttonWidth = 300;
        float buttonHeight = 55;
        float buttonX = (float)SCREEN_WIDTH/2 - buttonWidth/2;

        if (DrawQuizButton({buttonX, 200, buttonWidth, buttonHeight}, "Arithmetic Practice", MEDIUM_DARK_BUTTON, LIGHT_DARK_BUTTON_HOVER, LIGHT_LAVENDER_TEXT)) {
            selectedCategory = STUDY_ARITHMETIC;
            currentScreen = STUDY_QUIZ_TYPE_SELECTION;
        }
        if (DrawQuizButton({buttonX, 275, buttonWidth, buttonHeight}, "Algebra Practice", MEDIUM_DARK_BUTTON, LIGHT_DARK_BUTTON_HOVER, LIGHT_LAVENDER_TEXT)) {
            selectedCategory = STUDY_ALGEBRA;
            currentScreen = STUDY_QUIZ_TYPE_SELECTION;
        }
        if (DrawQuizButton({buttonX, 350, buttonWidth, buttonHeight}, "Formula Practice", MEDIUM_DARK_BUTTON, LIGHT_DARK_BUTTON_HOVER, LIGHT_LAVENDER_TEXT)) {
            selectedCategory = STUDY_FORMULAS;
            currentScreen = STUDY_QUIZ_TYPE_SELECTION;
        }
        DrawText("v1.0 - SimpleFun Edition", 10, SCREEN_HEIGHT - 20, 10, Fade(LIGHT_LAVENDER_TEXT, 0.5f));
    }

    void DrawQuizModeScreen(StudyScreen& currentScreen, QuizSession& quizSession, StudyCategory chosenCategory, StudyCategory& categoryForRetry) {
        std::string modeName;
        switch(chosenCategory) {
            case STUDY_ARITHMETIC: modeName = "Arithmetic"; break;
            case STUDY_ALGEBRA: modeName = "Algebra"; break;
            case STUDY_FORMULAS: modeName = "Formulas"; break;
            default: modeName = "Unknown Mode"; break;
        }
        const char* titleText = TextFormat("Selected Mode: %s", modeName.c_str());
        const char* infoText = "A 60-second timed quiz. Get ready!";
        int titleFontSize = 28;
        int infoFontSize = 20;

        DrawText(titleText, SCREEN_WIDTH / 2 - MeasureText(titleText, titleFontSize) / 2, 180, titleFontSize, GOLD_ACCENT);
        DrawText(infoText, SCREEN_WIDTH / 2 - MeasureText(infoText, infoFontSize) / 2, 240, infoFontSize, LIGHT_LAVENDER_TEXT);
        
        Rectangle startButtonRect = CalculateButtonRect("START QUIZ", (float)SCREEN_WIDTH/2, 300, 50, 20, 220);
        if (DrawQuizButton(startButtonRect, "START QUIZ", LIME, DARKGREEN, VERY_DARK_TEXT)) {
            quizSession = QuizSession();
            quizSession.currentCategory = chosenCategory;
            QuestionGenerator::generateNewQuestion(quizSession);
            PerformanceTracker::clearPerformanceData();
            currentScreen = STUDY_QUIZ_IN_PROGRESS;
            categoryForRetry = chosenCategory;
        }
        Rectangle backButtonRect = CalculateButtonRect("Back to Menu", (float)SCREEN_WIDTH/2, 370, 50, 20, 220);
        if (DrawQuizButton(backButtonRect, "Back to Menu", DARKGRAY, GRAY, LIGHT_LAVENDER_TEXT)) {
            currentScreen = STUDY_MAIN_MENU;
        }
    }

    void DrawQuizGameplayScreen(StudyScreen& currentScreen, QuizSession& session) {
        int textFontSize = 22;
        int questionFontSize = 26;

        DrawText(TextFormat("Time Left: %02.0fs", session.timeLeft), SCREEN_WIDTH - 200, 30, textFontSize, RED);
        DrawText(TextFormat("Score: %d", session.score), 30, 30, textFontSize, LIME);

        DrawText("Solve this problem:", 50, 100, textFontSize, LIGHT_LAVENDER_TEXT);
        DrawText(session.currentQuestionText.c_str(), 50, 140, questionFontSize, GOLD_ACCENT);

        int maxInputChars = (session.currentCategory == STUDY_FORMULAS) ? MAX_INPUT_CHARS_LONG : MAX_INPUT_CHARS_SHORT;
        float inputWidth = (session.currentCategory == STUDY_FORMULAS) ? 450.0f : 300.0f;
        session.inputBoxRect = {(float)SCREEN_WIDTH/2 - inputWidth/2, 220, inputWidth, 45};
        
        HandleTextInput(session.userInput, session.inputLength, maxInputChars);
        DrawInputBox(session.inputBoxRect, session.userInput, true);

        if (session.currentCategory == STUDY_FORMULAS && session.inputLength > 0) {
            if(session.suggestions.empty() || IsKeyPressed(KEY_BACKSPACE) || GetCharPressed() != 0) {
                 session.suggestions = FormulaSuggester::findMatchingPatterns(session.userInput);
            }
            int suggestionY = session.inputBoxRect.y + session.inputBoxRect.height + 10;
            for (size_t i = 0; i < session.suggestions.size() && i < 4; ++i) {
                Rectangle suggestionBox = {session.inputBoxRect.x, (float)suggestionY + i * 30, session.inputBoxRect.width, 25.0f};
                bool hovered = CheckCollisionPointRec(GetMousePosition(), suggestionBox);
                DrawRectangleRec(suggestionBox, hovered ? LIGHT_DARK_BUTTON_HOVER : MEDIUM_DARK_BUTTON);
                DrawText(session.suggestions[i].c_str(), suggestionBox.x + 10, suggestionBox.y + 4, 18, LIGHT_LAVENDER_TEXT);
                if (hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                    session.userInput = session.suggestions[i];
                    session.inputLength = session.userInput.length();
                    session.suggestions.clear();
                }
            }
        }

        Rectangle submitButtonRect = CalculateButtonRect("Submit Answer", (float)SCREEN_WIDTH/2, 320, 45, 20, 180);
        bool submittedViaButton = DrawQuizButton(submitButtonRect, "Submit Answer", BLUE, SKYBLUE, WHITE);
        bool submittedViaEnter = IsKeyPressed(KEY_ENTER) && session.inputLength > 0;

        if (submittedViaButton || submittedViaEnter) {
            if (session.inputLength > 0) {
                bool isCorrect = false;
                std::string categoryKey;

                switch (session.currentCategory) {
                    case STUDY_ARITHMETIC:
                        isCorrect = QuestionGenerator::checkArithmeticAnswer(session.currentArithmeticQ, session.userInput);
                        categoryKey = QuestionGenerator::getCategoryNameForStats(session.currentCategory, &session.currentArithmeticQ);
                        break;
                    case STUDY_ALGEBRA:
                        isCorrect = QuestionGenerator::checkAlgebraAnswer(session.currentAlgebraQ, session.userInput);
                        categoryKey = QuestionGenerator::getCategoryNameForStats(session.currentCategory);
                        break;
                    case STUDY_FORMULAS:
                        isCorrect = QuestionGenerator::checkFormulaAnswer(session.currentFormulaQ, session.userInput);
                        categoryKey = QuestionGenerator::getCategoryNameForStats(session.currentCategory);
                        break;
                    case STUDY_NONE_SELECTED: break;
                }

                PerformanceTracker::recordAttempt(categoryKey, isCorrect);
                session.questionsAttempted++;
                if (isCorrect) session.score++;
                
                session.wasLastAnswerCorrect = isCorrect;
                session.showFeedback = true;
                session.feedbackTimer = 1.2f;

                QuestionGenerator::generateNewQuestion(session);
            }
        }

        if (session.showFeedback) {
            ShowAnswerFeedback(session.wasLastAnswerCorrect ? "CORRECT!" : "TRY AGAIN!", session.wasLastAnswerCorrect ? LIME : ORANGE);
            session.feedbackTimer -= GetFrameTime();
            if (session.feedbackTimer <= 0) session.showFeedback = false;
        }
        
        Rectangle endButtonRect = CalculateButtonRect("End Quiz Now", SCREEN_WIDTH - 120, SCREEN_HEIGHT - 55, 45, 20, 180);
        if (DrawQuizButton(endButtonRect, "End Quiz Now", MAROON, RED, WHITE)) {
            currentScreen = STUDY_QUIZ_SUMMARY;
        }

        session.timeLeft -= GetFrameTime();
        if (session.timeLeft <= 0) {
            session.timeLeft = 0;
            currentScreen = STUDY_QUIZ_SUMMARY;
        }
    }

    void DrawSummaryScreen(StudyScreen& currentScreen, StudyCategory& categoryForRetryOption, const QuizSession& session, const StudyCategory lastPlayedCategory) {
        const char* title = "Quiz Summary";
        int titleFontSize = 30;
        int detailFontSize = 22;
        int categoryStatFontSize = 20;

        DrawText(title, SCREEN_WIDTH / 2 - MeasureText(title, titleFontSize) / 2, 50, titleFontSize, GOLD_ACCENT);

        DrawText(TextFormat("Total Questions Attempted: %d", session.questionsAttempted), 100, 120, detailFontSize, LIGHT_LAVENDER_TEXT);
        DrawText(TextFormat("Correct Answers: %d", session.score), 100, 155, detailFontSize, LIME);
        float accuracy = (session.questionsAttempted > 0) ? ((float)session.score / session.questionsAttempted) * 100.0f : 0.0f;
        DrawText(TextFormat("Accuracy: %.1f%%", accuracy), 100, 190, detailFontSize, (accuracy >= 60.0f ? SKYBLUE : ORANGE));

        DrawText("Performance by Sub-Category:", 100, 250, 24, GOLD_ACCENT);
        int yPos = 290;
        if (PerformanceTracker::categoryPerformance.empty()) {
             DrawText("No specific category data recorded.", 120, yPos, categoryStatFontSize, Fade(LIGHT_LAVENDER_TEXT,0.7f));
             yPos += 30;
        }
        for (const auto& record : PerformanceTracker::categoryPerformance) {
            DrawText(TextFormat("  %s: %d / %d correct", record.first.c_str(), record.second.first, record.second.second), 120, yPos, categoryStatFontSize, LIGHT_LAVENDER_TEXT);
            yPos += 30;
        }
        
        float buttonY = (float)SCREEN_HEIGHT - 100;
        float buttonHeight = 50;

        Rectangle retryButtonRect = CalculateButtonRect("Retry Same Mode", SCREEN_WIDTH * 0.3f, buttonY, buttonHeight, 20, 220);
        if (DrawQuizButton(retryButtonRect, "Retry Same Mode", GREEN, LIME, VERY_DARK_TEXT)) {
            categoryForRetryOption = lastPlayedCategory;
            currentScreen = STUDY_QUIZ_TYPE_SELECTION;
        }
        Rectangle menuButtonRect = CalculateButtonRect("Back to Main Menu", SCREEN_WIDTH * 0.7f, buttonY, buttonHeight, 20, 220);
        if (DrawQuizButton(menuButtonRect, "Back to Main Menu", MEDIUM_DARK_BUTTON, LIGHT_DARK_BUTTON_HOVER, LIGHT_LAVENDER_TEXT)) {
            currentScreen = STUDY_MAIN_MENU;
        }
    }
}

int main(void) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Math Challenge - Fun Learning Game");
    SetTargetFPS(60);
    SetRandomSeed(time(0));

    QuestionGenerator::loadFormulasToList();

    StudyScreen activeScreen = STUDY_MAIN_MENU;
    StudyCategory chosenQuizCategory = STUDY_NONE_SELECTED;
    StudyCategory lastCategoryPlayed = STUDY_NONE_SELECTED;
    QuizSession activeQuizSession;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(DARK_BLUE_BACKGROUND);

        switch (activeScreen) {
            case STUDY_MAIN_MENU:
                UIRenderer::DrawMainMenuScreen(activeScreen, chosenQuizCategory);
                break;
            case STUDY_QUIZ_TYPE_SELECTION:
                UIRenderer::DrawQuizModeScreen(activeScreen, activeQuizSession, chosenQuizCategory, lastCategoryPlayed);
                break;
            case STUDY_QUIZ_IN_PROGRESS:
                UIRenderer::DrawQuizGameplayScreen(activeScreen, activeQuizSession);
                break;
            case STUDY_QUIZ_SUMMARY:
                UIRenderer::DrawSummaryScreen(activeScreen, chosenQuizCategory, activeQuizSession, lastCategoryPlayed);
                break;
        }
        
        EndDrawing();
    }

    FormulaSuggester::clearSuggesterData();
    CloseWindow();
    return 0;
}