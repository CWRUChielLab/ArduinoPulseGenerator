#include <QApplication>
#include <QHBoxLayout>
#include <QSlider>
#include <QSpinBox>

#include "ProgramGuiWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    ProgramGuiWindow* window = new ProgramGuiWindow();
    window->show();
    return app.exec();
}

