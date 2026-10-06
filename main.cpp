#include "MouseClickRecorder.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    MouseClickRecorder w;
    w.show();

    return a.exec();
}