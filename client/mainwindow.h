#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRegisterButtonClicked();
    void onLoginButtonClicked();
    void onSelectUserButtonClicked();
    void onReadyRead();

private:
    Ui::MainWindow *ui;
    QTcpSocket *socket;

    void loadUserList();
    void openChatWindow(const QString &username);
    void sendCommand(const QString &command);
};

#endif // MAINWINDOW_H
