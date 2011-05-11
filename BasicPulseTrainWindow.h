#ifndef BASICPULSETRAINWINDOW_H
#define BASICPULSETRAINWINDOW_H 
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QProgressBar>

// A window with controls for creating and running simple pulse trains.
// The user can specify the pulse width, frequency, and duration of the 
// pulse train, along with starting and stopping the pulse generator.  
class BasicPulseTrainWindow : public QWidget
{
    Q_OBJECT
    
    static const int numChannels = 4;

    // column headers
    QLabel* m_labelPulseWidth;
    QLabel* m_labelPeriod;
    QLabel* m_labelNumPulses;
    QLabel* m_labelDuration;
    QLabel* m_labelIntertrainInterval;
    QLabel* m_labelNumTrains;
    QLabel* m_labelState;

    // row contents
    QLabel* m_labelChannels[numChannels];
    QDoubleSpinBox* m_dspinPulseWidths[numChannels];
    QDoubleSpinBox* m_dspinPeriods[numChannels];
    QSpinBox* m_spinNumPulses[numChannels];
    QDoubleSpinBox* m_dspinDurations[numChannels];
    QDoubleSpinBox* m_dspinIntertrainInterval[numChannels];
    QSpinBox* m_spinNumTrains[numChannels];
    QLabel* m_labelStates[numChannels];
    QPushButton* m_buttonStartStops[numChannels];
    
    // bottom buttons
    QPushButton* m_buttonQuit;

public:
    BasicPulseTrainWindow(QWidget* parent = NULL);
};
#endif /* BASICPULSETRAINWINDOW_H */
