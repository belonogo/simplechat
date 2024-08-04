#ifndef CHATWINDOW_H
#define CHATWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>

namespace Ui {
class ChatWindow;
}

class ChatWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ChatWindow(const QString &recipient, QTcpSocket *socket, QWidget *parent = nullptr);
    ~ChatWindow();

private slots:
    void onSendMessageButtonClicked();
    void onReadyRead();

private:
    Ui::ChatWindow *ui;
    QString recipient;
    QTcpSocket *socket;

    void sendMessage(const QString &message);
};

#endif // CHATWINDOW_H
