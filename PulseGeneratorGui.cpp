#include <QApplication>
#include <QHBoxLayout>
#include <QSlider>
#include <QSpinBox>

#include "BasicPulseTrainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    BasicPulseTrainWindow* window = new BasicPulseTrainWindow;
    window->show();
    return app.exec();
}

