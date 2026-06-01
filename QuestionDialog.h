#ifndef QUESTIONDIALOG_H
#define QUESTIONDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QString>
#include <QStringList>
#include <QTimer>
// 彻底断绝外部包含，直接在这里定义结构体
struct Question {
    int id;
    QString questionText;
    QStringList options;
    int correctAnswer;
    QString category;
    int difficulty;
};

// 确保编译器百分之百认识 QDialog
class QuestionDialog : public QDialog
{
    Q_OBJECT
public:
    QuestionDialog(const Question &q, QWidget *parent = nullptr);
    bool isCorrect() const { return m_isCorrect; }
    void setTimeLimit(int seconds);
    bool isTimeout() const { return m_isTimeout; }
    // 在 QuestionDialog 类中增加 signals
signals:
    void answerFinished(bool isCorrect, bool isTimeout); // 答题结束信号
private slots:
    // ⚠️新增：倒计时处理槽函数
    void onTimerTick();
private:
    Question m_question;
    bool m_isCorrect;
    // ⚠️新增：限时相关的变量
    QTimer *m_timer;
    int m_timeLeft;
    bool m_isTimeout;
};

#endif // QUESTIONDIALOG_H
