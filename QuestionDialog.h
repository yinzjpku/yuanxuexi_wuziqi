#ifndef QUESTIONDIALOG_H
#define QUESTIONDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QString>
#include <QStringList>

// 彻底断绝外部包含，直接在这里定义结构体
struct Question {
    int id;
    QString questionText;
    QStringList options;
    int correctAnswer;
    QString category;
};

// 确保编译器百分之百认识 QDialog
class QuestionDialog : public QDialog
{
    Q_OBJECT
public:
    QuestionDialog(const Question &q, QWidget *parent = nullptr);
    bool isCorrect() const { return m_isCorrect; }

private:
    Question m_question;
    bool m_isCorrect;
};

#endif // QUESTIONDIALOG_H
