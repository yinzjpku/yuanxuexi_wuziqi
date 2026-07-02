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
#include "NetworkManager.h"
#include <QList>
#include <QJsonArray>
enum GameMode { MENU_MODE, PVP_MODE, NETWORK_MODE };
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
    void showNetworkDialog();
    void startNetworkGame(bool localIsBlack);
    bool isLocalTurn() const;
    void sendGameState(const QString &reason);
    void applyGameState(const QJsonObject &state);
    QJsonObject buildGameState(const QString &reason) const;
    void recordWrongQuestion(bool isTimeout);
    QString exportReviewFile(const QString &resultText);
    void showWrongQuestionReview();
    void finishGame(const QString &title, const QString &message, const QString &resultText);
    QJsonObject questionToJson(const Question &question) const;
    Question questionFromJson(const QJsonObject &object) const;

    GameModel *game;
    int clickPosRow, clickPosCol;
    GameMode currentMode;
    Difficulty difficulty;
    bool m_isEliminatingCross = false;
    bool m_isSkippingQuestion = false; // ⚠️【新增】是否处于跳过答题的预备状态
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
    int pendingRow; // 记录玩家当前点击、等待答题验证的行
    int pendingCol; // 记录玩家当前点击、等待答题验证的列
    std::vector<std::pair<int, int>> wrongPositions;
    QList<Question> m_questionBank;
    QList<Question> m_filteredQuestionBank;
    void loadQuestionsFromJson();
    Question getRandomQuestion();
    void handleAnswerResult(bool isCorrect, bool isTimeout);
    // ⚠️新增：全局时限系统
    QTimer *m_globalTimer;
    int m_p1TimeLeft; // 白方剩余秒数
    int m_p2TimeLeft; // 黑方剩余秒数
    void handleGlobalTimeout(); // 处理全局超时
    bool m_isAnswering;
    QuestionDialog *m_currentDlg = nullptr; // 记录当前弹出的答题窗口
    NetworkManager *m_network;
    bool m_isNetworkGame = false;
    bool m_localIsBlack = true;
    bool m_applyingRemoteState = false;
    Question m_currentQuestion;
    bool m_hasCurrentQuestion = false;
    QJsonArray m_wrongQuestionRecords;
    bool m_reviewExported = false;
private slots:
    void chessOneByPerson();
    void onStartPVPClicked();
    void onNetworkBattleClicked();
    void onBackToMenuClicked();
    void filterQuestions();
    void onGlobalTimerTick();
    void onNetworkConnected();
    void onNetworkDisconnected();
    void onNetworkMessageReceived(const QJsonObject &message);
};

#endif // MAINWINDOW_H
