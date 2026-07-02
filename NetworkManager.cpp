#include "NetworkManager.h"

#include <QHostAddress>
#include <QNetworkInterface>
#include <QJsonDocument>

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent),
      m_server(new QTcpServer(this)),
      m_socket(nullptr),
      m_isHosting(false)
{
    connect(m_server, &QTcpServer::newConnection, this, &NetworkManager::onNewConnection);
}

NetworkManager::~NetworkManager()
{
    disconnectFromPeer();
}

bool NetworkManager::host(quint16 port)
{
    disconnectFromPeer();
    m_isHosting = true;

    if (!m_server->listen(QHostAddress::AnyIPv4, port)) {
        m_errorText = m_server->errorString();
        emit errorOccurred("创建房间失败：" + m_errorText);
        return false;
    }

    emit statusChanged(QString("房间已创建，端口 %1，等待对方加入...").arg(port));
    return true;
}

void NetworkManager::join(const QString &hostAddress, quint16 port)
{
    disconnectFromPeer();
    m_isHosting = false;

    QTcpSocket *socket = new QTcpSocket(this);
    setSocket(socket);
    emit statusChanged("正在连接房间...");
    socket->connectToHost(hostAddress, port);
}

void NetworkManager::disconnectFromPeer()
{
    if (m_server->isListening()) {
        m_server->close();
    }
    clearSocket();
    m_buffer.clear();
}

bool NetworkManager::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

QString NetworkManager::localAddressText() const
{
    QStringList addresses;
    const QList<QHostAddress> allAddresses = QNetworkInterface::allAddresses();
    for (const QHostAddress &address : allAddresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol &&
            address != QHostAddress::LocalHost) {
            addresses << address.toString();
        }
    }

    if (addresses.isEmpty()) {
        return "127.0.0.1";
    }
    return addresses.join(" / ");
}

void NetworkManager::sendMessage(const QJsonObject &message)
{
    if (!isConnected()) {
        return;
    }

    QJsonDocument doc(message);
    m_socket->write(doc.toJson(QJsonDocument::Compact));
    m_socket->write("\n");
    m_socket->flush();
}

void NetworkManager::onNewConnection()
{
    if (m_socket) {
        QTcpSocket *extraSocket = m_server->nextPendingConnection();
        extraSocket->disconnectFromHost();
        extraSocket->deleteLater();
        return;
    }

    setSocket(m_server->nextPendingConnection());
    emit statusChanged("对方已加入房间，可以开始联网对战。");
    emit connected();
}

void NetworkManager::onReadyRead()
{
    if (!m_socket) {
        return;
    }

    m_buffer.append(m_socket->readAll());
    int newlineIndex = -1;
    while ((newlineIndex = m_buffer.indexOf('\n')) >= 0) {
        QByteArray line = m_buffer.left(newlineIndex).trimmed();
        m_buffer.remove(0, newlineIndex + 1);

        if (line.isEmpty()) {
            continue;
        }

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(line, &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            emit messageReceived(doc.object());
        }
    }
}

void NetworkManager::onSocketDisconnected()
{
    clearSocket();
    emit statusChanged("连接已断开。");
    emit disconnected();
}

void NetworkManager::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    if (!m_socket) {
        return;
    }
    m_errorText = m_socket->errorString();
    emit errorOccurred("网络错误：" + m_errorText);
}

void NetworkManager::setSocket(QTcpSocket *socket)
{
    clearSocket();
    m_socket = socket;
    m_socket->setParent(this);
    connect(m_socket, &QTcpSocket::connected, this, [this]() {
        emit statusChanged("已连接到房间，可以开始联网对战。");
        emit connected();
    });
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkManager::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred,
            this, &NetworkManager::onSocketError);
}

void NetworkManager::clearSocket()
{
    if (!m_socket) {
        return;
    }

    QTcpSocket *oldSocket = m_socket;
    m_socket = nullptr;
    oldSocket->disconnect(this);
    if (oldSocket->state() != QAbstractSocket::UnconnectedState) {
        oldSocket->disconnectFromHost();
    }
    oldSocket->deleteLater();
}
