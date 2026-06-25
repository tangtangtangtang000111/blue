#ifndef HEATMAPIMAGEPROVIDER_H
#define HEATMAPIMAGEPROVIDER_H

#include <QQuickImageProvider>
#include <QObject>
#include <QImage>
#include <QVariantList>
#include <QVector>
#include <QMutex>

class HeatmapImageProvider : public QObject, public QQuickImageProvider
{
    Q_OBJECT

public:
    explicit HeatmapImageProvider();

    // QQuickImageProvider interface
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

public slots:
    /// Receive new 32x24 matrix data (768 values, row-major)
    void updateMatrix(const QVariantList &data);

    /// Set the output image size for interpolation
    void setOutputSize(int width, int height);

private:
    void buildColormapLUT();
    void renderHeatmap();
    QColor valueToColor(float normalizedValue) const;

    // Source matrix: 32 columns x 24 rows (row-major)
    float m_sourceMatrix[32 * 24];
    bool m_hasData = false;

    // Output image
    QImage m_outputImage;
    int m_imageVersion = 0;   // Increments on each update, used in URL to bust QML cache
    QMutex m_mutex;

    // Output dimensions
    int m_outputWidth = 640;
    int m_outputHeight = 480;

    // Source dimensions (fixed)
    static constexpr int SRC_COLS = 32;
    static constexpr int SRC_ROWS = 24;

    // Colormap lookup table (1024 entries)
    static constexpr int LUT_SIZE = 1024;
    QRgb m_colormapLUT[LUT_SIZE];

    // Current auto-range
    float m_currentMin = 0.0f;
    float m_currentMax = 255.0f;
};

#endif // HEATMAPIMAGEPROVIDER_H
