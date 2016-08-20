#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>

#include "databaseManager.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    app.setOrganizationName("piHome");
    app.setApplicationName("databaseManager");

    qDebug() << "Database Manager";

    DatabaseManager dm;

    if(!dm.connectService())
        qDebug() << "Cannot register the database manager service to session bus";

//    //Used for testing
//    HomeAlarmInfo entry1("Living", "Node1", "Sensor1", "2016/11/11 18:54:42");
//    dm.insertHomeAlarmEntry(entry1);

//    HomeAlarmInfo entry2("Hall", "Node1", "Sensor12");
//    dm.insertHomeAlarmEntry(entry2);

//    HomeAlarmInfo entry3("Bedroom", "Node1", "Sensor17");
//    dm.insertHomeAlarmEntry(entry3);

    int ret = app.exec();

    return ret;
}