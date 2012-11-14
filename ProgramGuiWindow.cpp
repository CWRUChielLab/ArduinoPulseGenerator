#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPalette>
#include <QApplication>
#include "qextserialport.h"
#include "qextserialenumerator.h"

#include "ProgramGuiWindow.h"

ProgramGuiWindow::ProgramGuiWindow(QWidget* parent) :
    QWidget(parent)
{
    // create the controls
    m_texteditProgram = new QTextEdit();
    //m_texteditProgram->setText(
    m_texteditProgram->insertPlainText(
            "change channel 1 to repeat 15 ms on 85 ms off\n"
            "wait 3 s\n"
            "turn off channel 1\n"
            "end\n"
            );
    m_texteditProgram->setLineWrapMode(QTextEdit::NoWrap);

    m_texteditStatus = new QTextEdit();
    m_texteditStatus->setReadOnly(true);
    QPalette p = m_texteditStatus->palette();
    p.setBrush(QPalette::Base, QApplication::palette().window());
    m_texteditStatus->setPalette(p);

    m_comboPort = new QComboBox();
    foreach (QextPortInfo info, QextSerialEnumerator::getPorts())
        m_comboPort->addItem(info.portName);
    m_comboPort->setEditable(true);
    int index_ttyACM0 = m_comboPort->findText("ttyACM0");
    if (index_ttyACM0 > -1) {
        m_comboPort->setCurrentIndex(index_ttyACM0);
    }


    m_buttonOpen = new QPushButton("Open");
    m_buttonSave = new QPushButton("Save");
    m_buttonRun = new QPushButton("Run");


    // lay out the controls
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_comboPort);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_buttonOpen);
    buttonLayout->addWidget(m_buttonSave);
    buttonLayout->addWidget(m_buttonRun);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_texteditProgram);
    mainLayout->setStretchFactor(m_texteditProgram, 4);
    //mainLayout->addWidget(m_labelStatus);
    mainLayout->addWidget(m_texteditStatus);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);

    // attach signals as needed
    QObject::connect(m_buttonRun, SIGNAL(clicked()), this, SLOT(run()));
    QObject::connect(m_buttonOpen, SIGNAL(clicked()), this, SLOT(open()));
    QObject::connect(m_buttonSave, SIGNAL(clicked()), this, SLOT(save()));
    QObject::connect(m_comboPort, SIGNAL(editTextChanged(QString)), SLOT(onPortChanged()));

    m_port = NULL;
    onPortChanged();
};


QSize ProgramGuiWindow::sizeHint() const {
    return QSize(600,600);
}


void ProgramGuiWindow::run() {
    m_port->write(m_texteditProgram->toPlainText().toUtf8());
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
        //file.write(m_texteditProgram->toPlainText().toUtf8());
        out << m_texteditProgram->toPlainText();
    }
}

void ProgramGuiWindow::onNewSerialData() {
    if (m_port->bytesAvailable()) {
        m_texteditStatus->moveCursor(QTextCursor::End);
        m_texteditStatus->insertPlainText(QString::fromUtf8(m_port->readAll()).replace("\n",""));
    }
}

void ProgramGuiWindow::onPortChanged() {
    if (m_port) {
        m_port->close();
        delete m_port;
    }

    // create the serial port
    PortSettings settings = {BAUD9600, DATA_8, PAR_NONE, STOP_1, FLOW_OFF, 10};
    m_port = new QextSerialPort(m_comboPort->currentText(), settings, QextSerialPort::EventDriven);
    QObject::connect(m_port, SIGNAL(readyRead()), SLOT(onNewSerialData()));
    m_port->open(QIODevice::ReadWrite);
}
