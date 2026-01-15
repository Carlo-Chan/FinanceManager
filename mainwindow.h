#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlRelationalTableModel>
#include <QtCharts>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionAddRecord_triggered();

    void on_actionExport_triggered();

    void on_actionExit_triggered();

    void on_actionAbout_triggered();

    void on_btn_Filter_clicked();

    void on_btn_Reset_clicked();

    void on_btn_Delete_clicked();

private:
    Ui::MainWindow *ui;

    QSqlRelationalTableModel *model; // 关系型表格模型

    // 初始化函数
    void initModelView();
};
#endif // MAINWINDOW_H
