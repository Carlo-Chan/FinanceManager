#ifndef CATEGORYDIALOG_H
#define CATEGORYDIALOG_H

#include <QDialog>
#include <QListWidget>

namespace Ui {
class CategoryDialog;
}

class CategoryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CategoryDialog(QWidget *parent = nullptr);
    ~CategoryDialog();

private slots:
    void on_btnAdd_clicked();

    void on_btnDelete_clicked();

private:
    Ui::CategoryDialog *ui;

    void loadAllCategories();
    // 辅助：获取当前激活的列表控件和类型
    QListWidget* getCurrentList(int &type);
};

#endif // CATEGORYDIALOG_H
