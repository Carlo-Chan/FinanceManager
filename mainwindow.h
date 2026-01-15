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

    void on_filterTypeChanged(int index);

private:
    Ui::MainWindow *ui;

    QSqlRelationalTableModel *model; // 关系型表格模型

    // 图表对象
    QChart *barChart;
    QChart *pieChart;

    // 初始化函数
    void initModelView();
    void initCharts();
    void updateCharts(); // 刷新图表数据

    // 辅助函数：加载主界面的筛选分类
    void loadFilterCategories(int type); // type: 0支出, 1收入, -1全部
};
#endif // MAINWINDOW_H
