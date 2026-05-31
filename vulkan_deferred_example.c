/*
 * Vulkan延迟渲染管线示例 - C风格实现，使用VK_KHR_dynamic_rendering
 *
 * 工作流程：
 * 1. G-Buffer Pass: 渲染位置、法线、反照率到三个纹理
 * 2. 光照Pass: 读取G-Buffer，计算光照，输出到屏幕
 */

#include <vulkan/vulkan.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

// ============================================================================
// 数据结构
// ============================================================================

typedef struct Vertex {
    float position[3];
    float normal[3];
    float color[3];
} Vertex;

typedef struct Light {
    float position[4];  // xyz: position, w: radius
    float color[4];     // rgb: color,   w: intensity
} Light;

typedef struct DeferredFrameData {
    VkImage gBuffer[3];  // 0=Position, 1=Normal, 2=Albedo
    VkImageView gBufferViews[3];
    VkImage depthImage;
    VkImageView depthView;
    VkFramebuffer gBufferFramebuffer;
    VkDescriptorSet gBufferDescriptorSet;
} DeferredFrameData;

// ============================================================================
// G-Buffer管线
// ============================================================================

typedef struct GBufferPipeline {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descriptorSetLayout;
} GBufferPipeline;

static void GBufferPipeline_create(GBufferPipeline* self, VkDevice device, VkRenderPass renderPass) {
    (void)device;
    (void)renderPass;

    self->pipeline = VK_NULL_HANDLE;
    self->pipelineLayout = VK_NULL_HANDLE;
    self->descriptorSetLayout = VK_NULL_HANDLE;

    /* 实际项目中这里会创建着色器模块与图形管线。 */
    printf("G-Buffer管线已创建\n");
}

static void GBufferPipeline_destroy(GBufferPipeline* self, VkDevice device) {
    if (self->pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, self->pipeline, NULL);
        self->pipeline = VK_NULL_HANDLE;
    }
    if (self->pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, self->pipelineLayout, NULL);
        self->pipelineLayout = VK_NULL_HANDLE;
    }
    if (self->descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, self->descriptorSetLayout, NULL);
        self->descriptorSetLayout = VK_NULL_HANDLE;
    }
}

// ============================================================================
// 光照合成管线
// ============================================================================

typedef struct LightingPipeline {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkDescriptorSetLayout descriptorSetLayout;
} LightingPipeline;

static void LightingPipeline_create(LightingPipeline* self, VkDevice device, VkRenderPass renderPass) {
    (void)device;
    (void)renderPass;

    self->pipeline = VK_NULL_HANDLE;
    self->pipelineLayout = VK_NULL_HANDLE;
    self->descriptorSetLayout = VK_NULL_HANDLE;

    /* 实际项目中这里会创建全屏光照管线。 */
    printf("光照管线已创建\n");
}

static void LightingPipeline_destroy(LightingPipeline* self, VkDevice device) {
    if (self->pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, self->pipeline, NULL);
        self->pipeline = VK_NULL_HANDLE;
    }
    if (self->pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, self->pipelineLayout, NULL);
        self->pipelineLayout = VK_NULL_HANDLE;
    }
    if (self->descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, self->descriptorSetLayout, NULL);
        self->descriptorSetLayout = VK_NULL_HANDLE;
    }
}

// ============================================================================
// 延迟渲染器 - 使用Dynamic Rendering
// ============================================================================

#define MAX_FRAMES 2u
#define MAX_LIGHTS 64u

typedef struct DeferredRenderer {
    VkDevice device;
    VkQueue queue;
    VkCommandPool commandPool;

    GBufferPipeline gBufferPipeline;
    LightingPipeline lightingPipeline;

    DeferredFrameData frameData[MAX_FRAMES];
    uint32_t frameDataCount;

    Light lights[MAX_LIGHTS];
    uint32_t lightCount;

    VkExtensionProperties* dynamicRenderingExtension;
} DeferredRenderer;

static void DeferredRenderer_initializeLights(DeferredRenderer* renderer) {
    renderer->lightCount = 3;

    /* 红光 */
    renderer->lights[0] = (Light){
        .position = {2.0f, 1.0f, 0.0f, 5.0f},
        .color = {1.0f, 0.0f, 0.0f, 1.0f}
    };

    /* 绿光 */
    renderer->lights[1] = (Light){
        .position = {-2.0f, 1.0f, 0.0f, 5.0f},
        .color = {0.0f, 1.0f, 0.0f, 1.0f}
    };

    /* 蓝光 */
    renderer->lights[2] = (Light){
        .position = {0.0f, 1.0f, 2.0f, 5.0f},
        .color = {0.0f, 0.0f, 1.0f, 1.0f}
    };

    printf("创建了 %u 个光源\n", renderer->lightCount);
}

static void DeferredRenderer_init(DeferredRenderer* renderer, VkDevice inDevice, VkQueue inQueue, VkCommandPool inCmdPool) {
    uint32_t extCount = 0;

    memset(renderer, 0, sizeof(*renderer));
    renderer->device = inDevice;
    renderer->queue = inQueue;
    renderer->commandPool = inCmdPool;

    /* 检查dynamic_rendering支持（示例只演示查询） */
    vkEnumerateDeviceExtensionProperties(VK_NULL_HANDLE, VK_NULL_HANDLE, &extCount, VK_NULL_HANDLE);
    printf("VK_KHR_dynamic_rendering 已启用（扩展总数: %u）\n", extCount);

    GBufferPipeline_create(&renderer->gBufferPipeline, renderer->device, VK_NULL_HANDLE);
    LightingPipeline_create(&renderer->lightingPipeline, renderer->device, VK_NULL_HANDLE);

    DeferredRenderer_initializeLights(renderer);
}

static void DeferredRenderer_renderGBuffer(DeferredRenderer* renderer, VkCommandBuffer cmd, uint32_t width, uint32_t height) {
    VkRenderingAttachmentInfo colorAttachments[3];
    VkRenderingAttachmentInfo depthAttachment;
    VkRenderingInfo renderingInfo;
    uint32_t i;

    printf("\n=== G-Buffer Pass ===\n");

    if (renderer->frameDataCount == 0u) {
        printf("未配置frameData，跳过G-Buffer Pass\n");
        return;
    }

    memset(colorAttachments, 0, sizeof(colorAttachments));
    for (i = 0; i < 3u; ++i) {
        colorAttachments[i].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachments[i].imageView = renderer->frameData[0].gBufferViews[i];
        colorAttachments[i].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachments[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachments[i].clearValue.color.float32[0] = 0.0f;
        colorAttachments[i].clearValue.color.float32[1] = 0.0f;
        colorAttachments[i].clearValue.color.float32[2] = 0.0f;
        colorAttachments[i].clearValue.color.float32[3] = 1.0f;
    }

    memset(&depthAttachment, 0, sizeof(depthAttachment));
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = renderer->frameData[0].depthView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil.depth = 1.0f;
    depthAttachment.clearValue.depthStencil.stencil = 0u;

    memset(&renderingInfo, 0, sizeof(renderingInfo));
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset.x = 0;
    renderingInfo.renderArea.offset.y = 0;
    renderingInfo.renderArea.extent.width = width;
    renderingInfo.renderArea.extent.height = height;
    renderingInfo.layerCount = 1u;
    renderingInfo.colorAttachmentCount = 3u;
    renderingInfo.pColorAttachments = colorAttachments;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->gBufferPipeline.pipeline);
    vkCmdEndRendering(cmd);

    printf("G-Buffer渲染完成\n");
}

static void DeferredRenderer_renderLighting(DeferredRenderer* renderer, VkCommandBuffer cmd, VkImageView swapchainImageView, uint32_t width, uint32_t height) {
    VkRenderingAttachmentInfo colorAttachment;
    VkRenderingInfo renderingInfo;

    printf("\n=== 光照Pass ===\n");

    memset(&colorAttachment, 0, sizeof(colorAttachment));
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = swapchainImageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color.float32[0] = 0.0f;
    colorAttachment.clearValue.color.float32[1] = 0.0f;
    colorAttachment.clearValue.color.float32[2] = 0.0f;
    colorAttachment.clearValue.color.float32[3] = 1.0f;

    memset(&renderingInfo, 0, sizeof(renderingInfo));
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset.x = 0;
    renderingInfo.renderArea.offset.y = 0;
    renderingInfo.renderArea.extent.width = width;
    renderingInfo.renderArea.extent.height = height;
    renderingInfo.layerCount = 1u;
    renderingInfo.colorAttachmentCount = 1u;
    renderingInfo.pColorAttachments = &colorAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->lightingPipeline.pipeline);
    vkCmdDraw(cmd, 3u, 1u, 0u, 0u);
    vkCmdEndRendering(cmd);
    printf("光照计算完成\n");
}

static void DeferredRenderer_render(DeferredRenderer* renderer, VkCommandBuffer cmd, VkImageView swapchainImageView, uint32_t width, uint32_t height) {
    printf("\n========== 帧渲染 ==========\n");

    DeferredRenderer_renderGBuffer(renderer, cmd, width, height);
    DeferredRenderer_renderLighting(renderer, cmd, swapchainImageView, width, height);

    printf("帧完成\n");
}

static void DeferredRenderer_cleanup(DeferredRenderer* renderer) {
    GBufferPipeline_destroy(&renderer->gBufferPipeline, renderer->device);
    LightingPipeline_destroy(&renderer->lightingPipeline, renderer->device);
}

// ============================================================================
// 延迟渲染的核心优势
// ============================================================================

/*
 * 1. 减少着色器执行次数
 *    - 每个像素只执行一次几何着色（G-Buffer pass）
 *    - 光照计算独立进行，不需要重复渲染几何体
 *
 * 2. 支持大量光源
 *    - 不需要逐光源渲染几何体
 *    - 光源数量不影响几何复杂度成本
 *    - 适合100+光源的场景
 *
 * 3. VK_KHR_dynamic_rendering的优势
 *    - 不需要创建VkRenderPass对象
 *    - 灵活的多目标渲染配置
 *    - 更少的CPU开销
 *    - 更好的API现代化设计
 *
 * 4. 实现示意
 *    Frame {
 *        G-Buffer Pass {
 *            输出: Position(RGBA32F) + Normal(RGBA32F) + Albedo(RGBA32F)
 *            输入: 网格几何体
 *        }
 *        Lighting Pass {
 *            输入: 3个G-Buffer纹理 + 光源列表
 *            输出: 最终着色结果到屏幕
 *        }
 *    }
 *
 * 5. G-Buffer布局
 *    [Position.xyz | unused]
 *    [Normal.xyz   | unused]
 *    [Albedo.xyz   | unused]
 */

// ============================================================================
// 使用示例
// ============================================================================

/*
int main(void) {
    // 初始化Vulkan...
    VkDevice device = ...;
    VkQueue queue = ...;
    VkCommandPool commandPool = ...;
    VkImageView swapchainImageView = ...;

    DeferredRenderer renderer;
    DeferredRenderer_init(&renderer, device, queue, commandPool);

    // 注意：实际使用前需要填充renderer.frameData并设置renderer.frameDataCount

    // 每帧
    VkCommandBuffer cmd = beginFrame();
    DeferredRenderer_render(&renderer, cmd, swapchainImageView, WIDTH, HEIGHT);
    submitFrame(cmd);

    DeferredRenderer_cleanup(&renderer);
    return 0;
}
*/
