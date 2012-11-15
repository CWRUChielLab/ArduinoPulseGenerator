#ifndef PROGRAMGUIWINDOW_H
#define PROGRAMGUIWINDOW_H
#include <QWidget>
#include <QPushButton>
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

    // status display
    QTextEdit* m_texteditStatus;

    // bottom buttons
    QLabel* m_labelPort;
    QComboBox* m_comboPort;
    QPushButton* m_buttonOpen;
    QPushButton* m_buttonSave;
    QPushButton* m_buttonRun;

    // the serial port
    QextSerialPort* m_port;

    // the tab container
    QTabWidget* m_tabs;

private Q_SLOTS:
    void run();
    void save();
    void open();
    void onNewSerialData();
    void onPortChanged();

public:
    ProgramGuiWindow(QWidget* parent = NULL);

    virtual QSize sizeHint() const;
};

#endif /* PROGRAMGUIWINDOW_H */
