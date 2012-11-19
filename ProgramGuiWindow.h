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

class QextSerialPort;

// A window with a basic text area for entering and editing a program.
class ProgramGuiWindow : public QWidget
{
    Q_OBJECT

    QTextEdit* m_texteditProgram;

    // status display
    QTextEdit* m_texteditStatus;

    // bottom buttons
    QComboBox* m_comboPort;
    QPushButton* m_buttonOpen;
    QPushButton* m_buttonSave;
    QPushButton* m_buttonRun;

    // the serial port
    QextSerialPort* m_port;

    // lines buffered to send to the device
    QStringList m_sendBuffer;

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
