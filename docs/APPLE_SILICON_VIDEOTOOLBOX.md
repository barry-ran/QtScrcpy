# Apple Silicon (M1) VideoToolbox + Metal 极致性能方案

## 背景

QtScrcpy 在 macOS M1 笔记本上 CPU 占用 68%：
- ~50% 消耗在 FFmpeg 单线程 H.264 软件解码
- ~8% 消耗在 glTexSubImage2D 将 YUV420P 纹理上传到 GPU
- ~10% 消耗在网络 IO、ADB 通信、系统开销

更深层的问题：
1. **捆绑 FFmpeg 不含 VideoToolbox** — `--disable-everything --disable-pthreads` 仅启用了基础 H.264 软解
2. **OpenGL 在 M1 上不是原生的** — Apple 的 OpenGL 实现通过 GL→Metal 翻译层运行，GL_LUMINANCE 格式走慢路径
3. **没有利用 M1 的 Unified Memory Architecture** — 软件解码 YUV 数据在 CPU 内存，再拷到 GPU，完全违背 UMA 设计

---

## 方案对比

| | 当前（基准） | A: FFmpeg VT | B: VT+OpenGL | **C: VT+Metal（推荐）** |
|------|:---:|:---:|:---:|:---:|
| 解码 CPU | ~50% | ~0% | ~0% | **~0%** |
| 纹理/上传 | ~8% | ~8% | ~2% | **~0%** |
| GL→Metal 翻译 | ~2% | ~2% | ~2% | **无** |
| 总 CPU | ~68% | ~20-25% | ~14-17% | **~5-8%** |
| GPU↔CPU 拷贝 | 无 | 有(GPU→CPU→GPU) | 无 | **无** |
| API 弃用风险 | QOpenGLWidget | QOpenGLWidget | CVOpenGLTextureCache | **无** |
| 需改 FFmpeg dylib | 否 | **是** | 否 | **否** |
| 新增代码 | 0 | ~30行 | ~300行 | ~400行 |
| Qt 版本要求 | Qt5/Qt6 | Qt5/Qt6 | Qt5/Qt6 | Qt5/Qt6 |
| 非 macOS 影响 | - | dylib 变更 | 无 | **无** |

结论：**方案 C 是唯一在每个环节都做到极致的路径。**

---

## 方案 C 核心架构

### 数据流

```
┌── Demuxer 线程 ────────────────────────────────────────────┐
│                                                             │
│  AVPacket (完整 H.264 访问单元)                              │
│      │                                                      │
│      ▼                                                      │
│  VTDecoder::decode(data, size, pts)                         │
│      │                                                      │
│      ├─ CMBlockBufferCreateWithMemoryBlock(data) ← 零拷贝    │
│      ├─ CMSampleBufferCreate(formatDesc, blockBuffer)       │
│      ├─ VTDecompressionSessionDecodeFrame()                 │
│      │     → M1 Media Engine 硬件解码，~0% CPU              │
│      │     → 异步回调，dispatch_semaphore 同步等待           │
│      ├─ CMBlockBuffer / CMSampleBuffer 释放（VT 已处理完）    │
│      │                                                      │
│      └─ 返回 CVPixelBufferRef (IOSurface, NV12, GPU 端)     │
│            │                                                │
│  CVPixelBufferRetain() → emit signal → QueuedConnection     │
└─────────────────────────────────────────────────────────────┘
                         │
┌── 主线程 (GUI) ─────────────────────────────────────────────┐
│                                                             │
│  onFrameMetal(CVPixelBufferRef pb, int w, int h)            │
│      │                                                      │
│      ├─ CVMetalTextureCacheCreateTextureFromImage()         │
│      │     plane 0 → MTLTexture (R8Unorm, W×H)     ← 零拷贝 │
│      │     plane 1 → MTLTexture (RG8Unorm, W/2×H/2)        │
│      │                                                      │
│      ├─ MTLCommandBuffer → MTLRenderCommandEncoder          │
│      │     vertex shader: 全屏四边形                         │
│      │     fragment shader: NV12 → BT.709 RGB               │
│      │                                                      │
│      ├─ [cmdBuffer presentDrawable:layer.nextDrawable]      │
│      ├─ [cmdBuffer addCompletedHandler: CVPixelBufferRelease]│
│      └─ [cmdBuffer commit]                                  │
│                                                             │
│  MetalVideoWindow 通过 QWidget::createWindowContainer       │
│  嵌入 KeepRatioWidget → 0 改动现有 UI 布局                   │
└─────────────────────────────────────────────────────────────┘
```

---

## 关键技术细节

### 1. SPS/PPS 获取（延迟初始化）

Demuxer 将第一个 config packet + data packet 拼接后在 `getFrame` 中一起发出。第一个 AVPacket 的结构：

```
[AVCDecoderConfigurationRecord][H.264 NAL units of first IDR frame]
```

VTDecoder 采用懒初始化策略：
1. 第一次调用 `decode()` 时，解析 AVCDecoderConfigurationRecord 提取 SPS/PPS NAL 单元
2. 调用 `CMVideoFormatDescriptionCreateFromH264ParameterSets()` 创建格式描述
3. 创建 VTDecompressionSession（配置 Metal 兼容性 + IOSurface 支持）
4. 跳过 config 前缀字节，只将 NAL 单元数据送入解码
5. 首次创建 session 约 2-5ms（一次性开销）

AVCDecoderConfigurationRecord 解析（约 50 行手动比特解析）：
```
byte[0]    version (=1)
byte[1]    AVCProfileIndication
byte[2]    profile_compatibility
byte[3]    AVCLevelIndication
byte[4]    0xFC | lengthSizeMinusOne
byte[5]    0xE0 | numOfSequenceParameterSets
  → for each SPS: uint16 length + N bytes NAL data
byte[next] numOfPictureParameterSets
  → for each PPS: uint16 length + N bytes NAL data

totalSize = 5 + 1 + Σ(SPS len+2) + 1 + Σ(PPS len+2)
frameData starts at totalSize
```

### 2. 线程同步

```
Demuxer 线程
  → VTDecoder::decode()
    → VTDecompressionSessionDecodeFrame()  // 异步，立即返回
    → dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER)  // 阻塞等待

VT 内部线程
  → 解码完成回调
    → CVPixelBufferRetain(imageBuffer)
    → 存储到 m_outputPixelBuffer
    → dispatch_semaphore_signal(sem)  // 唤醒 Demuxer 线程

Demuxer 线程（被唤醒）
  → 读取 m_outputPixelBuffer
  → emit frameReady(pixelBuffer, width, height)  // Qt::QueuedConnection
  → 返回（此时 AVPacket 仍有效，在 push() 返回后才释放）

主线程（QueuedConnection）
  → Metal 渲染
  → CVPixelBufferRelease(pb)  // 通过 MTLCommandBuffer completion handler
```

**关键保证**：`CMBlockBufferCreateWithMemoryBlock` 使用的 `kCFAllocatorNull` 不拷贝数据，指针直接指向 AVPacket->data。由于 decode() 是同步阻塞的（dispatch_semaphore_wait），VT 处理完成前 AVPacket 不会被释放。

### 3. AVPacket 到 CMSampleBuffer 的包装

```objc
// 零拷贝：blockBuffer 直接引用 AVPacket->data
CMBlockBufferCreateWithMemoryBlock(
    kCFAllocatorDefault,
    (void*)data,          // AVPacket->data 指针
    size,                 // AVPacket->size
    kCFAllocatorNull,     // 不自定义释放器
    NULL, NULL, 0, size, 0,
    &blockBuffer
);

// 创建 sampleBuffer
CMSampleTimingInfo timing = {
    .duration = kCMTimeInvalid,
    .presentationTimeStamp = CMTimeMake(pts, 1000000),
    .decodeTimeStamp = kCMTimeInvalid
};
CMSampleBufferCreateReady(
    kCFAllocatorDefault,
    blockBuffer, m_formatDesc,
    1, 1, &timing, 0, NULL,
    &sampleBuffer
);
```

### 4. VTDecompressionSession 创建

```objc
// 要求硬件解码 + Metal 兼容 + IOSurface
CFMutableDictionaryRef decoderSpec = CFDictionaryCreateMutable(...);
CFDictionarySetValue(decoderSpec,
    kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder,
    kCFBooleanTrue);

CFMutableDictionaryRef destAttrs = CFDictionaryCreateMutable(...);
CFDictionarySetValue(destAttrs,
    kCVPixelBufferPixelFormatTypeKey,
    @(kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange));  // NV12
CFDictionarySetValue(destAttrs,
    kCVPixelBufferMetalCompatibilityKey, kCFBooleanTrue);
CFDictionarySetValue(destAttrs,
    kCVPixelBufferIOSurfacePropertiesKey, @{});

VTDecompressionSessionCreate(kCFAllocatorDefault,
    m_formatDesc, decoderSpec, destAttrs,
    &outputCallbackStruct, &m_session);
```

### 5. Metal 渲染

```objc
// 从 CVPixelBuffer 创建 Metal 纹理（零拷贝）
CVMetalTextureCacheCreateTextureFromImage(
    NULL, m_textureCache, pixelBuffer, NULL,
    MTLPixelFormatR8Unorm,  width,      height,      0, &yRef);   // Y 平面
CVMetalTextureCacheCreateTextureFromImage(
    NULL, m_textureCache, pixelBuffer, NULL,
    MTLPixelFormatRG8Unorm, width >> 1, height >> 1, 1, &uvRef);  // UV 平面

id<MTLTexture> yTex  = CVMetalTextureGetTexture(yRef);
id<MTLTexture> uvTex = CVMetalTextureGetTexture(uvRef);

// 渲染
id<MTLCommandBuffer> cmdBuf = [m_commandQueue commandBuffer];
MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
pass.colorAttachments[0].texture = drawable.texture;
// ... bind pipeline, vertices, textures ...
[cmdBuf presentDrawable:drawable];
[cmdBuf addCompletedHandler:^(id<MTLCommandBuffer> _) {
    CVPixelBufferRelease(pixelBuffer);  // GPU 用完后释放
}];
[cmdBuf commit];

CFRelease(yRef); CFRelease(uvRef);
```

Metal 着色器（BT.709 YUV→RGB，与现有 GLSL 着色器系数一致）：

```metal
fragment float4 nv12_frag(VertexOut in [[stage_in]],
    texture2d<float, access::sample> texY  [[texture(0)]],
    texture2d<float, access::sample> texUV [[texture(1)]])
{
    constexpr sampler s(filter::linear);
    float y  = texY.sample(s, in.texCoord).r;
    float2 uv = texUV.sample(s, in.texCoord).rg;
    y = y - 0.0625;
    uv = uv - 0.5;
    return float4(
        dot(float3(y, uv.x, uv.y), float3(1.1644,  0.0000,  1.7927)),
        dot(float3(y, uv.x, uv.y), float3(1.1644, -0.2132, -0.5329)),
        dot(float3(y, uv.x, uv.y), float3(1.1644,  2.1124,  0.0000)),
        1.0);
}
```

### 6. Metal 嵌入 QWidget

```cpp
// VideoForm::initUI() 中
#ifdef Q_OS_MACOS
if (VTDecoder::isAvailable()) {
    auto metalWin = new MetalVideoWindow();
    m_metalContainer = QWidget::createWindowContainer(metalWin, this);
    ui->keepRatioWidget->setWidget(m_metalContainer);
    
    // FPS label 改为 VideoForm 的直接子控件
    m_fpsLabel = new QLabel(this);
    // ...
} else
#endif
{
    // 现有 OpenGL 路径（不变）
    m_videoWidget = new QYUVOpenGLWidget();
    ui->keepRatioWidget->setWidget(m_videoWidget);
    // FPS label 为 m_videoWidget 的子控件（不变）
}

// 鼠标跟踪：对 container/QWindow 设置
m_metalContainer->setMouseTracking(true);
```

### 7. CVPixelBuffer 生命周期

```
VT 回调 → CVPixelBufferRetain(pb) → 存储
    ↓ (emit signal, QueuedConnection)
主线程 → CVMetalTextureCacheCreateTextureFromImage(pb) → MTLTexture(共享 IOSurface)
    ↓
[cmdBuf addCompletedHandler: CVPixelBufferRelease(pb)]
    ↓ (GPU 渲染完成)
CVPixelBufferRelease → IOSurface 引用计数 -1
```

**帧间保证**：最多 1 帧在渲染中。如果新帧到来而上一帧仍在渲染，丢弃旧帧或跳过新帧（参考现有 VideoBuffer 的跳帧逻辑）。

### 8. 分辨率变化处理

```
解析到新尺寸的 SPS
  → VTDecompressionSessionInvalidate(oldSession)
  → 释放旧 formatDesc / session
  → 用新 SPS/PPS 创建 formatDesc
  → 创建新 VTDecompressionSession
  → 从下一个 IDR 开始解码
```

VTDecoder 内部缓存当前宽高，每次 SPS 变化时重建 session。

### 9. FPS 计数

VideoBuffer 的 FpsCounter 不适用于 VT 路径。改为在 VTDecoder 或 Decoder 中添加简单计数：
- 每次成功解码 → `m_rendered++`
- 每次 Metal 渲染完成 → `emit updateFPS(rendered)`
- 每秒通过 QTimer 汇总

### 10. 截屏适配 (peekFrame)

```cpp
void Decoder::peekFrame(std::function<void(int,int,uint8_t*)> onFrame) {
    if (m_decodeMode == MODE_VT_METAL && m_lastPixelBuffer) {
        // 锁 CVPixelBuffer 读取 NV12 → sws_scale → RGB
        CVPixelBufferLockBaseAddress(m_lastPixelBuffer, kCVPixelBufferLock_ReadOnly);
        uint8_t* y  = CVPixelBufferGetBaseAddressOfPlane(m_lastPixelBuffer, 0);
        uint8_t* uv = CVPixelBufferGetBaseAddressOfPlane(m_lastPixelBuffer, 1);
        // NV12 → RGB via sws_scale
        CVPixelBufferUnlockBaseAddress(m_lastPixelBuffer, kCVPixelBufferLock_ReadOnly);
    } else {
        m_vb->peekRenderedFrame(onFrame);  // 现有路径不变
    }
}
```

### 11. 录制兼容

录制的 Recorder 直接接收 AVPacket（不解码），VT 路径对此完全透明，不需要任何改动。

---

## 文件清单

### 新增文件（3 个）

| 文件 | 行数 | 说明 |
|------|:---:|------|
| `QtScrcpyCore/src/device/decoder/vtdecoder.h` | ~50 | C++ 接口：`isAvailable()`, `open()`, `close()`, `decode()` |
| `QtScrcpyCore/src/device/decoder/vtdecoder.mm` | ~250 | ObjC++ 实现：AVCDecoderConfigurationRecord 解析、VT Session 管理、CMSampleBuffer 包装 |
| `QtScrcpy/render/metalvideowindow.h` + `.mm` | ~180 | QWindow 子类：CAMetalLayer、CVMetalTextureCache、Metal 渲染管线、NV12 着色器 |

### 修改文件（7 个）

| 文件 | 改动行数 | 说明 |
|------|:---:|------|
| `QtScrcpyCore/CMakeLists.txt` | ~5 | 链接 `VideoToolbox`/`CoreMedia`/`CoreVideo`；新增 `.mm` 编译；Metal framework |
| `QtScrcpyCore/src/device/decoder/decoder.h` | ~5 | 新增 `VTDecoder* m_vtDecoder`、`int m_decodeMode` |
| `QtScrcpyCore/src/device/decoder/decoder.cpp` | ~40 | `open()` 平台检测 → VT 路径；`push()` 路由到 VTDecoder |
| `QtScrcpyCore/include/QtScrcpyCore.h` | ~3 | `DeviceObserver` 新增 `onFrameMetal(void* cvPixelBuffer, int w, int h)` |
| `QtScrcpyCore/src/device/device.cpp` | ~5 | `getConfigFrame` 路由到 Decoder（传递 SPS/PPS） |
| `QtScrcpy/ui/videoform.h` + `.cpp` | ~40 | VT 路径用 MetalVideoWindow；FPS label 适配；input 事件适配 |
| `QtScrcpy/CMakeLists.txt` | ~3 | 链接 CoreVideo、Metal |

**合计**：新增 ~480 行，修改 ~100 行，总计 ~580 行。

---

## 逐帧流程详解

### 第一帧（含 Config）

```
Demuxer: pushPacket → getFrame(packet)
    packet->data = [AVCDecoderConfigurationRecord][H.264 NALs of IDR]

Decoder::push(packet) [Demuxer 线程, DirectConnection]
    VTDecoder::decode(data, size, pts)
        1. 首次：解析 AVCDecoderConfigurationRecord
           → 提取 SPS/PPS NAL 单元
           → CMVideoFormatDescriptionCreateFromH264ParameterSets()
           → VTDecompressionSessionCreate(..., Metal兼容, IOSurface, ...)
           → 计算 configSize，offset = configSize
        2. CMBlockBufferCreateWithMemoryBlock(data + offset, size - offset)
        3. CMSampleBufferCreate(blockBuffer, formatDesc, pts)
        4. VTDecompressionSessionDecodeFrame(session, sampleBuffer)
        5. dispatch_semaphore_wait(sem)
        6. 回调中: CVPixelBufferRetain(outputPixelBuffer)
        7. CFRelease(sampleBuffer), CFRelease(blockBuffer)
        8. if CVPixelBuffer: Retain → emit frameMetal(pb, w, h)
        9. 返回 true

    AVPacket 在 push() 返回后被 Demuxer 释放（安全，VT 已完成处理）
```

### 后续帧

```
Decoder::push(packet)
    VTDecoder::decode(data, size, pts)
        1. CMBlockBuffer → CMSampleBuffer → VTDecodeFrame
        2. dispatch_semaphore_wait
        3. return CVPixelBuffer
    释放 CMBlockBuffer/CMSampleBuffer

Main Thread:
    onFrameMetal(pb, w, h)
        1. CVMetalTextureCacheCreate ×2 → yTex, uvTex
        2. MTLCommandBuffer → render → present → commit
        3. Completion block: CVPixelBufferRelease(pb)
```

---

## 风险与边界

| 风险 | 应对方案 |
|------|---------|
| `createWindowContainer` 限制（不能叠加透明控件） | 视频占满容器，FPS 标签改为 VideoForm 的直接子控件 |
| 首个 config packet 格式异常 | 解析 AVCDecoderConfigurationRecord 时校验 version=1，失败回退 |
| CVPixelBuffer 尺寸变化 | VTDecoder 缓存 lastWidth/Height，检测变化后重建 session |
| VT 解码错误（丢帧/花屏） | 返回 false，Decoder 跳过该帧；下一个 IDR 自动恢复 |
| Metal 管线编译失败 | 回退到 OpenGL 软解路径 |
| `dispatch_semaphore_wait` 超时 | 设置 500ms 超时，超时视为解码失败 |
| Rosetta 2（x86_64 跑在 M1） | `isAvailable()` 运行时检查 arm64 |
| 非 macOS 编译 | `#ifdef Q_OS_MACOS` + `#ifdef __arm64__` 双重隔离 |

---

## 预期效果

| 指标 | 当前 | 方案 C |
|------|:---:|:---:|
| H.264 解码 CPU | ~50% | **~0%**（M1 Media Engine 硬件） |
| 纹理上传 CPU | ~8% | **~0%**（CVMetalTextureCache 零拷贝） |
| GL→Metal 翻译 CPU | ~2% | **~0%**（直接 Metal） |
| 网络/IO/系统 | ~8% | ~5-8% |
| **总 CPU** | **~68%** | **~5-8%** |
| 解码延迟 | 软解 2-4ms | 硬解 <0.5ms |
| 渲染延迟 | GL 翻译 0.5-2ms | Metal 直接 <0.2ms |
| API 弃用风险 | 有（GL_LUMINANCE 等） | **无** |
| 适用平台 | 所有 | macOS arm64（其他平台不变） |
