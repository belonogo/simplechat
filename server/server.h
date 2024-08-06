#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QTextStream>
#include <QSet>
#include <QMap>

class Server : public QTcpServer
{
    Q_OBJECT

public:
    Server(QObject *parent = nullptr);
    bool startServer();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onReadyRead();
    void onClientDisconnected();

private:
    QSet<QTcpSocket*> clients; // Список подключенных клиентов
    QMap<QTcpSocket*, QString> userMap; // Отображение клиентов на имена пользователей
    QSet<QString> activeSessions; // Активные сессии пользователей

    void processMessage(QTcpSocket *client, const QString &message);
    void registerUser(QTcpSocket *client, const QString &username, const QString &password);
    void loginUser(QTcpSocket *client, const QString &username, const QString &password);
    void broadcastMessage(const QString &sender, const QString &message);
    void updateClientList();
    void logAction(const QString &action);

    const QString userFilePath = "users.txt"; // Путь к файлу с пользователями

    bool userExists(const QString &username);
    QString getPasswordForUser(const QString &username);
    QString getUserList() const;
};

#endif // SERVER_H
