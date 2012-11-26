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
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

class QextSerialPort;

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
    QPushButton* m_buttonOpen;
    QPushButton* m_buttonSave;
    QPushButton* m_buttonSimulate;
    QLabel* m_labelPort;
    QComboBox* m_comboPort;
    QCheckBox* m_checkboxLock;
    QPushButton* m_buttonRun;

    // the serial port
    QextSerialPort* m_port;

    // the tab container
    QTabWidget* m_tabs;

    // lines buffered to send to the device
    QStringList m_sendBuffer;

private Q_SLOTS:
    void help();
    void save();
    void open();
    void simulate();
    void run();

    void onNewSerialData();
    void onPortChanged();
    void onLockStateChanged(int state);

public:
    ProgramGuiWindow(QWidget* parent = NULL);

    virtual QSize sizeHint() const;
};

#endif /* PROGRAMGUIWINDOW_H */
