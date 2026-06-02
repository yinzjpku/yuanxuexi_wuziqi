#ifndef QUESTIONDIALOG_H
#define QUESTIONDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QProgressBar>
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
    void addTime(int secs) ;
    void forceAllowClose() { m_allowClose = true; }
    void excludeWrongOption();
protected:
    void closeEvent(QCloseEvent *event) override;
    void reject() override; // 拦截 Esc 键和右上角 X
    // 在 QuestionDialog 类中增加 signals
signals:
    void answerFinished(bool isCorrect, bool isTimeout); // 答题结束信号
private slots:
    // ⚠️新增：倒计时处理槽函数
    void onTimerTick();
private:
    Question m_question;
    bool m_isCorrect;
    QTimer *m_timer;
    int m_timeLeft;
    bool m_isTimeout;
    bool m_allowClose = false;
    QList<QPushButton*> m_optionButtons;
    QProgressBar *m_progressBar;
};

#endif // QUESTIONDIALOG_H
