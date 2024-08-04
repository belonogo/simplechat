#include "registerdialog.h"
#include "ui_registerdialog.h"

RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog)
{
    ui->setupUi(this);
    connect(ui->registerButton, &QPushButton::clicked, this, &RegisterDialog::accept);
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

QString RegisterDialog::getUsername() const
{
    return ui->usernameLineEdit->text().trimmed();
}

QString RegisterDialog::getPassword() const
{
    return ui->passwordLineEdit->text().trimmed();
}
