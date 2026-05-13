#include <QPainter>
#include <QTimer>
#include <QMouseEvent>
#include <QMessageBox>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <math.h>
#include "mainwindow.h"

const int kBoardMargin = 30;
const int kRadius = 15;
const int kMarkSize = 6;
const int kBlockSize = 40;
const int kPosDelta = 20;
const int kAIDelay = 700;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentMode(MENU_MODE), difficulty(MEDIUM)
{
    setFixedSize(400, 380);
    setWindowTitle("五子棋编程对战");
    setMouseTracking(true);

    game = nullptr;
    gameWidget = nullptr;
    skillPanel = nullptr;
    skipBtn = nullptr;
    blockBtn = nullptr;
    hintBtn = nullptr;
    energyBtn = nullptr;
    difficultyCombo = nullptr;
    topicCombo = nullptr;
    clickPosRow = -1;
    clickPosCol = -1;
    currentMode = MENU_MODE;

    menuWidget = new QWidget(this);
    menuWidget->setGeometry(0, 0, 400, 380);
    setupMenuMode();
}

MainWindow::~MainWindow()
{
    if (game)
    {
        delete game;
        game = nullptr;
    }
    if (skillPanel)
    {
        delete skillPanel;
        skillPanel = nullptr;
    }
    if (menuWidget)
    {
        delete menuWidget;
        menuWidget = nullptr;
    }
}

void MainWindow::setupMenuMode()
{
    menuWidget->setStyleSheet("background-color: rgb(220, 220, 220);");
    QVBoxLayout *mainLayout = new QVBoxLayout(menuWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(80, 30, 80, 30);

    QLabel *titleLabel = new QLabel("五子棋编程对战", menuWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(22);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    QLabel *difficultyLabel = new QLabel("难度选择:", menuWidget);
    difficultyLabel->setStyleSheet("font-size: 14px;");
    mainLayout->addWidget(difficultyLabel);

    difficultyCombo = new QComboBox(menuWidget);
    difficultyCombo->addItem("简单", EASY);
    difficultyCombo->addItem("中等", MEDIUM);
    difficultyCombo->addItem("困难", HARD);
    difficultyCombo->setStyleSheet("font-size: 14px; padding: 5px;");
    difficultyCombo->setCurrentIndex(1);
    mainLayout->addWidget(difficultyCombo);

    QLabel *topicLabel = new QLabel("知识点选择:", menuWidget);
    topicLabel->setStyleSheet("font-size: 14px;");
    mainLayout->addWidget(topicLabel);

    topicCombo = new QComboBox(menuWidget);
    topicCombo->addItem("全部");
    topicCombo->addItem("指针");
    topicCombo->addItem("类与对象");
    topicCombo->addItem("STL");
    topicCombo->addItem("算法");
    topicCombo->addItem("数据结构");
    topicCombo->setStyleSheet("font-size: 14px; padding: 5px;");
    mainLayout->addWidget(topicCombo);

    mainLayout->addSpacing(15);

    QPushButton *pvpBtn = new QPushButton("双人对战\n(两名玩家轮流落子)", menuWidget);
    pvpBtn->setStyleSheet("QPushButton { font-size: 14px; padding: 10px; background-color: #4CAF50; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #45a049; }");
    connect(pvpBtn, SIGNAL(clicked()), this, SLOT(onStartPVPClicked()));
    mainLayout->addWidget(pvpBtn);

    QPushButton *pveBtn = new QPushButton("人机对战\n(玩家vs电脑)", menuWidget);
    pveBtn->setStyleSheet("QPushButton { font-size: 14px; padding: 10px; background-color: #2196F3; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #0b7dda; }");
    connect(pveBtn, SIGNAL(clicked()), this, SLOT(onStartPVEClicked()));
    mainLayout->addWidget(pveBtn);

    QPushButton *exitBtn = new QPushButton("退出", menuWidget);
    exitBtn->setStyleSheet("QPushButton { font-size: 14px; padding: 10px; background-color: #f44336; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #da190b; }");
    connect(exitBtn, SIGNAL(clicked()), this, SLOT(close()));
    mainLayout->addWidget(exitBtn);

    mainLayout->addStretch();
    menuWidget->show();
}

void MainWindow::startGame(GameMode mode)
{
    currentMode = mode;
    difficulty = static_cast<Difficulty>(difficultyCombo->currentData().toInt());

    menuWidget->hide();

    if (!game)
        game = new GameModel;

    if (mode == PVP_MODE)
    {
        game_type = PERSON;
    }
    else
    {
        game_type = BOT;
    }

    game->gameStatus = PLAYING;
    game->startGame(game_type);

    int boardSize = kBoardMargin * 2 + kBlockSize * kBoardSizeNum;
    int skillPanelWidth = 150;
    setFixedSize(boardSize + skillPanelWidth, boardSize + 60);

    if (skillPanel)
    {
        delete skillPanel;
        skillPanel = nullptr;
    }
    skillPanel = new QWidget(this);
    skillPanel->setGeometry(boardSize, 50, skillPanelWidth, boardSize);
    skillPanel->setStyleSheet("background-color: rgb(240, 240, 240);");

    QVBoxLayout *skillLayout = new QVBoxLayout(skillPanel);
    skillLayout->setContentsMargins(10, 20, 10, 20);
    skillLayout->setSpacing(15);

    QLabel *skillTitle = new QLabel("技能面板", skillPanel);
    skillTitle->setAlignment(Qt::AlignCenter);
    skillTitle->setStyleSheet("font-size: 16px; font-weight: bold;");
    skillLayout->addWidget(skillTitle);

    skipBtn = new QPushButton("跳过答题\n(高能耗)", skillPanel);
    skipBtn->setStyleSheet("QPushButton { font-size: 14px; padding: 10px; background-color: #9C27B0; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #7B1FA2; } QPushButton:disabled { background-color: #BDBDBD; }");
    skipBtn->setToolTip("消耗能量跳过答题，直接落子");
    skillLayout->addWidget(skipBtn);

    blockBtn = new QPushButton("封锁对手\n(中能耗)", skillPanel);
    blockBtn->setStyleSheet("QPushButton { font-size: 14px; padding: 10px; background-color: #F44336; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #D32F2F; } QPushButton:disabled { background-color: #BDBDBD; }");
    blockBtn->setToolTip("消耗能量，禁止对手在指定区域落子");
    skillLayout->addWidget(blockBtn);

    hintBtn = new QPushButton("提示答案\n(低能耗)", skillPanel);
    hintBtn->setStyleSheet("QPushButton { font-size: 14px; padding: 10px; background-color: #FF9800; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #F57C00; } QPushButton:disabled { background-color: #BDBDBD; }");
    hintBtn->setToolTip("消耗能量，排除两个错误选项");
    skillLayout->addWidget(hintBtn);

    energyBtn = new QPushButton("能量: 0", skillPanel);
    energyBtn->setStyleSheet("QPushButton { font-size: 16px; padding: 10px; background-color: #2196F3; color: white; border: none; border-radius: 5px; }");
    energyBtn->setToolTip("当前能量值，连续答对可获得额外能量");
    energyBtn->setEnabled(false);
    skillLayout->addWidget(energyBtn);

    QLabel *extraLabel = new QLabel("更多技能", skillPanel);
    extraLabel->setAlignment(Qt::AlignCenter);
    extraLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #666; margin-top: 10px;");
    skillLayout->addWidget(extraLabel);

    QPushButton *skill4 = new QPushButton("", skillPanel);
    skill4->setStyleSheet("QPushButton { font-size: 14px; padding: 8px; background-color: #795548; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #5D4037; }");
    skillLayout->addWidget(skill4);

    QPushButton *skill5 = new QPushButton("", skillPanel);
    skill5->setStyleSheet("QPushButton { font-size: 14px; padding: 8px; background-color: #00BCD4; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #0097A7; }");
    skillLayout->addWidget(skill5);

    QPushButton *skill6 = new QPushButton("", skillPanel);
    skill6->setStyleSheet("QPushButton { font-size: 14px; padding: 8px; background-color: #8BC34A; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #689F38; }");
    skillLayout->addWidget(skill6);

    skillLayout->addStretch();

    QPushButton *backBtn = new QPushButton("返回主菜单", skillPanel);
    backBtn->setStyleSheet("QPushButton { font-size: 14px; padding: 10px; background-color: #607D8B; color: white; border: none; border-radius: 5px; } QPushButton:hover { background-color: #546E7A; }");
    backBtn->setToolTip("返回游戏主菜单");
    connect(backBtn, SIGNAL(clicked()), this, SLOT(onBackToMenuClicked()));
    skillLayout->addWidget(backBtn);

    skillPanel->show();

    menuBar()->clear();
    QMenu *gameMenu = menuBar()->addMenu("游戏");
    QAction *backAction = new QAction("返回主菜单", this);
    connect(backAction, SIGNAL(triggered()), this, SLOT(onBackToMenuClicked()));
    gameMenu->addAction(backAction);

    QAction *restartAction = new QAction("重新开始", this);
    if (mode == PVP_MODE)
        connect(restartAction, SIGNAL(triggered()), this, SLOT(onStartPVPClicked()));
    else
        connect(restartAction, SIGNAL(triggered()), this, SLOT(onStartPVEClicked()));
    gameMenu->addAction(restartAction);

    QAction *difficultyAction = new QAction(QString("当前难度: %1").arg(
        difficultyCombo->currentText()), this);
    difficultyAction->setEnabled(false);
    gameMenu->addAction(difficultyAction);

    QAction *topicAction = new QAction(QString("当前主题: %1").arg(
        topicCombo->currentText()), this);
    topicAction->setEnabled(false);
    gameMenu->addAction(topicAction);

    update();
}

void MainWindow::onStartPVPClicked()
{
    startGame(PVP_MODE);
}

void MainWindow::onStartPVEClicked()
{
    startGame(PVE_MODE);
}

void MainWindow::onBackToMenuClicked()
{
    if (game)
    {
        delete game;
        game = nullptr;
    }
    if (skillPanel)
    {
        delete skillPanel;
        skillPanel = nullptr;
    }
    currentMode = MENU_MODE;
    setFixedSize(400, 380);
    menuBar()->clear();
    menuWidget->show();
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (currentMode == MENU_MODE)
    {
        painter.fillRect(rect(), QColor(220, 220, 220));
        return;
    }

    painter.fillRect(rect(), QColor(210, 180, 140));

    int boardWidth = kBoardMargin * 2 + kBlockSize * kBoardSizeNum;
    int boardHeight = boardWidth;
    for (int i = 0; i < kBoardSizeNum + 1; i++)
    {
        painter.drawLine(kBoardMargin + kBlockSize * i, kBoardMargin,
                         kBoardMargin + kBlockSize * i, boardHeight - kBoardMargin);
        painter.drawLine(kBoardMargin, kBoardMargin + kBlockSize * i,
                         boardWidth - kBoardMargin, kBoardMargin + kBlockSize * i);
    }

    QBrush brush;
    brush.setStyle(Qt::SolidPattern);
    if (clickPosRow > 0 && clickPosRow < kBoardSizeNum &&
        clickPosCol > 0 && clickPosCol < kBoardSizeNum &&
        game && game->gameMapVec[clickPosRow][clickPosCol] == 0)
    {
        if (game->playerFlag)
            brush.setColor(Qt::white);
        else
            brush.setColor(Qt::black);
        painter.setBrush(brush);
        painter.drawRect(kBoardMargin + kBlockSize * clickPosCol - kMarkSize / 2,
                         kBoardMargin + kBlockSize * clickPosRow - kMarkSize / 2,
                         kMarkSize, kMarkSize);
    }

    if (game)
    {
        for (int i = 0; i < kBoardSizeNum; i++)
            for (int j = 0; j < kBoardSizeNum; j++)
            {
                if (game->gameMapVec[i][j] == 1)
                {
                    brush.setColor(Qt::white);
                    painter.setBrush(brush);
                    painter.drawEllipse(kBoardMargin + kBlockSize * j - kRadius,
                                        kBoardMargin + kBlockSize * i - kRadius,
                                        kRadius * 2, kRadius * 2);
                }
                else if (game->gameMapVec[i][j] == -1)
                {
                    brush.setColor(Qt::black);
                    painter.setBrush(brush);
                    painter.drawEllipse(kBoardMargin + kBlockSize * j - kRadius,
                                        kBoardMargin + kBlockSize * i - kRadius,
                                        kRadius * 2, kRadius * 2);
                }
            }

        if (clickPosRow > 0 && clickPosRow < kBoardSizeNum &&
            clickPosCol > 0 && clickPosCol < kBoardSizeNum &&
            (game->gameMapVec[clickPosRow][clickPosCol] == 1 ||
             game->gameMapVec[clickPosRow][clickPosCol] == -1))
        {
            if (game->isWin(clickPosRow, clickPosCol) && game->gameStatus == PLAYING)
            {
                game->gameStatus = WIN;
                QString str;
                if (game->gameMapVec[clickPosRow][clickPosCol] == 1)
                    str = "白方";
                else
                    str = "黑方";
                QMessageBox::StandardButton btnValue = QMessageBox::information(this, "胜利", str + " 获胜!");

                if (btnValue == QMessageBox::Ok)
                {
                    game->startGame(game_type);
                    game->gameStatus = PLAYING;
                }
            }
        }

        if (game->isDeadGame())
        {
            QMessageBox::StandardButton btnValue = QMessageBox::information(this, "平局", "和棋!");
            if (btnValue == QMessageBox::Ok)
            {
                game->startGame(game_type);
                game->gameStatus = PLAYING;
            }
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!game || currentMode == MENU_MODE) return;

    int x = event->position().x();
    int y = event->position().y();

    int boardWidth = kBoardMargin * 2 + kBlockSize * kBoardSizeNum;
    int boardHeight = boardWidth;
    int validX = boardWidth - kBoardMargin;
    int validY = boardHeight;

    clickPosRow = -1;
    clickPosCol = -1;

    if (x >= kBoardMargin + kBlockSize / 2 &&
            x < validX &&
            y >= kBoardMargin + kBlockSize / 2 &&
            y < validY)
    {
        int col = x / kBlockSize;
        int row = y / kBlockSize;

        int leftTopPosX = kBoardMargin + kBlockSize * col;
        int leftTopPosY = kBoardMargin + kBlockSize * row;

        int len = sqrt((x - leftTopPosX) * (x - leftTopPosX) + (y - leftTopPosY) * (y - leftTopPosY));
        if (len < kPosDelta) { clickPosRow = row; clickPosCol = col; }
        len = sqrt((x - leftTopPosX - kBlockSize) * (x - leftTopPosX - kBlockSize) + (y - leftTopPosY) * (y - leftTopPosY));
        if (len < kPosDelta) { clickPosRow = row; clickPosCol = col + 1; }
        len = sqrt((x - leftTopPosX) * (x - leftTopPosX) + (y - leftTopPosY - kBlockSize) * (y - leftTopPosY - kBlockSize));
        if (len < kPosDelta) { clickPosRow = row + 1; clickPosCol = col; }
        len = sqrt((x - leftTopPosX - kBlockSize) * (x - leftTopPosX - kBlockSize) + (y - leftTopPosY - kBlockSize) * (y - leftTopPosY - kBlockSize));
        if (len < kPosDelta) { clickPosRow = row + 1; clickPosCol = col + 1; }
    }

    update();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (!game || currentMode == MENU_MODE) return;

    if (!(game_type == BOT && !game->playerFlag))
    {
        chessOneByPerson();
        if (game->gameType == BOT && !game->playerFlag)
        {
            QTimer::singleShot(kAIDelay, this, SLOT(chessOneByAI()));
        }
    }
}

void MainWindow::chessOneByPerson()
{
    if (clickPosRow != -1 && clickPosCol != -1 && game &&
        clickPosRow >= 0 && clickPosRow < kBoardSizeNum &&
        clickPosCol >= 0 && clickPosCol < kBoardSizeNum &&
        game->gameMapVec[clickPosRow][clickPosCol] == 0)
    {
        game->actionByPerson(clickPosRow, clickPosCol);
        update();
    }
}

void MainWindow::chessOneByAI()
{
    if (game)
    {
        game->actionByAI(clickPosRow, clickPosCol);
        update();
    }
}
