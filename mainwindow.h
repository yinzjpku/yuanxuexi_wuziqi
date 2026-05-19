#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include "GameModel.h"

enum GameMode { MENU_MODE, PVP_MODE, PVE_MODE };
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
    GameType game_type;
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
    int boardOffsetX;

private slots:
    void chessOneByPerson();
    void chessOneByAI();
    void onStartPVPClicked();
    void onStartPVEClicked();
    void onBackToMenuClicked();
};

#endif // MAINWINDOW_H
