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

int DatabaseManager::getUncategorizedId(int type)
{
    QSqlQuery query;
    // 先查找是否存在名为“未分类”且类型匹配的记录
    query.prepare("SELECT id FROM category WHERE name = '未分类' AND type = :type");
    query.bindValue(":type", type);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }

    // 如果不存在，则创建一个
    query.prepare("INSERT INTO category (name, type) VALUES ('未分类', :type)");
    query.bindValue(":type", type);
    if (query.exec()) {
        return query.lastInsertId().toInt(); // 返回新生成的ID
    }

    return -1; // 出错
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
bool DatabaseManager::insertRecord(double amount, const QDateTime& datetime, const QString& note, int cid)
{
    QSqlQuery query;
    query.prepare("INSERT INTO record (amount, timestamp, note, cid) "
                  "VALUES (:amount, :time, :note, :cid)");

    query.bindValue(":amount", amount);
    // 统一处理日期转时间戳，存储为 Unix 时间戳 (秒)
    query.bindValue(":time", datetime.toSecsSinceEpoch());
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

bool DatabaseManager::addCategory(const QString &name, int type)
{
    if (isCategoryNameExist(name, type)) return false; // 防止重复

    QSqlQuery query;
    query.prepare("INSERT INTO category (name, type) VALUES (:name, :type)");
    query.bindValue(":name", name);
    query.bindValue(":type", type);
    return query.exec();
}

bool DatabaseManager::removeCategory(int id, int type, bool keepRecords)
{
    m_db.transaction(); // 开启事务，保证原子性

    // 如果选择保留记录
    if (keepRecords) {
        int targetId = getUncategorizedId(type);
        if (targetId == -1) {
            m_db.rollback();
            return false;
        }

        // 防止用户试图删除“未分类”本身导致逻辑死循环
        if (targetId == id) {
            // 如果用户就是在删“未分类”，那就不能保留了，只能拒绝或清空
            // 这里简单处理：直接返回失败，不允许删除“未分类”
            m_db.rollback();
            return false;
        }

        // 执行转移：把原分类下的账单移动到“未分类”
        QSqlQuery updateQuery;
        updateQuery.prepare("UPDATE record SET cid = :newId WHERE cid = :oldId");
        updateQuery.bindValue(":newId", targetId);
        updateQuery.bindValue(":oldId", id);

        if (!updateQuery.exec()) {
            m_db.rollback();
            return false;
        }
    }

    // 删除分类
    // (如果keepRecords为真，此时该分类下已经没有账单了，删除安全)
    // (如果keepRecords为假，Cascade机制会自动删除关联账单)
    QSqlQuery deleteQuery;
    deleteQuery.prepare("DELETE FROM category WHERE id = :id");
    deleteQuery.bindValue(":id", id);

    if (deleteQuery.exec()) {
        m_db.commit(); // 提交事务
        return true;
    } else {
        m_db.rollback(); // 回滚
        return false;
    }
}

bool DatabaseManager::isCategoryNameExist(const QString &name, int type)
{
    QSqlQuery query;
    query.prepare("SELECT count(*) FROM category WHERE name = :name AND type = :type");
    query.bindValue(":name", name);
    query.bindValue(":type", type);
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}
