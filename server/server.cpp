#include "server.h"
#include <QDebug>
#include <QDateTime>

Server::Server(QObject *parent) : QTcpServer(parent) {}

bool Server::startServer()
{
    return listen(QHostAddress::Any, 1234); // Слушаем на порту 1234
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *clientSocket = new QTcpSocket(this);
    if (clientSocket->setSocketDescriptor(socketDescriptor)) {
        connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
        connect(clientSocket, &QTcpSocket::disconnected, this, &Server::onClientDisconnected);
        clients.insert(clientSocket);
        logAction("New client connected");
    } else {
        delete clientSocket;
    }
}

void Server::onReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (!client) return;

    QByteArray data = client->readAll();
    QString message = QString::fromUtf8(data).trimmed();
    processMessage(client, message);
}

void Server::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (client) {
        QString username = userMap.value(client, "Unknown");
        clients.remove(client);
        userMap.remove(client);
        activeSessions.remove(username);
        updateClientList();
        client->deleteLater();
        logAction(username + " disconnected");
    }
}

void Server::processMessage(QTcpSocket *client, const QString &message)
{
    QStringList parts = message.split(" ", Qt::SkipEmptyParts);
    if (parts.isEmpty()) return;

    QString command = parts[0];

    if (command == "REGISTER" && parts.size() == 3) {
        registerUser(client, parts[1], parts[2]);
    } else if (command == "LOGIN" && parts.size() == 3) {
        loginUser(client, parts[1], parts[2]);
    } else if (command == "MSG" && parts.size() > 2) {
        QString recipient = parts[1];
        QString chatMessage = message.section(' ', 2); // Извлекаем сообщение без команды "MSG" и получателя
        QString sender = userMap.value(client, "Unknown");
        if (recipient == "ALL") {
            broadcastMessage(sender, chatMessage);
        } else {
            for (QTcpSocket *otherClient : qAsConst(clients)) {
                if (userMap.value(otherClient) == recipient) {
                    otherClient->write((sender + ": " + chatMessage + "\n").toUtf8());
                    logAction(sender + " sent message to " + recipient + ": " + chatMessage);
                    break;
                }
            }
        }
    } else if (command == "LIST") {
        client->write((getUserList() + "\n").toUtf8());
    } else {
        client->write("ERROR Invalid command\n");
    }
}

void Server::registerUser(QTcpSocket *client, const QString &username, const QString &password)
{
    if (userExists(username)) {
        client->write("ERROR User already exists\n");
        logAction("Failed registration attempt for existing user " + username);
        return;
    }

    QFile file(userFilePath);
    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        client->write("ERROR Cannot open user file\n");
        logAction("Failed to open user file for registration");
        return;
    }

    QTextStream out(&file);
    out << username << " " << password << "\n";
    file.close();

    client->write("OK Registered successfully\n");
    logAction("User registered successfully: " + username);
}

void Server::loginUser(QTcpSocket *client, const QString &username, const QString &password)
{
    if (!userExists(username)) {
        client->write("ERROR User does not exist\n");
        logAction("Failed login attempt for non-existing user " + username);
        return;
    }

    if (activeSessions.contains(username)) {
        client->write("ERROR User already logged in\n");
        logAction("Failed login attempt for already logged in user " + username);
        return;
    }

    QString storedPassword = getPasswordForUser(username);
    if (storedPassword == password) {
        client->write("OK Logged in successfully\n");
        userMap[client] = username;
        activeSessions.insert(username);
        updateClientList();
        logAction("User logged in successfully: " + username);
    } else {
        client->write("ERROR Invalid password\n");
        logAction("Failed login attempt with invalid password for user " + username);
    }
}

bool Server::userExists(const QString &username)
{
    QFile file(userFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QStringList line = in.readLine().split(" ");
        if (line.size() >= 2 && line[0] == username) {
            file.close();
            return true;
        }
    }

    file.close();
    return false;
}

QString Server::getPasswordForUser(const QString &username)
{
    QFile file(userFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return QString();

    QTextStream in(&file);
    while (!in.atEnd()) {
        QStringList line = in.readLine().split(" ");
        if (line.size() >= 2 && line[0] == username) {
            file.close();
            return line[1];
        }
    }

    file.close();
    return QString();
}

QString Server::getUserList() const
{
    QStringList userList;
    for (QTcpSocket *client : clients) {
        userList << userMap.value(client, "Unknown");
    }
    return userList.join("\n");
}

void Server::updateClientList()
{
    QString userList = getUserList();
    for (QTcpSocket *client : clients) {
        client->write((userList + "\n").toUtf8());
    }
}

void Server::broadcastMessage(const QString &sender, const QString &message)
{
    for (QTcpSocket *client : qAsConst(clients)) {
        client->write((sender + ": " + message + "\n").toUtf8());
    }
    logAction("Broadcast message from " + sender + ": " + message);
}

void Server::logAction(const QString &action)
{
    QString logEntry = QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss] ") + action;
    qDebug() << logEntry;
}
