#version 450

// ============================================================================
// 按 TknVertexInputLayout 绑定的 GLSL 示例
// ============================================================================

// ============================================================================
// Binding 0 (TKN_VERTEX_BINDING_DESCRIPTION) - Per-Vertex Data
// 来自 pTknMeshVertexInputLayout->names[] = {"position", "color", "normal", "pbr"}
// stride 自动计算：vec3(12) + uint(4) + uint(4) + uint(4) = 24 字节
// ============================================================================

layout(location = 0) in vec3 position;      // offset = 0
layout(location = 1) in uint color;         // offset = 12
layout(location = 2) in uint normal;        // offset = 16
layout(location = 3) in uint pbr;           // offset = 20

// ============================================================================
// Binding 1 (TKN_INSTANCE_BINDING_DESCRIPTION) - Per-Instance Data
// 来自 pTknInstanceVertexInputLayout->names[] = {"model"}
// mat4 会自动展开成 4 个 vec4：
//   location 4: model[0] (row 0)
//   location 5: model[1] (row 1)
//   location 6: model[2] (row 2)
//   location 7: model[3] (row 3)
// stride 自动计算：4 * 16 = 64 字节
// ============================================================================

layout(location = 4) in mat4 model;         // locations 4-7, offset = 0-64

void main()
{
    // 变换顶点位置
    gl_Position = model * vec4(position, 1.0);
    
    // 解包数据
    vec4 colorVec = unpackUnorm4x8(color);
    vec4 normalVec = unpackSnorm4x8(normal);
    vec4 pbrVec = unpackUnorm4x8(pbr);
    
    // ... 后续处理
}


// ============================================================================
// 调用方式 (C 代码)
// ============================================================================
/*

// 定义顶点数据结构（内存中的packing顺序必须和names[]一致）
typedef struct {
    float position[3];      // offset 0, 12 bytes
    uint32_t color;         // offset 12, 4 bytes
    uint32_t normal;        // offset 16, 4 bytes
    uint32_t pbr;           // offset 20, 4 bytes
} VertexData;              // stride = 24 bytes

typedef struct {
    float model[16];        // offset 0, 64 bytes
} InstanceData;             // stride = 64 bytes (per-instance)

// 创建两个布局
const char *meshNames[]     = {"position", "color", "normal", "pbr"};
const char *instanceNames[] = {"model"};

TknVertexInputLayout meshLayout = {
    .tknAttributeCount = 4,
    .names = meshNames,
    .tknReferencePtrHashSet = tknCreateHashSet(sizeof(TknPipeline*))
};

TknVertexInputLayout instanceLayout = {
    .tknAttributeCount = 1,
    .names = instanceNames,
    .tknReferencePtrHashSet = tknCreateHashSet(sizeof(TknPipeline*))
};

// 创建管线 - 只需传入布局，所有绑定信息自动从SPIRV生成
TknPipeline *pipeline = tknCreatePipelinePtr(
    pGfxContext,
    colorAttachmentCount,
    pColorFormats,
    depthFormat,
    pRenderPassDescriptorSet,
    spvCount,
    spvPaths,
    &meshLayout,        // binding 0, per-vertex
    &instanceLayout,    // binding 1, per-instance
    ...other states...
);

// VkVertexInputBindingDescription 会自动生成：
// [0] = {binding: 0, stride: 24, inputRate: VK_VERTEX_INPUT_RATE_VERTEX}
// [1] = {binding: 1, stride: 64, inputRate: VK_VERTEX_INPUT_RATE_INSTANCE}

// VkVertexInputAttributeDescription 会自动生成（从 SPIRV Reflect）：
// [0] = {location: 0, binding: 0, format: VK_FORMAT_R32G32B32_SFLOAT, offset: 0}
// [1] = {location: 1, binding: 0, format: VK_FORMAT_R32_UINT, offset: 12}
// [2] = {location: 2, binding: 0, format: VK_FORMAT_R32_UINT, offset: 16}
// [3] = {location: 3, binding: 0, format: VK_FORMAT_R32_UINT, offset: 20}
// [4] = {location: 4, binding: 1, format: VK_FORMAT_R32G32B32A32_SFLOAT, offset: 0}   // model[0]
// [5] = {location: 5, binding: 1, format: VK_FORMAT_R32G32B32A32_SFLOAT, offset: 16}  // model[1]
// [6] = {location: 6, binding: 1, format: VK_FORMAT_R32G32B32A32_SFLOAT, offset: 32}  // model[2]
// [7] = {location: 7, binding: 1, format: VK_FORMAT_R32G32B32A32_SFLOAT, offset: 48}  // model[3]

*/
