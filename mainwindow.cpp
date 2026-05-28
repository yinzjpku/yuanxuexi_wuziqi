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
#include "QuestionDialog.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRandomGenerator>
const int kBoardMargin = 30;
const int kRadius = 15;
const int kMarkSize = 6;
const int kBlockSize = 40;
const int kPosDelta = 20;
const int kAIDelay = 700;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentMode(MENU_MODE), difficulty(MEDIUM)
{
    setFixedSize(440, 480);
    setWindowTitle("五子棋编程对战");
    setMouseTracking(true);

    game = nullptr;
    gameWidget = nullptr;
    skillPanel = nullptr;
    skillPanel2 = nullptr;
    boardOffsetX = 0;
    skipBtn = nullptr;
    blockBtn = nullptr;
    hintBtn = nullptr;
    energyBtn = nullptr;
    difficultyCombo = nullptr;
    topicCombo = nullptr;
    clickPosRow = -1;
    clickPosCol = -1;
    lastMoveRow = -1;
    lastMoveCol = -1;
    currentMode = MENU_MODE;

    menuWidget = new QWidget(this);
    menuWidget->setGeometry(0, 0, 440, 480);
    setupMenuMode();
    loadQuestionsFromJson();
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
    if (skillPanel2)
    {
        delete skillPanel2;
        skillPanel2 = nullptr;
    }
    if (menuWidget)
    {
        delete menuWidget;
        menuWidget = nullptr;
    }
}

void MainWindow::setupMenuMode()
{
    menuWidget->setStyleSheet("background-color: transparent;");
    QVBoxLayout *mainLayout = new QVBoxLayout(menuWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(60, 30, 60, 30);

    QLabel *titleLabel = new QLabel("五子棋编程对战", menuWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "color: white;"
        "font-size: 28px;"
        "font-weight: bold;"
        "padding: 5px;"
        "background: transparent;"
    );
    mainLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel("答题落子 · 策略对决", menuWidget);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet(
        "color: rgba(255,255,255,180);"
        "font-size: 14px;"
        "background: transparent;"
        "padding-bottom: 10px;"
    );
    mainLayout->addWidget(subtitleLabel);

    QWidget *cardWidget = new QWidget(menuWidget);
    cardWidget->setStyleSheet(
        "QWidget {"
        "  background-color: rgba(255,255,255,230);"
        "  border-radius: 12px;"
        "}"
    );
    QVBoxLayout *cardLayout = new QVBoxLayout(cardWidget);
    cardLayout->setSpacing(8);
    cardLayout->setContentsMargins(15, 15, 15, 15);

    QLabel *diffLabel = new QLabel("难度选择", cardWidget);
    diffLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #333; background: transparent;");
    cardLayout->addWidget(diffLabel);

    difficultyCombo = new QComboBox(cardWidget);
    difficultyCombo->addItem("简单", EASY);
    difficultyCombo->addItem("中等", MEDIUM);
    difficultyCombo->addItem("困难", HARD);
    difficultyCombo->setStyleSheet(
        "QComboBox {"
        "  font-size: 13px; padding: 6px 10px;"
        "  border: 1px solid #ccc; border-radius: 6px;"
        "  background: white; color: #333;"
        "}"
        "QComboBox:hover { border-color: #3F51B5; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
    );
    difficultyCombo->setCurrentIndex(1);
    cardLayout->addWidget(difficultyCombo);

    QLabel *topicLabel = new QLabel("知识点选择", cardWidget);
    topicLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #333; background: transparent;");
    cardLayout->addWidget(topicLabel);

    topicCombo = new QComboBox(cardWidget);
    topicCombo->addItem("全部");
    topicCombo->addItem("类与对象基础");
    topicCombo->addItem("继承与多态");
    topicCombo->addItem("运算符重载");
    topicCombo->addItem("STL");
    topicCombo->addItem("文件，模版，c++新特性");
    topicCombo->setStyleSheet(
        "QComboBox {"
        "  font-size: 13px; padding: 6px 10px;"
        "  border: 1px solid #ccc; border-radius: 6px;"
        "  background: white; color: #333;"
        "}"
        "QComboBox:hover { border-color: #3F51B5; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
    );
    cardLayout->addWidget(topicCombo);

    mainLayout->addWidget(cardWidget);

    connect(difficultyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::filterQuestions);
    connect(topicCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::filterQuestions);
    filterQuestions();
    mainLayout->addSpacing(8);

    QPushButton *pvpBtn = new QPushButton("开始对战", menuWidget);
    pvpBtn->setStyleSheet(
        "QPushButton {"
        "  font-size: 16px; font-weight: bold; padding: 12px;"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #4CAF50, stop:1 #388E3C);"
        "  color: white; border: none; border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #66BB6A, stop:1 #43A047);"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #388E3C, stop:1 #2E7D32);"
        "  padding-top: 14px; padding-bottom: 10px;"
        "}"
    );
    connect(pvpBtn, SIGNAL(clicked()), this, SLOT(onStartPVPClicked()));
    mainLayout->addWidget(pvpBtn);

    QPushButton *exitBtn = new QPushButton("退出游戏", menuWidget);
    exitBtn->setStyleSheet(
        "QPushButton {"
        "  font-size: 14px; padding: 10px;"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #EF5350, stop:1 #D32F2F);"
        "  color: white; border: none; border-radius: 8px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #E57373, stop:1 #E53935);"
        "}"
        "QPushButton:pressed {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #D32F2F, stop:1 #C62828);"
        "  padding-top: 12px; padding-bottom: 8px;"
        "}"
    );
    connect(exitBtn, SIGNAL(clicked()), this, SLOT(close()));
    mainLayout->addWidget(exitBtn);

    mainLayout->addStretch();
    menuWidget->show();
}

void MainWindow::startGame(GameMode mode)
{
    currentMode = mode;
    difficulty = static_cast<Difficulty>(difficultyCombo->currentData().toInt());

    if (menuWidget) {
        menuWidget->hide();
        menuWidget->lower(); // 将菜单置于底层，防止隐形遮罩拦截鼠标
    }

    if (!game)
        game = new GameModel;

    game->startGame();
    wrongPositions.clear();
    m_winPieces.clear();
    lastMoveRow = -1;
    lastMoveCol = -1;
    int boardSize = kBoardMargin * 2 + kBlockSize * kBoardSizeNum;
    int skillPanelWidth = 150;

    // 纯PVP模式：固定左右两侧均有技能面板
    boardOffsetX = skillPanelWidth;
    setFixedSize(boardSize + skillPanelWidth * 2, boardSize + 60);

    // ================== 1. 先安全清理旧面板 ==================
    if (skillPanel) {
        delete skillPanel;
        skillPanel = nullptr;
    }
    if (skillPanel2) {
        delete skillPanel2;
        skillPanel2 = nullptr;
    }

    // ================== 2. 定义 createPanel (必须在使用前定义) ==================
    auto createPanel = [this, boardSize, skillPanelWidth](int x, bool isPrimary) {
        QWidget *panel = new QWidget(this);
        panel->setGeometry(x, 40, skillPanelWidth, boardSize); // 40 为预留的顶部菜单栏高度
        panel->setStyleSheet("background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FAFAFA, stop:1 #F0F0F0); border: 1px solid #D0D0D0; border-radius: 4px;");

        // 使用垂直布局，让 6 个控件从上到下等间距紧凑排列
        QVBoxLayout *layout = new QVBoxLayout(panel);
        layout->setContentsMargins(10, 15, 10, 15);
        layout->setSpacing(12); // 按钮之间的间距
        // 【新增：系统控制按钮区域 - 放在最顶部】
        QHBoxLayout *controlLayout = new QHBoxLayout();
        controlLayout->setSpacing(5);

        QPushButton *restartBtn = new QPushButton("重开", panel);
        restartBtn->setStyleSheet("QPushButton { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #757575, stop:1 #616161); color: white; font-size: 11px; font-weight: bold; min-height: 28px; border-radius: 6px; } QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #9E9E9E, stop:1 #757575); } QPushButton:pressed { background-color: #424242; }");
        connect(restartBtn, SIGNAL(clicked()), this, SLOT(onStartPVPClicked()));

        QPushButton *menuBtn = new QPushButton("主菜单", panel);
        menuBtn->setStyleSheet("QPushButton { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #757575, stop:1 #616161); color: white; font-size: 11px; font-weight: bold; min-height: 28px; border-radius: 6px; } QPushButton:hover { background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #9E9E9E, stop:1 #757575); } QPushButton:pressed { background-color: #424242; }");
        connect(menuBtn, SIGNAL(clicked()), this, SLOT(onBackToMenuClicked()));

        controlLayout->addWidget(restartBtn);
        controlLayout->addWidget(menuBtn);
        layout->addLayout(controlLayout); // 将控制按钮加入垂直布局的最上方
        struct SkillInfo {
            QString name;
            int cost;
            QString color;
            QString hoverColor;
        };

        std::vector<SkillInfo> skills = {
            {"跳过答题\n(消耗 5)", 5, "#9C27B0", "#AB47BC"},
            {"强力封锁\n(消耗 4)", 4, "#E91E63", "#EC407A"},
            {"封锁对手\n(消耗 3)", 3, "#F44336", "#EF5350"},
            {"置换棋子\n(消耗 2)", 2, "#FF9800", "#FFA726"},
            {"提示答案\n(消耗 1)", 1, "#03A9F4", "#29B6F6"}
        };

        for (int i = 0; i < 5; ++i) {
            QPushButton *btn = new QPushButton(skills[i].name, panel);

            btn->setStyleSheet(QString(
                                   "QPushButton {"
                                   "  background-color: %1;"
                                   "  color: white;"
                                   "  font-size: 13px;"
                                   "  font-weight: bold;"
                                   "  border: none;"
                                   "  border-radius: 8px;"
                                   "  min-height: 50px;"
                                   "  padding: 4px;"
                                   "}"
                                   "QPushButton:hover {"
                                   "  background-color: %2;"
                                   "}"
                                   "QPushButton:pressed {"
                                   "  background-color: #424242;"
                                   "  padding-left: 3px;"
                                   "  padding-top: 3px;"
                                   "}"
                                   "QPushButton:disabled {"
                                   "  background-color: #BDBDBD;"
                                   "  color: #E0E0E0;"
                                   "}"
                                   ).arg(skills[i].color, skills[i].hoverColor));

            // 测试专用弹窗连接：点击会直接弹窗提示，百分百保证能点
            connect(btn, &QPushButton::clicked, this, [this, isPrimary, cost = skills[i].cost]() {
                  return;
            });

            layout->addWidget(btn);
        }

        QPushButton *energyDisplay = new QPushButton("能量: 0", panel);
        energyDisplay->setFocusPolicy(Qt::NoFocus);
        energyDisplay->setStyleSheet(
            "QPushButton {"
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            "    stop:0 #66BB6A, stop:1 #43A047);"
            "  color: white;"
            "  font-size: 15px;"
            "  font-weight: bold;"
            "  border: 2px solid #2E7D32;"
            "  border-radius: 8px;"
            "  min-height: 55px;"
            "}"
            "QPushButton:hover, QPushButton:pressed {"
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            "    stop:0 #81C784, stop:1 #66BB6A);"
            "}"
            );

        layout->addWidget(energyDisplay);

        if (isPrimary) {
            p1EnergyLabel = energyDisplay;
        } else {
            p2EnergyLabel = energyDisplay;
        }

        return panel;
    }; // Lambda 定义结束

    // ================== 3. 实例化并显示新面板 ==================
    skillPanel  = createPanel(0, true);                      // 左侧玩家1
    skillPanel2 = createPanel(boardOffsetX + boardSize, false); // 右侧玩家2

    // 强行提到最顶层，防止点击被棋盘层挡住
    skillPanel->raise();
    skillPanel2->raise();

    skillPanel->show();
    skillPanel2->show();

    // 调整菜单栏
    menuBar()->clear();
    QMenu *gameMenu = menuBar()->addMenu("游戏");
    QAction *backAction = new QAction("返回主菜单", this);
    connect(backAction, SIGNAL(triggered()), this, SLOT(onBackToMenuClicked()));
    gameMenu->addAction(backAction);

    QAction *restartAction = new QAction("重新开始", this);
    connect(restartAction, SIGNAL(triggered()), this, SLOT(onStartPVPClicked()));
    gameMenu->addAction(restartAction);

    update();
}

void MainWindow::onStartPVPClicked()
{
    startGame(PVP_MODE);
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
    if (skillPanel2)
    {
        delete skillPanel2;
        skillPanel2 = nullptr;
    }
    wrongPositions.clear();
    m_winPieces.clear();
    lastMoveRow = -1;
    lastMoveCol = -1;
    currentMode = MENU_MODE;
    setFixedSize(440, 480);
    menuBar()->clear();
    menuWidget->show();
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    if (game) {
        if (p1EnergyLabel)
            p1EnergyLabel->setText(QString("能量: %1").arg(game->p1Energy));
        if (p2EnergyLabel)
            p2EnergyLabel->setText(QString("能量: %1").arg(game->p2Energy));
    }
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    if (currentMode == MENU_MODE)
    {
        QLinearGradient menuGrad(0, 0, 0, height());
        menuGrad.setColorAt(0.0, QColor(63, 81, 181));
        menuGrad.setColorAt(0.5, QColor(48, 63, 159));
        menuGrad.setColorAt(1.0, QColor(26, 35, 126));
        painter.fillRect(rect(), menuGrad);
        return;
    }

    int boardWidth = kBoardMargin * 2 + kBlockSize * kBoardSizeNum;
    int boardHeight = boardWidth;
    int boardLeft = boardOffsetX;
    int boardTop = 0;

    QLinearGradient bgGrad(0, 0, width(), 0);
    bgGrad.setColorAt(0.0, QColor(220, 220, 220));
    bgGrad.setColorAt(0.5, QColor(210, 210, 210));
    bgGrad.setColorAt(1.0, QColor(220, 220, 220));
    painter.fillRect(rect(), bgGrad);

    painter.fillRect(boardLeft + 4, boardTop + 4, boardWidth, boardHeight, QColor(0, 0, 0, 50));

    QLinearGradient woodGrad(boardLeft, boardTop, boardLeft + boardWidth, boardTop);
    woodGrad.setColorAt(0.0, QColor(210, 180, 140));
    woodGrad.setColorAt(0.15, QColor(200, 170, 130));
    woodGrad.setColorAt(0.3, QColor(215, 185, 145));
    woodGrad.setColorAt(0.5, QColor(195, 165, 125));
    woodGrad.setColorAt(0.7, QColor(210, 180, 140));
    woodGrad.setColorAt(0.85, QColor(190, 160, 120));
    woodGrad.setColorAt(1.0, QColor(205, 175, 135));
    painter.fillRect(boardLeft, boardTop, boardWidth, boardHeight, woodGrad);

    painter.setPen(QPen(QColor(100, 70, 40), 2));
    painter.drawRect(boardLeft, boardTop, boardWidth, boardHeight);

    painter.setPen(QPen(QColor(80, 50, 30), 1));
    for (int i = 0; i <= kBoardSizeNum; i++)
    {
        painter.drawLine(kBoardMargin + kBlockSize * i + boardOffsetX, kBoardMargin,
                         kBoardMargin + kBlockSize * i + boardOffsetX, boardHeight - kBoardMargin);
        painter.drawLine(kBoardMargin + boardOffsetX, kBoardMargin + kBlockSize * i,
                         boardWidth - kBoardMargin + boardOffsetX, kBoardMargin + kBlockSize * i);
    }

    painter.setBrush(QColor(80, 50, 30));
    painter.setPen(Qt::NoPen);
    int stars[] = {3, 7, 11};
    for (int r : stars) {
        for (int c : stars) {
            int sx = kBoardMargin + c * kBlockSize + kBlockSize / 2 + boardOffsetX;
            int sy = kBoardMargin + r * kBlockSize + kBlockSize / 2;
            painter.drawEllipse(sx - 4, sy - 4, 8, 8);
        }
    }

    if (!m_winPieces.empty()) {
        for (const auto& pos : m_winPieces) {
            int i = pos.first, j = pos.second;
            int cx = boardOffsetX + kBoardMargin + j * kBlockSize + (kBlockSize / 2);
            int cy = kBoardMargin + i * kBlockSize + (kBlockSize / 2);
            QRadialGradient glow(cx, cy, kRadius * 2);
            glow.setColorAt(0.0, QColor(255, 215, 0, 180));
            glow.setColorAt(0.5, QColor(255, 215, 0, 60));
            glow.setColorAt(1.0, QColor(255, 215, 0, 0));
            painter.setBrush(glow);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(cx - kRadius * 2, cy - kRadius * 2, kRadius * 4, kRadius * 4);
        }
    }

    if (game)
    {
        for (int i = 0; i < kBoardSizeNum; i++) {
            for (int j = 0; j < kBoardSizeNum; j++) {

                int cx = boardOffsetX + kBoardMargin + j * kBlockSize + (kBlockSize / 2);
                int cy = kBoardMargin + i * kBlockSize + (kBlockSize / 2);

                if (game->gameMapVec[i][j] == 1)
                {
                    painter.setBrush(QColor(0, 0, 0, 60));
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(cx - kRadius + 2, cy - kRadius + 2, kRadius * 2, kRadius * 2);

                    QRadialGradient wGrad(cx - 4, cy - 4, kRadius);
                    wGrad.setColorAt(0.0, QColor(255, 255, 255));
                    wGrad.setColorAt(0.5, QColor(245, 245, 245));
                    wGrad.setColorAt(1.0, QColor(200, 200, 200));
                    painter.setBrush(wGrad);
                    painter.drawEllipse(cx - kRadius, cy - kRadius, kRadius * 2, kRadius * 2);
                }
                else if (game->gameMapVec[i][j] == -1)
                {
                    painter.setBrush(QColor(0, 0, 0, 80));
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(cx - kRadius + 2, cy - kRadius + 2, kRadius * 2, kRadius * 2);

                    QRadialGradient bGrad(cx - 4, cy - 4, kRadius);
                    bGrad.setColorAt(0.0, QColor(80, 80, 80));
                    bGrad.setColorAt(0.5, QColor(40, 40, 40));
                    bGrad.setColorAt(1.0, QColor(20, 20, 20));
                    painter.setBrush(bGrad);
                    painter.drawEllipse(cx - kRadius, cy - kRadius, kRadius * 2, kRadius * 2);
                }

                if (i == lastMoveRow && j == lastMoveCol &&
                    (game->gameMapVec[i][j] == 1 || game->gameMapVec[i][j] == -1))
                {
                    painter.setBrush(QColor(255, 60, 60));
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(cx - 4, cy - 4, 8, 8);
                }

                bool shouldDrawCross = false;
                for (const auto& pos : wrongPositions) {
                    if (pos.first == i && pos.second == j) {
                        shouldDrawCross = true;
                        break;
                    }
                }
                if (shouldDrawCross)
                {
                    QPen pen(Qt::red);
                    pen.setWidth(3);
                    painter.setPen(pen);
                    painter.setBrush(Qt::NoBrush);
                    int r = kBlockSize / 4;
                    painter.drawLine(cx - r, cy - r, cx + r, cy + r);
                    painter.drawLine(cx + r, cy - r, cx - r, cy + r);
                }
            }
        }

        if (clickPosRow >= 0 && clickPosRow < kBoardSizeNum &&
            clickPosCol >= 0 && clickPosCol < kBoardSizeNum &&
            game->gameMapVec[clickPosRow][clickPosCol] == 0)
        {
            int cx = boardOffsetX + kBoardMargin + clickPosCol * kBlockSize + (kBlockSize / 2);
            int cy = kBoardMargin + clickPosRow * kBlockSize + (kBlockSize / 2);

            QRadialGradient hGrad(cx - 4, cy - 4, kRadius);
            if (game->playerFlag) {
                hGrad.setColorAt(0.0, QColor(60, 60, 60, 160));
                hGrad.setColorAt(1.0, QColor(20, 20, 20, 80));
            } else {
                hGrad.setColorAt(0.0, QColor(255, 255, 255, 160));
                hGrad.setColorAt(1.0, QColor(200, 200, 200, 80));
            }
            painter.setBrush(hGrad);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(cx - kRadius, cy - kRadius, kRadius * 2, kRadius * 2);
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!game || currentMode == MENU_MODE) return;

    int x = event->position().x() - boardOffsetX;
    int y = event->position().y();

    clickPosRow = -1;
    clickPosCol = -1;

    for (int c = 0; c < kBoardSizeNum; c++)
    {
        int cellLeft = kBoardMargin + kBlockSize * c;
        int cellRight = cellLeft + kBlockSize;
        if (x >= cellLeft && x < cellRight)
        {
            for (int r = 0; r < kBoardSizeNum; r++)
            {
                int cellTop = kBoardMargin + kBlockSize * r;
                int cellBottom = cellTop + kBlockSize;
                if (y >= cellTop && y < cellBottom)
                {
                    clickPosRow = r;
                    clickPosCol = c;
                }
            }
        }
    }

    update();
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (!game || currentMode == MENU_MODE || game->gameStatus != PLAYING) return;

    int x = event->position().x() - boardOffsetX;
    int y = event->position().y();

    int row = (y - kBoardMargin) / kBlockSize;
    int col = (x - kBoardMargin) / kBlockSize;

    if (row < 0 || row >= kBoardSizeNum || col < 0 || col >= kBoardSizeNum) return;

    for (const auto& pos : wrongPositions) {
        if (row == pos.first && col == pos.second) {
            QMessageBox::warning(this, "已被封锁", "这个格子你刚才已经答错被封锁了！请选其他格子。");
            return;
        }
    }

    if (!game->isValidMove(row, col)) return;

    pendingRow = row;
    pendingCol = col;

    Question currentQ = getRandomQuestion();
    QuestionDialog dlg(currentQ, this);
    dlg.exec();

    if (dlg.isCorrect())
    {
        game->actionByPerson(pendingRow, pendingCol);
        lastMoveRow = pendingRow;
        lastMoveCol = pendingCol;
        wrongPositions.clear();

        if (game->playerFlag) game->p2Energy += 1; else game->p1Energy += 1;

        if (game->isWin(pendingRow, pendingCol)) {
            game->gameStatus = WIN;
            m_winPieces = game->getWinLine(pendingRow, pendingCol);
            update();
            QTimer::singleShot(1500, this, [this]() {
                QString str = (game->gameMapVec[lastMoveRow][lastMoveCol] == 1) ? "白方" : "黑方";
                QMessageBox::information(this, "胜利", str + " 获胜!");
                onBackToMenuClicked();
            });
            return;
        }

        if (game->isDeadGame()) {
            game->gameStatus = DEAD;
            update();
            QTimer::singleShot(500, this, [this]() {
                QMessageBox::information(this, "平局", "和棋!");
                onBackToMenuClicked();
            });
            return;
        }

        QMessageBox::information(this, "回答正确", "回答正确，成功落子！并为你积攒了能量。");
    }
    else
    {
        wrongPositions.push_back({pendingRow, pendingCol});
        QMessageBox::warning(this, "回答错误", "回答错误！该格子已被你封锁，请换个格子重新尝试。");
    }

    update();
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
void MainWindow::loadQuestionsFromJson()
{
    // 假设你的 json 文件放在可执行文件同目录下，或者放到了 Qt 资源文件(qrc)中
    QFile file(":/questions.json"); // 如果用 qrc 资源文件，前面加冒号。如果是本地路径直接写 "questions.json"

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "错误", "无法加载题库文件！");
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(fileData);
    if (!doc.isArray()) return;

    QJsonArray jsonArray = doc.array();

    // 遍历 JSON 数组，将每一项转为 Question 结构体并存入 m_questionBank
    for (int i = 0; i < jsonArray.size(); ++i) {
        QJsonObject jsonObj = jsonArray[i].toObject();

        Question q;
        q.id = jsonObj["id"].toInt();
        q.questionText = jsonObj["questionText"].toString();
        q.correctAnswer = jsonObj["correctAnswer"].toInt();
        q.category = jsonObj["category"].toString();
        q.difficulty = jsonObj["difficulty"].toInt();
        QJsonArray optionsArray = jsonObj["options"].toArray();
        for (int j = 0; j < optionsArray.size(); ++j) {
            q.options.append(optionsArray[j].toString());
        }

        m_questionBank.append(q);
    }
}

Question MainWindow::getRandomQuestion()
{
    if (m_filteredQuestionBank.isEmpty()) {
        if (!m_questionBank.isEmpty()) {
            int randomIndex = QRandomGenerator::global()->bounded(m_questionBank.size());
            return m_questionBank.at(randomIndex);
        } else {
            Question fallbackQ;
            fallbackQ.questionText = "题库加载失败或为空？";
            fallbackQ.options = {"A. 是", "B. 否", "C. 也许", "D. 不确定"};
            fallbackQ.correctAnswer = 0;
            return fallbackQ;
        }
    }

    int randomIndex = QRandomGenerator::global()->bounded(m_filteredQuestionBank.size());
    return m_filteredQuestionBank.at(randomIndex);
}
void MainWindow::filterQuestions()
{
    m_filteredQuestionBank.clear(); // 先清空过滤题库

    // 获取当前下拉框选中的值
    // difficultyCombo 的第二参数是 UserData，用 currentData().toInt() 获取 (比如你写的 EASY, MEDIUM)
    int selectedDifficulty = difficultyCombo->currentData().toInt();
    // topicCombo 我们直接拿文字来匹配
    QString selectedTopic = topicCombo->currentText();

    // 遍历总题库，进行筛选
    for (int i = 0; i < m_questionBank.size(); ++i) {
        const Question &q = m_questionBank.at(i);

        // 条件1：难度匹配
        bool matchDifficulty = (q.difficulty == selectedDifficulty);

        // 条件2：知识点匹配（如果选了"全部"，则默认全匹配；否则要求字符串完全相等）
        bool matchTopic = (selectedTopic == "全部" || q.category == selectedTopic);

        // 两个条件都满足，才加入当前题库
        if (matchDifficulty && matchTopic) {
            m_filteredQuestionBank.append(q);
        }
    }

    // 调试信息：看看当前条件下筛选出了多少题
    qDebug() << "筛选完成！当前条件下共有题目数量：" << m_filteredQuestionBank.size();
}
