#include <QDir>

#include "actuatorInstantiator.h"

actuatorInstantiator::actuatorInstantiator(QObject *parent) : QObject(parent)
{
    QString filePath = QDir::homePath().append(QDir::separator()).append(".piHome").append(QDir::separator()).append("actuators.ini");
    m_settings = new QSettings(filePath, QSettings::NativeFormat);
}

actuatorInstantiator::~actuatorInstantiator()
{
    delete m_settings;
}

void actuator::debugActuator() const
{
    qDebug() << "==============================================";
    qDebug() << systemTypeToString(m_system);
    qDebug() << actuatorTypeToString(m_type);
    qDebug() << hardwareTypeToString(m_hard);
    qDebug() << m_zone;
    qDebug() << m_node;
    qDebug() << m_address;
    qDebug() << m_value;
}

QList<actuator> actuatorInstantiator::loadActuators() const
{
    int numActuators = -1;
    QList<actuator> list;

    m_settings->beginGroup("GenericSettings");
    numActuators = m_settings->value("NumberOfActuators").toInt();
    m_settings->endGroup();

    for(int i = 1; i <= numActuators; i++) // for humans not machines
    {
        QString group = "Actuator";
        group.append(QString::number(i));
        m_settings->beginGroup(group);
        actuator tmp (
        StringToSystemType(m_settings->value("SystemType").toString()),
        StringToActuatorType(m_settings->value("ActuatorType").toString()),
        StringToHardwareType(m_settings->value("HardwareType").toString()),
        m_settings->value("Zone").toString(),
        m_settings->value("Node").toString(),
        m_settings->value("Address").toString());
        list.append(tmp);
        m_settings->endGroup();
    }

    return list;
}

bool actuatorInstantiator::firstRunInitActuators()
{
    bool returnCode = false;

    QString settingsPath = QDir::homePath().
            append(QDir::separator()).
            append(".piHome").
            append(QDir::separator()).
            append("actuators.ini");

    qDebug() << "firstRunInitActuators()" << settingsPath << " " << QFile(settingsPath).exists();

    if(!QFile(settingsPath).exists())
    {
        returnCode = true;

        QSettings *settings = new QSettings(settingsPath, QSettings::NativeFormat);

        settings->clear();
        settings->beginGroup("GenericSettings");
        settings->setValue("NumberOfActuators", 6);
        settings->endGroup();
        settings->beginGroup("Actuator1");
        settings->setValue("SystemType", systemTypeToString(HomeAlarm));
        settings->setValue("ActuatorType", actuatorTypeToString(Actuator_Siren));
        settings->setValue("HardwareType", hardwareTypeToString(Wireless));
        settings->setValue("Zone", "Hall");
        settings->setValue("Node", "Door");
        settings->setValue("Address", "SPI_0");
        settings->endGroup();
        settings->beginGroup("Actuator2");
        settings->setValue("SystemType", systemTypeToString(HomeAlarm));
        settings->setValue("ActuatorType", actuatorTypeToString(Actuator_Buzzer));
        settings->setValue("HardwareType", hardwareTypeToString(Wireless));
        settings->setValue("Zone", "Kitchen");
        settings->setValue("Node", "Box1");
        settings->setValue("Address", "SPI_0");
        settings->endGroup();
        settings->beginGroup("Actuator3");
        settings->setValue("SystemType", systemTypeToString(HomeAlarm));
        settings->setValue("ActuatorType", actuatorTypeToString(Actuator_Relay));
        settings->setValue("HardwareType", hardwareTypeToString(Wireless));
        settings->setValue("Zone", "Kitchen");
        settings->setValue("Node", "Box1");
        settings->setValue("Address", "SPI_0");
        settings->endGroup();
        settings->beginGroup("Actuator4");
        settings->setValue("SystemType", systemTypeToString(HomeAlarm));
        settings->setValue("ActuatorType", actuatorTypeToString(Actuator_DoorBell));
        settings->setValue("HardwareType", hardwareTypeToString(Wireless));
        settings->setValue("Zone", "Hall");
        settings->setValue("Node", "Door");
        settings->setValue("Address", "SPI_0");
        settings->endGroup();
        settings->beginGroup("Actuator5");
        settings->setValue("SystemType", systemTypeToString(HomeAlarm));
        settings->setValue("ActuatorType", actuatorTypeToString(Actuator_IR));
        settings->setValue("HardwareType", hardwareTypeToString(Wireless));
        settings->setValue("Zone", "Hall");
        settings->setValue("Node", "Box2");
        settings->setValue("Address", "SPI_0");
        settings->endGroup();
        settings->beginGroup("Actuator6");
        settings->setValue("SystemType", systemTypeToString(HomeAlarm));
        settings->setValue("ActuatorType", actuatorTypeToString(Actuator_Pump));
        settings->setValue("HardwareType", hardwareTypeToString(Wireless));
        settings->setValue("Zone", "Balcony");
        settings->setValue("Node", "Box1");
        settings->setValue("Address", "SPI_0");
        settings->endGroup();

        delete settings;
    }

    return returnCode;
}
