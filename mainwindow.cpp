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

    if (menuWidget) {
        menuWidget->hide();
        menuWidget->lower(); // 将菜单置于底层，防止隐形遮罩拦截鼠标
    }

    if (!game)
        game = new GameModel;

    // 重置游戏状态
    game->startGame();
    wrongPositions.clear();
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
        panel->setStyleSheet("background-color: #F0F0F0; border: 1px solid #DCDCDC;");

        // 使用垂直布局，让 6 个控件从上到下等间距紧凑排列
        QVBoxLayout *layout = new QVBoxLayout(panel);
        layout->setContentsMargins(10, 15, 10, 15);
        layout->setSpacing(12); // 按钮之间的间距
        // 【新增：系统控制按钮区域 - 放在最顶部】
        QHBoxLayout *controlLayout = new QHBoxLayout();
        controlLayout->setSpacing(5);

        QPushButton *restartBtn = new QPushButton("重开", panel);
        restartBtn->setStyleSheet("QPushButton { background-color: #757575; color: white; font-size: 11px; font-weight: bold; min-height: 28px; border-radius: 4px; } QPushButton:hover { background-color: #616161; } QPushButton:pressed { background-color: #424242; }");
        connect(restartBtn, SIGNAL(clicked()), this, SLOT(onStartPVPClicked())); // 绑定重新开始

        QPushButton *menuBtn = new QPushButton("主菜单", panel);
        menuBtn->setStyleSheet("QPushButton { background-color: #757575; color: white; font-size: 11px; font-weight: bold; min-height: 28px; border-radius: 4px; } QPushButton:hover { background-color: #616161; } QPushButton:pressed { background-color: #424242; }");
        connect(menuBtn, SIGNAL(clicked()), this, SLOT(onBackToMenuClicked()));   // 绑定返回主菜单

        controlLayout->addWidget(restartBtn);
        controlLayout->addWidget(menuBtn);
        layout->addLayout(controlLayout); // 将控制按钮加入垂直布局的最上方
        // 1. 定义 5 个技能的名字、消耗以及颜色
        struct SkillInfo {
            QString name;
            int cost;
            QString color;
        };

        std::vector<SkillInfo> skills = {
            {"跳过答题\n(消耗 5)", 5, "#9C27B0"}, // 紫色
            {"强力封锁\n(消耗 4)", 4, "#E91E63"}, // 粉红
            {"封锁对手\n(消耗 3)", 3, "#F44336"}, // 红色
            {"置换棋子\n(消耗 2)", 2, "#FF9800"}, // 橙色
            {"提示答案\n(消耗 1)", 1, "#03A9F4"}  // 蓝色
        };

        // 2. 循环生成前 5 个技能按钮
        for (int i = 0; i < 5; ++i) {
            QPushButton *btn = new QPushButton(skills[i].name, panel);

            btn->setStyleSheet(QString(
                                   "QPushButton {"
                                   "  background-color: %1;"      // 基础颜色
                                   "  color: white;"
                                   "  font-size: 13px;"
                                   "  font-weight: bold;"
                                   "  border: none;"
                                   "  border-radius: 6px;"
                                   "  min-height: 50px;"
                                   "}"
                                   "QPushButton:hover {"
                                   "  background-color: %1;"
                                   "  opacity: 0.85;"             // 鼠标悬停时：变淡一点点
                                   "}"
                                   "QPushButton:pressed {"
                                   "  background-color: #333333;" // 核心修改：鼠标按下时，按钮瞬间变成深灰色
                                   "  padding-left: 3px;"         // 核心修改：让文字往右下角微调 1 像素，模拟真实的物理下沉按压感
                                   "  padding-top: 3px;"
                                   "}"
                                   "QPushButton:disabled {"
                                   "  background-color: #BDBDBD;"
                                   "  color: #E0E0E0;"
                                   "}"
                                   ).arg(skills[i].color));

            // 测试专用弹窗连接：点击会直接弹窗提示，百分百保证能点
            connect(btn, &QPushButton::clicked, this, [this, isPrimary, cost = skills[i].cost]() {
                  return;
            });

            layout->addWidget(btn);
        }

        // 3. 创建最下方的能量显示按钮
        QPushButton *energyDisplay = new QPushButton("能量: 0", panel);
        energyDisplay->setFocusPolicy(Qt::NoFocus);
        energyDisplay->setStyleSheet(
            "QPushButton {"
            "  background-color: #4CAF50;"
            "  color: white;"
            "  font-size: 15px;"
            "  font-weight: bold;"
            "  border: 2px solid #388E3C;"
            "  border-radius: 6px;"
            "  min-height: 55px;"
            "}"
            "QPushButton:hover, QPushButton:pressed {"
            "  background-color: #4CAF50;"
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
    currentMode = MENU_MODE;
    setFixedSize(400, 380);
    menuBar()->clear();
    menuWidget->show();
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    // 1. 同步能量条显示
    if (game) {
        if (p1EnergyLabel) {
            p1EnergyLabel->setText(QString("能量: %1").arg(game->p1Energy));
        }
        if (p2EnergyLabel) {
            p2EnergyLabel->setText(QString("能量: %1").arg(game->p2Energy));
        }
    }
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (currentMode == MENU_MODE)
    {
        painter.fillRect(rect(), QColor(220, 220, 220));
        return;
    }

    // 画背景和棋盘格子线
    painter.fillRect(rect(), QColor(210, 180, 140));

    int boardWidth = kBoardMargin * 2 + kBlockSize * kBoardSizeNum;
    int boardHeight = boardWidth;

    // 【重要：画格子线前先重置画笔，确保格子线是正常的黑色细线】
    painter.setPen(QPen(Qt::black, 1));
    for (int i = 0; i <= kBoardSizeNum; i++)
    {
        painter.drawLine(kBoardMargin + kBlockSize * i + boardOffsetX, kBoardMargin,
                         kBoardMargin + kBlockSize * i + boardOffsetX, boardHeight - kBoardMargin);
        painter.drawLine(kBoardMargin + boardOffsetX, kBoardMargin + kBlockSize * i,
                         boardWidth - kBoardMargin + boardOffsetX, kBoardMargin + kBlockSize * i);
    }

    // =================================================================
    // 解决问题 1：修复鼠标悬停提示颜色不一致
    // =================================================================
    QBrush brush;
    brush.setStyle(Qt::SolidPattern);

    if (clickPosRow >= 0 && clickPosRow < kBoardSizeNum &&
        clickPosCol >= 0 && clickPosCol < kBoardSizeNum &&
        game && game->gameMapVec[clickPosRow][clickPosCol] == 0)
    {
        // ⚠️ 核心修改：如果你是用 !game->playerFlag 或者 game->playerFlag 决定的，
        // 请确保这里的 if 条件和 actionByPerson 内部放子前的判断完全对齐！
        // 假设当前谁落子，提示框就是谁的颜色：
        if (game->playerFlag)
            brush.setColor(Qt::white); // 白方回合显示白提示
        else
            brush.setColor(Qt::black); // 黑方回合显示黑提示

        painter.setBrush(brush);
        // 重置画笔，防止悬停框带红边
        painter.setPen(Qt::NoPen);

        painter.drawRect(kBoardMargin + kBlockSize * clickPosCol + kBlockSize / 2 - kMarkSize / 2 + boardOffsetX,
                         kBoardMargin + kBlockSize * clickPosRow + kBlockSize / 2 - kMarkSize / 2,
                         kMarkSize, kMarkSize);
    }

    // =================================================================
    // 解决问题 2 & 3：遍历棋盘绘制棋子与红叉
    // =================================================================
    if (game)
    {
        for (int i = 0; i < kBoardSizeNum; i++) {
            for (int j = 0; j < kBoardSizeNum; j++) {

                int centerX = boardOffsetX + kBoardMargin + j * kBlockSize + (kBlockSize / 2);
                int centerY = kBoardMargin + i * kBlockSize + (kBlockSize / 2);

                if (game->gameMapVec[i][j] == 1) // 白子
                {
                    brush.setColor(Qt::white);
                    painter.setBrush(brush);
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(centerX - kRadius, centerY - kRadius, kRadius * 2, kRadius * 2);
                }
                else if (game->gameMapVec[i][j] == -1) // 黑子
                {
                    brush.setColor(Qt::black);
                    painter.setBrush(brush);
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(centerX - kRadius, centerY - kRadius, kRadius * 2, kRadius * 2);
                }

                // =================================================================
                // ⚠️ 【核心修改】：检查当前格子 (i, j) 是否在答错的红叉列表里
                // =================================================================
                bool shouldDrawCross = false;
                for (const auto& pos : wrongPositions) {
                    if (pos.first == i && pos.second == j) {
                        shouldDrawCross = true;
                        break;
                    }
                }

                if (shouldDrawCross)
                {
                    painter.setRenderHint(QPainter::Antialiasing, true);
                    QPen pen(Qt::red);
                    pen.setWidth(3);
                    painter.setPen(pen);
                    painter.setBrush(Qt::NoBrush);

                    int r = kBlockSize / 4;
                    painter.drawLine(centerX - r, centerY - r, centerX + r, centerY + r);
                    painter.drawLine(centerX + r, centerY - r, centerX - r, centerY + r);
                }
            }
        }

        // 游戏输赢判定保持原样...
        if (clickPosRow >= 0 && clickPosRow < kBoardSizeNum &&
            clickPosCol >= 0 && clickPosCol < kBoardSizeNum &&
            (game->gameMapVec[clickPosRow][clickPosCol] == 1 ||
             game->gameMapVec[clickPosRow][clickPosCol] == -1))
        {
            if (game->isWin(clickPosRow, clickPosCol) && game->gameStatus == PLAYING)
            {
                game->gameStatus = WIN;
                QString str = (game->gameMapVec[clickPosRow][clickPosCol] == 1) ? "白方" : "黑方";
                QMessageBox::StandardButton btnValue = QMessageBox::information(this, "胜利", str + " 获胜!");
                if (btnValue == QMessageBox::Ok) {
                    QTimer::singleShot(0, this, SLOT(onBackToMenuClicked()));
                }
            }
        }

        if (game->isDeadGame())
        {
            QMessageBox::StandardButton btnValue = QMessageBox::information(this, "平局", "和棋!");
            if (btnValue == QMessageBox::Ok) {
                QTimer::singleShot(0, this, SLOT(onBackToMenuClicked()));
            }
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

    // =================================================================
    // ⚠️ 【核心修改】：遍历红叉列表，点中任何一个历史红叉都直接拦截
    // =================================================================
    for (const auto& pos : wrongPositions) {
        if (row == pos.first && col == pos.second) {
            QMessageBox::warning(this, "已被封锁", "这个格子你刚才已经答错被封锁了！请选其他格子。");
            return;
        }
    }

    if (!game->isValidMove(row, col)) return;

    pendingRow = row;
    pendingCol = col;

    Question testQ; // 你的题库数据...
    testQ.questionText = "以下哪个关键字用于在 C++ 中申请动态内存？";
    testQ.options = {"A. malloc", "B. new", "C. alloc", "D. choice"};
    testQ.correctAnswer = 1;
    testQ.category = "基础语法";

    QuestionDialog dlg(testQ, this);
    dlg.exec();

    if (dlg.isCorrect())
    {
        // 情况 A: 答对了！
        game->actionByPerson(pendingRow, pendingCol);

        // ⚠️ 【核心修改】：既然成功落子进入下一回合，一口气清空本回合所有的红叉记录
        wrongPositions.clear();

        QMessageBox::information(this, "回答正确", "回答正确，成功落子！并为你积攒了能量。");
        if (game->playerFlag) game->p2Energy += 1; else game->p1Energy += 1;
    }
    else
    {
        // 情况 B: 答错了！
        // ⚠️ 【核心修改】：不覆盖旧的，而是把新的答错坐标追加到列表里，让他们并存
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

