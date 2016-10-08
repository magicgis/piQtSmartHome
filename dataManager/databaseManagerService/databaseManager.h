#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>

#include "smartHomeInfo.h"
#include "homeAlarmInfo.h"
#include "watchdogHelper.h"

#define DATABASE_MANAGER_SERVICE_NAME "org.raspberrypi.piHome.DatabaseManager"
#define DATABASE_MANAGER_SERVICE_PATH "/"
#define DATABASE_MANAGER_SERVICE_INT  "org.raspberrypi.piHome.DatabaseManager.DatabaseOperations"

class QSettings;

class DatabaseManager : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", DATABASE_MANAGER_SERVICE_INT)

public:
    explicit DatabaseManager(QObject *parent = 0);
    virtual ~DatabaseManager();

    bool connectService();

    static bool firstRunConfiguration();

public slots:
    Q_SCRIPTABLE bool insertHomeAlarmEntry(const HomeAlarmInfo&) const;
    Q_SCRIPTABLE bool insertSmartHomeEntry(const SmartHomeInfo&) const;
    Q_SCRIPTABLE bool insertIO(const QString&,
                               const QString&,
                               const QString&,
                               const QString&,
                               const QString&,
                               const QString&,
                               const QString&) const;
    Q_SCRIPTABLE QList<HomeAlarmInfo> getAllHomeAlarmEntries() const;
    Q_SCRIPTABLE QList<SmartHomeInfo> getAllSmartHomeEntries() const;
    Q_SCRIPTABLE QStringList getAllZones() const;
    Q_SCRIPTABLE QStringList getAllCategories() const;
    Q_SCRIPTABLE QStringList getAllFromZone(const QString&) const;
    Q_SCRIPTABLE QStringList getAllFromCategory(const QString&) const;

private:
    void loadDatabaseSettings();

    bool connectToDatabase();
    bool createDatabaseAndTable();

    QSqlDatabase m_db;
    QSettings *m_settings;

    QString m_databaseFilePath;
    QString m_databaseFileName;

    WatchdogHelper *m_watchdog;
};

#endif // DATABASEMANAGER_H
