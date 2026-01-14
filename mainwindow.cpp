#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QChart>
#include <QChartView>

QT_BEGIN_NAMESPACE
class QChartView;
class QChart;
QT_END_NAMESPACE

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
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

