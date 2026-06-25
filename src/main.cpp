#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "bluetoothcontroller.h"
#include "heatmapimageprovider.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Material style for Android look
    QQuickStyle::setStyle("Material");

    // Create instances
    BluetoothController bluetoothController;
    HeatmapImageProvider *heatmapProvider = new HeatmapImageProvider();

    QQmlApplicationEngine engine;

    // Expose BluetoothController to QML
    engine.rootContext()->setContextProperty("bluetoothController", &bluetoothController);

    // Register heatmap image provider (accessed as "image://heatmap/current")
    engine.addImageProvider(QStringLiteral("heatmap"), heatmapProvider);

    // Connect: matrix data received → update heatmap renderer
    QObject::connect(&bluetoothController, &BluetoothController::matrixDataReceived,
                     heatmapProvider, &HeatmapImageProvider::updateMatrix);

    // Load main QML
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
