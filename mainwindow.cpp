#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutdialog.h"
#include "addrecorddialog.h"
#include "databasemanager.h"

#include <QMessageBox>
#include <QSqlRecord>
#include <QSqlRelationalDelegate>
#include <QFileDialog>
#include <QTextStream>
#include <QDateTime>
#include <QHeaderView>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 连接数据库
    if (!DatabaseManager::instance().openDatabase("F:/Users/22783/Documents/Qt/FinanceManager/finance.db")) {
        QMessageBox::critical(this, "错误", "无法连接数据库！");
    }

    // 初始化模块
    initModelView();

    // 初始化筛选控件
    ui->dateEdit_Start->setDate(QDate::currentDate().addMonths(-1)); // 默认查最近一个月
    ui->dateEdit_End->setDate(QDate::currentDate());

    // 初始化筛选类型 ComboBox
    ui->comboBox_FilterType->clear();
    ui->comboBox_FilterType->addItem("全部", -1);
    ui->comboBox_FilterType->addItem("支出", 0);
    ui->comboBox_FilterType->addItem("收入", 1);

    // 联动连接：当筛选类型改变时，更新筛选分类
    connect(ui->comboBox_FilterType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_filterTypeChanged(int)));

    // 初始加载所有分类
    loadFilterCategories(-1);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionAddRecord_triggered()
{
    AddRecordDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        auto data = dlg.getRecordData();

        // 插入数据库
        bool success = DatabaseManager::instance().insertRecord(
            data.amount, data.date, data.note, data.categoryId
            );

        if (success) {
            model->select(); // 刷新表格
            QMessageBox::information(this, "成功", "账单添加成功！");
        } else {
            QMessageBox::warning(this, "失败", "添加失败，请检查数据库。");
        }
    }
}


void MainWindow::on_actionExport_triggered()
{

}


void MainWindow::on_actionExit_triggered()
{
    close();
}


void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dlg;
    dlg.exec();
}


void MainWindow::on_btn_Filter_clicked()
{
    QString filterStr = "1=1"; // 初始条件

    // 日期筛选
    qint64 startSec = QDateTime(ui->dateEdit_Start->date(), QTime(0,0)).toSecsSinceEpoch();
    qint64 endSec = QDateTime(ui->dateEdit_End->date(), QTime(23,59,59)).toSecsSinceEpoch();
    filterStr += QString(" AND timestamp >= %1 AND timestamp <= %2").arg(startSec).arg(endSec);

    // 类型筛选 (如果是 -1 则全部)
    int type = ui->comboBox_FilterType->currentData().toInt();
    if (type != -1) {
        filterStr += QString(" AND cid IN (SELECT id FROM category WHERE type = %1)").arg(type);
    }

    // 分类筛选
    int categoryId = ui->comboBox_FilterCategory->currentData().toInt();
    if (categoryId != -1) {
        filterStr += QString(" AND cid = %1").arg(categoryId);
    }

    // 备注搜索 (模糊查询)
    QString note = ui->lineEdit_Search->text().trimmed();
    if (!note.isEmpty()) {
        filterStr += QString(" AND note LIKE '%%1%'").arg(note);
    }

    model->setFilter(filterStr);
    model->select();
}


void MainWindow::on_btn_Reset_clicked()
{
    ui->dateEdit_Start->setDate(QDate::currentDate().addMonths(-1));
    ui->dateEdit_End->setDate(QDate::currentDate());
    ui->comboBox_FilterType->setCurrentIndex(0); // 全部
    ui->lineEdit_Search->clear();

    model->setFilter("");
    model->select();
}


void MainWindow::on_btn_Delete_clicked()
{
    QModelIndexList selection = ui->tableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择要删除的行");
        return;
    }

    int ret = QMessageBox::question(this, "确认", "确定要删除选中的记录吗？", QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        // 从最后一行开始删，防止索引变化
        for(int i = selection.count() - 1; i >= 0; i--) {
            model->removeRow(selection.at(i).row());
        }
        model->submitAll(); // 提交到数据库
        model->select();
    }
}

void MainWindow::on_filterTypeChanged(int index)
{
    int type = ui->comboBox_FilterType->currentData().toInt();
    loadFilterCategories(type); // 重新加载分类
}

// 时间戳转换代理 (TimeDelegate)
// 作用：将数据库里的 Unix 时间戳 (秒) 转换为 "yyyy-MM-dd HH:mm" 格式显示
class TimeDelegate : public QStyledItemDelegate {
public:
    explicit TimeDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    QString displayText(const QVariant &value, const QLocale &locale) const override {
        bool ok = false;
        qint64 timestamp = value.toLongLong(&ok);

        // 如果转换成功(ok为true) 且 数值大于0 (有效时间戳)
        if (ok && timestamp > 0) {
            return QDateTime::fromSecsSinceEpoch(timestamp).toString("yyyy-MM-dd HH:mm");
        }

        return QStyledItemDelegate::displayText(value, locale);
    }
};

void MainWindow::initModelView()
{
    // 初始化模型
    model = new QSqlRelationalTableModel(this);
    model->setTable("record");

    // 设置外键关系
    model->setRelation(4, QSqlRelation("category", "id", "name"));

    // 加载数据
    model->select();

    // 设置表头名称
    model->setHeaderData(1, Qt::Horizontal, "金额");
    model->setHeaderData(2, Qt::Horizontal, "时间");
    model->setHeaderData(3, Qt::Horizontal, "备注");
    model->setHeaderData(4, Qt::Horizontal, "分类");

    // 绑定模型到视图
    ui->tableView->setModel(model);

    // 隐藏数据库 ID 列
    ui->tableView->setColumnHidden(0, true);

    // 调整列的视觉顺序 (分类 -> 金额 -> 时间 -> 备注)
    QHeaderView *header = ui->tableView->horizontalHeader();
    header->moveSection(4, 1);

    // 应用代理 (Delegate)
    ui->tableView->setItemDelegateForColumn(2, new TimeDelegate(ui->tableView));
    ui->tableView->setItemDelegateForColumn(4, new QSqlRelationalDelegate(ui->tableView));

    // 开启左侧行号
    ui->tableView->verticalHeader()->setVisible(true);
    ui->tableView->verticalHeader()->setDefaultSectionSize(30);

    // 调整列宽策略
    // 分类：给固定宽度 100
    header->setSectionResizeMode(4, QHeaderView::Interactive);
    ui->tableView->setColumnWidth(4, 100);

    // 金额：给固定宽度 100
    header->setSectionResizeMode(1, QHeaderView::Interactive);
    ui->tableView->setColumnWidth(1, 100);

    // 时间：给足够显示的宽度 160
    header->setSectionResizeMode(2, QHeaderView::Interactive);
    ui->tableView->setColumnWidth(2, 160);

    // 备注：自动拉伸 (Stretch)，填满剩余空间
    header->setSectionResizeMode(3, QHeaderView::Stretch);
}

void MainWindow::loadFilterCategories(int type)
{
    ui->comboBox_FilterCategory->clear();
    ui->comboBox_FilterCategory->addItem("全部", -1); // 默认项

    QSqlQuery query = DatabaseManager::instance().getCategories(type);
    while (query.next()) {
        ui->comboBox_FilterCategory->addItem(query.value("name").toString(), query.value("id").toInt());
    }
}

