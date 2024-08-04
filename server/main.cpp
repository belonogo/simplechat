#include <QCoreApplication>
#include "server.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Server server;
    if (!server.startServer()) {
        qDebug() << "Server failed to start!";
        return 1;
    }

    qDebug() << "Server started successfully.";

    return a.exec();
}
