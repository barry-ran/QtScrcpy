#include <QTimer>
#include <QResizeEvent>
#include <QShowEvent>
#include "metalvideowindow.h"

#ifdef Q_OS_MACOS

#include <QDebug>

#import <Metal/Metal.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CATransaction.h>
#import <AppKit/AppKit.h>

static NSString *shaderSource = @R"(
#include <metal_stdlib>
using namespace metal;
struct VertexOut { float4 position [[position]]; float2 texCoord; };
vertex VertexOut vs(uint vid [[vertex_id]]) {
    float2 p[]={float2(-1,-1),float2(1,-1),float2(-1,1),float2(1,-1),float2(1,1),float2(-1,1)};
    float2 t[]={float2(0,1),float2(1,1),float2(0,0),float2(1,1),float2(1,0),float2(0,0)};
    return {float4(p[vid],0,1),t[vid]};
}
fragment float4 fs(VertexOut in [[stage_in]],
                    texture2d<float,access::sample> tY [[texture(0)]],
                    texture2d<float,access::sample> tUV[[texture(1)]]) {
    constexpr sampler s(filter::linear);
    float y=tY.sample(s,in.texCoord).r-0.0625;
    float2 uv=tUV.sample(s,in.texCoord).rg-0.5;
    return float4(dot(float3(y,uv.x,uv.y),float3(1.1644,0,1.7927)),
                  dot(float3(y,uv.x,uv.y),float3(1.1644,-0.2132,-0.5329)),
                  dot(float3(y,uv.x,uv.y),float3(1.1644,2.1124,0)),1.0);
}
)";

struct MetalVideoWidget::Impl {
    CVPixelBufferRef lastBuffer=nullptr;
    int lastWidth=0,lastHeight=0;
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> cq = nil;
    id<MTLRenderPipelineState> ps = nil;
    CVMetalTextureCacheRef tc = nullptr;
    CAMetalLayer *layer = nil;
    // 缓存上次 frame 尺寸避免无谓的 NSEqualRects
    CGFloat lastFrameW = 0, lastFrameH = 0;
    // drawable 耗尽诊断
    int drawableFailCount = 0;
    int drawableTotalCount = 0;
};

MetalVideoWidget::MetalVideoWidget(QWidget *parent)
    : QWidget(parent), d(new Impl)
{
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAutoFillBackground(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);

    d->device = MTLCreateSystemDefaultDevice();
    if (!d->device) { qWarning("MetalWidget: no Metal device"); return; }

    NSError *e = nil;
    id<MTLLibrary> lib = [d->device newLibraryWithSource:shaderSource options:nil error:&e];
    if (!lib) { qWarning("MetalWidget: shader error"); return; }

    id<MTLFunction> vf = [lib newFunctionWithName:@"vs"];
    id<MTLFunction> ff = [lib newFunctionWithName:@"fs"];
    if (!vf || !ff) { qWarning("MetalWidget: functions missing"); return; }

    MTLRenderPipelineDescriptor *pd = [MTLRenderPipelineDescriptor new];
    pd.vertexFunction = vf; pd.fragmentFunction = ff;
    pd.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    d->ps = [d->device newRenderPipelineStateWithDescriptor:pd error:&e];
    if (!d->ps) { qWarning("MetalWidget: pipeline error"); return; }

    d->cq = [d->device newCommandQueue];
    if (CVMetalTextureCacheCreate(kCFAllocatorDefault, nil, d->device, nil, &d->tc) != kCVReturnSuccess) {
        qWarning("MetalWidget: texture cache error"); return;
    }

    m_metalReady = true;
}

MetalVideoWidget::~MetalVideoWidget()
{
    if(d->lastBuffer)CVPixelBufferRelease(d->lastBuffer);
    if (d->tc) { CVMetalTextureCacheFlush(d->tc, 0); CFRelease(d->tc); }
    delete d;
}

void MetalVideoWidget::paintEvent(QPaintEvent *)
{
    if(d->lastBuffer)renderFrame(d->lastBuffer,d->lastWidth,d->lastHeight);
}
void MetalVideoWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    QTimer::singleShot(0,this,[this](){if(d->lastBuffer)renderFrame(d->lastBuffer,d->lastWidth,d->lastHeight);});
}
void MetalVideoWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    QTimer::singleShot(0,this,[this](){if(d->lastBuffer)renderFrame(d->lastBuffer,d->lastWidth,d->lastHeight);});
}

void MetalVideoWidget::renderFrame(CVPixelBufferRef pixelBuffer, int width, int height)
{
    if (!m_metalReady || !pixelBuffer || width <= 0 || height <= 0) return;

    if(d->lastBuffer!=pixelBuffer){CVPixelBufferRetain(pixelBuffer);if(d->lastBuffer)CVPixelBufferRelease(d->lastBuffer);d->lastBuffer=pixelBuffer;}
    d->lastWidth=width;d->lastHeight=height;
    QSize sz = size();
    if (sz.width() <= 0 || sz.height() <= 0) return;

    @autoreleasepool {
        NSView *view = (__bridge NSView *)reinterpret_cast<void *>(winId());
        if (!view) return;

        // Qt owns this NSView's frame, including aspect-fit centering. Never
        // reset it to (0,0): only the Metal layer uses view-local bounds.
        CGFloat newW = (CGFloat)sz.width(), newH = (CGFloat)sz.height();

        if (!d->layer) {
            view.wantsLayer = YES;
            d->layer = [CAMetalLayer layer];
            d->layer.device = d->device;
            d->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
            d->layer.opaque = YES;
            d->layer.framebufferOnly = YES;

        }

        // Keep Qt's backing layer intact; fullscreen may replace/reparent it.
        // Our drawable layer is a child, reattached whenever the backing changes.
        view.wantsLayer=YES;
        if(d->layer.superlayer!=view.layer){[d->layer removeFromSuperlayer];[view.layer addSublayer:d->layer];}
        [CATransaction begin];[CATransaction setDisableActions:YES];
        d->layer.frame=NSMakeRect(0,0,newW,newH);
        [CATransaction commit];
        const QPoint origin=pos();
        const double expectedY=view.superview.isFlipped?origin.y():view.superview.bounds.size.height-origin.y()-newH;
        setProperty("nativeGeometryMatchesQt",qAbs(view.frame.origin.x-origin.x())<1&&qAbs(view.frame.origin.y-expectedY)<1);
        setProperty("nativeFrameX", double(view.frame.origin.x));
        setProperty("nativeFrameY", double(view.frame.origin.y));
        CGFloat scale = view.window ? view.window.backingScaleFactor : 1.0;
        if (scale <= 0) scale = 1.0;
        d->layer.drawableSize = CGSizeMake(newW * scale, newH * scale);

        id<CAMetalDrawable> draw = [d->layer nextDrawable];
        if (!draw) {
            d->drawableFailCount++;
            // 每 60 次失败报告一次，避免日志风暴
            if (d->drawableFailCount % 60 == 1) {
                qWarning("MetalWidget: nextDrawable nil (fail %d/%d)",
                         d->drawableFailCount, d->drawableTotalCount);
            }
            return;
        }
        d->drawableTotalCount++;
        setProperty("renderedFrames",d->drawableTotalCount);

        CVMetalTextureRef yR = nil, uvR = nil;
        if (CVMetalTextureCacheCreateTextureFromImage(kCFAllocatorDefault, d->tc, pixelBuffer, nil,
                MTLPixelFormatR8Unorm, width, height, 0, &yR) != kCVReturnSuccess || !yR) return;
        if (CVMetalTextureCacheCreateTextureFromImage(kCFAllocatorDefault, d->tc, pixelBuffer, nil,
                MTLPixelFormatRG8Unorm, width>>1, height>>1, 1, &uvR) != kCVReturnSuccess || !uvR) {
            CFRelease(yR); return;
        }

        id<MTLTexture> yT = CVMetalTextureGetTexture(yR), uvT = CVMetalTextureGetTexture(uvR);
        id<MTLCommandBuffer> cb = [d->cq commandBuffer];
        MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = draw.texture;
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;

        id<MTLRenderCommandEncoder> enc = [cb renderCommandEncoderWithDescriptor:pass];
        [enc setRenderPipelineState:d->ps];
        [enc setFragmentTexture:yT atIndex:0];
        [enc setFragmentTexture:uvT atIndex:1];
        [enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
        [enc endEncoding];
        [cb presentDrawable:draw];

        // 对 pixelBuffer Retain，GPU 完成时 Release
        // 这确保了 pixel buffer 在整个 GPU 管线生命周期内有效
        CVPixelBufferRetain(pixelBuffer);
        CFRetain(yR); CFRetain(uvR);
        [cb addCompletedHandler:^(id<MTLCommandBuffer> buf) {
            if (buf.status == MTLCommandBufferStatusError) {
                const char *errStr = buf.error.localizedDescription.UTF8String;
                qWarning("MetalWidget: command buffer error: %s",
                         errStr ? errStr : "unknown");
            }
            CVPixelBufferRelease(pixelBuffer);
            CFRelease(yR);
            CFRelease(uvR);
        }];
        [cb commit];

        // 释放 Create 规则的引用：GPU handler 中的 CFRelease 只平衡了
        // 上面的 CFRetain。Create 的原始引用在此处释放。
        CFRelease(yR);
        CFRelease(uvR);
    }
}

#else
MetalVideoWidget::MetalVideoWidget(QWidget *parent) : QWidget(parent), d(nullptr) {}
MetalVideoWidget::~MetalVideoWidget() {}
void MetalVideoWidget::paintEvent(QPaintEvent *) {}
void MetalVideoWidget::resizeEvent(QResizeEvent *e) { QWidget::resizeEvent(e); }
void MetalVideoWidget::showEvent(QShowEvent *e) { QWidget::showEvent(e); }
bool MetalVideoWidget::initMetal() { return false; }
void MetalVideoWidget::renderFrame(CVPixelBufferRef, int, int) {}
#endif
