#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "registerdialog.h"
#include "logindialog.h"
#include "chatwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , socket(new QTcpSocket(this))
{
    ui->setupUi(this);

    connect(ui->registerButton, &QPushButton::clicked, this, &MainWindow::onRegisterButtonClicked);
    connect(ui->loginButton, &QPushButton::clicked, this, &MainWindow::onLoginButtonClicked);
    connect(ui->selectUserButton, &QPushButton::clicked, this, &MainWindow::onSelectUserButtonClicked);

    connect(socket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);

    // Подключаемся к серверу
    socket->connectToHost("192.168.120.179", 1234); // Замените IP на адрес вашего сервера

    loadUserList();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onRegisterButtonClicked()
{
    RegisterDialog registerDialog(this);
    if (registerDialog.exec() == QDialog::Accepted) {
        QString username = registerDialog.getUsername();
        QString password = registerDialog.getPassword();
        sendCommand("REGISTER " + username + " " + password);
    }
}

void MainWindow::onLoginButtonClicked()
{
    LoginDialog loginDialog(this);
    if (loginDialog.exec() == QDialog::Accepted) {
        QString username = loginDialog.getUsername();
        QString password = loginDialog.getPassword();
        sendCommand("LOGIN " + username + " " + password);
    }
}

void MainWindow::onSelectUserButtonClicked()
{
    QString selectedUser = ui->userListWidget->currentItem()->text();
    if (!selectedUser.isEmpty()) {
        openChatWindow(selectedUser);
    }
}

void MainWindow::onReadyRead()
{
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    QStringList lines = response.split("\n");

    ui->userListWidget->clear();

    for (const QString &line : lines) {
        if (line.startsWith("OK Logged in successfully")) {
            loadUserList();
        } else if (line.startsWith("ERROR")) {
            ui->statusLabel->setText(line);
        } else if (!line.isEmpty()) {
            ui->userListWidget->addItem(line);
        }
    }
}

void MainWindow::loadUserList()
{
    sendCommand("LIST");
}

void MainWindow::openChatWindow(const QString &username)
{
    ChatWindow *chatWindow = new ChatWindow(username, socket, this);
    chatWindow->show();
}

void MainWindow::sendCommand(const QString &command)
{
    if (socket->state() == QTcpSocket::ConnectedState) {
        socket->write((command + "\n").toUtf8());
    }
}
