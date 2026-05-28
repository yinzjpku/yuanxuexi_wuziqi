#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include "GameModel.h"
#include <vector>
#include "QuestionDialog.h"
#include <QList>
enum GameMode { MENU_MODE, PVP_MODE};
enum Difficulty { EASY, MEDIUM, HARD };

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private:
    void setupMenuMode();
    void setupGameMode();
    void showMenu();
    void startGame(GameMode mode);

    GameModel *game;
    int clickPosRow, clickPosCol;
    GameMode currentMode;
    Difficulty difficulty;

    QWidget *menuWidget;
    QWidget *gameWidget;
    QComboBox *difficultyCombo;
    QComboBox *topicCombo;
    QWidget *skillPanel;
    QWidget *skillPanel2;
    QPushButton *skipBtn;
    QPushButton *blockBtn;
    QPushButton *hintBtn;
    QPushButton *energyBtn;
    QPushButton *p1EnergyLabel;
    QPushButton *p2EnergyLabel;
    int boardOffsetX;
    int pendingRow;
    int pendingCol;
    int lastMoveRow;
    int lastMoveCol;
    std::vector<std::pair<int, int>> wrongPositions;
    std::vector<std::pair<int, int>> m_winPieces;
    QList<Question> m_questionBank;
    QList<Question> m_filteredQuestionBank;
    void loadQuestionsFromJson();
    Question getRandomQuestion();
private slots:
    void chessOneByPerson();
    void onStartPVPClicked();
    void onBackToMenuClicked();
    void filterQuestions();
};

#endif // MAINWINDOW_H
