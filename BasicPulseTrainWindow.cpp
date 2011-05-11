#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "BasicPulseTrainWindow.h"

BasicPulseTrainWindow::BasicPulseTrainWindow(QWidget* parent) :
    QWidget(parent)
{
    QGridLayout* tableLayout = new QGridLayout;
    
    //
    // Build the table of controls
    //

    int row = 0;
    int col = 0;
    
    // create and layout the headers
    col++; // blank header
    m_labelPulseWidth = new QLabel("Pulse Width (ms)");
    tableLayout->addWidget(m_labelPulseWidth, row, col++);
    m_labelPeriod = new QLabel("Period (ms)");
    tableLayout->addWidget(m_labelPeriod, row, col++);
    m_labelNumPulses = new QLabel("Num Pulses");
    tableLayout->addWidget(m_labelNumPulses, row, col++);
    m_labelDuration = new QLabel("Duration (s)");
    tableLayout->addWidget(m_labelDuration, row, col++);
    m_labelIntertrainInterval = new QLabel("Intertrain Gap (s)");
    tableLayout->addWidget(m_labelIntertrainInterval, row, col++);
    m_labelNumTrains = new QLabel("Num Trains");
    tableLayout->addWidget(m_labelNumTrains, row, col++);
    m_labelState = new QLabel("State");
    tableLayout->addWidget(m_labelState, row, col++);

    // create and layout the controls for each channel
    for (int chan = 0; chan < numChannels; ++chan) {
        // start a new row
        col = 0; row++;

        m_labelChannels[chan] = new QLabel(QString("Channel %1").arg(chan));
        tableLayout->addWidget(m_labelChannels[chan], row, col++);
        m_dspinPulseWidths[chan] = new QDoubleSpinBox();
        tableLayout->addWidget(m_dspinPulseWidths[chan], row, col++);
        m_dspinPeriods[chan] = new QDoubleSpinBox();
        tableLayout->addWidget(m_dspinPeriods[chan], row, col++);
        m_spinNumPulses[chan] = new QSpinBox();
        tableLayout->addWidget(m_spinNumPulses[chan], row, col++);
        m_dspinDurations[chan] = new QDoubleSpinBox();
        tableLayout->addWidget(m_dspinDurations[chan], row, col++);
        m_dspinIntertrainInterval[chan] = new QDoubleSpinBox();
        tableLayout->addWidget(m_dspinIntertrainInterval[chan], row, col++);
        m_spinNumTrains[chan] = new QSpinBox();
        tableLayout->addWidget(m_spinNumTrains[chan], row, col++);
        m_labelStates[chan] = new QLabel("initializing...      ");
        tableLayout->addWidget(m_labelStates[chan], row, col++);
        m_buttonStartStops[chan] = new QPushButton("Start");
        m_buttonStartStops[chan]->setEnabled(false);
        tableLayout->addWidget(m_buttonStartStops[chan], row, col++);
    }

    //
    // build the other elements of the window
    //

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    m_buttonQuit = new QPushButton("Quit");
    QObject::connect(m_buttonQuit, SIGNAL(clicked()),
            this, SLOT(close()));
    buttonLayout->addWidget(m_buttonQuit);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(tableLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
};

