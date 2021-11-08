#include "JpgToSerial.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    JpgToSerial w;
    w.show();
    return a.exec();
}
