#include "categorydialog.h"
#include "ui_categorydialog.h"
#include "databasemanager.h"
#include <QInputDialog>
#include <QMessageBox>

CategoryDialog::CategoryDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CategoryDialog)
{
    ui->setupUi(this);
    loadAllCategories();
}

CategoryDialog::~CategoryDialog()
{
    delete ui;
}

void CategoryDialog::on_btnAdd_clicked()
{
    int type;
    getCurrentList(type); // 获取当前是收入页还是支出页

    bool ok;
    QString text = QInputDialog::getText(this, "新增分类",
                                         "请输入分类名称:", QLineEdit::Normal,
                                         "", &ok);
    if (ok && !text.isEmpty()) {
        text = text.trimmed();
        if (DatabaseManager::instance().addCategory(text, type)) {
            loadAllCategories(); // 刷新列表
        } else {
            QMessageBox::warning(this, "错误", "添加失败，可能分类名已存在。");
        }
    }
}


void CategoryDialog::on_btnDelete_clicked()
{
    int type;
    QListWidget *list = getCurrentList(type);

    QListWidgetItem *item = list->currentItem();
    if (!item) {
        QMessageBox::warning(this, "提示", "请先选择一个分类");
        return;
    }

    // 获取之前存进去的 ID
    int id = item->data(Qt::UserRole).toInt();
    QString name = item->text();

    // 禁止删除“未分类”
    if (name == "未分类") {
        QMessageBox::warning(this, "禁止操作", "系统预置分类“未分类”不可删除！");
        return;
    }

    // 创建询问框
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("删除分类");
    msgBox.setText(QString("您确定要删除分类“%1”吗？").arg(name));
    msgBox.setInformativeText("该分类下可能包含已记录的账单。\n您希望如何处理这些账单？");
    msgBox.setIcon(QMessageBox::Question);

    // 添加自定义按钮
    // 按钮1：保留账单
    QAbstractButton *btnKeep = msgBox.addButton("保留账单 (归入未分类)", QMessageBox::AcceptRole);
    // 按钮2：全部删除
    QAbstractButton *btnDeleteAll = msgBox.addButton("连同账单一起删除", QMessageBox::DestructiveRole);
    // 按钮3：取消
    QAbstractButton *btnCancel = msgBox.addButton("取消", QMessageBox::RejectRole);

    msgBox.exec(); // 显示弹窗

    if (msgBox.clickedButton() == btnCancel) {
        return; // 用户点了取消
    }

    // 判断用户选了哪个
    bool keepRecords = (msgBox.clickedButton() == btnKeep);

    // 调用删除接口
    if (DatabaseManager::instance().removeCategory(id, type, keepRecords)) {
        loadAllCategories(); // 刷新列表
        QMessageBox::information(this, "成功", "分类已删除");
    } else {
        QMessageBox::critical(this, "错误", "删除失败");
    }
}

void CategoryDialog::loadAllCategories()
{
    ui->listExpense->clear();
    ui->listIncome->clear();

    // 加载支出 (type=0)
    QSqlQuery query0 = DatabaseManager::instance().getCategories(0);
    while (query0.next()) {
        QListWidgetItem *item = new QListWidgetItem(query0.value("name").toString());
        // 把 ID 存在 Item 的 UserRole 里，删除时要用
        item->setData(Qt::UserRole, query0.value("id").toInt());
        ui->listExpense->addItem(item);
    }

    // 加载收入 (type=1)
    QSqlQuery query1 = DatabaseManager::instance().getCategories(1);
    while (query1.next()) {
        QListWidgetItem *item = new QListWidgetItem(query1.value("name").toString());
        item->setData(Qt::UserRole, query1.value("id").toInt());
        ui->listIncome->addItem(item);
    }
}

QListWidget *CategoryDialog::getCurrentList(int &type)
{
    if (ui->tabWidget->currentIndex() == 0) {
        type = 0; // 支出
        return ui->listExpense;
    } else {
        type = 1; // 收入
        return ui->listIncome;
    }
}

