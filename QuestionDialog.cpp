#include "QuestionDialog.h"
#include <QCloseEvent>
#include <QRandomGenerator>
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
        m_optionButtons.append(btn);
        QObject::connect(btn, &QPushButton::clicked, this, [this, i]() {
            m_timer->stop();
            bool correct = (i == m_question.correctAnswer);
            emit answerFinished(correct, false); // 发送信号
            m_allowClose = true;
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
        m_allowClose = true;
        this->close();
    } else {
        setWindowTitle(QString("答题倒计时: %1 秒").arg(m_timeLeft)); // 实时更新标题栏
    }
}
void QuestionDialog::addTime(int secs)
{
    m_timeLeft += secs;

    // ⚠️ 修正：因为时间是显示在窗口标题上的，加完时间立刻同步刷新标题栏！
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
