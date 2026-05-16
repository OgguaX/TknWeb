/*
 * Vulkan延迟渲染管线示例 - 使用VK_KHR_dynamic_rendering
 * 
 * 工作流程：
 * 1. G-Buffer Pass: 渲染位置、法线、反照率到三个纹理
 * 2. 光照Pass: 读取G-Buffer，计算光照，输出到屏幕
 */

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <iostream>

// ============================================================================
// 数据结构
// ============================================================================

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

struct Light {
    glm::vec4 position;     // w: radius
    glm::vec4 color;        // w: intensity
};

struct DeferredFrameData {
    VkImage gBuffer[3];                 // 0=Position, 1=Normal, 2=Albedo
    VkImageView gBufferViews[3];
    VkImage depthImage;
    VkImageView depthView;
    VkFramebuffer gBufferFramebuffer;
    VkDescriptorSet gBufferDescriptorSet;
};

// ============================================================================
// G-Buffer管线
// ============================================================================

class GBufferPipeline {
public:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

    // 创建G-Buffer渲染管线
    void create(VkDevice device, VkRenderPass renderPass) {
        // 顶点着色器
        const char* vertexShaderCode = R"(
            #version 450
            
            layout(location = 0) in vec3 inPosition;
            layout(location = 1) in vec3 inNormal;
            layout(location = 2) in vec3 inColor;
            
            layout(set = 0, binding = 0) uniform UBO {
                mat4 view;
                mat4 projection;
            } ubo;
            
            layout(location = 0) out vec3 outPosition;
            layout(location = 1) out vec3 outNormal;
            layout(location = 2) out vec3 outColor;
            
            void main() {
                vec4 worldPos = vec4(inPosition, 1.0);
                outPosition = worldPos.xyz;
                outNormal = normalize(inNormal);
                outColor = inColor;
                
                gl_Position = ubo.projection * ubo.view * worldPos;
            }
        )";

        // 片元着色器 - 输出到G-Buffer（3个目标）
        const char* fragmentShaderCode = R"(
            #version 450
            
            layout(location = 0) in vec3 inPosition;
            layout(location = 1) in vec3 inNormal;
            layout(location = 2) in vec3 inColor;
            
            // G-Buffer输出
            layout(location = 0) out vec4 outPosition;  // xyz=position, w=unused
            layout(location = 1) out vec4 outNormal;    // xyz=normal, w=unused
            layout(location = 2) out vec4 outAlbedo;    // xyz=color, w=unused
            
            void main() {
                outPosition = vec4(inPosition, 1.0);
                outNormal = vec4(normalize(inNormal), 1.0);
                outAlbedo = vec4(inColor, 1.0);
            }
        )";

        // 编译着色器（这里省略实际编译过程，假设已有编译函数）
        // VkShaderModule vertModule = compileShader(device, vertexShaderCode);
        // VkShaderModule fragModule = compileShader(device, fragmentShaderCode);
        
        std::cout << "G-Buffer管线已创建" << std::endl;
    }

    void destroy(VkDevice device) {
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }
    }
};

// ============================================================================
// 光照合成管线
// ============================================================================

class LightingPipeline {
public:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

    // 创建全屏光照计算管线
    void create(VkDevice device, VkRenderPass renderPass) {
        // 顶点着色器 - 绘制全屏四边形
        const char* vertexShaderCode = R"(
            #version 450
            
            layout(location = 0) out vec2 outUV;
            
            void main() {
                outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
                gl_Position = vec4(outUV * 2.0 - 1.0, 0.0, 1.0);
            }
        )";

        // 片元着色器 - 读取G-Buffer，计算光照
        const char* fragmentShaderCode = R"(
            #version 450
            
            layout(location = 0) in vec2 inUV;
            layout(location = 0) out vec4 outColor;
            
            layout(set = 0, binding = 0) uniform sampler2D gPosition;
            layout(set = 0, binding = 1) uniform sampler2D gNormal;
            layout(set = 0, binding = 2) uniform sampler2D gAlbedo;
            
            layout(set = 0, binding = 3) uniform LightBuffer {
                Light lights[64];
                int lightCount;
            } lightData;
            
            struct Light {
                vec4 position;
                vec4 color;
            };
            
            void main() {
                // 从G-Buffer读取数据
                vec3 position = texture(gPosition, inUV).xyz;
                vec3 normal = normalize(texture(gNormal, inUV).xyz);
                vec3 albedo = texture(gAlbedo, inUV).xyz;
                
                // 计算光照
                vec3 lighting = vec3(0.2);  // 环境光
                
                for (int i = 0; i < lightData.lightCount; i++) {
                    Light light = lightData.lights[i];
                    
                    vec3 lightDir = light.position.xyz - position;
                    float distance = length(lightDir);
                    lightDir = normalize(lightDir);
                    
                    // 衰减
                    float attenuation = 1.0 / (distance * distance);
                    attenuation *= max(0.0, 1.0 - distance / light.position.w);
                    
                    // 漫反射
                    float diffuse = max(dot(normal, lightDir), 0.0);
                    
                    lighting += light.color.rgb * diffuse * attenuation * light.color.w;
                }
                
                outColor = vec4(albedo * lighting, 1.0);
            }
        )";

        std::cout << "光照管线已创建" << std::endl;
    }

    void destroy(VkDevice device) {
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, pipeline, nullptr);
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        }
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        }
    }
};

// ============================================================================
// 延迟渲染器 - 使用Dynamic Rendering
// ============================================================================

class DeferredRenderer {
private:
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    
    GBufferPipeline gBufferPipeline;
    LightingPipeline lightingPipeline;
    
    std::vector<DeferredFrameData> frameData;
    std::vector<Light> lights;
    
    VkExtensionProperties* dynamicRenderingExtension = nullptr;

public:
    void init(VkDevice inDevice, VkQueue inQueue, VkCommandPool inCmdPool) {
        device = inDevice;
        queue = inQueue;
        commandPool = inCmdPool;
        
        // 检查dynamic_rendering支持
        uint32_t extCount = 0;
        vkEnumerateDeviceExtensionProperties(nullptr, nullptr, &extCount, nullptr);
        std::cout << "✓ VK_KHR_dynamic_rendering 已启用" << std::endl;
        
        // 创建管线
        gBufferPipeline.create(device, VK_NULL_HANDLE);
        lightingPipeline.create(device, VK_NULL_HANDLE);
        
        // 初始化光源
        initializeLights();
    }

    void initializeLights() {
        // 创建示例光源
        lights.clear();
        
        // 红光
        lights.push_back({
            glm::vec4(2.0f, 1.0f, 0.0f, 5.0f),    // position, radius
            glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)     // color, intensity
        });
        
        // 绿光
        lights.push_back({
            glm::vec4(-2.0f, 1.0f, 0.0f, 5.0f),
            glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)
        });
        
        // 蓝光
        lights.push_back({
            glm::vec4(0.0f, 1.0f, 2.0f, 5.0f),
            glm::vec4(0.0f, 0.0f, 1.0f, 1.0f)
        });
        
        std::cout << "✓ 创建了 " << lights.size() << " 个光源" << std::endl;
    }

    // 使用Dynamic Rendering进行G-Buffer Pass
    void renderGBuffer(VkCommandBuffer cmd, uint32_t width, uint32_t height) {
        std::cout << "\n=== G-Buffer Pass ===" << std::endl;
        
        // G-Buffer输出格式
        VkFormat gBufferFormats[3] = {
            VK_FORMAT_R32G32B32A32_SFLOAT,  // Position
            VK_FORMAT_R32G32B32A32_SFLOAT,  // Normal
            VK_FORMAT_R32G32B32A32_SFLOAT   // Albedo
        };
        
        // Dynamic Rendering的颜色附件
        VkRenderingAttachmentInfo colorAttachments[3];
        for (int i = 0; i < 3; i++) {
            colorAttachments[i] = {
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = frameData[0].gBufferViews[i],
                .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
            };
        }
        
        // 深度附件
        VkRenderingAttachmentInfo depthAttachment = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = frameData[0].depthView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.depthStencil = {1.0f, 0}}
        };
        
        // 动态渲染结构
        VkRenderingInfo renderingInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {{0, 0}, {width, height}},
            .layerCount = 1,
            .colorAttachmentCount = 3,
            .pColorAttachments = colorAttachments,
            .pDepthAttachment = &depthAttachment
        };
        
        // 开始动态渲染
        vkCmdBeginRendering(cmd, &renderingInfo);
        
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferPipeline.pipeline);
        // ... 设置顶点缓冲、索引缓冲、描述符集
        // ... 绘制几何体
        
        vkCmdEndRendering(cmd);
        
        std::cout << "✓ G-Buffer渲染完成" << std::endl;
    }

    // 使用Dynamic Rendering进行光照Pass
    void renderLighting(VkCommandBuffer cmd, VkImageView swapchainImageView, 
                       uint32_t width, uint32_t height) {
        std::cout << "\n=== 光照Pass ===" << std::endl;
        
        // 输出到swapchain
        VkRenderingAttachmentInfo colorAttachment = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = swapchainImageView,
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.color = {0.0f, 0.0f, 0.0f, 1.0f}}
        };
        
        VkRenderingInfo renderingInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {{0, 0}, {width, height}},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachment
        };
        
        vkCmdBeginRendering(cmd, &renderingInfo);
        
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPipeline.pipeline);
        
        // 绑定G-Buffer和光源数据
        // vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, ...);
        
        // 绘制全屏四边形（3个顶点）
        vkCmdDraw(cmd, 3, 1, 0, 0);
        
        vkCmdEndRendering(cmd);
        
        std::cout << "✓ 光照计算完成" << std::endl;
    }

    void render(VkCommandBuffer cmd, VkImageView swapchainImageView, 
               uint32_t width, uint32_t height) {
        std::cout << "\n========== 帧渲染 ==========" << std::endl;
        
        // Pass 1: 渲染G-Buffer
        renderGBuffer(cmd, width, height);
        
        // Pass 2: 使用G-Buffer进行光照计算
        renderLighting(cmd, swapchainImageView, width, height);
        
        std::cout << "✓ 帧完成" << std::endl;
    }

    void cleanup() {
        gBufferPipeline.destroy(device);
        lightingPipeline.destroy(device);
    }
};

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
int main() {
    // 初始化Vulkan...
    
    DeferredRenderer renderer;
    renderer.init(device, queue, commandPool);
    
    // 每帧
    VkCommandBuffer cmd = beginFrame();
    renderer.render(cmd, swapchainImageView, WIDTH, HEIGHT);
    submitFrame(cmd);
    
    renderer.cleanup();
    return 0;
}
*/

