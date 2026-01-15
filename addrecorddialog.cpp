#include "addrecorddialog.h"
#include "ui_addrecorddialog.h"
#include "databasemanager.h"
#include <QMessageBox>

AddRecordDialog::AddRecordDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AddRecordDialog)
{
    ui->setupUi(this);

    // 初始化收支类型 ComboBox
    // 先清空 UI 设计器里的数据
    ui->combo_Type->clear();
    // 添加带 UserData 的项：显示文本, 实际值(0或1)
    ui->combo_Type->addItem("支出", 0);
    ui->combo_Type->addItem("收入", 1);

    // 设置日期默认为今天
    ui->dateEdit->setDate(QDate::currentDate());

    // 触发一次分类加载（默认加载支出的分类）
    loadCategories(0);

}

AddRecordDialog::~AddRecordDialog()
{
    delete ui;
}

void AddRecordDialog::loadCategories(int type)
{
    ui->combo_Category->clear();

    // 从数据库获取分类
    QSqlQuery query = DatabaseManager::instance().getCategories(type);
    while (query.next()) {
        QString name = query.value("name").toString();
        int id = query.value("id").toInt();
        // ItemData 存储 ID
        ui->combo_Category->addItem(name, id);
    }
}

void AddRecordDialog::on_combo_Type_currentIndexChanged(int index)
{
    // 获取当前选中的类型 (0或1)
    int type = ui->combo_Type->currentData().toInt();
    loadCategories(type);
}

AddRecordDialog::RecordData AddRecordDialog::getRecordData() const
{
    RecordData data;
    data.amount = ui->lineEdit_Amount->text().toDouble();
    data.date = ui->dateEdit->date();
    data.note = ui->textEdit_Note->toPlainText();
    // 获取选中的分类ID
    data.categoryId = ui->combo_Category->currentData().toInt();
    return data;
}
