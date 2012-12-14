#ifndef PROGRAMGUIWINDOW_H
#define PROGRAMGUIWINDOW_H
#include <QWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QTabWidget>
#include <QDoubleSpinBox>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

class QextSerialPort;
class QextSerialEnumerator;

// A window with a basic text area for entering and editing a program.
class ProgramGuiWindow : public QWidget
{
    Q_OBJECT

    // Program editor
    QTextEdit* m_texteditProgram;

    // Plot from simulation
    QwtPlot *m_plot;
    QVector<QwtPlotCurve *> m_curves;
    QVector<QVector<QPointF> > m_points;

    // status display
    QTextEdit* m_texteditStatus;

    // bottom buttons
    QPushButton* m_buttonHelp;
    QPushButton* m_buttonNew;
    QPushButton* m_buttonOpen;
    QPushButton* m_buttonSave;
    QPushButton* m_buttonSimulate;
    QLabel* m_labelPort;
    QComboBox* m_comboPort;
    QCheckBox* m_checkboxLock;
    QPushButton* m_buttonRun;

    // the serial port
    QextSerialPort* m_port;
    QextSerialEnumerator* m_portEnumerator;

    // the tab container
    QTabWidget* m_tabsOutput;
    QTabWidget* m_tabsProgram;

    // the traditional controls
    QWidget* m_widgetTraditional;

    QLabel* m_labelChannel;
    QSlider* m_sliderChannel;
    QSpinBox* m_spinChannel;

    QLabel* m_labelPulseWidth;
    QSlider* m_sliderPulseWidth;
    QDoubleSpinBox* m_spinPulseWidth;
    QComboBox* m_comboPulseWidth;

    QLabel* m_labelPulseTrain;
    QCheckBox* m_checkboxPulseTrain;

    QLabel* m_labelPulseFrequency;
    QSlider* m_sliderPulseFrequency;
    QDoubleSpinBox* m_spinPulseFrequency;
    QComboBox* m_comboPulseFrequency;

    QLabel* m_labelTrainDuration;
    QSlider* m_sliderTrainDuration;
    QDoubleSpinBox* m_spinTrainDuration;
    QComboBox* m_comboTrainDuration;

    QLabel* m_labelNumTrains;
    QSlider* m_sliderNumTrains;
    QSpinBox* m_spinNumTrains;

    QLabel* m_labelTrainDelay;
    QSlider* m_sliderTrainDelay;
    QDoubleSpinBox* m_spinTrainDelay;
    QComboBox* m_comboTrainDelay;

    QLabel* m_labelTraditionalProgram;
    QTextEdit* m_texteditTraditionalProgram;

    // lines buffered to send to the device
    QStringList m_sendBuffer;

private Q_SLOTS:
    void help();
    void newDocument();
    void open();
    void save();
    void simulate();
    void run();

    void changePulseWidth(int newVal);
    void changePulseWidth(double newVal);

    void changePulseFrequency(int newVal);
    void changePulseFrequency(double newVal);

    void changeTrainDuration(int newVal);
    void changeTrainDuration(double newVal);

    void changeTrainDelay(int newVal);
    void changeTrainDelay(double newVal);

    void onNewSerialData();
    void onLockStateChanged(int state);
    void updateTraditionalDisabledControls();

    void repopulatePortComboBox();
    void updateEquivalentProgram();

public:
    ProgramGuiWindow(QWidget* parent = NULL);

    virtual QSize sizeHint() const;
    void updateProgramName(const QString& name);
};

#endif /* PROGRAMGUIWINDOW_H */
