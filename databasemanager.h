#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDate>

class DatabaseManager
{
public:
    // 获取单例实例 (静态方法)
    static DatabaseManager& instance();

    // 连接并打开数据库
    bool openDatabase(const QString& path);

    // 自动初始化表结构
    void initTables();

    // 封装一些常用的业务操作
    bool insertRecord(double amount, const QDateTime& datetime, const QString& note, int cid);
    QSqlQuery getCategories(int type); // 获取分类列表

    bool addCategory(const QString& name, int type);
    bool removeCategory(int id, int type, bool keepRecords);
    bool isCategoryNameExist(const QString& name, int type); // 防止同名

private:
    // 构造函数私有化，禁止外部 new
    DatabaseManager();
    ~DatabaseManager();

    QSqlDatabase m_db;

    // 辅助：获取（或创建）“未分类”的ID
    int getUncategorizedId(int type);
};

#endif // DATABASEMANAGER_H
