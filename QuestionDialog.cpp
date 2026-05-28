#include "QuestionDialog.h"
#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFont>

QuestionDialog::QuestionDialog(const Question &q, QWidget *parent)
    : QDialog(parent), m_question(q), m_isCorrect(false)
{
    setWindowTitle("请答题以完成落子！");
    setFixedSize(460, 340);
    setWindowModality(Qt::WindowModal);
    setStyleSheet(
        "QDialog {"
        "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "    stop:0 #ECEFF1, stop:1 #CFD8DC);"
        "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    QLabel *titleLabel = new QLabel("请答题以完成落子！", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 16px; font-weight: bold; color: #37474F;"
        "padding: 5px; background: transparent;"
    );
    layout->addWidget(titleLabel);

    QLabel *qLabel = new QLabel(m_question.questionText, this);
    qLabel->setWordWrap(true);
    qLabel->setMinimumHeight(60);
    qLabel->setStyleSheet(
        "font-size: 14px; color: #455A64; padding: 10px;"
        "background: white; border-radius: 8px;"
    );
    layout->addWidget(qLabel);

    layout->addSpacing(5);

    const QString colors[] = {"#1565C0", "#2E7D32", "#E65100", "#6A1B9A"};
    const QString hoverColors[] = {"#1976D2", "#388E3C", "#EF6C00", "#7B1FA2"};
    const QString labels[] = {"A.", "B.", "C.", "D."};

    for (int i = 0; i < m_question.options.size(); ++i) {
        QPushButton *btn = new QPushButton(labels[i] + "  " + m_question.options[i], this);
        btn->setStyleSheet(QString(
            "QPushButton {"
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            "    stop:0 %1, stop:1 %2);"
            "  color: white; font-size: 13px; font-weight: bold;"
            "  border: none; border-radius: 8px;"
            "  padding: 10px; text-align: left;"
            "}"
            "QPushButton:hover {"
            "  background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
            "    stop:0 %3, stop:1 %1);"
            "}"
            "QPushButton:pressed {"
            "  background-color: #333333;"
            "  padding-left: 13px;"
            "  padding-top: 12px;"
            "}"
        ).arg(colors[i % 4], colors[i % 4], hoverColors[i % 4]));

        QObject::connect(btn, &QPushButton::clicked, this, [this, i]() {
            if (i == m_question.correctAnswer) {
                m_isCorrect = true;
            } else {
                m_isCorrect = false;
            }
            this->accept();
        });
        layout->addWidget(btn);
    }
}
