#ifndef ADDRECORDDIALOG_H
#define ADDRECORDDIALOG_H

#include <QDialog>
#include <QDate>

namespace Ui {
class AddRecordDialog;
}

class AddRecordDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddRecordDialog(QWidget *parent = nullptr);
    ~AddRecordDialog();

    // 定义结构体方便传递数据
    struct RecordData {
        double amount;
        QDate date;
        QString note;
        int categoryId;
    };
    RecordData getRecordData() const;

private:
    Ui::AddRecordDialog *ui;
    void loadCategories(int type); // 辅助函数

private slots:
    // 类型改变时触发（如从支出变收入）
    void on_combo_Type_currentIndexChanged(int index);
};

#endif // ADDRECORDDIALOG_H
