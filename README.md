# 蓝牙小车遥控器 (Bluetooth Car Remote)

基于 Qt 6 + QML 的 Android 蓝牙遥控器 APP，用于控制 STM32 蓝牙小车，同时接收并显示 32×24 传感器热力图。

## 功能

- **蓝牙连接**: 经典蓝牙 SPP (HC-05/HC-06)，支持设备扫描、连接、断开
- **方向控制**: 十字方向键（前进/后退/左转/右转），支持按住连续发送
- **速度控制**: 垂直滑块 + 加减按钮，0-255 速度调节
- **热力图显示**: 接收 32×24 矩阵数据，双线性插值放大 + Jet 色映射渲染
- **深色主题**: Material Dark 风格，适合户外使用

## 构建环境要求

- **Qt 6.5+** (for Android)
  - 模块: Quick, QuickControls2, Bluetooth
- **Android NDK** 25+
- **Android SDK** 33+ (API Level 33)
- **JDK** 17+

## 构建步骤

### 1. 配置 Qt for Android

确保已安装 Qt for Android 并配置好环境变量：
```bash
export ANDROID_NDK_ROOT=/path/to/ndk
export ANDROID_SDK_ROOT=/path/to/sdk
export JAVA_HOME=/path/to/jdk-17
```

### 2. 编译
```bash
cd BluetoothCar
mkdir build && cd build

# 根据你的 Qt 版本和 Android ABI 调整
qt-cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DANDROID_ABI=arm64-v8a

cmake --build . --parallel
```

### 3. 生成 APK
```bash
androiddeployqt \
    --input android-BluetoothCar-deployment-settings.json \
    --output android-build \
    --android-platform android-33 \
    --gradle
```

APK 文件位于 `build/android-build/build/outputs/apk/debug/` 或 `release/` 目录。

## 通信协议

### 发送 (App → STM32): ASCII 字符串

| 操作 | 按下 | 松开 |
|------|------|------|
| 前进 | `ONA` | `ONF` |
| 后退 | `ONB` | `ONF` |
| 左转 | `ONC` | `ONF` |
| 右转 | `OND` | `ONF` |
| 停止 | `ONE` | `ONF` |

速度指令 (独立发送):
- `ON0` ~ `ON9` — 速度等级 0(最慢) ~ 9(最快)
- 滑块拖动或加减按钮按下时发送

> **按下逻辑**: 方向键按下时立即发送对应指令，松开时若其他方向键仍按住则重发该方向指令，全部松开才发送 `ONF`。
> 无需帧头帧尾，直接发送 ASCII 字符串即可。

### 接收 (STM32 → App): 32×24 矩阵

| 帧头 (2B) | 数据 (768B) | 帧尾 (2B) |
|-----------|-------------|-----------|
| `0xAA 0x55` | 32×24 行优先 uint8 | `0x55 0xAA` |

- APP 连接后若 3 秒内未收到矩阵数据，热力图区域显示"等待热力图数据..."
- 收到有效数据后自动开始渲染
- 小车未实现矩阵发送时，遥控功能不受影响

## 项目结构

```
BluetoothCar/
├── CMakeLists.txt                    # 构建配置
├── android/AndroidManifest.xml       # Android 权限
├── src/
│   ├── main.cpp                      # 入口，注册 QML 类型
│   ├── bluetoothcontroller.h/cpp     # 蓝牙 SPP 通信
│   └── heatmapimageprovider.h/cpp    # 热力图渲染引擎
├── qml/
│   ├── main.qml                      # 主窗口
│   ├── BluetoothPanel.qml            # 蓝牙面板
│   ├── DirectionPad.qml              # 方向键
│   ├── DPadButton.qml                # 方向按钮组件
│   ├── SpeedControl.qml              # 速度滑块
│   └── HeatmapDisplay.qml            # 热力图显示
└── qml.qrc                           # QML 资源
```

## 热力图技术说明

- **输入**: 32 列 × 24 行 = 768 个传感器值
- **插值**: 双线性插值 (Bilinear Interpolation)，输出默认 640×480
- **色映射**: Jet/Thermal 风格查找表 (1024 级)
  - 深蓝(冷) → 青 → 绿 → 黄 → 红(热)
- **自适应**: 每帧自动 min/max 归一化，最大化对比度
- **刷新率**: ~10 FPS

## 注意事项

1. 首次运行需授予**位置权限**（Android 蓝牙扫描要求）
2. Android 12+ 额外需要 `BLUETOOTH_SCAN` 和 `BLUETOOTH_CONNECT` 权限
3. 确保手机蓝牙已开启
4. HC-05/06 模块默认配对密码通常为 `1234` 或 `0000`
5. STM32 端需实现对应的串口协议解析
