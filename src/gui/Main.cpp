#include <QApplication>

#include "Window.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    auto* window = new Window();
    window->show();

    return QApplication::exec();
}
