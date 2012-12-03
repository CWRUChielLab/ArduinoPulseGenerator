#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPalette>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <qextserialport.h>
#include <qextserialenumerator.h>

#include "pulseStateMachine.h"
#include "ProgramGuiWindow.h"


// the default Qwt minimum plot size is far too large, so we need to
// subclass the plot.
class QwtShortPlot : public QwtPlot
{
    virtual QSize sizeHint()	const {
        return QSize(32, 32);
    }
    virtual QSize minimumSizeHint()	const {
        return QSize(32, 32);
    }
};


ProgramGuiWindow::ProgramGuiWindow(QWidget* parent) :
    QWidget(parent)
{
    // create the controls

    // first the program editor
    m_texteditProgram = new QTextEdit();
    //m_texteditProgram->setText(
    m_texteditProgram->insertPlainText(
            "repeat 3 times:\n"
            "\tset channel 1 to 15 ms pulses at 10 Hz\n"
            "\twait 1.5 s\n"
            "\tturn off channel 1\n"
            "\twait 500 ms\n"
            "end repeat\n"
            "end program\n"
            );
    m_texteditProgram->setLineWrapMode(QTextEdit::NoWrap);
    m_texteditProgram->setTabStopWidth(16);

    // then the plot
    m_plot = new QwtShortPlot();
    m_plot->setAxisScale(QwtPlot::yLeft,  -0.5 - numChannels, -0.5, 1);
    for (unsigned int i = 0; i < numChannels; ++i) {
        m_curves.push_back(new QwtPlotCurve("Channel " + QString::number(i)));
        m_curves.back()->attach(m_plot);
        m_points.push_back(QVector<QPointF>());
    }

    // then the status box
    m_texteditStatus = new QTextEdit();
    m_texteditStatus->setReadOnly(true);

    // Qt 4.4 seems to have drawing problems when scrolling with a gray
    // background on OS X.
#ifndef Q_WS_MAC
    // draw the read-only status box with a gray background.
    QPalette p = m_texteditStatus->palette();
    p.setBrush(QPalette::Base, QApplication::palette().window());
    m_texteditStatus->setPalette(p);
#endif

    // the port selection combo box...
    m_labelPort = new QLabel("Port");
    m_comboPort = new QComboBox();
    Q_FOREACH (QextPortInfo info, QextSerialEnumerator::getPorts())
        m_comboPort->addItem(info.portName);
    m_comboPort->setEditable(true);
    // the last port is much more likely to be the Arduino (since the first
    // ports are usually built-in serial ports), so we select the last by
    // default.
    m_comboPort->setCurrentIndex(m_comboPort->count() - 1);

    // and the buttons
    m_buttonHelp = new QPushButton("Help");
    m_buttonRun = new QPushButton("Run");
    m_buttonOpen = new QPushButton("Open");
    m_buttonSave = new QPushButton("Save");
    m_buttonSimulate = new QPushButton("Simulate");
    m_buttonRun = new QPushButton("Run on Device");

    m_checkboxLock = new QCheckBox("Lock");

    // lay out the controls
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_buttonHelp);
    buttonLayout->addWidget(m_buttonOpen);
    buttonLayout->addWidget(m_buttonSave);
    buttonLayout->addWidget(m_buttonSimulate);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_labelPort);
    buttonLayout->addWidget(m_comboPort);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_checkboxLock);
    buttonLayout->addWidget(m_buttonRun);

    m_tabs = new QTabWidget();
    m_tabs->addTab(m_texteditStatus, "Status");
    m_tabs->addTab(m_plot, "Simulation Results");
    m_tabs->setTabEnabled(m_tabs->indexOf(m_plot), false);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_texteditProgram);
    mainLayout->setStretchFactor(m_texteditProgram, 20);
    mainLayout->addWidget(m_tabs);
    //mainLayout->addWidget(m_plot);
    //mainLayout->addWidget(m_texteditStatus);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    // attach signals as needed
    QObject::connect(m_buttonHelp, SIGNAL(clicked()), this, SLOT(help()));
    QObject::connect(m_buttonOpen, SIGNAL(clicked()), this, SLOT(open()));
    QObject::connect(m_buttonSave, SIGNAL(clicked()), this, SLOT(save()));
    QObject::connect(m_buttonSimulate, SIGNAL(clicked()), this, SLOT(simulate()));
    QObject::connect(m_buttonRun, SIGNAL(clicked()), this, SLOT(run()));
    QObject::connect(m_comboPort, SIGNAL(editTextChanged(QString)), SLOT(onPortChanged()));
    QObject::connect(m_checkboxLock, SIGNAL(stateChanged(int)), SLOT(onLockStateChanged(int)));

    m_port = NULL;
    onPortChanged();
};


QSize ProgramGuiWindow::sizeHint() const {
    return QSize(600,700);
}


void ProgramGuiWindow::help() {
    QDesktopServices::openUrl(QUrl("http://kms15.github.com/ArduinoPulseGenerator/manual/"));
}


void ProgramGuiWindow::onNewSerialData() {
    if (m_port->bytesAvailable()) {
        QString newData = QString::fromUtf8(m_port->readAll()).replace("\n","");

        // If we're done, close the serial port.  The bell character (ascii
        // character 7) signals the end of the transmission or an error.
        // N.B.: this message must be kept in sync with
        // PulseGeneratorFirmware.pde
        if (newData.contains('\07')) {
            m_port->close();
            delete m_port;
            m_port = NULL;
        } else {
            // queue up one additional line per prompt
            int numPrompts = newData.count(':');
            while (numPrompts > 0 && !m_sendBuffer.isEmpty()) {
                m_port->write((m_sendBuffer.front() + "\n").toUtf8());
                m_sendBuffer.pop_front();
                --numPrompts;
            }
        }

        // display the new data
        m_texteditStatus->moveCursor(QTextCursor::End);
        m_texteditStatus->insertPlainText(newData);

    }
}


void ProgramGuiWindow::onPortChanged() {
}


void ProgramGuiWindow::open() {
    QString fileName = QFileDialog::getOpenFileName(this,
            tr("Open Pulse Sequence"), "",
            tr("Pulse Sequence (*.psq);;All Files (*)"));

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::information(this, tr("Unable to open file"),
                    file.errorString());
            return;
        }
        QTextStream in(&file);
        m_texteditProgram->setText(in.readAll());

        // disable the old simulation results and switch to the status tab
        m_tabs->setCurrentIndex(m_tabs->indexOf(m_texteditStatus));
        m_tabs->setTabEnabled(m_tabs->indexOf(m_plot), false);
    }
}


void ProgramGuiWindow::save() {
    QString fileName = QFileDialog::getSaveFileName(this,
            tr("Save Pulse Sequence"), "",
            tr("Pulse Sequence (*.psq);;All Files (*)"));

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file"),
                    file.errorString());
            return;
        }
        QTextStream out(&file);
        out << m_texteditProgram->toPlainText();
    }
}


void ProgramGuiWindow::simulate() {
    // disable the old simulation results and switch to the status tab
    m_tabs->setCurrentIndex(m_tabs->indexOf(m_texteditStatus));
    m_tabs->setTabEnabled(m_tabs->indexOf(m_plot), false);

    // clear any previous plot points
    for (unsigned int i = 0; i < numChannels; ++i) {
        m_points[i].clear();
    }


    m_texteditStatus->moveCursor(QTextCursor::End);
    m_texteditStatus->insertPlainText("\n\nParsing...\n");


    // Parse the program
    QStringList lines = m_texteditProgram->toPlainText().split('\n');
    QVector<PulseStateCommand> commands;
    const char* error = NULL;
    unsigned repeatDepth = 0;
    bool endProgramFound = 0;

    for (int i = 0; i < lines.size(); ++i) {
        m_texteditStatus->moveCursor(QTextCursor::End);
        m_texteditStatus->insertPlainText(QString::number(i+1) + "> " + lines[i] + "\n");
        if (lines[i].size() != 0) {
            commands.push_back(PulseStateCommand());
            commands.back().parseFromString(lines[i].toUtf8(), &error, &repeatDepth);

            if (!error) {
                if (commands.back().type == PulseStateCommand::endProgram) {
                    endProgramFound = true;
                } else if (endProgramFound &&
                        commands.back().type != PulseStateCommand::noOp) {
                    error = "unexpected command found after end of program";
                }
            }

            if (error) {
                m_texteditStatus->moveCursor(QTextCursor::End);
                m_texteditStatus->insertPlainText(QString::fromUtf8(error) + "\n");
                return;
            }
        }
    }

    if (!endProgramFound) {
        m_texteditStatus->moveCursor(QTextCursor::End);
        m_texteditStatus->insertPlainText("missing \"end program\"\n");
        return;
    }


    // run the program
    const float low = -0.4;
    const float high = 0.4;

    PulseChannel channels[numChannels];
    RepeatStack stack;
    int runningLine = 0;
    Microseconds time = 0;
    Microseconds timeInState = 0;
    int steps = 0;
    int maxSteps = 1000000;
    const float us = 1e-6f;

    // mark the starting state
    for (unsigned int i = 0; i < numChannels; ++i) {
        m_points[i].append(QPointF(time * us,
                    (channels[i].on() ? high : low) - i - 1));
    }

    while (commands[runningLine].type != PulseStateCommand::endProgram &&
            steps < maxSteps) {
        ++steps;

        // calculate the maximum amount of time before a channel changes
        Microseconds timeStep = forever;
        for (unsigned int i = 0; i < numChannels; ++i) {
            timeStep = std::min(timeStep, channels[i].timeUntilNextStateChange());
        }

        Microseconds commandTimeAvailable = timeStep;

        // if the command finishes, advance to the next command
        int step = commands[runningLine].execute(
                    channels, &stack, runningLine,
                    timeInState, &commandTimeAvailable);
        if (step != 0) {
            runningLine += step;
            timeInState = 0;
            timeStep -= commandTimeAvailable;
        } else {
            timeInState += timeStep;
        }

        time += timeStep;

        // mark the channel on/off state before the change
        for (unsigned int i = 0; i < numChannels; ++i) {
            m_points[i].append(QPointF(time * us,
                        (channels[i].on() ? high : low) - i - 1));
        }

        // update the channel states
        for (unsigned int i = 0; i < numChannels; ++i) {
            channels[i].advanceTime(timeStep);
        }

        // mark the channel on/off state after the change
        for (unsigned int i = 0; i < numChannels; ++i) {
            m_points[i].append(QPointF(time * us,
                        (channels[i].on() ? high : low) - i - 1));
        }
    }

    // add some extra time before and after the simulation to bracket
    // things nicely
    float timeStart = std::min(-0.025f * time * us, -1 * us);
    float timeEnd = std::max(((steps < maxSteps) ? 1.025f : 1) * time * us, 1 * us);
    m_plot->setAxisScale(QwtPlot::xBottom,  timeStart, timeEnd);
    for (unsigned int i = 0; i < numChannels; ++i) {
        m_points[i].prepend(QPointF(timeStart, low - i - 1));
        m_points[i].append(QPointF(timeEnd,
                    (channels[i].on() ? high : low) - i - 1));
    }


    // update the plot
    for (unsigned int i = 0; i < numChannels; ++i) {
        m_curves[i]->setSamples(m_points[i]);
    }
    m_plot->replot();

    // display the results in the simulation tab
    m_tabs->setTabEnabled(m_tabs->indexOf(m_plot), true);
    m_tabs->setCurrentIndex(m_tabs->indexOf(m_plot));

}


void ProgramGuiWindow::onLockStateChanged(int state) {
    m_buttonRun->setEnabled(state == Qt::Unchecked);
}


void ProgramGuiWindow::run() {
    // close the serial port, if one is currently open
    if (m_port) {
        m_port->close();
        delete m_port;
        m_port = NULL;
    }

    // create the serial port
    PortSettings settings = {BAUD9600, DATA_8, PAR_NONE, STOP_1, FLOW_OFF, 10};
    m_port = new QextSerialPort(m_comboPort->currentText(), settings, QextSerialPort::EventDriven);

    QObject::connect(m_port, SIGNAL(readyRead()), SLOT(onNewSerialData()));
    m_port->open(QIODevice::ReadWrite);

    // switch to the status tab
    m_tabs->setCurrentIndex(m_tabs->indexOf(m_texteditStatus));

    // split the text into a series of lines
    m_sendBuffer = m_texteditProgram->toPlainText().split('\n');

    // wait for the arduino to prompt us before sending the first line.

    // // send the first line (note that we can't safely send the entire program
    // // because the arduino has a very small receive buffer).
    // m_port->write((m_sendBuffer.front() + "\n").toUtf8());
    // m_sendBuffer.pop_front();
}


