#include "heatmapimageprovider.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>
#include <cstring>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QQmlImageProviderBase>
#endif

HeatmapImageProvider::HeatmapImageProvider()
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    : QQuickImageProvider(QQuickImageProvider::Image)
#else
    : QQuickImageProvider(QQmlImageProviderBase::Image)
#endif
{
    std::memset(m_sourceMatrix, 0, sizeof(m_sourceMatrix));
    buildColormapLUT();

    // Create initial blank image
    m_outputImage = QImage(m_outputWidth, m_outputHeight, QImage::Format_ARGB32);
    m_outputImage.fill(qRgb(0, 0, 64)); // dark blue background
}

// ---- Colormap LUT ----
// Jet-like colormap: dark blue -> cyan -> green -> yellow -> red

void HeatmapImageProvider::buildColormapLUT()
{
    for (int i = 0; i < LUT_SIZE; ++i) {
        float t = static_cast<float>(i) / (LUT_SIZE - 1); // 0.0 .. 1.0

        int r, g, b;

        if (t < 0.125f) {
            // Dark blue to blue
            float s = t / 0.125f;
            r = 0;
            g = 0;
            b = static_cast<int>(128 + 127 * s);
        } else if (t < 0.375f) {
            // Blue to cyan
            float s = (t - 0.125f) / 0.25f;
            r = 0;
            g = static_cast<int>(255 * s);
            b = 255;
        } else if (t < 0.625f) {
            // Cyan to green (to yellow-green)
            float s = (t - 0.375f) / 0.25f;
            r = 0;
            g = 255;
            b = static_cast<int>(255 * (1.0f - s));
        } else if (t < 0.875f) {
            // Green to yellow
            float s = (t - 0.625f) / 0.25f;
            r = static_cast<int>(255 * s);
            g = 255;
            b = 0;
        } else {
            // Yellow to red
            float s = (t - 0.875f) / 0.125f;
            r = 255;
            g = static_cast<int>(255 * (1.0f - s));
            b = 0;
        }

        // Clamp
        r = std::clamp(r, 0, 255);
        g = std::clamp(g, 0, 255);
        b = std::clamp(b, 0, 255);

        m_colormapLUT[i] = qRgb(r, g, b);
    }
}

QColor HeatmapImageProvider::valueToColor(float t) const
{
    // Clamp to [0, 1]
    if (t <= 0.0f) return QColor::fromRgb(m_colormapLUT[0]);
    if (t >= 1.0f) return QColor::fromRgb(m_colormapLUT[LUT_SIZE - 1]);

    int idx = static_cast<int>(t * (LUT_SIZE - 1));
    idx = std::clamp(idx, 0, LUT_SIZE - 1);
    return QColor::fromRgb(m_colormapLUT[idx]);
}

// ---- Matrix Update ----

void HeatmapImageProvider::updateMatrix(const QVariantList &data)
{
    if (data.size() < SRC_COLS * SRC_ROWS) {
        qWarning() << "Heatmap: received" << data.size()
                    << "values, expected" << SRC_COLS * SRC_ROWS;
        return;
    }

    QMutexLocker locker(&m_mutex);

    // Copy data to source matrix and find min/max for auto-range
    m_currentMin = std::numeric_limits<float>::max();
    m_currentMax = std::numeric_limits<float>::lowest();

    for (int i = 0; i < SRC_COLS * SRC_ROWS; ++i) {
        float val = data.at(i).toFloat();
        m_sourceMatrix[i] = val;
        m_currentMin = std::min(m_currentMin, val);
        m_currentMax = std::max(m_currentMax, val);
    }

    // Prevent division by zero when all values are the same
    if (m_currentMax - m_currentMin < 0.001f) {
        m_currentMax = m_currentMin + 1.0f;
    }

    m_hasData = true;
    locker.unlock();

    renderHeatmap();
}

void HeatmapImageProvider::setOutputSize(int width, int height)
{
    if (width <= 0 || height <= 0) return;
    QMutexLocker locker(&m_mutex);
    m_outputWidth = width;
    m_outputHeight = height;
    locker.unlock();
    if (m_hasData) {
        renderHeatmap();
    }
}

// ---- Bilinear Interpolation Rendering ----

void HeatmapImageProvider::renderHeatmap()
{
    QMutexLocker locker(&m_mutex);

    if (m_outputImage.width() != m_outputWidth || m_outputImage.height() != m_outputHeight) {
        m_outputImage = QImage(m_outputWidth, m_outputHeight, QImage::Format_ARGB32);
    }

    // Scale factors: map output pixel center to source coordinate
    // output pixel (x_out, y_out) maps to source coordinate (x_src, y_src)
    float scaleX = static_cast<float>(SRC_COLS - 1) / (m_outputWidth - 1);
    float scaleY = static_cast<float>(SRC_ROWS - 1) / (m_outputHeight - 1);

    float invRange = 1.0f / (m_currentMax - m_currentMin);

    for (int y = 0; y < m_outputHeight; ++y) {
        // Source Y coordinate (floating point)
        float srcY = y * scaleY;
        int y0 = static_cast<int>(srcY);
        int y1 = std::min(y0 + 1, SRC_ROWS - 1);
        float fy = srcY - y0; // fractional part

        QRgb *scanLine = reinterpret_cast<QRgb *>(m_outputImage.scanLine(y));

        for (int x = 0; x < m_outputWidth; ++x) {
            // Source X coordinate (floating point)
            float srcX = x * scaleX;
            int x0 = static_cast<int>(srcX);
            int x1 = std::min(x0 + 1, SRC_COLS - 1);
            float fx = srcX - x0; // fractional part

            // Get 4 corner values from source matrix
            float v00 = m_sourceMatrix[y0 * SRC_COLS + x0];
            float v10 = m_sourceMatrix[y0 * SRC_COLS + x1];
            float v01 = m_sourceMatrix[y1 * SRC_COLS + x0];
            float v11 = m_sourceMatrix[y1 * SRC_COLS + x1];

            // Bilinear interpolation
            float vTop = v00 * (1.0f - fx) + v10 * fx;
            float vBottom = v01 * (1.0f - fx) + v11 * fx;
            float interpolated = vTop * (1.0f - fy) + vBottom * fy;

            // Normalize to [0, 1] using auto-range
            float normalized = (interpolated - m_currentMin) * invRange;
            normalized = std::clamp(normalized, 0.0f, 1.0f);

            // Look up color from LUT
            int lutIdx = static_cast<int>(normalized * (LUT_SIZE - 1));
            lutIdx = std::clamp(lutIdx, 0, LUT_SIZE - 1);
            scanLine[x] = m_colormapLUT[lutIdx];
        }
    }

    locker.unlock();
    m_imageVersion++;
}

// ---- QQuickImageProvider interface ----

QImage HeatmapImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(id)
    Q_UNUSED(requestedSize)

    QMutexLocker locker(&m_mutex);

    if (size) {
        *size = m_outputImage.size();
    }

    return m_outputImage.copy();
}
