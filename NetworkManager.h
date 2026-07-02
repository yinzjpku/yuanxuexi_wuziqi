#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>

class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    bool host(quint16 port);
    void join(const QString &hostAddress, quint16 port);
    void disconnectFromPeer();
    bool isConnected() const;
    bool isHosting() const { return m_isHosting; }

    QString localAddressText() const;
    QString errorText() const { return m_errorText; }
    void sendMessage(const QJsonObject &message);

signals:
    void connected();
    void disconnected();
    void messageReceived(const QJsonObject &message);
    void statusChanged(const QString &status);
    void errorOccurred(const QString &message);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    void setSocket(QTcpSocket *socket);
    void clearSocket();

    QTcpServer *m_server;
    QTcpSocket *m_socket;
    QByteArray m_buffer;
    QString m_errorText;
    bool m_isHosting;
};

#endif // NETWORKMANAGER_H
