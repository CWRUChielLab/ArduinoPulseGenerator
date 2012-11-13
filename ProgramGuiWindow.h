#ifndef PROGRAMGUIWINDOW_H
#define PROGRAMGUIWINDOW_H
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>

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
    QPushButton* m_buttonLoad;
    QPushButton* m_buttonSave;
    QPushButton* m_buttonRun;

    // the serial port
    QextSerialPort* m_port;

private Q_SLOTS:
    // void onPortNameChanged(const QString &name);
    void run();
    void onNewSerialData();
    void onPortChanged();

public:
    ProgramGuiWindow(QWidget* parent = NULL);

    virtual QSize sizeHint() const;
};

#endif /* PROGRAMGUIWINDOW_H */
