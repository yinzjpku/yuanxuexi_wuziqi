#include "QuestionDialog.h"
#include <QCloseEvent>
#include <QRandomGenerator>
#include <QProgressBar>
QuestionDialog::QuestionDialog(const Question &q, QWidget *parent)
    : QDialog(parent), m_question(q), m_isCorrect(false), m_isTimeout(false), m_timeLeft(0)
{
    setWindowTitle("请答题以完成落子！");
    setFixedSize(440, 360);
    setStyleSheet(R"(
        QDialog#questionDlg {
            background-color: #F5F0E8;
        }
        QLabel#questionLabel {
            font-size: 15px;
            color: #2C2C2C;
            padding: 14px 18px;
            background: white;
            border: 1px solid #E0D5C0;
            border-radius: 8px;
            min-height: 50px;
        }
    )");
    setObjectName("questionDlg");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 14, 18, 18);
    layout->setSpacing(10);

    // 倒计时进度条
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 20);
    m_progressBar->setValue(20);
    m_progressBar->setFormat("剩余 %v 秒");
    layout->addWidget(m_progressBar);

    // 题目文本（卡片容器）
    QLabel *qLabel = new QLabel(m_question.questionText, this);
    qLabel->setObjectName("questionLabel");
    qLabel->setWordWrap(true);
    layout->addWidget(qLabel);

    layout->addSpacing(4);

    // 初始化单题定时器
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &QuestionDialog::onTimerTick);

    for (int i = 0; i < m_question.options.size(); ++i) {
        QPushButton *btn = new QPushButton(m_question.options[i], this);
        btn->setStyleSheet(R"(
            QPushButton {
                background-color: white;
                color: #333;
                font-size: 14px;
                padding: 10px 18px;
                border: 2px solid #D4C5A0;
                border-radius: 8px;
                text-align: left;
            }
            QPushButton:hover {
                background-color: #E8F0FE;
                border-color: #4A90D9;
            }
            QPushButton:pressed {
                background-color: #D0E0F0;
                padding-left: 14px;
                padding-top: 12px;
            }
            QPushButton:disabled {
                background-color: #F5F5F5;
                color: #B0B0B0;
                border-color: #E0E0E0;
            }
        )");
        layout->addWidget(btn);
        m_optionButtons.append(btn);
        QObject::connect(btn, &QPushButton::clicked, this, [this, i]() {
            m_timer->stop();
            bool correct = (i == m_question.correctAnswer);
            emit answerFinished(correct, false);
            m_allowClose = true;
            this->close();
        });
    }
}

void QuestionDialog::setTimeLimit(int seconds)
{
    m_timeLeft = seconds;
    m_progressBar->setRange(0, seconds);
    m_progressBar->setValue(seconds);
    setWindowTitle(QString("答题倒计时: %1 秒").arg(m_timeLeft));
    m_timer->start(1000);
}

void QuestionDialog::onTimerTick()
{
    m_timeLeft--;
    m_progressBar->setValue(m_timeLeft);
    setWindowTitle(QString("答题倒计时: %1 秒").arg(m_timeLeft));
    if (m_timeLeft <= 0) {
        m_timer->stop();
        emit answerFinished(false, true);
        m_allowClose = true;
        this->close();
    }
}

void QuestionDialog::addTime(int secs)
{
    m_timeLeft += secs;
    m_progressBar->setMaximum(m_progressBar->maximum() + secs);
    m_progressBar->setValue(m_timeLeft);
    setWindowTitle(QString("答题倒计时: %1 秒").arg(m_timeLeft));
}
// ⚠️【新增】重写 closeEvent：如果是用户点 X，直接无视掉
void QuestionDialog::closeEvent(QCloseEvent *event)
{
    if (m_allowClose) {
        QDialog::closeEvent(event); // 允许关闭
    } else {
        event->ignore(); // ❌ 拦截：点右上角 X 毫无反应
    }
}

// ⚠️【新增】重写 reject：如果是用户按 Esc 键或点 X，默认 QDialog 会调用 reject()，必须在这里拦截
void QuestionDialog::reject()
{
    if (m_allowClose) {
        QDialog::reject(); // 允许关闭
    } else {
        // ❌ 拦截：按 Esc 键毫无反应，什么也不做
    }
}
// ⚠️【新增】排除一个错误选项的实现
void QuestionDialog::excludeWrongOption()
{
    // 收集所有当前“还没被排除”且“本身是错误答案”的按钮索引
    QList<int> wrongIndices;
    for (int i = 0; i < m_optionButtons.size(); ++i) {
        // 如果这个索引不是正确答案，且按钮目前还是正常的（没被禁用的）
        if (i != m_question.correctAnswer && m_optionButtons[i]->isEnabled()) {
            wrongIndices.append(i);
        }
    }

    // 如果还有错误选项可以排除
    if (!wrongIndices.isEmpty()) {
        // 随机挑一个错误选项的索引
        int randomIndex = wrongIndices[QRandomGenerator::global()->bounded(wrongIndices.size())];

        // 把这个按钮禁用掉，并改一下文字提示玩家
        m_optionButtons[randomIndex]->setEnabled(false);
        m_optionButtons[randomIndex]->setText("[已排除] " + m_question.options[randomIndex]);
    }
}
