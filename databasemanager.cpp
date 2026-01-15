#include "databasemanager.h"
#include <QDateTime>

// 单例实现
DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager _instance;
    return _instance;
}

DatabaseManager::DatabaseManager()
{
    // 在构造时不做连接，留给 openDatabase 显式调用
}

DatabaseManager::~DatabaseManager()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DatabaseManager::openDatabase(const QString& path)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(path);

    if (!m_db.open()) {
        qDebug() << "Error: connection with database failed";
        return false;
    }

    // 连接成功后，顺便检查一下表存不存在
    initTables();
    return true;
}

void DatabaseManager::initTables()
{
    QSqlQuery query;

    // 开启外键
    query.exec("PRAGMA foreign_keys = ON;");

    // 建 Category 表
    query.exec("CREATE TABLE IF NOT EXISTS category ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "name TEXT NOT NULL, "
               "type INTEGER NOT NULL DEFAULT 0)");

    // 建 Record 表
    query.exec("CREATE TABLE IF NOT EXISTS record ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "amount REAL NOT NULL, "
               "timestamp INTEGER NOT NULL, "
               "note TEXT, "
               "cid INTEGER NOT NULL, "
               "FOREIGN KEY (cid) REFERENCES category(id) ON DELETE CASCADE)");

    // 检查是否需要初始化基础数据(如果分类表为空)
    query.exec("SELECT count(*) FROM category");
    if (query.next() && query.value(0).toInt() == 0) {
        // 插入一些默认数据
        QStringList expenses = {"餐饮美食", "交通出行", "生活日用", "娱乐休闲", "住房水电"};
        QStringList incomes = {"工资薪金", "奖金补贴", "投资理财"};

        QSqlQuery insertQuery;
        insertQuery.prepare("INSERT INTO category (name, type) VALUES (?, ?)");

        for(const auto& name : expenses) {
            insertQuery.addBindValue(name);
            insertQuery.addBindValue(0); // 支出
            insertQuery.exec();
        }
        for(const auto& name : incomes) {
            insertQuery.addBindValue(name);
            insertQuery.addBindValue(1); // 收入
            insertQuery.exec();
        }
    }
}

// 封装插入操作
bool DatabaseManager::insertRecord(double amount, const QDate& date, const QString& note, int cid)
{
    QSqlQuery query;
    query.prepare("INSERT INTO record (amount, timestamp, note, cid) "
                  "VALUES (:amount, :time, :note, :cid)");

    query.bindValue(":amount", amount);
    // 统一处理日期转时间戳，存储为 Unix 时间戳 (秒)
    query.bindValue(":time", QDateTime(date, QTime(0,0)).toSecsSinceEpoch());
    query.bindValue(":note", note);
    query.bindValue(":cid", cid);

    if (!query.exec()) {
        qDebug() << "Insert error:" << query.lastError().text();
        return false;
    }
    return true;
}

// 封装查询分类
QSqlQuery DatabaseManager::getCategories(int type)
{
    QSqlQuery query;
    // 如果 type == -1，则查询所有分类（用于主界面筛选）
    if (type == -1) {
        query.prepare("SELECT name, id FROM category");
    } else {
        query.prepare("SELECT name, id FROM category WHERE type = :type");
        query.bindValue(":type", type);
    }
    query.exec();
    return query;
}
