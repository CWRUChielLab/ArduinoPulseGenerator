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

const QString runButtonText = "Run on Device";
const QString interruptButtonText = "Interrupt Program";

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
    const int tabStopWidth = 16;

    // create the controls

    // first the program editor
    m_texteditProgram = new QTextEdit();
    //m_texteditProgram->setText(
    m_texteditProgram->insertPlainText(
            "# This is an example of a program that generates three pulse\n"
            "# trains of 10 Hz pulses.\n"
            "\n"
            "repeat 3 times:\n"
            "\tset channel 1 to 15 ms pulses at 10 Hz\n"
            "\twait 1.5 s\n"
            "\tturn off channel 1\n"
            "\twait 500 ms\n"
            "end repeat\n"
            "\n"
            "end program\n"
            );
    m_texteditProgram->setLineWrapMode(QTextEdit::NoWrap);
    m_texteditProgram->setTabStopWidth(tabStopWidth);

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
    m_texteditStatus->setLineWrapMode(QTextEdit::NoWrap);
    m_texteditStatus->setTabStopWidth(tabStopWidth);

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
    m_comboPort->setEditable(true);
    repopulatePortComboBox();
#ifdef Q_WS_MAC
    // The Arduino usually isn't the last serial port on the mac, but it
    // seems to be given the name cu.usbmodemXXXX where the Xs are some
    // hexidecimal number.
    for (int i = m_comboPort->count() - 1; i >= 0; --i) {
        if (m_comboPort->itemText(i).contains("usbmodem")) {
            m_comboPort->setCurrentIndex(i);
        }
    }
#else
    // the last port is much more likely to be the Arduino (since the first
    // ports are usually built-in serial ports), so we select the last by
    // default.
    m_comboPort->setCurrentIndex(m_comboPort->count() - 1);
#endif

    // and the buttons
    m_buttonHelp = new QPushButton("Help");
    m_buttonNew = new QPushButton("New");
    m_buttonOpen = new QPushButton("Open");
    m_buttonSave = new QPushButton("Save");
    m_buttonSimulate = new QPushButton("Simulate");
    m_buttonRun = new QPushButton(runButtonText);

    m_checkboxLock = new QCheckBox("Lock");

    // lay out the controls
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_buttonHelp);
    buttonLayout->addWidget(m_buttonNew);
    buttonLayout->addWidget(m_buttonOpen);
    buttonLayout->addWidget(m_buttonSave);
    buttonLayout->addWidget(m_buttonSimulate);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_labelPort);
    buttonLayout->addWidget(m_comboPort);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_checkboxLock);
    buttonLayout->addWidget(m_buttonRun);

    m_widgetTraditional = new QWidget();

    m_labelChannel = new QLabel("Channel");
    m_sliderChannel = new QSlider(Qt::Horizontal);
    m_sliderChannel->setRange(1, numChannels);
    m_spinChannel = new QSpinBox();
    m_spinChannel->setRange(1, numChannels);

    m_labelPulseWidth = new QLabel("Pulse Width");
    m_sliderPulseWidth = new QSlider(Qt::Horizontal);
    m_sliderPulseWidth->setRange(0, 1000);
    m_sliderPulseWidth->setValue(6);
    m_spinPulseWidth = new QDoubleSpinBox();
    m_spinPulseWidth->setRange(0, 99999.99);
    m_spinPulseWidth->setValue(m_sliderPulseWidth->value());
    m_comboPulseWidth = new QComboBox();
    m_comboPulseWidth->addItem("s");
    m_comboPulseWidth->addItem("ms");
    m_comboPulseWidth->addItem("\xB5s");
    m_comboPulseWidth->setCurrentIndex(1);

    m_labelPulseTrain = new QLabel("Pulse Train");
    m_checkboxPulseTrain = new QCheckBox();

    m_labelPulseFrequency = new QLabel("Pulse Frequency");
    m_sliderPulseFrequency = new QSlider(Qt::Horizontal);
    m_sliderPulseFrequency->setRange(0, 100);
    m_sliderPulseFrequency->setValue(10);
    m_spinPulseFrequency = new QDoubleSpinBox();
    m_spinPulseFrequency->setRange(0, 99999.99);
    m_spinPulseFrequency->setValue(m_sliderPulseFrequency->value());
    m_comboPulseFrequency = new QComboBox();
    m_comboPulseFrequency->addItem("Hz");
    m_comboPulseFrequency->addItem("kHz");
    m_comboPulseFrequency->setCurrentIndex(0);

    m_labelTrainDuration = new QLabel("Train Duration");
    m_sliderTrainDuration = new QSlider(Qt::Horizontal);
    m_sliderTrainDuration->setRange(0, 60);
    m_sliderTrainDuration->setValue(3);
    m_spinTrainDuration = new QDoubleSpinBox();
    m_spinTrainDuration->setRange(0, 99999.99);
    m_spinTrainDuration->setValue(m_sliderTrainDuration->value());
    m_comboTrainDuration = new QComboBox();
    m_comboTrainDuration->addItem("s");
    m_comboTrainDuration->addItem("ms");
    m_comboTrainDuration->addItem("\xB5s");
    m_comboTrainDuration->setCurrentIndex(0);

    m_labelNumTrains = new QLabel("Number of Trains");
    m_sliderNumTrains = new QSlider(Qt::Horizontal);
    m_sliderNumTrains->setRange(1, 20);
    m_sliderNumTrains->setValue(1);
    m_spinNumTrains = new QSpinBox();
    m_spinNumTrains->setRange(1, 99999);
    m_spinNumTrains->setValue(m_sliderNumTrains->value());

    m_labelTrainDelay = new QLabel("Delay between Trains");
    m_sliderTrainDelay = new QSlider(Qt::Horizontal);
    m_sliderTrainDelay->setRange(0, 300);
    m_sliderTrainDelay->setValue(20);
    m_spinTrainDelay = new QDoubleSpinBox();
    m_spinTrainDelay->setRange(0, 99999.99);
    m_spinTrainDelay->setValue(m_sliderTrainDelay->value());
    m_comboTrainDelay = new QComboBox();
    m_comboTrainDelay->addItem("s");
    m_comboTrainDelay->addItem("ms");
    m_comboTrainDelay->addItem("\xB5s");
    m_comboTrainDelay->setCurrentIndex(0);

    updateTraditionalDisabledControls();

    m_labelTraditionalProgram = new QLabel("Equivalent Program:");
    m_texteditTraditionalProgram = new QTextEdit();
    m_texteditTraditionalProgram->setReadOnly(true);
    m_texteditTraditionalProgram->setLineWrapMode(QTextEdit::NoWrap);
    m_texteditTraditionalProgram->setTabStopWidth(tabStopWidth);
    // Qt 4.4 seems to have drawing problems when scrolling with a gray
    // background on OS X.
#ifndef Q_WS_MAC
    // draw the read-only Traditional program box with a gray background.
    m_texteditTraditionalProgram->setPalette(p);
#endif
    updateEquivalentProgram();

    QGridLayout* layoutTraditional = new QGridLayout();
    int row = 0;
    layoutTraditional->addWidget(m_labelChannel, row, 0);
    layoutTraditional->addWidget(m_spinChannel, row, 1);
    layoutTraditional->addWidget(m_sliderChannel, row, 3);
    ++row;
    layoutTraditional->addWidget(m_labelPulseWidth, row, 0);
    layoutTraditional->addWidget(m_spinPulseWidth, row, 1);
    layoutTraditional->addWidget(m_comboPulseWidth, row, 2);
    layoutTraditional->addWidget(m_sliderPulseWidth, row, 3);
    ++row;
    layoutTraditional->addWidget(m_labelPulseTrain, row, 0);
    layoutTraditional->addWidget(m_checkboxPulseTrain, row, 1);
    ++row;
    layoutTraditional->addWidget(m_labelPulseFrequency, row, 0);
    layoutTraditional->addWidget(m_spinPulseFrequency, row, 1);
    layoutTraditional->addWidget(m_comboPulseFrequency, row, 2);
    layoutTraditional->addWidget(m_sliderPulseFrequency, row, 3);
    ++row;
    layoutTraditional->addWidget(m_labelTrainDuration, row, 0);
    layoutTraditional->addWidget(m_spinTrainDuration, row, 1);
    layoutTraditional->addWidget(m_comboTrainDuration, row, 2);
    layoutTraditional->addWidget(m_sliderTrainDuration, row, 3);
    ++row;
    layoutTraditional->addWidget(m_labelNumTrains, row, 0);
    layoutTraditional->addWidget(m_spinNumTrains, row, 1);
    layoutTraditional->addWidget(m_sliderNumTrains, row, 3);
    ++row;
    layoutTraditional->addWidget(m_labelTrainDelay, row, 0);
    layoutTraditional->addWidget(m_spinTrainDelay, row, 1);
    layoutTraditional->addWidget(m_comboTrainDelay, row, 2);
    layoutTraditional->addWidget(m_sliderTrainDelay, row, 3);
    ++row;

    QVBoxLayout* layoutTraditionalStretch = new QVBoxLayout;
    layoutTraditionalStretch->addLayout(layoutTraditional);
    layoutTraditionalStretch->addWidget(m_labelTraditionalProgram);
    layoutTraditionalStretch->addWidget(m_texteditTraditionalProgram);
    //layoutTraditionalStretch->addStretch();
    m_widgetTraditional->setLayout(layoutTraditionalStretch);

    m_tabsProgram = new QTabWidget();
    m_tabsProgram->addTab(m_texteditProgram, "<new program>");
    m_tabsProgram->addTab(m_widgetTraditional, "Traditional Controls");

    m_tabsOutput = new QTabWidget();
    m_tabsOutput->addTab(m_texteditStatus, "Status");
    m_tabsOutput->addTab(m_plot, "Simulation Results");
    m_tabsOutput->setTabEnabled(m_tabsOutput->indexOf(m_plot), false);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    //mainLayout->addWidget(m_texteditProgram);
    //mainLayout->setStretchFactor(m_texteditProgram, 20);
    mainLayout->addWidget(m_tabsProgram);
    mainLayout->setStretchFactor(m_tabsProgram, 20);
    mainLayout->addWidget(m_tabsOutput);
    //mainLayout->addWidget(m_plot);
    //mainLayout->addWidget(m_texteditStatus);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    // set up the serial port support
    m_port = NULL;
    m_portEnumerator = new QextSerialEnumerator(this);
    m_portEnumerator->setUpNotifications();

    // attach signals to main controls
    QObject::connect(m_buttonHelp, SIGNAL(clicked()), this, SLOT(help()));
    QObject::connect(m_buttonNew, SIGNAL(clicked()), this, SLOT(newDocument()));
    QObject::connect(m_buttonOpen, SIGNAL(clicked()), this, SLOT(open()));
    QObject::connect(m_buttonSave, SIGNAL(clicked()), this, SLOT(save()));
    QObject::connect(m_buttonSimulate, SIGNAL(clicked()), this, SLOT(simulate()));
    QObject::connect(m_buttonRun, SIGNAL(clicked()), this, SLOT(run()));
    QObject::connect(m_checkboxLock, SIGNAL(stateChanged(int)), SLOT(onLockStateChanged(int)));
    QObject::connect(m_portEnumerator, SIGNAL(deviceDiscovered(QextPortInfo)),
            SLOT(repopulatePortComboBox()));
    QObject::connect(m_portEnumerator, SIGNAL(deviceRemoved(QextPortInfo)),
            SLOT(repopulatePortComboBox()));

    // link traditional controls together
    QObject::connect(m_sliderChannel, SIGNAL(valueChanged(int)), m_spinChannel, SLOT(setValue(int)));
    QObject::connect(m_spinChannel, SIGNAL(valueChanged(int)), m_sliderChannel, SLOT(setValue(int)));
    QObject::connect(m_sliderNumTrains, SIGNAL(valueChanged(int)), m_spinNumTrains, SLOT(setValue(int)));
    QObject::connect(m_spinNumTrains, SIGNAL(valueChanged(int)), m_sliderNumTrains, SLOT(setValue(int)));
    QObject::connect(m_sliderPulseWidth, SIGNAL(valueChanged(int)), this, SLOT(changePulseWidth(int)));
    QObject::connect(m_spinPulseWidth, SIGNAL(valueChanged(double)), this, SLOT(changePulseWidth(double)));
    QObject::connect(m_sliderPulseFrequency, SIGNAL(valueChanged(int)), this, SLOT(changePulseFrequency(int)));
    QObject::connect(m_spinPulseFrequency, SIGNAL(valueChanged(double)), this, SLOT(changePulseFrequency(double)));
    QObject::connect(m_sliderTrainDuration, SIGNAL(valueChanged(int)), this, SLOT(changeTrainDuration(int)));
    QObject::connect(m_spinTrainDuration, SIGNAL(valueChanged(double)), this, SLOT(changeTrainDuration(double)));
    QObject::connect(m_sliderTrainDelay, SIGNAL(valueChanged(int)), this, SLOT(changeTrainDelay(int)));
    QObject::connect(m_spinTrainDelay, SIGNAL(valueChanged(double)), this, SLOT(changeTrainDelay(double)));
    QObject::connect(m_checkboxPulseTrain, SIGNAL(stateChanged(int)), SLOT(updateTraditionalDisabledControls()));
    QObject::connect(m_spinNumTrains, SIGNAL(valueChanged(int)), SLOT(updateTraditionalDisabledControls()));
    QObject::connect(m_sliderNumTrains, SIGNAL(valueChanged(int)), SLOT(updateTraditionalDisabledControls()));

    // update the program whenever a value changes
    QObject::connect(m_spinChannel, SIGNAL(valueChanged(int)), this, SLOT(updateEquivalentProgram()));
    QObject::connect(m_spinPulseWidth, SIGNAL(valueChanged(double)), this, SLOT(updateEquivalentProgram()));
    QObject::connect(m_comboPulseWidth, SIGNAL(currentIndexChanged(int)), this, SLOT(updateEquivalentProgram()));
    QObject::connect(m_spinPulseFrequency, SIGNAL(valueChanged(double)), this, SLOT(updateEquivalentProgram()));
    QObject::connect(m_comboPulseFrequency, SIGNAL(currentIndexChanged(int)), this, SLOT(updateEquivalentProgram()));
    QObject::connect(m_spinTrainDuration, SIGNAL(valueChanged(double)), this, SLOT(updateEquivalentProgram()));
    QObject::connect(m_comboTrainDuration, SIGNAL(currentIndexChanged(int)), this, SLOT(updateEquivalentProgram()));
    QObject::connect(m_spinTrainDelay, SIGNAL(valueChanged(double)), this, SLOT(updateEquivalentProgram()));
    QObject::connect(m_comboTrainDelay, SIGNAL(currentIndexChanged(int)), this, SLOT(updateEquivalentProgram()));
    QObject::connect(m_checkboxPulseTrain, SIGNAL(stateChanged(int)), this, SLOT(updateEquivalentProgram()));
    QObject::connect(m_spinNumTrains, SIGNAL(valueChanged(int)), this, SLOT(updateEquivalentProgram()));

    updateProgramName("<new program>");
};


void ProgramGuiWindow::changePulseWidth(int newVal) {
    m_spinPulseWidth->setValue(newVal);
}


void ProgramGuiWindow::changePulseWidth(double newVal) {
    m_sliderPulseWidth->blockSignals(true);
    m_sliderPulseWidth->setValue((int)newVal);
    m_sliderPulseWidth->blockSignals(false);
}


void ProgramGuiWindow::changePulseFrequency(int newVal) {
    m_spinPulseFrequency->setValue(newVal);
}


void ProgramGuiWindow::changePulseFrequency(double newVal) {
    m_sliderPulseFrequency->blockSignals(true);
    m_sliderPulseFrequency->setValue((int)newVal);
    m_sliderPulseFrequency->blockSignals(false);
}


void ProgramGuiWindow::changeTrainDuration(int newVal) {
    m_spinTrainDuration->setValue(newVal);
}


void ProgramGuiWindow::changeTrainDuration(double newVal) {
    m_sliderTrainDuration->blockSignals(true);
    m_sliderTrainDuration->setValue((int)newVal);
    m_sliderTrainDuration->blockSignals(false);
}


void ProgramGuiWindow::changeTrainDelay(int newVal) {
    m_spinTrainDelay->setValue(newVal);
}


void ProgramGuiWindow::changeTrainDelay(double newVal) {
    m_sliderTrainDelay->blockSignals(true);
    m_sliderTrainDelay->setValue((int)newVal);
    m_sliderTrainDelay->blockSignals(false);
}


QSize ProgramGuiWindow::sizeHint() const {
    return QSize(600,700);
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
            m_buttonRun->setText(runButtonText);
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


void ProgramGuiWindow::help() {
    QDesktopServices::openUrl(QUrl("http://kms15.github.com/ArduinoPulseGenerator/manual/"));
}


void ProgramGuiWindow::newDocument() {
    ProgramGuiWindow* newWindow = new ProgramGuiWindow();
    newWindow->m_comboPort->setCurrentIndex(m_comboPort->currentIndex());
    newWindow->show();
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

        ProgramGuiWindow* newWindow = new ProgramGuiWindow();

        newWindow->updateProgramName(QFileInfo(fileName).baseName());
        newWindow->m_comboPort->setCurrentIndex(m_comboPort->currentIndex());
        newWindow->m_texteditProgram->setText(in.readAll());
        newWindow->show();
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

        updateProgramName(QFileInfo(fileName).baseName());
    }
}


void ProgramGuiWindow::simulate() {
    // disable the old simulation results and switch to the status tab
    m_tabsOutput->setCurrentIndex(m_tabsOutput->indexOf(m_texteditStatus));
    m_tabsOutput->setTabEnabled(m_tabsOutput->indexOf(m_plot), false);

    // clear any previous plot points
    for (unsigned int i = 0; i < numChannels; ++i) {
        m_points[i].clear();
    }


    m_texteditStatus->moveCursor(QTextCursor::End);
    m_texteditStatus->insertPlainText("\n\nParsing...\n");

    // read the program from the appropriate tab
    QStringList lines;
    if (m_tabsProgram->currentIndex() == m_tabsProgram->indexOf(m_texteditProgram)) {
        lines = m_texteditProgram->toPlainText().split('\n');
    } else {
        lines = m_texteditTraditionalProgram->toPlainText().split('\n');
    }

    // Parse the program
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
        m_points[i].append(QPointF(time * us, low - i - 1));
        m_points[i].append(QPointF(timeEnd, low - i - 1));
    }


    // update the plot
    for (unsigned int i = 0; i < numChannels; ++i) {
        m_curves[i]->setSamples(m_points[i]);
    }
    m_plot->replot();

    // display the results in the simulation tab
    m_tabsOutput->setTabEnabled(m_tabsOutput->indexOf(m_plot), true);
    m_tabsOutput->setCurrentIndex(m_tabsOutput->indexOf(m_plot));

}


void ProgramGuiWindow::onLockStateChanged(int state) {
    m_buttonRun->setEnabled(state == Qt::Unchecked);
}


void ProgramGuiWindow::updateTraditionalDisabledControls() {
    bool bEnableTrains = m_checkboxPulseTrain->isChecked();

    m_labelPulseFrequency->setEnabled(bEnableTrains);
    m_spinPulseFrequency->setEnabled(bEnableTrains);
    m_sliderPulseFrequency->setEnabled(bEnableTrains);
    m_comboPulseFrequency->setEnabled(bEnableTrains);

    m_labelTrainDuration->setEnabled(bEnableTrains);
    m_spinTrainDuration->setEnabled(bEnableTrains);
    m_sliderTrainDuration->setEnabled(bEnableTrains);
    m_comboTrainDuration->setEnabled(bEnableTrains);

    m_labelNumTrains->setEnabled(bEnableTrains);
    m_spinNumTrains->setEnabled(bEnableTrains);
    m_sliderNumTrains->setEnabled(bEnableTrains);

    bool bEnableDelay = (bEnableTrains && m_spinNumTrains->value() > 1);
    m_labelTrainDelay->setEnabled(bEnableDelay);
    m_spinTrainDelay->setEnabled(bEnableDelay);
    m_sliderTrainDelay->setEnabled(bEnableDelay);
    m_comboTrainDelay->setEnabled(bEnableDelay);
}


void ProgramGuiWindow::updateEquivalentProgram() {
    bool bEnableTrains = m_checkboxPulseTrain->isChecked();
    int channel = m_spinChannel->value();
    double pulseWidth = m_spinPulseWidth->value();
    QString unitsPulseWidth = m_comboPulseWidth->currentText();
    double pulseFrequency = m_spinPulseFrequency->value();
    QString unitsPulseFrequency = m_comboPulseFrequency->currentText();
    double trainDuration = m_spinTrainDuration->value();
    QString unitsTrainDuration = m_comboTrainDuration->currentText();
    double trainDelay = m_spinTrainDelay->value();
    QString unitsTrainDelay = m_comboTrainDelay->currentText();
    int numTrains = m_spinNumTrains->value();

    if (!bEnableTrains) {
        m_texteditTraditionalProgram->setText(
                "# generate a single pulse\n"
                "turn on channel " + QString::number(channel) + "\n"
                "wait " + QString::number(pulseWidth, 'f', 2) + " " + unitsPulseWidth + "\n"
                "turn off channel " + QString::number(channel) + "\n"
                "\n"
                "end program"
            );
    } else if (numTrains == 1) {
        m_texteditTraditionalProgram->setText(
                "# generate a pulse train\n"
                "set channel " + QString::number(channel) + " to "
                    + QString::number(pulseWidth, 'f', 2) + " " + unitsPulseWidth +
                    " pulses at "
                    + QString::number(pulseFrequency, 'f', 2) + " " + unitsPulseFrequency +
                    + "\n"
                "wait " + QString::number(trainDuration, 'f', 2) + " " + unitsTrainDuration + "\n"
                "turn off channel " + QString::number(channel) + "\n"
                "\n"
                "end program"
            );
    } else if (numTrains == 2) {
        m_texteditTraditionalProgram->setText(
                "# generate the first pulse train\n"
                "set channel " + QString::number(channel) + " to "
                    + QString::number(pulseWidth, 'f', 2) + " " + unitsPulseWidth +
                    " pulses at "
                    + QString::number(pulseFrequency, 'f', 2) + " " + unitsPulseFrequency +
                    + "\n"
                "wait " + QString::number(trainDuration, 'f', 2) + " " + unitsTrainDuration + "\n"
                "turn off channel " + QString::number(channel) + "\n"
                "\n"
                "# delay between pulse trains\n"
                "wait " + QString::number(trainDelay, 'f', 2) + " " + unitsTrainDelay + "\n"
                "\n"
                "# generate the second pulse train\n"
                "set channel " + QString::number(channel) + " to "
                    + QString::number(pulseWidth, 'f', 2) + " " + unitsPulseWidth +
                    " pulses at "
                    + QString::number(pulseFrequency, 'f', 2) + " " + unitsPulseFrequency +
                    + "\n"
                "wait " + QString::number(trainDuration, 'f', 2) + " " + unitsTrainDuration + "\n"
                "turn off channel " + QString::number(channel) + "\n"
                "\n"
                "end program"
            );
    } else {
        m_texteditTraditionalProgram->setText(
                "# The last pulse train isn't followed by a delay, so we use\n"
                "# a loop to generate all but the last pulse train and the\n"
                "# delay after each of these pulse trains.\n"
                "repeat " + QString::number(numTrains - 1) + " times:\n"
                "\t# generate a pulse train\n"
                "\tset channel " + QString::number(channel) + " to "
                    + QString::number(pulseWidth, 'f', 2) + " " + unitsPulseWidth +
                    " pulses at "
                    + QString::number(pulseFrequency, 'f', 2) + " " + unitsPulseFrequency +
                    + "\n"
                "\twait " + QString::number(trainDuration, 'f', 2) + " " + unitsTrainDuration + "\n"
                "\tturn off channel " + QString::number(channel) + "\n"
                "\t\n"
                "\t# delay between pulse trains\n"
                "\twait " + QString::number(trainDelay, 'f', 2) + " " + unitsTrainDelay + "\n"
                "end repeat\n"
                "\n"
                "# generate the last pulse train (with no delay after it)\n"
                "set channel " + QString::number(channel) + " to "
                    + QString::number(pulseWidth, 'f', 2) + " " + unitsPulseWidth +
                    " pulses at "
                    + QString::number(pulseFrequency, 'f', 2) + " " + unitsPulseFrequency +
                    + "\n"
                "wait " + QString::number(trainDuration, 'f', 2) + " " + unitsTrainDuration + "\n"
                "turn off channel " + QString::number(channel) + "\n"
                "\n"
                "end program"
            );
    }
}


void ProgramGuiWindow::run() {
    // close the serial port, if one is currently open
    if (m_port) {
        m_port->close();
        delete m_port;
        m_port = NULL;

        // discard any remaining commands
        m_sendBuffer.clear();

        // Add a dummy line at the beginning to work around a race condition
        // when the Arduino resets.
        m_sendBuffer.push_back("# ArduinoPulseGeneratorGui v1.0");

        for (int i = 1; i <= (int)numChannels; ++i) {
            m_sendBuffer.push_back("turn off channel " + QString::number(i));
        }
        m_sendBuffer.push_back("end program");

    } else {
        m_buttonRun->setText(interruptButtonText);

        // switch to the status tab
        m_tabsOutput->setCurrentIndex(m_tabsOutput->indexOf(m_texteditStatus));

        // read the program from the appropriate tab
        QStringList lines;
        if (m_tabsProgram->currentIndex() == m_tabsProgram->indexOf(m_texteditProgram)) {
            m_sendBuffer = m_texteditProgram->toPlainText().split('\n');
        } else {
            m_sendBuffer = m_texteditTraditionalProgram->toPlainText().split('\n');
        }

        // Add a dummy line at the beginning to work around a race condition
        // when the Arduino resets.
        m_sendBuffer.push_front("# ArduinoPulseGeneratorGui v1.0");

    }

    // create the serial port
    PortSettings settings = {BAUD9600, DATA_8, PAR_NONE, STOP_1, FLOW_OFF, 10};
    m_port = new QextSerialPort(m_comboPort->currentText(), settings, QextSerialPort::EventDriven);

    QObject::connect(m_port, SIGNAL(readyRead()), SLOT(onNewSerialData()));
    m_port->open(QIODevice::ReadWrite);

    // wait for the arduino to prompt us before sending the first line.
}


void ProgramGuiWindow::repopulatePortComboBox()
{
    QString currentPortName = m_comboPort->currentText();

    m_comboPort->clear();
    Q_FOREACH (QextPortInfo info, QextSerialEnumerator::getPorts()) {
        m_comboPort->addItem(info.portName);
    }

    m_comboPort->setCurrentIndex(m_comboPort->findText(currentPortName));
}


void ProgramGuiWindow::updateProgramName(const QString& name) {
    setWindowTitle("Arduino Pulse Generator - " + name);
    m_tabsProgram->setTabText(m_tabsProgram->indexOf(m_texteditProgram), name);
}

