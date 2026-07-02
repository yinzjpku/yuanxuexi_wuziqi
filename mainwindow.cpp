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
#include <QFrame>
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QJsonValue>
#include <QDateTime>
#include <QDir>
#include <QCoreApplication>
#include <math.h>
#include "mainwindow.h"
#include "QuestionDialog.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRandomGenerator>
#include <QGraphicsDropShadowEffect>
const int kBoardMargin = 30;
const int kRadius = 15;
const int kMarkSize = 6;
const int kBlockSize = 40;
const int kPosDelta = 20;
const int kAIDelay = 700;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), currentMode(MENU_MODE), difficulty(MEDIUM)
{
    setFixedSize(420, 520);
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
    m_isAnswering = false;
    menuWidget = new QWidget(this);
    menuWidget->setGeometry(0, 0, 420, 520);
    setupMenuMode();
    loadQuestionsFromJson();

    m_globalTimer = new QTimer(this);
    connect(m_globalTimer, &QTimer::timeout, this, &MainWindow::onGlobalTimerTick);

    m_network = new NetworkManager(this);
    connect(m_network, &NetworkManager::connected, this, &MainWindow::onNetworkConnected);
    connect(m_network, &NetworkManager::disconnected, this, &MainWindow::onNetworkDisconnected);
    connect(m_network, &NetworkManager::messageReceived, this, &MainWindow::onNetworkMessageReceived);
    connect(m_network, &NetworkManager::errorOccurred, this, [this](const QString &message) {
        QMessageBox::warning(this, "联网对战", message);
    });
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
    menuWidget->setStyleSheet("background: transparent;");
    QVBoxLayout *mainLayout = new QVBoxLayout(menuWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(50, 22, 50, 24);

    // --- 标题 ---
    QLabel *titleLabel = new QLabel("🎮 五子棋编程对战", menuWidget);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(R"(
        QLabel {
            font-size: 26px;
            font-weight: bold;
            color: #FFFFFF;
            padding: 4px 0 8px 0;
            background: transparent;
        }
    )");
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(18);
    shadow->setOffset(3, 3);
    shadow->setColor(QColor(0, 0, 0, 100));
    titleLabel->setGraphicsEffect(shadow);
    mainLayout->addWidget(titleLabel);

    // --- 设置卡片 ---
    QWidget *card = new QWidget(menuWidget);
    card->setObjectName("menuCard");
    card->setStyleSheet(R"(
        QWidget#menuCard {
            background: rgba(255, 255, 255, 220);
            border: 1px solid rgba(255,255,255,180);
            border-radius: 14px;
        }
    )");
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 16, 22, 16);
    cardLayout->setSpacing(8);

    QLabel *difficultyLabel = new QLabel("难度选择", card);
    difficultyLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #555; background: transparent;");
    cardLayout->addWidget(difficultyLabel);

    difficultyCombo = new QComboBox(card);
    difficultyCombo->addItem("简单", EASY);
    difficultyCombo->addItem("中等", MEDIUM);
    difficultyCombo->addItem("困难", HARD);
    difficultyCombo->setCurrentIndex(1);
    difficultyCombo->setMinimumHeight(34);
    cardLayout->addWidget(difficultyCombo);

    QLabel *topicLabel = new QLabel("知识点选择", card);
    topicLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #555; background: transparent;");
    cardLayout->addWidget(topicLabel);

    topicCombo = new QComboBox(card);
    topicCombo->addItem("全部");
    topicCombo->addItem("类与对象基础");
    topicCombo->addItem("继承与多态");
    topicCombo->addItem("运算符重载");
    topicCombo->addItem("STL");
    topicCombo->addItem("文件，模版，c++新特性");
    topicCombo->setMinimumHeight(34);
    cardLayout->addWidget(topicCombo);

    connect(difficultyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::filterQuestions);
    connect(topicCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::filterQuestions);
    loadQuestionsFromJson();
    filterQuestions();
    mainLayout->addWidget(card);
    mainLayout->addSpacing(6);

    // --- 本地对战按钮 ---
    QPushButton *pvpBtn = new QPushButton("🎯  本地对战", menuWidget);
    pvpBtn->setCursor(Qt::PointingHandCursor);
    pvpBtn->setStyleSheet(R"(
        QPushButton {
            font-size: 17px;
            font-weight: bold;
            padding: 12px;
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #4CAF50, stop:1 #45a049);
            color: white;
            border: none;
            border-radius: 10px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #66BB6A, stop:1 #4CAF50);
        }
        QPushButton:pressed {
            padding-top: 16px;
            padding-left: 4px;
        }
    )");
    connect(pvpBtn, SIGNAL(clicked()), this, SLOT(onStartPVPClicked()));
    mainLayout->addWidget(pvpBtn);

    QPushButton *networkBtn = new QPushButton("🌐  联网对战", menuWidget);
    networkBtn->setCursor(Qt::PointingHandCursor);
    networkBtn->setStyleSheet(R"(
        QPushButton {
            font-size: 17px;
            font-weight: bold;
            padding: 12px;
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #2196F3, stop:1 #1976D2);
            color: white;
            border: none;
            border-radius: 10px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #42A5F5, stop:1 #1E88E5);
        }
        QPushButton:pressed {
            padding-top: 16px;
            padding-left: 4px;
        }
    )");
    connect(networkBtn, SIGNAL(clicked()), this, SLOT(onNetworkBattleClicked()));
    mainLayout->addWidget(networkBtn);

    // --- 退出按钮 ---
    QPushButton *exitBtn = new QPushButton("✕  退出", menuWidget);
    exitBtn->setCursor(Qt::PointingHandCursor);
    exitBtn->setStyleSheet(R"(
        QPushButton {
            font-size: 14px;
            padding: 9px;
            background: transparent;
            color: rgba(255,255,255,180);
            border: 1px solid rgba(255,255,255,100);
            border-radius: 8px;
        }
        QPushButton:hover {
            background: rgba(255,255,255,30);
            color: white;
            border-color: rgba(255,255,255,200);
        }
        QPushButton:pressed {
            padding-top: 12px;
            padding-left: 2px;
        }
    )");
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
    m_wrongQuestionRecords = QJsonArray();
    m_reviewExported = false;
    m_hasCurrentQuestion = false;
    // ⚠️【核心修复】：新游戏开始，彻底解除答题状态和恢复正常光标
    m_isAnswering = false;
    setCursor(Qt::ArrowCursor);
    m_currentDlg = nullptr;
    m_isSkippingQuestion = false;
    m_isEliminatingCross = false;
    int boardSize = kBoardMargin * 2 + kBlockSize * kBoardSizeNum;
    int skillPanelWidth = 150;
    m_p1TimeLeft = 600; // 10分钟
    m_p2TimeLeft = 600;
    m_globalTimer->start(1000); // 启动全局倒计时
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
        panel->setGeometry(x, 40, skillPanelWidth, boardSize);
        panel->setObjectName(isPrimary ? "panelP1" : "panelP2");
        panel->setStyleSheet(R"(
            QWidget#panelP1, QWidget#panelP2 {
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #FAFAFA, stop:1 #F0F0F0);
                border: 1px solid #DCDCDC;
                border-radius: 6px;
            }
        )");

        QVBoxLayout *layout = new QVBoxLayout(panel);
        layout->setContentsMargins(8, 10, 8, 12);
        layout->setSpacing(8);

        // --- 玩家标识 ---
        QLabel *playerLabel = new QLabel(isPrimary ? "⚫ 黑方" : "⚪ 白方", panel);
        playerLabel->setAlignment(Qt::AlignCenter);
        playerLabel->setStyleSheet(R"(
            QLabel {
                font-size: 15px;
                font-weight: bold;
                color: #444;
                padding: 4px 0;
                background: transparent;
            }
        )");
        layout->addWidget(playerLabel);

        // --- 控制按钮 ---
        QHBoxLayout *controlLayout = new QHBoxLayout();
        controlLayout->setSpacing(4);

        QString ctrlStyle = R"(
            QPushButton {
                background-color: #757575;
                color: white;
                font-size: 11px;
                font-weight: bold;
                min-height: 26px;
                border-radius: 5px;
                padding: 0 6px;
            }
            QPushButton:hover {
                background-color: #616161;
            }
            QPushButton:pressed {
                background-color: #424242;
                padding-top: 2px;
                padding-left: 2px;
            }
        )";

        QPushButton *restartBtn = new QPushButton("⟳ 重开", panel);
        restartBtn->setStyleSheet(ctrlStyle);
        connect(restartBtn, &QPushButton::clicked, this, [this]() {
            if (m_isNetworkGame && m_network && m_network->isConnected()) {
                QJsonObject message;
                message["type"] = "restart";
                m_network->sendMessage(message);
                startNetworkGame(m_localIsBlack);
            } else {
                onStartPVPClicked();
            }
        });

        QPushButton *menuBtn = new QPushButton("☰ 菜单", panel);
        menuBtn->setStyleSheet(ctrlStyle);
        connect(menuBtn, SIGNAL(clicked()), this, SLOT(onBackToMenuClicked()));

        controlLayout->addWidget(restartBtn);
        controlLayout->addWidget(menuBtn);
        layout->addLayout(controlLayout);

        // --- 分隔线 ---
        QFrame *line = new QFrame(panel);
        line->setFrameShape(QFrame::HLine);
        line->setStyleSheet("color: #E0E0E0; background: transparent;");
        layout->addWidget(line);

        // --- 技能 ---
        struct SkillInfo {
            QString icon;
            QString name;
            int cost;
            QString color;
        };

        std::vector<SkillInfo> skills = {
            {"⏭", "跳过答题", 4, "#9C27B0"},
            {"❌", "消除红叉", 2, "#E91E63"},
            {"✨", "排除错项", 2, "#F44336"},
            {"🔄", "换一道题", 1, "#FF9800"},
            {"⏱", "延长10s", 1, "#03A9F4"}
        };

        for (int i = 0; i < 5; ++i) {
            QString text = skills[i].icon + " " + skills[i].name + "\n(消耗 " + QString::number(skills[i].cost) + ")";
            QPushButton *btn = new QPushButton(text, panel);

            btn->setStyleSheet(QString(
                "QPushButton {"
                "  background-color: %1;"
                "  color: white;"
                "  font-size: 12px;"
                "  font-weight: bold;"
                "  border: none;"
                "  border-radius: 8px;"
                "  min-height: 48px;"
                "  padding: 4px 6px;"
                "}"
                "QPushButton:hover {"
                "  background-color: %1;"
                "  opacity: 0.85;"
                "}"
                "QPushButton:pressed {"
                "  background-color: #333333;"
                "  padding-left: 8px;"
                "  padding-top: 6px;"
                "}"
                "QPushButton:disabled {"
                "  background-color: #D0D0D0;"
                "  color: #F0F0F0;"
                "}"
            ).arg(skills[i].color));

            connect(btn, &QPushButton::clicked, this, [this, isPrimary, i]() {

                // 判断当前点技能的是不是正在思考的玩家 (假设 game->playerFlag true 为黑, false 为白)
                // 必须是当前回合玩家才能用自己的能量
                if (m_isNetworkGame && !isLocalTurn()) return;
                bool isCurrentTurn = (game->playerFlag == isPrimary);
                if (!isCurrentTurn) return;

                // 检查能量
                int& currentEnergy = isPrimary ? game->p1Energy : game->p2Energy;
                if (i == 1) {
                    if (m_isAnswering) return; // 答题阶段不可触发

                    if (currentEnergy < 2) {
                        QMessageBox::warning(this, "提示", "能量不足！消除红叉需要 2 能量。");
                        return;
                    }
                    if (wrongPositions.empty()) {
                        QMessageBox::warning(this, "提示", "棋盘上目前没有红叉可消！");
                        return;
                    }

                    m_isEliminatingCross = true;
                    m_isSkippingQuestion = false;
                    setCursor(Qt::CrossCursor);
                    QMessageBox::information(this, "技能激活", "已进入【消除红叉】模式，请点击棋盘上的红叉格子。");
                    return; // 进入模式后直接返回，等待玩家点击棋盘
                }
                if (i == 0) {
                    if (m_isAnswering) return; // 如果已经弹窗答题了，不可触发

                    if (currentEnergy < 4) {
                        QMessageBox::warning(this, "提示", "能量不足！跳过答题需要 4 能量。");
                        return;
                    }

                    // 如果开启了消除红叉模式，先关闭它
                    m_isEliminatingCross = false;

                    // 激活跳过答题状态
                    m_isSkippingQuestion = true;

                    // 改变光标样式（比如换成等待点击的手型或十字，这里用 SizeAllCursor 或 CrossCursor 提示都行）
                    setCursor(Qt::PointingHandCursor);

                    QMessageBox::information(this, "技能激活", "已进入【跳过答题】落子模式！请直接点击棋盘上任意合法格子，将免答题直接落子。");
                    return; // 进入模式，直接返回，等待点击棋盘
                }
                if (!m_isAnswering || !m_currentDlg) return;
                if (i == 4) {
                    // 1. 严格拦截：如果能量不足 1 点，直接黑盒提示并 return 拦截
                    if (currentEnergy < 1) {
                        QMessageBox msgBox(this);
                        msgBox.setWindowTitle("提示");
                        msgBox.setText("能量不足！无法发动延长时间技能。");
                        msgBox.setStyleSheet("QMessageBox { background-color: black; } QLabel { color: white; font-weight: bold; } QPushButton { background-color: #333; color: white; }");
                        msgBox.exec();
                        return; // 👈 极为关键！直接结束，不让代码往下走去扣能量
                    }

                    // 2. 只有能量充足时，才会走到这里执行
                    currentEnergy -= 1;         // 扣除 1 点能量
                    m_currentDlg->addTime(10);   // 弹窗增加 10 秒
                    update();                    // 刷新主界面能量条显示
                }
                else if (i == 2) {
                    if (currentEnergy < 2) { // 👈 注意：这个技能消耗 2 能量
                        QMessageBox::warning(this, "提示", "能量不足！排除错项需要 2 能量。");
                        return;
                    }
                    currentEnergy -= 2;                  // 1. 扣除 2 能量
                    m_currentDlg->excludeWrongOption(); // 2. 通知弹窗去掉一个错项
                    update();
                }
                else if (i == 3) {
                    if (currentEnergy < 1) {
                        QMessageBox msgBox(this);
                        msgBox.setWindowTitle("提示");
                        msgBox.setText("能量不足！");
                        msgBox.setStyleSheet("QMessageBox { background-color: black; } QLabel { color: white; font-weight: bold; } QPushButton { background-color: #333; color: white; }");
                        msgBox.exec();
                        return;
                    }
                    currentEnergy -= 1; // 扣除能量

                    // 🌟【核心安全重构】：彻底摧毁旧弹窗，防止信号残留引发崩溃
                    if (m_currentDlg) {
                        // 1. 显式断开旧弹窗和当前主窗口的所有信号连接
                        disconnect(m_currentDlg, &QuestionDialog::answerFinished, this, &MainWindow::handleAnswerResult);

                        m_currentDlg->forceAllowClose(); // 允许关闭
                        m_currentDlg->close();           // 关闭旧窗口

                        // 2. 强行立刻释放内存，而不是等系统有空再删，彻底杜绝后台“诈尸”
                        delete m_currentDlg;
                        m_currentDlg = nullptr;
                    }

                    // 状态锁重置
                    m_isAnswering = true;

                    // 3. 抽取新题目并实例化
                    Question testQ = getRandomQuestion();
                    m_currentQuestion = testQ;
                    m_hasCurrentQuestion = true;
                    m_currentDlg = new QuestionDialog(testQ, this);

                    // 4. 重新绑定新弹窗的信号槽
                    connect(m_currentDlg, &QuestionDialog::answerFinished, this, &MainWindow::handleAnswerResult);

                    m_currentDlg->setAttribute(Qt::WA_DeleteOnClose);
                    m_currentDlg->show();

                    int limit = 20;
                    m_currentDlg->setTimeLimit(limit);

                    update();
                }
            });

            layout->addWidget(btn);
        }

        // 3. 底部时间/能量面板
        QPushButton *energyDisplay = new QPushButton("⏱ 00:00\n⚡ 能量: 0", panel);
        energyDisplay->setFocusPolicy(Qt::NoFocus);
        energyDisplay->setStyleSheet(R"(
            QPushButton {
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #546E7A, stop:1 #455A64);
                color: white;
                font-size: 13px;
                font-weight: bold;
                border: none;
                border-radius: 8px;
                min-height: 58px;
                padding: 4px;
            }
            QPushButton:hover, QPushButton:pressed {
                background: qlineargradient(x1:0,y1:0,x2:0,y2:1,
                    stop:0 #607D8B, stop:1 #546E7A);
            }
        )");

        layout->addSpacing(4);
        layout->addWidget(energyDisplay);

        if (isPrimary) {
            p1EnergyLabel = energyDisplay;
        } else {
            p2EnergyLabel = energyDisplay;
        }

        return panel;
    };

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
    connect(restartAction, &QAction::triggered, this, [this]() {
        if (m_isNetworkGame && m_network && m_network->isConnected()) {
            QJsonObject message;
            message["type"] = "restart";
            m_network->sendMessage(message);
            startNetworkGame(m_localIsBlack);
        } else {
            onStartPVPClicked();
        }
    });
    gameMenu->addAction(restartAction);

    update();
}

void MainWindow::onStartPVPClicked()
{
    m_isNetworkGame = false;
    m_localIsBlack = true;
    if (m_network) {
        m_network->disconnectFromPeer();
    }
    startGame(PVP_MODE);
}

void MainWindow::onNetworkBattleClicked()
{
    showNetworkDialog();
}

void MainWindow::showNetworkDialog()
{
    QDialog dlg(this);
    dlg.setWindowTitle("联网对战");
    dlg.setFixedSize(430, 360);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(18, 16, 18, 18);
    layout->setSpacing(12);

    QLabel *tips = new QLabel("局域网对战需要两台电脑连接到同一个网络。创建房间的一方执黑先手，加入房间的一方执白。", &dlg);
    tips->setWordWrap(true);
    tips->setStyleSheet("font-size: 13px; color: #444;");
    layout->addWidget(tips);

    QLabel *localIpLabel = new QLabel("本机 IP：" + m_network->localAddressText(), &dlg);
    localIpLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    localIpLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #333;");
    layout->addWidget(localIpLabel);

    QGroupBox *hostGroup = new QGroupBox("创建房间", &dlg);
    QHBoxLayout *hostLayout = new QHBoxLayout(hostGroup);
    QSpinBox *hostPort = new QSpinBox(hostGroup);
    hostPort->setRange(1024, 65535);
    hostPort->setValue(6666);
    QPushButton *hostBtn = new QPushButton("创建并等待", hostGroup);
    hostLayout->addWidget(new QLabel("端口", hostGroup));
    hostLayout->addWidget(hostPort);
    hostLayout->addWidget(hostBtn);
    layout->addWidget(hostGroup);

    QGroupBox *joinGroup = new QGroupBox("加入房间", &dlg);
    QFormLayout *joinLayout = new QFormLayout(joinGroup);
    QLineEdit *ipEdit = new QLineEdit(joinGroup);
    ipEdit->setPlaceholderText("例如：192.168.1.10");
    QSpinBox *joinPort = new QSpinBox(joinGroup);
    joinPort->setRange(1024, 65535);
    joinPort->setValue(6666);
    QPushButton *joinBtn = new QPushButton("连接房间", joinGroup);
    joinLayout->addRow("对方 IP", ipEdit);
    joinLayout->addRow("端口", joinPort);
    joinLayout->addRow(joinBtn);
    layout->addWidget(joinGroup);

    QLabel *statusLabel = new QLabel("请选择创建房间或加入房间。", &dlg);
    statusLabel->setWordWrap(true);
    statusLabel->setStyleSheet("font-size: 13px; color: #555;");
    layout->addWidget(statusLabel);

    QPushButton *cancelBtn = new QPushButton("返回主菜单", &dlg);
    layout->addWidget(cancelBtn);

    connect(hostBtn, &QPushButton::clicked, this, [this, hostPort, statusLabel]() {
        if (m_network->host(static_cast<quint16>(hostPort->value()))) {
            statusLabel->setText("房间已创建。请让另一台电脑输入本机 IP 和端口加入。");
        }
    });
    connect(joinBtn, &QPushButton::clicked, this, [this, ipEdit, joinPort, statusLabel]() {
        if (ipEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "联网对战", "请输入创建房间电脑的 IP 地址。");
            return;
        }
        statusLabel->setText("正在连接房间...");
        m_network->join(ipEdit->text().trimmed(), static_cast<quint16>(joinPort->value()));
    });
    connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(m_network, &NetworkManager::statusChanged, statusLabel, &QLabel::setText);
    connect(m_network, &NetworkManager::connected, &dlg, &QDialog::accept);

    if (dlg.exec() != QDialog::Accepted && !m_network->isConnected()) {
        m_network->disconnectFromPeer();
    }
}

void MainWindow::startNetworkGame(bool localIsBlack)
{
    m_isNetworkGame = true;
    m_localIsBlack = localIsBlack;
    startGame(NETWORK_MODE);
    QMessageBox::information(this, "联网对战",
                             localIsBlack ? "连接成功！你是黑方，先手落子。"
                                          : "连接成功！你是白方，等待黑方落子。");
}


void MainWindow::onBackToMenuClicked()
{
    if (m_globalTimer) m_globalTimer->stop();
    if (m_isNetworkGame && m_network && m_network->isConnected()) {
        QJsonObject message;
        message["type"] = "back_to_menu";
        m_network->sendMessage(message);
    }
    m_isNetworkGame = false;
    if (m_network) {
        m_network->disconnectFromPeer();
    }
    // ⚠️【核心修复】：返回主菜单时，强制关闭未答完的弹窗并复位
    if (m_currentDlg) {
        // 先断开所有连接，防止触发 finished 槽引发二次调用
        m_currentDlg->disconnect();
        m_currentDlg->close();
        m_currentDlg = nullptr;
    }
    m_isAnswering = false;
    m_isEliminatingCross = false;
    m_isSkippingQuestion = false;
    setCursor(Qt::ArrowCursor); // 恢复正常光标
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
    setFixedSize(420, 520);
    menuWidget->setGeometry(0, 0, 420, 520);
    menuBar()->clear();
    menuWidget->show();
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    // 1. 同步能量条显示
    if (game) {
        if (p1EnergyLabel) {
            QString t1 = QString("%1:%2").arg(m_p1TimeLeft / 60, 2, 10, QChar('0')).arg(m_p1TimeLeft % 60, 2, 10, QChar('0'));
            p1EnergyLabel->setText(QString("⏱ %1\n⚡ 能量: %2").arg(t1).arg(game->p1Energy));
        }
        if (p2EnergyLabel) {
            QString t2 = QString("%1:%2").arg(m_p2TimeLeft / 60, 2, 10, QChar('0')).arg(m_p2TimeLeft % 60, 2, 10, QChar('0'));
            p2EnergyLabel->setText(QString("⏱ %1\n⚡ 能量: %2").arg(t2).arg(game->p2Energy));
        }
    }
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (currentMode == MENU_MODE)
    {
        QLinearGradient menuGrad(0, 0, 0, height());
        menuGrad.setColorAt(0, QColor(55, 55, 75));
        menuGrad.setColorAt(0.5, QColor(65, 55, 70));
        menuGrad.setColorAt(1, QColor(40, 40, 55));
        painter.fillRect(rect(), menuGrad);
        return;
    }

    // 画背景和棋盘格子线
    int boardWidth = kBoardMargin * 2 + kBlockSize * kBoardSizeNum;
    int boardHeight = boardWidth;
    int boardCenterX = boardOffsetX + boardWidth / 2;
    int boardCenterY = boardHeight / 2;

    // 木纹渐变背景
    QRadialGradient boardGrad(boardCenterX - 50, boardCenterY - 50, boardWidth / 2 + 50);
    boardGrad.setColorAt(0, QColor(235, 200, 148));
    boardGrad.setColorAt(0.5, QColor(215, 185, 140));
    boardGrad.setColorAt(1, QColor(190, 155, 110));
    painter.fillRect(rect(), boardGrad);

    // 棋盘边缘阴影
    QRect boardRect(boardOffsetX + kBoardMargin - 10, kBoardMargin - 10,
                    boardWidth - kBoardMargin * 2 + 20, boardHeight - kBoardMargin * 2 + 20);
    painter.setBrush(QColor(0, 0, 0, 30));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(boardRect.adjusted(4, 4, 4, 4), 6, 6);

    // 格子线：用深棕色更柔和
    painter.setPen(QPen(QColor(100, 75, 50), 1));
    for (int i = 0; i <= kBoardSizeNum; i++)
    {
        painter.drawLine(kBoardMargin + kBlockSize * i + boardOffsetX, kBoardMargin,
                         kBoardMargin + kBlockSize * i + boardOffsetX, boardHeight - kBoardMargin);
        painter.drawLine(kBoardMargin + boardOffsetX, kBoardMargin + kBlockSize * i,
                         boardWidth - kBoardMargin + boardOffsetX, kBoardMargin + kBlockSize * i);
    }

    // 画星位天元
    QBrush starBrush(QColor(80, 60, 40));
    painter.setBrush(starBrush);
    painter.setPen(Qt::NoPen);
    int starPos[5][2] = {{3,3}, {3,11}, {7,7}, {11,3}, {11,11}};
    for (auto& pos : starPos) {
        int sx = boardOffsetX + kBoardMargin + pos[1] * kBlockSize + kBlockSize / 2;
        int sy = kBoardMargin + pos[0] * kBlockSize + kBlockSize / 2;
        painter.drawEllipse(sx - 4, sy - 4, 8, 8);
    }

    // =================================================================
    // 悬停预览：半透明圆形棋子
    // =================================================================
    if (!m_isAnswering && clickPosRow >= 0 && clickPosRow < kBoardSizeNum &&
        clickPosCol >= 0 && clickPosCol < kBoardSizeNum &&
        game && game->gameMapVec[clickPosRow][clickPosCol] == 0)
    {
        int hx = boardOffsetX + kBoardMargin + clickPosCol * kBlockSize + kBlockSize / 2;
        int hy = kBoardMargin + clickPosRow * kBlockSize + kBlockSize / 2;

        if (game->playerFlag)
            painter.setBrush(QColor(0, 0, 0, 80));
        else
            painter.setBrush(QColor(255, 255, 255, 100));

        painter.setPen(Qt::NoPen);
        painter.drawEllipse(hx - kRadius, hy - kRadius, kRadius * 2, kRadius * 2);
    }

    // =================================================================
    // 遍历棋盘绘制棋子与红叉
    // =================================================================
    if (game)
    {
        for (int i = 0; i < kBoardSizeNum; i++) {
            for (int j = 0; j < kBoardSizeNum; j++) {

                int centerX = boardOffsetX + kBoardMargin + j * kBlockSize + (kBlockSize / 2);
                int centerY = kBoardMargin + i * kBlockSize + (kBlockSize / 2);

                if (game->gameMapVec[i][j] == 1) // 白子
                {
                    // 阴影
                    painter.setBrush(QColor(0, 0, 0, 35));
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(centerX - kRadius + 3, centerY - kRadius + 3, kRadius * 2, kRadius * 2);
                    // 3D 渐变主体
                    QRadialGradient wg(centerX - 4, centerY - 4, kRadius);
                    wg.setColorAt(0, Qt::white);
                    wg.setColorAt(0.6, QColor(240, 240, 240));
                    wg.setColorAt(1, QColor(200, 200, 200));
                    painter.setBrush(wg);
                    painter.drawEllipse(centerX - kRadius, centerY - kRadius, kRadius * 2, kRadius * 2);
                    // 细边框
                    painter.setPen(QPen(QColor(180, 180, 180), 1));
                    painter.setBrush(Qt::NoBrush);
                    painter.drawEllipse(centerX - kRadius, centerY - kRadius, kRadius * 2, kRadius * 2);
                }
                else if (game->gameMapVec[i][j] == -1) // 黑子
                {
                    // 阴影
                    painter.setBrush(QColor(0, 0, 0, 55));
                    painter.setPen(Qt::NoPen);
                    painter.drawEllipse(centerX - kRadius + 3, centerY - kRadius + 3, kRadius * 2, kRadius * 2);
                    // 3D 渐变主体
                    QRadialGradient bg(centerX - 4, centerY - 4, kRadius);
                    bg.setColorAt(0, QColor(100, 100, 100));
                    bg.setColorAt(0.5, QColor(50, 50, 50));
                    bg.setColorAt(1, Qt::black);
                    painter.setBrush(bg);
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
                finishGame("胜利", str + " 获胜!", str + "获胜");
            }
        }

        if (game->gameStatus == PLAYING && game->isDeadGame())
        {
            game->gameStatus = DEAD;
            finishGame("平局", "和棋!", "平局");
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!game || currentMode == MENU_MODE) return;
    if (m_isNetworkGame && !isLocalTurn()) {
        setCursor(Qt::ForbiddenCursor);
        clickPosRow = -1;
        clickPosCol = -1;
        update();
        return;
    }
    if (m_isEliminatingCross) {
        setCursor(Qt::CrossCursor);
        return;
    }

    // 1. 严格定义棋盘的实际像素范围
    // 起点：棋盘左上角第一个格子
    int startX = kBoardMargin + boardOffsetX;
    int startY = kBoardMargin;
    // 宽度和高度：正好是 15个格子 (kBlockSize * kBoardSizeNum)
    int boardDrawWidth = kBlockSize * kBoardSizeNum;

    // 这个矩形只覆盖棋盘格子区域，不包含边框间隙
    QRect boardRect(startX, startY, boardDrawWidth, boardDrawWidth);

    // 2. 答题状态处理
    if (m_isAnswering) {
        // 使用 event->pos() 和 boardRect 判断
        if (boardRect.contains(event->pos())) {
            setCursor(Qt::ForbiddenCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        return;
    }

    // 3. 正常逻辑：恢复正常光标
    setCursor(Qt::ArrowCursor);

    // 计算鼠标相对于棋盘左上角起点的偏移
    int relativeX = event->position().x() - startX;
    int relativeY = event->position().y() - startY;

    clickPosRow = -1;
    clickPosCol = -1;

    // 只有在棋盘格子内才计算
    if (relativeX >= 0 && relativeX < boardDrawWidth &&
        relativeY >= 0 && relativeY < boardDrawWidth) {

        clickPosCol = relativeX / kBlockSize;
        clickPosRow = relativeY / kBlockSize;
    }

    update();
}
void MainWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isNetworkGame && !isLocalTurn()) {
        QMessageBox::information(this, "联网对战", "现在是对方回合，请等待对方操作。");
        return;
    }
    if (m_isEliminatingCross) {
        int x = event->position().x() - boardOffsetX;
        int y = event->position().y();

        int row = (y - kBoardMargin) / kBlockSize;
        int col = (x - kBoardMargin) / kBlockSize;

        // 如果点到了棋盘外面，直接无视
        if (row < 0 || row >= kBoardSizeNum || col < 0 || col >= kBoardSizeNum) {
            QMessageBox::information(this, "消除红叉", "请点击棋盘上的红叉格子。");
            return;
        }

        // 检查点击的格子是不是红叉
        bool clickedOnCross = false;
        int crossIndex = -1;
        for (int i = 0; i < static_cast<int>(wrongPositions.size()); ++i) {
            if (wrongPositions[i].first == row && wrongPositions[i].second == col) {
                clickedOnCross = true;
                crossIndex = i;
                break;
            }
        }

        if (clickedOnCross) {
            int& currentEnergy = game->playerFlag ? game->p1Energy : game->p2Energy;
            if (currentEnergy < 2) {
                m_isEliminatingCross = false;
                setCursor(Qt::ArrowCursor);
                QMessageBox::warning(this, "提示", "能量不足！消除红叉需要 2 能量。");
                return;
            }

            // 1. 的确是红叉！将其从封锁列表中彻底移除
            wrongPositions.erase(wrongPositions.begin() + crossIndex);

            // 2. 扣除当前回合玩家的 2 点能量
            currentEnergy -= 2;

            // 3. 退出消除模式，恢复正常光标并更新界面
            m_isEliminatingCross = false;
            setCursor(Qt::ArrowCursor);

            QMessageBox::information(this, "技能成功", "成功消除该格子的封锁！现在可以重新选择此格子了。");
            update(); // 触发重绘，红叉消失
            sendGameState("remove_cross");
        } else {
            QMessageBox::information(this, "消除红叉", "这里不是红叉格子，请点击棋盘上的红叉。");
        }
        return; // ⚠️ 极其关键：消除红叉处理完后直接截断，不允许往下走落子答题流程！
    }
    if (m_isSkippingQuestion) {
        int x = event->position().x() - boardOffsetX;
        int y = event->position().y();

        int row = (y - kBoardMargin) / kBlockSize;
        int col = (x - kBoardMargin) / kBlockSize;

        // 1. 点到棋盘外，无事发生
        if (row < 0 || row >= kBoardSizeNum || col < 0 || col >= kBoardSizeNum) return;

        // 2. 如果点到了红叉封锁区域，提示并拦截
        for (const auto& pos : wrongPositions) {
            if (row == pos.first && col == pos.second) {
                QMessageBox::warning(this, "已被封锁", "该格子已被红叉封锁！即使跳过答题也不能下在这里。");
                return;
            }
        }

        // 3. 检查棋规落子合法性（是否有子）
        if (!game->isValidMove(row, col)) return;

        // 4. 条件全部满足，开始结算：
        pendingRow = row;
        pendingCol = col;

        // 扣除当前回合玩家的 4 点能量
        if (game->playerFlag) {
            game->p1Energy -= 4; // P1回合
        } else {
            game->p2Energy -= 4; // P2回合
        }

        // 退出技能模式，恢复正常箭头光标
        m_isSkippingQuestion = false;
        setCursor(Qt::ArrowCursor);

        // 5. Direct Call！绕过创建弹窗，直接调用结算落子函数，强行落子成功
        // 传入 (true, false) 代表答题正确，且未超时
        handleAnswerResult(true, false);

        return; // ⚠️ 极其关键：拦截后续正常的普通点击弹窗流程！
    }
    if (m_isAnswering) return;
    if (!game || currentMode == MENU_MODE || game->gameStatus != PLAYING) return;

    int x = event->position().x() - boardOffsetX;
    int y = event->position().y();

    int row = (y - kBoardMargin) / kBlockSize;
    int col = (x - kBoardMargin) / kBlockSize;

    if (row < 0 || row >= kBoardSizeNum || col < 0 || col >= kBoardSizeNum) return;

    // 遍历红叉列表，点中任何一个历史红叉都直接拦截
    for (const auto& pos : wrongPositions) {
        if (row == pos.first && col == pos.second) {
            QMessageBox::warning(this, "已被封锁", "这个格子你刚才已经答错被封锁了！请选其他格子。");
            return;
        }
    }

    if (!game->isValidMove(row, col)) return;

    pendingRow = row;
    pendingCol = col;

    // 准备答题，立刻暂停全局倒计时
    m_globalTimer->stop();
    m_isAnswering = true;

    Question testQ = getRandomQuestion();
    m_currentQuestion = testQ;
    m_hasCurrentQuestion = true;

    // ⚠️【核心修复】：只允许 new 一次！让全局指针指向它
    m_currentDlg = new QuestionDialog(testQ, this);

    // 连接信号槽，把结果传给 handleAnswerResult（统一使用 m_currentDlg）
    connect(m_currentDlg, &QuestionDialog::answerFinished, this, &MainWindow::handleAnswerResult);

    m_currentDlg->setAttribute(Qt::WA_DeleteOnClose);
    m_currentDlg->show();

    // 设置限时
    int limit = 20; // 根据难度设置...
    m_currentDlg->setTimeLimit(limit);
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

// 2. 随机抽题函数
Question MainWindow::getRandomQuestion()
{
    // 【边界情况处理】：如果某种难度+某个知识点组合下，一题都没有，怎么办？
    if (m_filteredQuestionBank.isEmpty()) {
        QMessageBox::warning(this, "题库枯竭", "当前选择的难度和知识点下没有题目！将为您随机抽取其他题目。");

        // 兜底方案：退回到从总题库抽，防止游戏崩溃
        if (!m_questionBank.isEmpty()) {
            int randomIndex = QRandomGenerator::global()->bounded(m_questionBank.size());
            return m_questionBank.at(randomIndex);
        } else {
            // 最惨的情况：总题库也是空的
            Question fallbackQ;
            fallbackQ.questionText = "题库加载失败或为空？";
            fallbackQ.options = {"A. 是", "B. 否", "C. 也许", "D. 不确定"};
            fallbackQ.correctAnswer = 0;
            return fallbackQ;
        }
    }

    // 正常情况：从过滤后的题库中随机抽取
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
void MainWindow::onGlobalTimerTick()
{
    if (!game || game->gameStatus != PLAYING) {
        m_globalTimer->stop();
        return;
    }
    // 假设 playerFlag: true为玩家1(黑), false为玩家2(白)。谁的回合扣谁的时间
    if (game->playerFlag) {
        m_p1TimeLeft--;
        if (m_p1TimeLeft <= 0) handleGlobalTimeout();
    } else {
        m_p2TimeLeft--;
        if (m_p2TimeLeft <= 0) handleGlobalTimeout();
    }
    update(); // 触发 paintEvent 刷新界面时间
}

void MainWindow::handleGlobalTimeout()
{
    m_globalTimer->stop();
    game->gameStatus = WIN;
    sendGameState("timeout_win");
    QString winner = game->playerFlag ? "白方" : "黑方"; // 谁的时间耗尽，对手获胜
    finishGame("超时结束", QString("时间耗尽！%1 获胜！").arg(winner), winner + "获胜");
}
void MainWindow::handleAnswerResult(bool isCorrect, bool isTimeout)
{
    m_isAnswering = false;

    // 这里放你原来写在 dlg.exec() 后面的那堆结算逻辑
    if (isCorrect)
    {
        game->actionByPerson(pendingRow, pendingCol);
        wrongPositions.clear();
        m_hasCurrentQuestion = false;
        // ⚠️【核心修改】改成黑色背景的“回答正确”提示框
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("回答正确");
        msgBox.setText("回答正确，成功落子！并为你积攒了能量。");
        msgBox.setStyleSheet("QMessageBox { background-color: black; } QLabel { color: white; font-weight: bold; } QPushButton { background-color: #333; color: white; }");
        msgBox.exec();
        if (game->playerFlag) game->p2Energy += 1; else game->p1Energy += 1;

        if (game->isWin(pendingRow, pendingCol) && game->gameStatus == PLAYING) {
            game->gameStatus = WIN;
            int piece = game->gameMapVec[pendingRow][pendingCol];
            QString winner = (piece == 1) ? "白方" : "黑方";
            sendGameState("game_over");
            finishGame("胜利", winner + " 获胜!", winner + "获胜");
            m_currentDlg = nullptr;
            update();
            return;
        }
        if (game->isDeadGame() && game->gameStatus == PLAYING) {
            game->gameStatus = DEAD;
            sendGameState("game_over");
            finishGame("平局", "和棋!", "平局");
            m_currentDlg = nullptr;
            update();
            return;
        }
    }
    else
    {
        wrongPositions.push_back({pendingRow, pendingCol});
        recordWrongQuestion(isTimeout);
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("回答错误");
        if (isTimeout) {
            msgBox.setText("答题超时！该格子已被红叉封锁，本回合落子失败。");
        } else {
            msgBox.setText("回答错误！该格子已被红叉封锁，本回合落子失败。");
        }
        msgBox.setStyleSheet("QMessageBox { background-color: black; } QLabel { color: white; font-weight: bold; } QPushButton { background-color: #333; color: white; }");
        msgBox.exec();
    }

    // ⚠️【核心3】答题结束，恢复全局倒计时！（此时如果落子成功换了手，就会自动开始扣对手的时间）
    if (game->gameStatus == PLAYING) {
        m_globalTimer->start(1000);
    }
    m_currentDlg = nullptr;
    update();
    sendGameState(isCorrect ? "answer_correct" : (isTimeout ? "answer_timeout" : "answer_wrong"));
}

QJsonObject MainWindow::questionToJson(const Question &question) const
{
    QJsonObject object;
    object["id"] = question.id;
    object["questionText"] = question.questionText;
    QJsonArray options;
    for (const QString &option : question.options) {
        options.append(option);
    }
    object["options"] = options;
    object["correctAnswer"] = question.correctAnswer;
    object["category"] = question.category;
    object["difficulty"] = question.difficulty;
    if (question.correctAnswer >= 0 && question.correctAnswer < question.options.size()) {
        object["correctOption"] = question.options[question.correctAnswer];
    }
    return object;
}

Question MainWindow::questionFromJson(const QJsonObject &object) const
{
    Question question;
    question.id = object["id"].toInt();
    question.questionText = object["questionText"].toString();
    QJsonArray options = object["options"].toArray();
    for (const QJsonValue &value : options) {
        question.options.append(value.toString());
    }
    question.correctAnswer = object["correctAnswer"].toInt();
    question.category = object["category"].toString();
    question.difficulty = object["difficulty"].toInt();
    return question;
}

void MainWindow::recordWrongQuestion(bool isTimeout)
{
    if (!m_hasCurrentQuestion) {
        return;
    }

    QJsonObject record;
    record["time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    record["player"] = game && game->playerFlag ? "黑方" : "白方";
    record["row"] = pendingRow;
    record["col"] = pendingCol;
    record["reason"] = isTimeout ? "timeout" : "wrong";
    record["question"] = questionToJson(m_currentQuestion);
    if (m_currentQuestion.correctAnswer >= 0 &&
        m_currentQuestion.correctAnswer < m_currentQuestion.options.size()) {
        record["correctOption"] = m_currentQuestion.options[m_currentQuestion.correctAnswer];
    }
    m_wrongQuestionRecords.append(record);
    m_hasCurrentQuestion = false;
}

QString MainWindow::exportReviewFile(const QString &resultText)
{
    if (m_reviewExported) {
        return QString();
    }
    m_reviewExported = true;

    QDir dir(QCoreApplication::applicationDirPath());
    if (!dir.exists("review")) {
        dir.mkdir("review");
    }
    dir.cd("review");

    QString fileName = "review_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".json";
    QString filePath = dir.filePath(fileName);

    QJsonObject root;
    root["gameTitle"] = "五子棋编程对战";
    root["mode"] = m_isNetworkGame ? "联网对战" : "本地对战";
    root["result"] = resultText;
    root["exportTime"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    root["wrongQuestionCount"] = m_wrongQuestionRecords.size();
    root["wrongQuestions"] = m_wrongQuestionRecords;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "复盘导出失败", "无法创建复盘文件。");
        return QString();
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    return filePath;
}

void MainWindow::showWrongQuestionReview()
{
    if (m_wrongQuestionRecords.isEmpty()) {
        QMessageBox::information(this, "错题复盘", "本局没有错题，表现不错！");
        return;
    }

    QDialog dlg(this);
    dlg.setFixedSize(520, 430);
    dlg.setStyleSheet(R"(
        QDialog {
            background-color: #F5F0E8;
        }
        QLabel#metaLabel {
            color: #6B5B3E;
            font-size: 13px;
            font-weight: bold;
        }
        QLabel#questionLabel {
            font-size: 15px;
            color: #2C2C2C;
            padding: 14px 18px;
            background: white;
            border: 1px solid #E0D5C0;
            border-radius: 8px;
            min-height: 58px;
        }
    )");

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(18, 14, 18, 18);
    layout->setSpacing(10);

    QLabel *metaLabel = new QLabel(&dlg);
    metaLabel->setObjectName("metaLabel");
    layout->addWidget(metaLabel);

    QLabel *qLabel = new QLabel(&dlg);
    qLabel->setObjectName("questionLabel");
    qLabel->setWordWrap(true);
    layout->addWidget(qLabel);

    QList<QPushButton*> optionButtons;
    for (int i = 0; i < 4; ++i) {
        QPushButton *btn = new QPushButton(&dlg);
        btn->setEnabled(false);
        layout->addWidget(btn);
        optionButtons.append(btn);
    }

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *skipBtn = new QPushButton("跳过复盘", &dlg);
    QPushButton *prevBtn = new QPushButton("上一题", &dlg);
    QPushButton *nextBtn = new QPushButton("下一题", &dlg);
    buttonLayout->addWidget(skipBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(prevBtn);
    buttonLayout->addWidget(nextBtn);
    layout->addLayout(buttonLayout);

    int currentIndex = 0;
    auto refreshReviewPage = [&]() {
        QJsonObject record = m_wrongQuestionRecords[currentIndex].toObject();
        Question question = questionFromJson(record["question"].toObject());
        QString reason = record["reason"].toString() == "timeout" ? "答题超时" : "回答错误";

        dlg.setWindowTitle(QString("错题复盘 %1/%2").arg(currentIndex + 1).arg(m_wrongQuestionRecords.size()));
        metaLabel->setText(QString("答错方：%1    原因：%2    知识点：%3")
                               .arg(record["player"].toString())
                               .arg(reason)
                               .arg(question.category));
        qLabel->setText(question.questionText);

        for (int optionIndex = 0; optionIndex < optionButtons.size(); ++optionIndex) {
            QPushButton *btn = optionButtons[optionIndex];
            if (optionIndex >= question.options.size()) {
                btn->hide();
                continue;
            }

            btn->show();
            if (optionIndex == question.correctAnswer) {
                btn->setText(question.options[optionIndex] + "    √ 正确答案");
                btn->setStyleSheet(R"(
                    QPushButton {
                        background-color: #E8F5E9;
                        color: #1B5E20;
                        font-size: 14px;
                        font-weight: bold;
                        padding: 10px 18px;
                        border: 2px solid #4CAF50;
                        border-radius: 8px;
                        text-align: left;
                    }
                )");
            } else {
                btn->setText(question.options[optionIndex]);
                btn->setStyleSheet(R"(
                    QPushButton {
                        background-color: white;
                        color: #666;
                        font-size: 14px;
                        padding: 10px 18px;
                        border: 1px solid #D4C5A0;
                        border-radius: 8px;
                        text-align: left;
                    }
                )");
            }
        }

        prevBtn->setEnabled(currentIndex > 0);
        nextBtn->setText(currentIndex + 1 == m_wrongQuestionRecords.size() ? "完成复盘" : "下一题");
    };

    connect(skipBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
    connect(prevBtn, &QPushButton::clicked, &dlg, [&]() {
        if (currentIndex > 0) {
            --currentIndex;
            refreshReviewPage();
        }
    });
    connect(nextBtn, &QPushButton::clicked, &dlg, [&]() {
        if (currentIndex + 1 >= m_wrongQuestionRecords.size()) {
            dlg.accept();
            return;
        }
        ++currentIndex;
        refreshReviewPage();
    });

    refreshReviewPage();
    dlg.exec();
}

void MainWindow::finishGame(const QString &title, const QString &message, const QString &resultText)
{
    QString reviewPath = exportReviewFile(resultText);
    QString finalMessage = message;
    if (!reviewPath.isEmpty()) {
        finalMessage += "\n\n本局错题复盘已导出：\n" + reviewPath;
    }

    QMessageBox::StandardButton btnValue = QMessageBox::information(this, title, finalMessage);
    if (btnValue == QMessageBox::Ok) {
        showWrongQuestionReview();
        QTimer::singleShot(0, this, SLOT(onBackToMenuClicked()));
    }
}

bool MainWindow::isLocalTurn() const
{
    if (!m_isNetworkGame || !game) {
        return true;
    }
    return game->playerFlag == m_localIsBlack;
}

QJsonObject MainWindow::buildGameState(const QString &reason) const
{
    QJsonObject root;
    root["type"] = "state";
    root["reason"] = reason;

    if (!game) {
        return root;
    }

    root["playerFlag"] = game->playerFlag;
    root["gameStatus"] = static_cast<int>(game->gameStatus);
    root["p1Energy"] = game->p1Energy;
    root["p2Energy"] = game->p2Energy;
    root["p1TimeLeft"] = m_p1TimeLeft;
    root["p2TimeLeft"] = m_p2TimeLeft;
    root["pendingRow"] = pendingRow;
    root["pendingCol"] = pendingCol;
    root["wrongQuestionRecords"] = m_wrongQuestionRecords;

    QJsonArray board;
    for (int i = 0; i < kBoardSizeNum; ++i) {
        QJsonArray row;
        for (int j = 0; j < kBoardSizeNum; ++j) {
            row.append(game->gameMapVec[i][j]);
        }
        board.append(row);
    }
    root["board"] = board;

    QJsonArray wrongArray;
    for (const auto &pos : wrongPositions) {
        QJsonArray item;
        item.append(pos.first);
        item.append(pos.second);
        wrongArray.append(item);
    }
    root["wrongPositions"] = wrongArray;

    return root;
}

void MainWindow::sendGameState(const QString &reason)
{
    if (!m_isNetworkGame || m_applyingRemoteState || !m_network || !m_network->isConnected()) {
        return;
    }
    m_network->sendMessage(buildGameState(reason));
}

void MainWindow::applyGameState(const QJsonObject &state)
{
    if (!game) {
        return;
    }

    m_applyingRemoteState = true;

    QJsonArray board = state["board"].toArray();
    for (int i = 0; i < kBoardSizeNum && i < board.size(); ++i) {
        QJsonArray row = board[i].toArray();
        for (int j = 0; j < kBoardSizeNum && j < row.size(); ++j) {
            game->gameMapVec[i][j] = row[j].toInt();
        }
    }

    game->playerFlag = state["playerFlag"].toBool(true);
    game->gameStatus = static_cast<GameStatus>(state["gameStatus"].toInt(PLAYING));
    game->p1Energy = state["p1Energy"].toInt(0);
    game->p2Energy = state["p2Energy"].toInt(0);
    m_p1TimeLeft = state["p1TimeLeft"].toInt(m_p1TimeLeft);
    m_p2TimeLeft = state["p2TimeLeft"].toInt(m_p2TimeLeft);
    pendingRow = state["pendingRow"].toInt(pendingRow);
    pendingCol = state["pendingCol"].toInt(pendingCol);

    wrongPositions.clear();
    QJsonArray wrongArray = state["wrongPositions"].toArray();
    for (const QJsonValue &value : wrongArray) {
        QJsonArray item = value.toArray();
        if (item.size() >= 2) {
            wrongPositions.push_back({item[0].toInt(), item[1].toInt()});
        }
    }

    m_wrongQuestionRecords = state["wrongQuestionRecords"].toArray();

    m_applyingRemoteState = false;
    update();
}

void MainWindow::onNetworkConnected()
{
    startNetworkGame(m_network->isHosting());
}

void MainWindow::onNetworkDisconnected()
{
    if (m_isNetworkGame) {
        QMessageBox::warning(this, "联网对战", "对方已断开连接，已返回主菜单。");
        onBackToMenuClicked();
    }
}

void MainWindow::onNetworkMessageReceived(const QJsonObject &message)
{
    QString type = message["type"].toString();
    if (type == "state") {
        applyGameState(message);
        int status = message["gameStatus"].toInt(PLAYING);
        QString reason = message["reason"].toString();
        if (status != PLAYING && !m_reviewExported) {
            if (status == DEAD) {
                finishGame("平局", "和棋!", "平局");
            } else if (reason == "timeout_win") {
                QString winner = game && game->playerFlag ? "白方" : "黑方";
                finishGame("超时结束", QString("时间耗尽！%1 获胜！").arg(winner), winner + "获胜");
            } else {
                int piece = 0;
                if (game && pendingRow >= 0 && pendingRow < kBoardSizeNum &&
                    pendingCol >= 0 && pendingCol < kBoardSizeNum) {
                    piece = game->gameMapVec[pendingRow][pendingCol];
                }
                QString winner = (piece == 1) ? "白方" : "黑方";
                finishGame("胜利", winner + " 获胜!", winner + "获胜");
            }
        }
    } else if (type == "restart") {
        startNetworkGame(m_localIsBlack);
    } else if (type == "back_to_menu") {
        QMessageBox::information(this, "联网对战", "对方已返回主菜单。");
        onBackToMenuClicked();
    }
}
