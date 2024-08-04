#include "chatwindow.h"
#include "ui_chatwindow.h"

ChatWindow::ChatWindow(const QString &recipient, QTcpSocket *socket, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ChatWindow),
    recipient(recipient),
    socket(socket)
{
    ui->setupUi(this);

    connect(ui->sendMessageButton, &QPushButton::clicked, this, &ChatWindow::onSendMessageButtonClicked);
    connect(socket, &QTcpSocket::readyRead, this, &ChatWindow::onReadyRead);

    setWindowTitle("Chat with " + recipient);
}

ChatWindow::~ChatWindow()
{
    delete ui;
}

void ChatWindow::onSendMessageButtonClicked()
{
    QString message = ui->messageLineEdit->text().trimmed();
    if (!message.isEmpty()) {
        sendMessage(message);
        ui->chatTextEdit->append("Me: " + message);
        ui->messageLineEdit->clear();
    }
}

void ChatWindow::onReadyRead()
{
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    QStringList messages = response.split("\n");
    for (const QString &message : messages) {
        ui->chatTextEdit->append(message);
    }
}

void ChatWindow::sendMessage(const QString &message)
{
    if (socket->state() == QTcpSocket::ConnectedState) {
        socket->write(("MSG " + recipient + " " + message + "\n").toUtf8());
    }
}
