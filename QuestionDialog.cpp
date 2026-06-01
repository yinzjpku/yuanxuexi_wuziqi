#include "QuestionDialog.h"

QuestionDialog::QuestionDialog(const Question &q, QWidget *parent)
    : QDialog(parent), m_question(q), m_isCorrect(false), m_isTimeout(false), m_timeLeft(0)
{
    setWindowTitle("请答题以完成落子！");
    setFixedSize(400, 300);

    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *qLabel = new QLabel(m_question.questionText, this);
    qLabel->setWordWrap(true);
    layout->addWidget(qLabel);

    // ⚠️新增：初始化单题定时器，但先不启动
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &QuestionDialog::onTimerTick);

    for (int i = 0; i < m_question.options.size(); ++i) {
        QPushButton *btn = new QPushButton(m_question.options[i], this);
        layout->addWidget(btn);

        QObject::connect(btn, &QPushButton::clicked, this, [this, i]() {
            m_timer->stop();
            bool correct = (i == m_question.correctAnswer);
            emit answerFinished(correct, false); // 发送信号
            this->close(); // 关闭弹窗
        });
    }
}

// ⚠️新增：接收主窗口传来的秒数并启动倒计时
void QuestionDialog::setTimeLimit(int seconds)
{
    m_timeLeft = seconds;
    setWindowTitle(QString("答题倒计时: %1 秒").arg(m_timeLeft));
    m_timer->start(1000); // 每 1000 毫秒 (1秒) 触发一次
}

// ⚠️新增：每秒流逝的处理
void QuestionDialog::onTimerTick()
{
    m_timeLeft--;
    if (m_timeLeft <= 0) {
        m_timer->stop();
        emit answerFinished(false, true); // 发送超时信号
        this->close();
    } else {
        setWindowTitle(QString("答题倒计时: %1 秒").arg(m_timeLeft)); // 实时更新标题栏
    }
}
