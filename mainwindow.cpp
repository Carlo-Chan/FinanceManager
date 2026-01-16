#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutdialog.h"
#include "addrecorddialog.h"
#include "databasemanager.h"
#include "categorydialog.h"

#include <QMessageBox>
#include <QSqlRecord>
#include <QSqlRelationalDelegate>
#include <QFileDialog>
#include <QTextStream>
#include <QDateTime>
#include <QHeaderView>
#include <QDateTimeEdit>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 连接数据库
    if (!DatabaseManager::instance().openDatabase("F:/Users/22783/Documents/Qt/FinanceManager/finance.db")) {
        QMessageBox::critical(this, "错误", "无法连接数据库！");
    }

    // 初始化各个模块
    initModelView();
    initCharts();

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

    // 初始刷新图表
    updateCharts();

    updateSummary();
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
            data.amount, data.dateTime, data.note, data.categoryId
            );

        if (success) {
            model->select(); // 刷新表格
            updateCharts();  // 刷新图表
            updateSummary(); // 刷新概览
            QMessageBox::information(this, "成功", "账单添加成功！");
        } else {
            QMessageBox::warning(this, "失败", "添加失败，请检查数据库。");
        }
    }
}


void MainWindow::on_actionExport_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, "导出数据", "", "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        // 写入 BOM 以解决 Excel 中文乱码
        out << QString::fromUtf8("\xEF\xBB\xBF");

        // 写表头
        out << "ID,金额,时间,备注,分类\n";

        // 写数据 (遍历 Model)
        for(int i = 0; i < model->rowCount(); ++i) {
            QString id = model->record(i).value("id").toString();
            QString amount = model->record(i).value("amount").toString();
            // 简单处理时间戳
            qint64 ts = model->record(i).value("timestamp").toLongLong();
            QString time = QDateTime::fromSecsSinceEpoch(ts).toString("yyyy-MM-dd HH:mm");
            QString note = model->record(i).value("note").toString();
            QString category = model->record(i).value("name").toString(); // 关联后的名字

            out << id << "," << amount << "," << time << "," << note << "," << category << "\n";
        }
        file.close();
        QMessageBox::information(this, "成功", "导出成功！");
    }
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

    // 刷新图表，让图表也反映筛选后的时间段
    updateCharts();

    updateSummary();
}


void MainWindow::on_btn_Reset_clicked()
{
    ui->dateEdit_Start->setDate(QDate::currentDate().addMonths(-1));
    ui->dateEdit_End->setDate(QDate::currentDate());
    ui->comboBox_FilterType->setCurrentIndex(0); // 全部
    ui->lineEdit_Search->clear();

    model->setFilter("");
    model->select();
    updateCharts();

    updateSummary();
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
        updateCharts();

        updateSummary();
    }
}

void MainWindow::on_filterTypeChanged(int index)
{
    int type = ui->comboBox_FilterType->currentData().toInt();
    loadFilterCategories(type); // 重新加载分类
}

// 时间戳转换代理 (TimeDelegate)
// 作用：将数据库里的 Unix 时间戳 (秒) 转换为 "yyyy-MM-dd HH:mm" 格式显示，也负责在编辑时提供“日期时间控件”
class TimeDelegate : public QStyledItemDelegate {
public:
    explicit TimeDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    // 负责将秒数渲染成可读字符串
    QString displayText(const QVariant &value, const QLocale &locale) const override {
        bool ok = false;
        qint64 timestamp = value.toLongLong(&ok);

        // 如果转换成功(ok为true) 且 数值大于0 (有效时间戳)
        if (ok && timestamp > 0) {
            return QDateTime::fromSecsSinceEpoch(timestamp).toString("yyyy-MM-dd HH:mm");
        }

        return QStyledItemDelegate::displayText(value, locale);
    }

    // 当用户双击时，创建一个 QDateTimeEdit
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        Q_UNUSED(option);
        Q_UNUSED(index);

        QDateTimeEdit *editor = new QDateTimeEdit(parent);
        editor->setDisplayFormat("yyyy-MM-dd HH:mm"); // 设定编辑时的显示格式
        editor->setCalendarPopup(true); // 开启日历弹窗
        return editor;
    }

    // 把模型里的秒数取出来，设置给编辑器
    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        qint64 timestamp = index.model()->data(index, Qt::EditRole).toLongLong();
        QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp);

        QDateTimeEdit *dateEditor = static_cast<QDateTimeEdit*>(editor);
        dateEditor->setDateTime(dt);
    }

    // 用户修完后，把编辑器的时间转回秒数，存回模型
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const override
    {
        QDateTimeEdit *dateEditor = static_cast<QDateTimeEdit*>(editor);
        QDateTime dt = dateEditor->dateTime();

        // 转回 Unix 时间戳 (秒)
        model->setData(index, dt.toSecsSinceEpoch(), Qt::EditRole);
    }

    // 保证编辑器大小合适
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
                              const QModelIndex &index) const override
    {
        Q_UNUSED(index);
        editor->setGeometry(option.rect);
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

    // 让表格里所有的输入框、下拉框背景都变白，以防单元格编辑时输入框背景透明导致文字重叠
    ui->tableView->setStyleSheet(
        "QTableView QLineEdit { background-color: white; color: black; }"
        "QTableView QComboBox { background-color: white; color: black; }"
        );

    // 当表格数据发生变化（用户编辑）时，重新画图
    connect(model, &QSqlTableModel::dataChanged, this, [this](){
        updateCharts();

        updateSummary();
    });
}

void MainWindow::initCharts()
{
    // 初始化柱状图
    barChart = new QChart();
    barChart->setTitle("收支趋势");
    barChart->setAnimationOptions(QChart::SeriesAnimations);
    ui->chartView_Bar->setChart(barChart);
    ui->chartView_Bar->setRenderHint(QPainter::Antialiasing);

    // 初始化饼图
    pieChart = new QChart();
    pieChart->setTitle("收支构成");
    pieChart->setAnimationOptions(QChart::SeriesAnimations);
    ui->chartView_Pie->setChart(pieChart);
    ui->chartView_Pie->setRenderHint(QPainter::Antialiasing);
}

void MainWindow::updateCharts()
{
    // 更新饼图 (按分类汇总金额)
    pieChart->removeAllSeries();
    QPieSeries *pieSeries = new QPieSeries();

    // 查询：根据当前日期范围汇总 (可复用筛选的时间)
    qint64 startSec = QDateTime(ui->dateEdit_Start->date(), QTime(0,0)).toSecsSinceEpoch();
    qint64 endSec = QDateTime(ui->dateEdit_End->date(), QTime(23,59,59)).toSecsSinceEpoch();

    QSqlQuery query;
    QString sql = "SELECT c.name, SUM(r.amount) FROM record r "
                  "JOIN category c ON r.cid = c.id "
                  "WHERE r.timestamp >= ? AND r.timestamp <= ? "
                  "GROUP BY c.name "
                  "ORDER BY SUM(r.amount) DESC";
    query.prepare(sql);
    query.addBindValue(startSec);
    query.addBindValue(endSec);

    if(query.exec()) {
        while(query.next()) {
            QString name = query.value(0).toString();
            double value = query.value(1).toDouble();
            if(value > 0) { // 只添加金额大于0的
                pieSeries->append(name, value);
            }
        }
    }

    // 设置饼图标签可见
    for(auto slice : pieSeries->slices()) {
        // 获取该切片的百分比 (0.0 ~ 1.0)
        double percent = slice->percentage();

        // 设置标签格式
        QString label = QString("%1: %2%").arg(slice->label()).arg(QString::number(percent * 100, 'f', 1));
        slice->setLabel(label);

        // 只有占比大于 3% 的才默认显示标签，防止重叠
        if (percent < 0.03) {
            slice->setLabelVisible(false);
        } else {
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelOutside); // 标签放外面
        }

        // 添加鼠标悬停交互 (Hover)
        // 当鼠标滑过任何切片（包括那些隐藏标签的小切片）时，突出显示并强制展示标签
        connect(slice, &QPieSlice::hovered, this, [slice, percent](bool state){
            // 鼠标移入(state=true)时炸开(Exploded)，移出复原
            slice->setExploded(state);

            // 鼠标移入时强制显示标签，移出时恢复默认逻辑(大于3%才显)
            if (state) {
                slice->setLabelVisible(true);
            } else {
                slice->setLabelVisible(percent >= 0.03);
            }
        });
    }

    // 设置图例 (Legend)
    // 对于隐藏了标签的小切片，用户可以通过右侧图例看颜色对应
    pieChart->legend()->setVisible(true);
    pieChart->legend()->setAlignment(Qt::AlignRight); // 图例放右边
    pieChart->addSeries(pieSeries);


    // 更新柱状图 (按收支类型汇总)
    barChart->removeAllSeries();
    // 清除旧坐标轴 (防止多次刷新后坐标轴残留)
    QList<QAbstractAxis*> axes = barChart->axes();
    for(auto axis : axes) barChart->removeAxis(axis);

    QBarSeries *barSeries = new QBarSeries();
    QBarSet *setIncome = new QBarSet("收入");
    QBarSet *setExpense = new QBarSet("支出");

    // 查询总收入和总支出
    double totalIncome = 0;
    double totalExpense = 0;

    // 查收入
    query.prepare("SELECT SUM(r.amount) FROM record r JOIN category c ON r.cid = c.id "
                  "WHERE c.type = 1 AND r.timestamp >= ? AND r.timestamp <= ?");
    query.addBindValue(startSec);
    query.addBindValue(endSec);
    if(query.exec() && query.next()) totalIncome = query.value(0).toDouble();

    // 查支出
    query.prepare("SELECT SUM(r.amount) FROM record r JOIN category c ON r.cid = c.id "
                  "WHERE c.type = 0 AND r.timestamp >= ? AND r.timestamp <= ?");
    query.addBindValue(startSec);
    query.addBindValue(endSec);
    if(query.exec() && query.next()) totalExpense = query.value(0).toDouble();

    *setIncome << totalIncome;
    *setExpense << totalExpense;

    // 设置颜色
    setIncome->setColor(QColor(60, 179, 113)); // MediumSeaGreen
    setExpense->setColor(QColor(220, 20, 60)); // Crimson

    barSeries->append(setIncome);
    barSeries->append(setExpense);

    // 显示数值标签 (在柱子上显示数字)
    barSeries->setLabelsVisible(true);

    barChart->addSeries(barSeries);

    // 创建坐标轴
    QStringList categories; categories << "总计";
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    barChart->addAxis(axisX, Qt::AlignBottom);
    barSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    barChart->addAxis(axisY, Qt::AlignLeft);
    // 让Y轴稍微高一点，避免柱子顶到头
    double maxVal = qMax(totalIncome, totalExpense);
    axisY->setRange(0, maxVal * 1.2);
    // 设置Y轴标签格式 (不显示小数)
    axisY->setLabelFormat("%.0f");

    barSeries->attachAxis(axisY);
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

void MainWindow::updateSummary()
{
    // 获取当前筛选的时间范围
    qint64 startSec = QDateTime(ui->dateEdit_Start->date(), QTime(0,0)).toSecsSinceEpoch();
    qint64 endSec = QDateTime(ui->dateEdit_End->date(), QTime(23,59,59)).toSecsSinceEpoch();

    QSqlQuery query;
    double totalIncome = 0.0;
    double totalExpense = 0.0;

    // 计算总收入 (type = 1)
    // 这里演示“基于当前时间范围”的统计：
    query.prepare("SELECT SUM(r.amount) FROM record r "
                  "JOIN category c ON r.cid = c.id "
                  "WHERE c.type = 1 AND r.timestamp >= ? AND r.timestamp <= ?");
    query.addBindValue(startSec);
    query.addBindValue(endSec);
    if (query.exec() && query.next()) {
        totalIncome = query.value(0).toDouble();
    }

    // 计算总支出 (type = 0)
    query.prepare("SELECT SUM(r.amount) FROM record r "
                  "JOIN category c ON r.cid = c.id "
                  "WHERE c.type = 0 AND r.timestamp >= ? AND r.timestamp <= ?");
    query.addBindValue(startSec);
    query.addBindValue(endSec);
    if (query.exec() && query.next()) {
        totalExpense = query.value(0).toDouble();
    }

    // 更新 UI
    ui->lbl_TotalIncome->setText(QString::number(totalIncome, 'f', 2));
    ui->lbl_TotalExpense->setText(QString::number(totalExpense, 'f', 2));

    double balance = totalIncome - totalExpense;
    ui->lbl_TotalBalance->setText(QString::number(balance, 'f', 2));

    // 结余颜色：正数黑色，负数红色
    if (balance >= 0) {
        ui->lbl_TotalBalance->setStyleSheet("color: black; font-weight: bold; font-size: 14px;");
    } else {
        ui->lbl_TotalBalance->setStyleSheet("color: red; font-weight: bold; font-size: 14px;");
    }
}


void MainWindow::on_actionManageCategory_triggered()
{
    CategoryDialog dlg(this);
    dlg.exec(); // 模态显示

    // 当窗口关闭后，主界面可能需要刷新
    // 刷新主界面的筛选下拉框（分类可能变了）
    int currentType = ui->comboBox_FilterType->currentData().toInt();
    loadFilterCategories(currentType);

    // 刷新表格（如果用户删除了分类，表格里的记录会变化）
    model->select();
    updateCharts();

    updateSummary();
}

