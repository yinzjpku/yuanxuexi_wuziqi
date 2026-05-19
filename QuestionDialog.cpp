#include "QuestionDialog.h"

// 补齐所有缺失的 Qt 核心布局和控件组件
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

QuestionDialog::QuestionDialog(const Question &q, QWidget *parent)
    : QDialog(parent), m_question(q), m_isCorrect(false)
{
    setWindowTitle("请答题以完成落子！");
    setFixedSize(400, 300);
    setWindowModality(Qt::WindowModal);

    QVBoxLayout *layout = new QVBoxLayout(this);

    // 显示题干（注意这里统一使用类的数组成员 m_question）
    QLabel *qLabel = new QLabel(m_question.questionText, this);
    qLabel->setWordWrap(true);
    layout->addWidget(qLabel);

    // 循环生成 A, B, C, D 四个选项按钮
    for (int i = 0; i < m_question.options.size(); ++i) {
        QPushButton *btn = new QPushButton(m_question.options[i], this);
        layout->addWidget(btn);

        // 核心修复：修正现代 Qt5/6 信号槽连接的 receiver 参数和 Lambda 捕获参数
        QObject::connect(btn, &QPushButton::clicked, this, [this, i]() {
            if (i == m_question.correctAnswer) {
                m_isCorrect = true;
            } else {
                m_isCorrect = false;
            }
            this->accept(); // 现在的 this 指向 QuestionDialog 对象，可以安全调用 accept()
        });
    }
}
