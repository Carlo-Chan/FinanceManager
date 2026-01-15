#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutdialog.h"
#include "addrecorddialog.h"
#include "databasemanager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 连接数据库
    if (!DatabaseManager::instance().openDatabase("F:/Users/22783/Documents/Qt/FinanceManager/finance.db")) {
        QMessageBox::critical(this, "错误", "无法连接数据库！");
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionAddRecord_triggered()
{

}


void MainWindow::on_actionExport_triggered()
{

}


void MainWindow::on_actionExit_triggered()
{

}


void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dlg;
    dlg.exec();
}


void MainWindow::on_btn_Filter_clicked()
{

}


void MainWindow::on_btn_Reset_clicked()
{

}


void MainWindow::on_btn_Delete_clicked()
{

}

