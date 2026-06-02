#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setStyleSheet(R"(
        * {
            font-family: "Microsoft YaHei", "PingFang SC", "SimHei", "Arial", sans-serif;
        }
        QDialog {
            background-color: #F5F0E8;
        }
        QComboBox {
            font-size: 14px;
            padding: 6px 12px;
            border: 2px solid #C8BFA8;
            border-radius: 6px;
            background: white;
            min-height: 24px;
        }
        QComboBox:hover {
            border-color: #A09070;
        }
        QComboBox::drop-down {
            border: none;
            width: 28px;
        }
        QComboBox::down-arrow {
            width: 10px;
            height: 10px;
        }
        QProgressBar {
            border: 2px solid #D4C5A0;
            border-radius: 8px;
            height: 22px;
            background: #EDE6D6;
            text-align: center;
            font-size: 12px;
            font-weight: bold;
            color: #333;
        }
        QProgressBar::chunk {
            border-radius: 6px;
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #4CAF50, stop:0.4 #8BC34A, stop:0.7 #FFC107, stop:1 #F44336);
        }
    )");

    MainWindow w;
    w.show();

    return a.exec();
}
