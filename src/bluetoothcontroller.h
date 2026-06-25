#ifndef BLUETOOTHCONTROLLER_H
#define BLUETOOTHCONTROLLER_H

#include <QObject>
#include <QBluetoothSocket>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QTimer>
#include <QStringList>
#include <QVariantList>
#include <QByteArray>

class BluetoothController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList discoveredDevices READ discoveredDevices NOTIFY devicesChanged)
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString connectedDeviceName READ connectedDeviceName NOTIFY connectedChanged)
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool hasHeatmapData READ hasHeatmapData NOTIFY hasHeatmapDataChanged)
    Q_PROPERTY(int heatmapVersion READ heatmapVersion NOTIFY heatmapVersionChanged)

public:
    explicit BluetoothController(QObject *parent = nullptr);
    ~BluetoothController();

    QStringList discoveredDevices() const { return m_discoveredDevices; }
    bool isConnected() const { return m_connected; }
    QString connectedDeviceName() const { return m_connectedDeviceName; }
    bool isScanning() const { return m_scanning; }
    QString statusText() const { return m_statusText; }
    bool hasHeatmapData() const { return m_hasHeatmapData; }
    int heatmapVersion() const { return m_heatmapVersion; }

public slots:
    void startDiscovery();
    void stopDiscovery();
    void connectToDevice(int index);
    void disconnectFromDevice();

    /// Send a command string directly (e.g. "ONA", "ONF", "ON0128")
    void sendCommandString(const QString &cmd);

signals:
    void devicesChanged();
    void connectedChanged();
    void scanningChanged();
    void statusTextChanged();
    void hasHeatmapDataChanged();
    void heatmapVersionChanged();
    void matrixDataReceived(const QVariantList &data);
    void statusMessage(const QString &msg);

private slots:
    void onDeviceDiscovered(const QBluetoothDeviceInfo &info);
    void onDiscoveryFinished();
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError(QBluetoothSocket::SocketError error);
    void checkHeatmapTimeout();

private:
    void setStatus(const QString &msg);

    QBluetoothDeviceDiscoveryAgent *m_discoveryAgent = nullptr;
    QBluetoothSocket *m_socket = nullptr;

    QStringList m_discoveredDevices;
    QList<QBluetoothDeviceInfo> m_deviceInfos;

    bool m_connected = false;
    bool m_scanning = false;
    QString m_connectedDeviceName;
    QString m_statusText;

    // Heatmap data tracking
    bool m_hasHeatmapData = false;
    int m_heatmapVersion = 0;
    static constexpr int HEATMAP_TIMEOUT_MS = 3000; // 3s timeout
    QTimer *m_heatmapWatchdog = nullptr;

    // Receive buffer for matrix data
    QByteArray m_rxBuffer;
    static const int MATRIX_SIZE = 768;           // 32 * 24
    static const int PACKET_TOTAL = MATRIX_SIZE + 4; // header(2) + data(768) + tail(2)
};

#endif // BLUETOOTHCONTROLLER_H
