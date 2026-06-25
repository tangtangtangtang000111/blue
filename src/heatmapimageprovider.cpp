#include "heatmapimageprovider.h"
#include <QDebug>
#include <QtMath>
#include <algorithm>
#include <cstring>

HeatmapImageProvider::HeatmapImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
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

// ---- Catmull-Rom cubic kernel ----

// Mitchell-Netravali cubic filter (B=1/3, C=1/3).
// All weights are non-negative → no overshoot/ringing, preserves temperature range.
static float hmpCubicKernel(float x)
{
    // Mitchell-Netravali cubic filter (B=1/3, C=1/3).  All weights non-negative.
    const float B = 1.0f / 3.0f;
    const float C = 1.0f / 3.0f;
    if (x < 0.0f) x = -x;
    if (x < 1.0f) {
        float v = ((12.0f - 9.0f*B - 6.0f*C) * x
                 + (-18.0f + 12.0f*B + 6.0f*C)) * x * x
                 + (6.0f - 2.0f*B);
        return v / 6.0f;
    } else if (x < 2.0f) {
        float v = ((-B - 6.0f*C) * x
                 + (6.0f*B + 30.0f*C)) * x * x
                 + (-12.0f*B - 48.0f*C) * x
                 + (8.0f*B + 24.0f*C);
        return v / 6.0f;
    }
    return 0.0f;
}

static float hmpBicubicSample(const float *matrix, int cols, int rows,
                              float srcX, float srcY)
{
    int xBase = (int)srcX;
    int yBase = (int)srcY;
    float fx = srcX - xBase;
    float fy = srcY - yBase;

    float cw[4], rw[4];
    cw[0] = hmpCubicKernel(-1.0f - fx);
    cw[1] = hmpCubicKernel(      - fx);
    cw[2] = hmpCubicKernel( 1.0f - fx);
    cw[3] = hmpCubicKernel( 2.0f - fx);
    rw[0] = hmpCubicKernel(-1.0f - fy);
    rw[1] = hmpCubicKernel(      - fy);
    rw[2] = hmpCubicKernel( 1.0f - fy);
    rw[3] = hmpCubicKernel( 2.0f - fy);

    float sum = 0.0f;
    for (int dy = -1; dy <= 2; ++dy) {
        int y = yBase + dy;
        if (y < 0) y = 0; else if (y >= rows) y = rows - 1;
        float rowSum = 0.0f;
        for (int dx = -1; dx <= 2; ++dx) {
            int x = xBase + dx;
            if (x < 0) x = 0; else if (x >= cols) x = cols - 1;
            rowSum += matrix[y * cols + x] * cw[dx + 1];
        }
        sum += rowSum * rw[dy + 1];
    }
    return sum;
}

// ---- Bicubic Interpolation Rendering (with horizontal mirror) ----

void HeatmapImageProvider::renderHeatmap()
{
    QMutexLocker locker(&m_mutex);

    const int outW = m_outputWidth;
    const int outH = m_outputHeight;

    if (m_outputImage.width() != outW || m_outputImage.height() != outH) {
        m_outputImage = QImage(outW, outH, QImage::Format_ARGB32);
    }

    const float scaleX = (float)(SRC_COLS - 1) / (outW - 1);
    const float scaleY = (float)(SRC_ROWS - 1) / (outH - 1);
    const float invRange = 1.0f / (m_currentMax - m_currentMin);

    for (int y = 0; y < outH; ++y) {
        QRgb *scanLine = (QRgb *)m_outputImage.scanLine(y);
        float srcY = y * scaleY;

        for (int x = 0; x < outW; ++x) {
            // Source coordinates with horizontal mirror (flip left-right)
            float srcX = (SRC_COLS - 1) - (x * scaleX);

            // Bicubic sample
            float interpolated = hmpBicubicSample(m_sourceMatrix, SRC_COLS, SRC_ROWS,
                                               srcX, srcY);

            float normalized = (interpolated - m_currentMin) * invRange;
            if (normalized < 0.0f) normalized = 0.0f;
            if (normalized > 1.0f) normalized = 1.0f;

            int idx = (int)(normalized * (LUT_SIZE - 1));
            if (idx < 0) idx = 0;
            if (idx >= LUT_SIZE) idx = LUT_SIZE - 1;
            scanLine[x] = m_colormapLUT[idx];
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
