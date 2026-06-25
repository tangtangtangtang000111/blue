#include "bluetoothcontroller.h"
#include <QDebug>
#include <QCoreApplication>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QJniEnvironment>

static bool androidHasPermissions()
{
    QJniEnvironment env;
    QJniObject ctx = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity",
        "()Landroid/app/Activity;");
    if (!ctx.isValid()) return false;

    const char *needed[] = {
        "android.permission.BLUETOOTH_SCAN",
        "android.permission.BLUETOOTH_CONNECT",
        "android.permission.ACCESS_FINE_LOCATION",
        nullptr
    };

    for (int i = 0; needed[i]; i++) {
        QJniObject perm = QJniObject::fromString(QLatin1String(needed[i]));
        jint r = ctx.callMethod<jint>("checkSelfPermission",
                                       "(Ljava/lang/String;)I",
                                       perm.object<jstring>());
        if (r != 0) return false;  // 0 = PERMISSION_GRANTED
    }
    return true;
}

static void androidRequestPermissions()
{
    QJniEnvironment env;
    QJniObject ctx = QJniObject::callStaticObjectMethod(
        "org/qtproject/qt/android/QtNative", "activity",
        "()Landroid/app/Activity;");
    if (!ctx.isValid()) return;

    const char *needed[] = {
        "android.permission.BLUETOOTH_SCAN",
        "android.permission.BLUETOOTH_CONNECT",
        "android.permission.ACCESS_FINE_LOCATION",
    };

    jclass strCls = env->FindClass("java/lang/String");
    jobjectArray arr = env->NewObjectArray(3, strCls, nullptr);
    for (int i = 0; i < 3; i++) {
        jstring s = env->NewStringUTF(needed[i]);
        env->SetObjectArrayElement(arr, i, s);
        env->DeleteLocalRef(s);
    }
    ctx.callMethod<void>("requestPermissions", "([Ljava/lang/String;I)V", arr, 1);
    env->DeleteLocalRef(arr);
    env->DeleteLocalRef(strCls);
}
#endif

// Standard SPP UUID for HC-05/06
static const QString SPP_UUID = QStringLiteral("00001101-0000-1000-8000-00805F9B34FB");

BluetoothController::BluetoothController(QObject *parent)
    : QObject(parent)
{
    m_heatmapWatchdog = new QTimer(this);
    m_heatmapWatchdog->setSingleShot(true);
    m_heatmapWatchdog->setInterval(HEATMAP_TIMEOUT_MS);
    connect(m_heatmapWatchdog, &QTimer::timeout, this, &BluetoothController::checkHeatmapTimeout);
}

BluetoothController::~BluetoothController()
{
    disconnectFromDevice();
    stopDiscovery();
}

void BluetoothController::setStatus(const QString &msg)
{
    m_statusText = msg;
    emit statusTextChanged();
    emit statusMessage(msg);
    qDebug() << "[BT]" << msg;
}

// ---- Device Discovery ----

void BluetoothController::startDiscovery()
{
    if (m_scanning) return;

#ifdef Q_OS_ANDROID
    if (!androidHasPermissions()) {
        androidRequestPermissions();
        setStatus(QStringLiteral("请授予蓝牙/位置权限后重新扫描"));
        return;
    }
#endif

    if (!m_discoveryAgent) {
        m_discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
        connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
                this, &BluetoothController::onDeviceDiscovered);
        connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
                this, &BluetoothController::onDiscoveryFinished);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        connect(m_discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
#else
        connect(m_discoveryAgent, QOverload<QBluetoothDeviceDiscoveryAgent::Error>::of(&QBluetoothDeviceDiscoveryAgent::error),
#endif
                this, [this](QBluetoothDeviceDiscoveryAgent::Error err) {
            Q_UNUSED(err)
            setStatus(QStringLiteral("扫描出错: %1").arg(m_discoveryAgent->errorString()));
            m_scanning = false;
            emit scanningChanged();
        });
    }

    m_discoveredDevices.clear();
    m_deviceInfos.clear();
    emit devicesChanged();

    m_scanning = true;
    emit scanningChanged();
    setStatus(QStringLiteral("正在扫描蓝牙设备..."));

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    m_discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::ClassicMethod);
#else
    m_discoveryAgent->start();
#endif
}

void BluetoothController::stopDiscovery()
{
    if (m_discoveryAgent && m_discoveryAgent->isActive()) {
        m_discoveryAgent->stop();
    }
    m_scanning = false;
    emit scanningChanged();
}

void BluetoothController::onDeviceDiscovered(const QBluetoothDeviceInfo &info)
{
    QString name = info.name().trimmed();
    if (name.isEmpty()) return;
    if (m_discoveredDevices.contains(name)) return;

    m_discoveredDevices.append(name);
    m_deviceInfos.append(info);
    emit devicesChanged();

    qDebug() << "[BT] Discovered:" << name << info.address().toString();
}

void BluetoothController::onDiscoveryFinished()
{
    m_scanning = false;
    emit scanningChanged();
    setStatus(QStringLiteral("扫描完成，发现 %1 个设备").arg(m_discoveredDevices.size()));
}

// ---- Connection ----

void BluetoothController::connectToDevice(int index)
{
    if (index < 0 || index >= m_deviceInfos.size()) {
        setStatus(QStringLiteral("设备索引无效"));
        return;
    }

    stopDiscovery();

    if (m_socket) {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    QBluetoothDeviceInfo info = m_deviceInfos.at(index);
    QString name = info.name();

    m_socket = new QBluetoothSocket(QBluetoothServiceInfo::RfcommProtocol, this);

    connect(m_socket, &QBluetoothSocket::connected,
            this, &BluetoothController::onConnected);
    connect(m_socket, &QBluetoothSocket::disconnected,
            this, &BluetoothController::onDisconnected);
    connect(m_socket, &QBluetoothSocket::readyRead,
            this, &BluetoothController::onReadyRead);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    connect(m_socket, &QBluetoothSocket::errorOccurred,
#else
    connect(m_socket, QOverload<QBluetoothSocket::SocketError>::of(&QBluetoothSocket::error),
#endif
            this, &BluetoothController::onSocketError);

    setStatus(QStringLiteral("正在连接到 %1 ...").arg(name));
    m_socket->connectToService(info.address(), QBluetoothUuid(SPP_UUID), QIODevice::ReadWrite);
}

void BluetoothController::disconnectFromDevice()
{
    m_heatmapWatchdog->stop();

    if (m_socket) {
        m_socket->close();
    }

    if (m_connected) {
        m_connected = false;
        m_connectedDeviceName.clear();
        emit connectedChanged();

        if (m_hasHeatmapData) {
            m_hasHeatmapData = false;
            emit hasHeatmapDataChanged();
        }

        setStatus(QStringLiteral("已断开连接"));
    }
}

void BluetoothController::onConnected()
{
    m_connected = true;
    m_connectedDeviceName = m_socket->peerName();
    if (m_connectedDeviceName.isEmpty()) {
        m_connectedDeviceName = m_socket->peerAddress().toString();
    }
    m_rxBuffer.clear();

    // Reset heatmap state and start watchdog
    m_hasHeatmapData = false;
    emit hasHeatmapDataChanged();
    m_heatmapWatchdog->start();

    emit connectedChanged();
    setStatus(QStringLiteral("已连接到 %1").arg(m_connectedDeviceName));
}

void BluetoothController::onDisconnected()
{
    m_heatmapWatchdog->stop();

    m_connected = false;
    m_connectedDeviceName.clear();
    emit connectedChanged();

    if (m_hasHeatmapData) {
        m_hasHeatmapData = false;
        emit hasHeatmapDataChanged();
    }

    setStatus(QStringLiteral("连接已断开"));
}

void BluetoothController::onSocketError(QBluetoothSocket::SocketError error)
{
    Q_UNUSED(error)
    setStatus(QStringLiteral("蓝牙错误: %1").arg(m_socket->errorString()));
}

// ---- Heatmap Watchdog ----

void BluetoothController::checkHeatmapTimeout()
{
    if (!m_hasHeatmapData) {
        // Still no data after timeout — notify QML
        emit hasHeatmapDataChanged(); // stays false
        qDebug() << "[BT] Heatmap data timeout — no matrix received";
    }
}

// ---- Data Receive (32x24 matrix) ----

void BluetoothController::onReadyRead()
{
    if (!m_socket) return;

    m_rxBuffer.append(m_socket->readAll());

    // Parse frames: header 0xAA 0x55 ... data ... tail 0x55 0xAA
    while (m_rxBuffer.size() >= PACKET_TOTAL) {
        static const QByteArray HEADER("\xAA\x55", 2);
        int headerIdx = m_rxBuffer.indexOf(HEADER);
        if (headerIdx < 0) {
            m_rxBuffer.clear();
            break;
        }

        if (headerIdx > 0) {
            m_rxBuffer.remove(0, headerIdx);
        }

        if (m_rxBuffer.size() < PACKET_TOTAL) break;

        quint8 tail0 = static_cast<quint8>(m_rxBuffer.at(PACKET_TOTAL - 2));
        quint8 tail1 = static_cast<quint8>(m_rxBuffer.at(PACKET_TOTAL - 1));

        if (tail0 == 0x55 && tail1 == 0xAA) {
            // Valid matrix packet
            QVariantList matrixData;
            matrixData.reserve(MATRIX_SIZE);
            for (int i = 2; i < 2 + MATRIX_SIZE; ++i) {
                matrixData.append(static_cast<quint8>(m_rxBuffer.at(i)));
            }
            emit matrixDataReceived(matrixData);

            m_heatmapVersion++;
            emit heatmapVersionChanged();

            // Mark heatmap data as received
            if (!m_hasHeatmapData) {
                m_hasHeatmapData = true;
                m_heatmapWatchdog->stop();
                emit hasHeatmapDataChanged();
            }

            m_rxBuffer.remove(0, PACKET_TOTAL);
        } else {
            m_rxBuffer.remove(0, 1);
        }
    }

    // Prevent buffer overflow on bad data
    if (m_rxBuffer.size() > PACKET_TOTAL * 3) {
        m_rxBuffer.clear();
    }
}

// ---- Command Sending (ASCII protocol) ----

void BluetoothController::sendCommandString(const QString &cmd)
{
    if (!m_connected || !m_socket || !m_socket->isOpen()) {
        setStatus(QStringLiteral("未连接，无法发送指令"));
        return;
    }

    QByteArray data = cmd.toUtf8();
    qint64 written = m_socket->write(data);
    if (written < 0) {
        setStatus(QStringLiteral("发送失败"));
    } else {
        qDebug() << "[BT] Sent:" << cmd;
    }
}
