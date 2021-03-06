#include <QDBusConnection>
#include <QDBusError>

#include <QSslSocket>
#include <QTextStream>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>

#include "mailManager.h"

#define SERVER_READY "220"
#define ACTION_COMPLETED "250"
#define USER_AUTH "334"
#define PASS_AUTH "235"
#define DATA_RESPONSE "354"
#define DISCONNECT_COMPLETED "221"

mailManager::mailManager(QObject *parent) : QObject(parent)
{
    loadServerCredentials();
    loadSendMailDetails();

    m_textStream = new QTextStream(m_socket);

    connect(m_socket, SIGNAL(connected()), this, SLOT(connected()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this,
            SLOT(errorReceived(QAbstractSocket::SocketError)));
    connect(m_socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this,
            SLOT(stateChanged(QAbstractSocket::SocketState)));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(disconnected()));

    m_watchdog = new WatchdogHelper("piHomeMail");
    m_watchdog->init();
}

mailManager::~mailManager()
{
    qDebug() << "mailManager destructor";
    delete m_textStream;
    delete m_socket;

    m_watchdog->stop();
    m_watchdog->deleteLater();
}

bool mailManager::firstRunConfiguration()
{
    bool returnCode = false;

    QString settingsPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation).
            append(QDir::separator()).
            append(QCoreApplication::organizationName()).
            append(QDir::separator()).
            append(QCoreApplication::applicationName()).
            append(".conf");
    qDebug() << "Path: " << settingsPath;

    if(!QFile(settingsPath).exists())
    {
        returnCode = true;

        QSettings settings(settingsPath, QSettings::NativeFormat);

        settings.clear();
        settings.beginGroup("ServerSettings");
        settings.setValue("ServerName", "smtp.mail.com");
        settings.setValue("User", "user@mail.com");
        settings.setValue("Password", "password"); //@TODO use encryption
        settings.setValue("Port", 465);
        settings.setValue("Timeout_ms","30000");
        settings.setValue("Protocol", SSL);
        settings.setValue("Login", Encrypted);
        settings.endGroup();

        settings.beginGroup("SendMailSettings");
        settings.setValue("Sender", "sender@mail.com");
        settings.setValue("NumberOfRecipients", 2);
        settings.setValue("Recipient_1", "recipient1@mail.com");
        settings.setValue("Recipient_2", "recipient2@mail.com");
        settings.endGroup();
    }

    return returnCode;
}

void mailManager::loadServerCredentials()
{
    QSettings settings;
    settings.beginGroup("ServerSettings");
    m_serverName = settings.value("ServerName").toString();
    m_userName = settings.value("User").toString();
    m_password = settings.value("Password").toString(); //@TODO use encryption
    m_port = settings.value("Port").toInt();
    m_timeout = settings.value("Timeout_ms").toInt();

    switch(settings.value("Protocol").toInt())
    {
    case None:
        m_connection = None;
        m_socket = new QTcpSocket(this);
        break;
    case SSL:
        m_connection = SSL;
        m_socket = new QSslSocket(this);
        break;
    case TLS:
        m_connection = TLS;
        m_socket = new QSslSocket(this);
        break;
    default:
        qCritical() << "Invalid connection type specified!";
        break;
    }

    switch (settings.value("Login").toInt())
    {
    case Plain:
        m_authentication = Plain;
        break;
    case Encrypted:
        m_authentication = Encrypted;
        break;
    default:
        qCritical() << "Invalid authenthication type specified!";
        break;
    }

    settings.endGroup();
}

void mailManager::loadSendMailDetails()
{
    int recipients = -1;
    QSettings settings;
    settings.beginGroup("SendMailSettings");
    m_sender = settings.value("Sender").toString();
    recipients = settings.value("NumberOfRecipients").toInt();
    for(int count = 1; count <= recipients; count++)
        m_recipients.append(settings.value(QString("Recipient_").append(QString::number(count))).toString());
    settings.endGroup();
}

bool mailManager::connectService()
{
    bool ret = true;

    if(!QDBusConnection::systemBus().registerService(MAIL_MANAGER_SERVICE_NAME))
    {
        qCritical() << QDBusConnection::systemBus().lastError().message();
        ret = false;
    }

    QDBusConnection::systemBus().registerObject(MAIL_MANAGER_SERVICE_PATH,
                                                 this,
                                                 QDBusConnection::ExportScriptableContents);

    if(ret == true)
        qDebug() << "Registered the mail manager service to DBUS system bus";

    return ret;
}

void mailManager::stateChanged(QAbstractSocket::SocketState socketState)
{
    qDebug() <<"stateChanged() - " << socketState;
}

void mailManager::errorReceived(QAbstractSocket::SocketError socketError)
{
    qDebug() << "errorReceived() - " << socketError << ": " << m_socket->errorString();
}

void mailManager::disconnected()
{
    qDebug() << "disconnected() - "  << m_socket->errorString();
}

void mailManager::connected()
{
    qDebug() << "connected()";
}

void mailManager::sendToServer(const QString &text)
{
    *m_textStream << text << "\r\n";
    m_textStream->flush();
}

QString mailManager::replyFromServer()
{
    int timeoutError = 0;
    do
    {
        if(!m_socket->waitForReadyRead())
        {
            qCritical() << "Communication timeout!";
            timeoutError++;
        }
        if(timeoutError >= 25)
            qFatal("Too many communication timeout errors!");

        QString lineResponse;
        while(m_socket->canReadLine())
        {
            lineResponse = m_socket->readLine();
            m_response += lineResponse;
        }

        if(lineResponse[3] == ' ')
        {
            lineResponse.truncate(3);
            qDebug() << "Reply: " << lineResponse;
            return lineResponse;
        }

    } while(true);

    return "";
}

bool mailManager::connectToServer()
{
    QString serverReplyCode;

    switch(m_connection)
    {
    case None:
    case TLS:
        m_socket->connectToHost(m_serverName, m_port);
        break;
    case SSL:
        ((QSslSocket *) m_socket)->connectToHostEncrypted(m_serverName, m_port);
        break;
    default:
        qWarning() << "Connection type not supported!";
        return false;
        break;
    }

    if(!m_socket->waitForConnected(m_timeout))
    {
        qWarning() << "Connection error: " << m_socket->errorString();
        return false;
    }

    serverReplyCode = replyFromServer();
    if(serverReplyCode != SERVER_READY)
    {
        qCritical() << "Cannot connect to server!";
        return false;
    }

    sendToServer("EHLO " + m_serverName); // "EHLO localhost"

    serverReplyCode = replyFromServer();

    if(serverReplyCode != ACTION_COMPLETED)
    {
        qCritical() << "No EHLO reply from server!";
        return false;
    }

    if(m_connection == TLS)
    {
        sendToServer("STARTTLS");

        serverReplyCode = replyFromServer();

        if(serverReplyCode != SERVER_READY)
        {
            qCritical() << "No STARTTLS reply from server!";
            return false;
        }

        ((QSslSocket*) m_socket)->startClientEncryption();

        if(!((QSslSocket*) m_socket)->waitForEncrypted(m_timeout))
        {
            qCritical() << ((QSslSocket*) m_socket)->errorString();
            return false;
        }

        sendToServer("EHLO " + m_serverName); // "EHLO localhost"

        serverReplyCode = replyFromServer();

        if(serverReplyCode != ACTION_COMPLETED)
        {
            qCritical() << "No EHLO encrypted reply from server!";
            return false;
        }
    }

    qDebug() << "Connected to smtp server";

    return true;
}

bool mailManager::loginToServer()
{
    QString serverReplyCode;

    switch(m_authentication)
    {
    case Plain:
        sendToServer("AUTH PLAIN " + QByteArray().append((char) 0).append(m_userName).append((char) 0).append(m_password).toBase64());

        serverReplyCode = replyFromServer();
        if(serverReplyCode != PASS_AUTH)
        {
            qCritical() << "No AUTH LOGIN reply from server!(Plain)";
            return false;
        }
        break;

    case Encrypted:
        sendToServer("AUTH LOGIN");

        serverReplyCode = replyFromServer();
        if(serverReplyCode != USER_AUTH)
        {
            qCritical() << "No AUTH LOGIN reply from server!(Login)";
            return false;
        }

        sendToServer(QByteArray().append(m_userName).toBase64());

        serverReplyCode = replyFromServer();
        if(serverReplyCode != USER_AUTH)
        {
            qCritical() << "No USER AUTH reply from server!";
            return false;
        }

        sendToServer(QByteArray().append(m_password).toBase64());

        serverReplyCode = replyFromServer();
        if(serverReplyCode != PASS_AUTH)
        {
            qCritical() << "No PASSWORD AUTH reply from server!";
            return false;
        }
        break;

    default:
        qCritical() << "Authentication method not supported";
        return false;
        break;
    }

    qDebug() << "Login to smtp server";

    return true;
}


bool mailManager::sendMail(const QString &subject,
                           const QString &message)
{
    QString serverReplyCode;

    m_message = "To: ";
    for(int count = 0; count < m_recipients.count(); count++)
        m_message.append(m_recipients.at(count)).append(", ");
    m_message.append("\n");
    m_message.append("From: " + m_sender + "\n");
    m_message.append("Subject: " + subject + "\n");
    m_message.append("Mime-Version: 1.0;\n");
    m_message.append("Content-Type: text/html; charset=\"utf8\";\n");
    m_message.append("Content-Transfer-Encoding: 8bit;\n");
    m_message.append("\n");
    m_message.append(message);
    m_message.replace(QString::fromLatin1("\n"),
                      QString::fromLatin1("\r\n"));

    m_message.replace(QString::fromLatin1("\r\n.\r\n"),
                      QString::fromLatin1("\r\n..\r\n"));

    qDebug() << "sendMail() :" << endl;
    qDebug() << m_message << endl;
    qDebug() << "-------" << endl;


    sendToServer("MAIL FROM:<"+ m_sender + ">");

    serverReplyCode = replyFromServer();
    if(serverReplyCode != ACTION_COMPLETED)
    {
        qCritical() << "No MAIL FROM reply from server!";
        return false;
    }

    for(int count = 0; count < m_recipients.count(); count++)
    {
        qCritical() << "RCPT TO: " << m_recipients.at(count);
        sendToServer("RCPT TO: <" + m_recipients.at(count) + ">");

        serverReplyCode = replyFromServer();
        if(serverReplyCode != ACTION_COMPLETED)
        {
            qCritical() << "No MAIL TO reply from server!";
            return false;
        }
    }

    sendToServer("DATA");

    serverReplyCode = replyFromServer();
    if(serverReplyCode != DATA_RESPONSE)
    {
        qCritical() << "No DATA reply from server!";
        return false;
    }

    sendToServer(m_message);
    sendToServer(".");

    serverReplyCode = replyFromServer();
    if(serverReplyCode != ACTION_COMPLETED)
    {
        qCritical() << "No MAIL MESSAGE reply from server!";
        return false;
    }

    qDebug() << "Mail sent to smtp server";

    return true;
}

bool mailManager::disconnectFromServer()
{
    QString serverReplyCode;

    sendToServer("QUIT");
    serverReplyCode = replyFromServer();
    if(serverReplyCode != DISCONNECT_COMPLETED)
    {
        qCritical() << "No QUIT reply from server!";
        return false;
    }

    qDebug() << "Disconnected from smtp server";

    return true;
}
