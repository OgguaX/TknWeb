/**
 * Tickernel WebGPU JavaScript Wrapper
 * 实现与 tkn.h 一致的 Buffer 和 Draw 命令 API
 * 支持 Vulkan 数值/枚举自动转换到 WebGPU
 *
 * ============================================================================
 * Vulkan 到 WebGPU 的转换支持
 * ============================================================================
 *
 * 所有 Lua 通过 C API 传来的 Vulkan 值会自动转换为 WebGPU 值：
 *
 * 【Buffer Usage Flags 转换】
 *   Vulkan (16 进制)              →  WebGPU (十进制)
 *   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT (0x0080)    →  GPUBufferUsage.VERTEX (64)
 *   VK_BUFFER_USAGE_INDEX_BUFFER_BIT  (0x0040)    →  GPUBufferUsage.INDEX (32)
 *   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT (0x0010)   →  GPUBufferUsage.UNIFORM (8)
 *   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT (0x0020)   →  GPUBufferUsage.STORAGE (16)
 *   VK_BUFFER_USAGE_TRANSFER_SRC_BIT   (0x0001)   →  GPUBufferUsage.COPY_SRC (1)
 *   VK_BUFFER_USAGE_TRANSFER_DST_BIT   (0x0002)   →  GPUBufferUsage.COPY_DST (2)
 *
 * 【Index Type 转换】
 *   Vulkan: VK_INDEX_TYPE_UINT16 (0)    →  WebGPU: "uint16"
 *   Vulkan: VK_INDEX_TYPE_UINT32 (1)    →  WebGPU: "uint32"
 *
 * 【使用示例（Lua）】
 *
 *   -- Lua 代码使用 Vulkan 枚举值
 *   local VK_BUFFER_USAGE_VERTEX_BUFFER_BIT = 0x0080
 *   local VK_BUFFER_USAGE_INDEX_BUFFER_BIT = 0x0040
 *   local VK_INDEX_TYPE_UINT32 = 1
 *
 *   -- 创建 vertex buffer (Lua 传入 Vulkan 值)
 *   local vertexBuf = tknCreateBufferPtr(device, 1024, 
 *     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, true, vertexData)
 *
 *   -- 创建 index buffer
 *   local indexBuf = tknCreateBufferPtr(device, 256,
 *     VK_BUFFER_USAGE_INDEX_BUFFER_BIT, false, indexData)
 *
 *   -- 绑定索引 buffer (Lua 传入 Vulkan 索引类型)
 *   tknBindIndexBuffer(state, indexBuf, VK_INDEX_TYPE_UINT32, 0)
 *
 *   -- 在 tkn.js 中自动转换为 WebGPU 值，无需 Lua 修改
 *
 * ============================================================================
 */

// ============================================================================
// Vulkan to WebGPU 转换映射
// ============================================================================

/**
 * Vulkan BufferUsageFlagBits to WebGPU GPUBufferUsage 转换
 * Vulkan 定义了位标志，WebGPU 也有对应的概念
 */
const VK_BUFFER_USAGE_FLAGS = {
  VK_BUFFER_USAGE_TRANSFER_SRC_BIT: 0x0001,      // 对应 COPY_SRC
  VK_BUFFER_USAGE_TRANSFER_DST_BIT: 0x0002,      // 对应 COPY_DST
  VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT: 0x0004,
  VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT: 0x0008,
  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT: 0x0010,    // 对应 UNIFORM
  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT: 0x0020,    // 对应 STORAGE
  VK_BUFFER_USAGE_INDEX_BUFFER_BIT: 0x0040,      // 对应 INDEX
  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT: 0x0080,     // 对应 VERTEX
  VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT: 0x0100,   // 对应 INDIRECT
  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT: 0x0200,
};

// Vulkan -> WebGPU 缓冲用途转换表
const VK_TO_WEBGPU_BUFFER_USAGE = {
  0x0001: GPUBufferUsage.COPY_SRC,      // TRANSFER_SRC
  0x0002: GPUBufferUsage.COPY_DST,      // TRANSFER_DST
  0x0010: GPUBufferUsage.UNIFORM,       // UNIFORM_BUFFER
  0x0020: GPUBufferUsage.STORAGE,       // STORAGE_BUFFER
  0x0040: GPUBufferUsage.INDEX,         // INDEX_BUFFER
  0x0080: GPUBufferUsage.VERTEX,        // VERTEX_BUFFER
  0x0100: GPUBufferUsage.INDIRECT,      // INDIRECT_BUFFER
};

/**
 * Vulkan 索引类型到 WebGPU 转换
 */
const VK_INDEX_TYPE_MAP = {
  0: "uint16",   // VK_INDEX_TYPE_UINT16
  1: "uint32",   // VK_INDEX_TYPE_UINT32
};

/**
 * 将 Vulkan buffer usage flags 转换为 WebGPU GPUBufferUsage
 * @param {number} vkUsageFlags - Vulkan 的 VkBufferUsageFlagBits
 * @returns {number} WebGPU GPUBufferUsage 的位组合
 */
function convertVulkanBufferUsageToWebGPU(vkUsageFlags) {
  let gpuUsage = 0;

  for (const [vkFlag, gpuFlag] of Object.entries(VK_TO_WEBGPU_BUFFER_USAGE)) {
    if (vkUsageFlags & parseInt(vkFlag)) {
      gpuUsage |= gpuFlag;
    }
  }

  // 总是添加 COPY_DST 以支持 buffer 更新
  gpuUsage |= GPUBufferUsage.COPY_DST;

  return gpuUsage;
}

/**
 * 将 Vulkan 索引类型转换为 WebGPU 格式
 * @param {number} vkIndexType - VkIndexType (0=uint16, 1=uint32)
 * @returns {string} WebGPU 索引类型 ("uint16" | "uint32")
 */
function convertVulkanIndexTypeToWebGPU(vkIndexType) {
  return VK_INDEX_TYPE_MAP[vkIndexType] || "uint32";
}

// ============================================================================
// Buffer 管理
// ============================================================================

/**
 * @typedef {Object} TknBuffer
 * @property {GPUBuffer} gpuBuffer - WebGPU buffer 句柄
 * @property {GPUBuffer|null} stagingBuffer - staging buffer（用于 DEVICE_LOCAL）
 * @property {ArrayBuffer|null} mappedData - 映射的内存（若 mappedAtCreation=true）
 * @property {number} size - buffer 大小
 * @property {number} usage - buffer 用途标志
 * @property {boolean} mappedAtCreation - 是否创建时映射
 * @property {number} memoryType - 内存类型（0=HOST_VISIBLE, 1=DEVICE_LOCAL）
 */

/**
 * 创建 GPU buffer
 * 对应 C API: tknCreateBufferPtr
 *
 * @param {GPUDevice} device - WebGPU 设备
 * @param {number} size - Buffer 大小（字节）
 * @param {number} usage - 缓冲用途（Vulkan VkBufferUsageFlagBits 或 WebGPU GPUBufferUsage）
 * @param {boolean} mappedAtCreation - 是否创建时映射到 CPU（对应 HOST_VISIBLE）
 * @param {ArrayBuffer|null} pData - 初始数据指针（可选）
 * @returns {TknBuffer} Buffer 对象
 */
export function tknCreateBufferPtr(device, size, usage, mappedAtCreation, pData = null) {
  if (!device) throw new Error("Device cannot be null");
  if (size <= 0) throw new Error("Buffer size must be greater than 0");

  // 自动检测并转换 Vulkan usage flags 为 WebGPU
  let gpuUsage = usage;
  if (typeof usage === "number" && usage > 0) {
    // 如果看起来像 Vulkan flags（通常是小的 16 进制值），进行转换
    if (usage <= 0x0200) {
      gpuUsage = convertVulkanBufferUsageToWebGPU(usage);
    } else {
      // 已经是 WebGPU 格式或其他格式
      gpuUsage = usage;
    }
  }

  const memoryType = mappedAtCreation ? 0 : 1; // 0=HOST_VISIBLE, 1=DEVICE_LOCAL
  let gpuBuffer, stagingBuffer = null, mappedData = null;

  if (mappedAtCreation) {
    // HOST_VISIBLE: 创建可映射 buffer，初始数据直接 memcpy
    gpuBuffer = device.createBuffer({
      size: size,
      usage: gpuUsage | GPUBufferUsage.COPY_SRC | GPUBufferUsage.COPY_DST,
      mappedAtCreation: true,
      label: `TknBuffer_HOST_VISIBLE_${size}`,
    });

    // 获取映射数据指针
    mappedData = gpuBuffer.getMappedRange();

    // 写入初始数据
    if (pData) {
      new Uint8Array(mappedData).set(new Uint8Array(pData));
    }

    // 保持映射状态（不调用 unmap），用于后续的持久访问
  } else {
    // DEVICE_LOCAL: 创建 GPU 优化的 buffer，初始数据通过 staging buffer 上传
    gpuBuffer = device.createBuffer({
      size: size,
      usage: gpuUsage | GPUBufferUsage.COPY_DST,
      label: `TknBuffer_DEVICE_LOCAL_${size}`,
    });

    // 如果有初始数据，创建 staging buffer
    if (pData) {
      stagingBuffer = device.createBuffer({
        size: size,
        usage: GPUBufferUsage.COPY_SRC,
        mappedAtCreation: true,
        label: `TknBuffer_staging_${size}`,
      });

      // 写入初始数据到 staging buffer
      new Uint8Array(stagingBuffer.getMappedRange()).set(new Uint8Array(pData));
      stagingBuffer.unmap();

      // 创建命令编码器并执行转移
      const commandEncoder = device.createCommandEncoder();
      commandEncoder.copyBufferToBuffer(
        stagingBuffer, 0,
        gpuBuffer, 0,
        size
      );
      device.queue.submit([commandEncoder.finish()]);
    }
  }

  return {
    gpuBuffer,
    stagingBuffer,
    mappedData,
    size,
    usage: gpuUsage,
    mappedAtCreation,
    memoryType,
  };
}

/**
 * 销毁 GPU buffer
 * 对应 C API: tknDestroyBufferPtr
 *
 * @param {TknBuffer|null} pBuffer - Buffer 对象（可以为 null）
 */
export function tknDestroyBufferPtr(pBuffer) {
  if (!pBuffer) {
    console.warn("tknDestroyBufferPtr: Buffer is null");
    return;
  }

  // unmap（如果已映射）
  if (pBuffer.gpuBuffer && pBuffer.mappedData) {
    pBuffer.gpuBuffer.unmap();
  }

  // 销毁 GPU buffer
  if (pBuffer.gpuBuffer) {
    pBuffer.gpuBuffer.destroy();
  }

  // 销毁 staging buffer
  if (pBuffer.stagingBuffer) {
    pBuffer.stagingBuffer.destroy();
  }
}

// ============================================================================
// Buffer 绑定
// ============================================================================

// 绑定位置常量（对应 C 中的 TknVertexBinding enum）
export const TKN_VERTEX_BINDING_DESCRIPTION = 0;
export const TKN_INSTANCE_BINDING_DESCRIPTION = 1;

/**
 * @typedef {Object} TknBindingState
 * @property {TknBuffer|null} vertexBuffer - 顶点 buffer (binding 0)
 * @property {number} vertexBufferOffset - 顶点 buffer offset
 * @property {TknBuffer|null} instanceBuffer - 实例 buffer (binding 1)
 * @property {number} instanceBufferOffset - 实例 buffer offset
 * @property {TknBuffer|null} indexBuffer - 索引 buffer
 * @property {number} indexBufferOffset - 索引 buffer offset
 * @property {string} indexType - 索引类型 ('uint16' | 'uint32')
 */

/**
 * 创建绑定状态对象
 * @returns {TknBindingState}
 */
export function tknCreateBindingState() {
  return {
    vertexBuffer: null,
    vertexBufferOffset: 0,
    instanceBuffer: null,
    instanceBufferOffset: 0,
    indexBuffer: null,
    indexBufferOffset: 0,
    indexType: "uint32",
  };
}

/**
 * 绑定顶点 buffer
 * 对应 C API: tknBindVertexBuffer
 * 绑定到固定位置 0 (TKN_VERTEX_BINDING_DESCRIPTION)
 *
 * @param {TknBindingState} state - 绑定状态
 * @param {TknBuffer} pBuffer - Buffer 对象
 * @param {number} offset - 起始偏移量
 */
export function tknBindVertexBuffer(state, pBuffer, offset = 0) {
  if (!state) throw new Error("Binding state cannot be null");
  if (!pBuffer) throw new Error("Vertex buffer cannot be null");
  if (offset < 0) throw new Error("Offset cannot be negative");
  if (offset + 1 > pBuffer.size) throw new Error("Vertex buffer offset out of bounds");

  state.vertexBuffer = pBuffer;
  state.vertexBufferOffset = offset;
}

/**
 * 绑定实例 buffer
 * 对应 C API: tknBindInstanceBuffer
 * 绑定到固定位置 1 (TKN_INSTANCE_BINDING_DESCRIPTION)
 *
 * @param {TknBindingState} state - 绑定状态
 * @param {TknBuffer} pBuffer - Buffer 对象
 * @param {number} offset - 起始偏移量
 */
export function tknBindInstanceBuffer(state, pBuffer, offset = 0) {
  if (!state) throw new Error("Binding state cannot be null");
  if (!pBuffer) throw new Error("Instance buffer cannot be null");
  if (offset < 0) throw new Error("Offset cannot be negative");
  if (offset + 1 > pBuffer.size) throw new Error("Instance buffer offset out of bounds");

  state.instanceBuffer = pBuffer;
  state.instanceBufferOffset = offset;
}

/**
 * 绑定索引 buffer
 * 对应 C API: tknBindIndexBuffer
 *
 * @param {TknBindingState} state - 绑定状态
 * @param {TknBuffer} pBuffer - Buffer 对象
 * @param {number|string} indexType - 索引类型 (Vulkan: 0=uint16/1=uint32 或 WebGPU: "uint16"/"uint32")
 * @param {number} offset - 起始偏移量
 */
export function tknBindIndexBuffer(state, pBuffer, indexType = 1, offset = 0) {
  if (!state) throw new Error("Binding state cannot be null");
  if (!pBuffer) throw new Error("Index buffer cannot be null");
  if (offset < 0) throw new Error("Offset cannot be negative");
  if (offset + 1 > pBuffer.size) throw new Error("Index buffer offset out of bounds");

  // 自动转换 Vulkan 索引类型到 WebGPU
  let webgpuIndexType = indexType;
  if (typeof indexType === "number") {
    // 从 Vulkan 值转换
    webgpuIndexType = convertVulkanIndexTypeToWebGPU(indexType);
  }

  if (webgpuIndexType !== "uint16" && webgpuIndexType !== "uint32") {
    throw new Error("Index type must be 'uint16', 'uint32', 0, or 1");
  }

  state.indexBuffer = pBuffer;
  state.indexBufferOffset = offset;
  state.indexType = webgpuIndexType;
}

// ============================================================================
// Buffer 更新
// ============================================================================

/**
 * 更新 buffer 数据
 * 对应 C API: tknUpdateBuffer
 * 自动选择最优路径：
 *   - HOST_VISIBLE: 直接 memcpy
 *   - DEVICE_LOCAL: 使用 staging buffer + GPU 转移
 *
 * @param {GPUDevice} device - WebGPU 设备
 * @param {GPUQueue} queue - GPU 命令队列
 * @param {TknBuffer} pBuffer - Buffer 对象
 * @param {number} offset - 写入偏移量
 * @param {number} size - 数据大小
 * @param {ArrayBuffer} pData - 数据指针
 */
export function tknUpdateBuffer(device, queue, pBuffer, offset, size, pData) {
  if (!device) throw new Error("Device cannot be null");
  if (!queue) throw new Error("Queue cannot be null");
  if (!pBuffer) throw new Error("Buffer cannot be null");
  if (!pData) throw new Error("Data cannot be null");
  if (size <= 0) throw new Error("Update size must be greater than 0");
  if (offset < 0) throw new Error("Offset cannot be negative");
  if (offset + size > pBuffer.size) throw new Error("Buffer update out of bounds");

  const dataUint8 = new Uint8Array(pData);

  if (pBuffer.memoryType === 0) {
    // HOST_VISIBLE: 直接 memcpy 到持久映射
    if (pBuffer.mappedData) {
      const targetView = new Uint8Array(pBuffer.mappedData, offset, size);
      targetView.set(dataUint8);
    } else {
      // 如果没有持久映射，创建临时映射
      throw new Error("HOST_VISIBLE buffer lost mapping");
    }
  } else {
    // DEVICE_LOCAL: 使用 staging buffer
    const stagingBuffer = device.createBuffer({
      size: size,
      usage: GPUBufferUsage.COPY_SRC,
      mappedAtCreation: true,
      label: `TknBuffer_update_staging_${size}`,
    });

    // 写入数据到 staging buffer
    new Uint8Array(stagingBuffer.getMappedRange()).set(dataUint8);
    stagingBuffer.unmap();

    // 创建转移命令
    const commandEncoder = device.createCommandEncoder();
    commandEncoder.copyBufferToBuffer(
      stagingBuffer, 0,
      pBuffer.gpuBuffer, offset,
      size
    );
    queue.submit([commandEncoder.finish()]);

    // 清理 staging buffer
    stagingBuffer.destroy();
  }
}

// ============================================================================
// Binding Group 管理
// ============================================================================

/**
 * @typedef {Object} TknBindingGroupLayout
 * @property {GPUBindGroupLayout} gpuBindGroupLayout - WebGPU 绑定组布局
 */

/**
 * @typedef {Object} TknBindingGroup
 * @property {GPUBindGroup} gpuBindGroup - WebGPU 绑定组
 * @property {TknBindingGroupLayout} pLayout - 布局引用
 * @property {GPUDevice} device - 设备引用
 */

/**
 * 创建 Binding Group
 * 对应 C API: tknCreateBindingGroup
 *
 * @param {GPUDevice} device - WebGPU 设备
 * @param {GPUBindGroupLayout} bindGroupLayout - WebGPU 绑定组布局
 * @param {Array<GPUBindingResource>} resources - 绑定的资源数组
 * @returns {TknBindingGroup} Binding Group 对象
 */
export function tknCreateBindingGroup(device, bindGroupLayout, resources = []) {
  if (!device) throw new Error("Device cannot be null");
  if (!bindGroupLayout) throw new Error("Bind group layout cannot be null");

  // 为每个资源创建绑定条目
  const entries = resources
    .map((resource, index) => ({
      binding: index,
      resource: resource,
    }))
    .filter((entry) => entry.resource !== null && entry.resource !== undefined);

  // 创建 WebGPU 绑定组
  const gpuBindGroup = device.createBindGroup({
    layout: bindGroupLayout,
    entries: entries,
    label: `TknBindingGroup_${entries.length}`,
  });

  return {
    gpuBindGroup,
    pLayout: { gpuBindGroupLayout: bindGroupLayout },
    device,
  };
}

/**
 * 销毁 Binding Group
 * 对应 C API: tknDestroyBindingGroup
 *
 * @param {TknBindingGroup|null} pBindingGroup - Binding Group 对象（可以为 null）
 */
export function tknDestroyBindingGroup(pBindingGroup) {
  if (!pBindingGroup) {
    console.warn("tknDestroyBindingGroup: Binding Group is null");
    return;
  }

  // WebGPU 的绑定组会自动垃圾回收，无需显式销毁
  // 这里只是清理引用
  pBindingGroup.gpuBindGroup = null;
  pBindingGroup.device = null;
}

/**
 * 更新 Binding Group 中的资源
 * 对应 C API: tknUpdateBindingGroup
 * 通过重新创建 bind group 来更新资源
 *
 * @param {GPUDevice} device - WebGPU 设备
 * @param {TknBindingGroup} pBindingGroup - Binding Group 对象
 * @param {Array<number>} indices - 要更新的 binding 位置数组
 * @param {Array<GPUBindingResource>} resources - 对应的资源数组
 */
export function tknUpdateBindingGroup(device, pBindingGroup, indices = [], resources = []) {
  if (!device) throw new Error("Device cannot be null");
  if (!pBindingGroup) throw new Error("Binding Group cannot be null");
  if (!pBindingGroup.pLayout || !pBindingGroup.pLayout.gpuBindGroupLayout) {
    throw new Error("Binding Group layout is invalid");
  }

  if (indices.length !== resources.length) {
    throw new Error("indices and resources arrays must have the same length");
  }

  // 为指定位置的资源创建绑定条目
  const entries = indices
    .map((binding, i) => ({
      binding: binding,
      resource: resources[i],
    }))
    .filter((entry) => entry.resource !== null && entry.resource !== undefined);

  // 销毁旧的绑定组（WebGPU 会自动处理）
  pBindingGroup.gpuBindGroup = null;

  // 创建新的绑定组
  pBindingGroup.gpuBindGroup = device.createBindGroup({
    layout: pBindingGroup.pLayout.gpuBindGroupLayout,
    entries: entries,
    label: `TknBindingGroup_updated_${entries.length}`,
  });
}

// ============================================================================
// 绘制命令
// ============================================================================

/**
 * 执行非索引绘制
 * 对应 C API: tknDraw
 *
 * @param {GPURenderPassEncoder} passEncoder - 渲染通道编码器
 * @param {TknBindingState} state - 绑定状态
 * @param {number} vertexCount - 顶点数
 * @param {number} instanceCount - 实例数（默认 1）
 * @param {number} firstVertex - 第一个顶点索引（默认 0）
 * @param {number} firstInstance - 第一个实例索引（默认 0）
 */
export function tknDraw(
  passEncoder,
  state,
  vertexCount,
  instanceCount = 1,
  firstVertex = 0,
  firstInstance = 0
) {
  if (!passEncoder) throw new Error("Pass encoder cannot be null");
  if (!state) throw new Error("Binding state cannot be null");
  if (vertexCount <= 0) throw new Error("Vertex count must be greater than 0");

  // 绑定 vertex buffer
  if (state.vertexBuffer) {
    passEncoder.setVertexBuffer(
      TKN_VERTEX_BINDING_DESCRIPTION,
      state.vertexBuffer.gpuBuffer,
      state.vertexBufferOffset
    );
  }

  // 绑定 instance buffer
  if (state.instanceBuffer) {
    passEncoder.setVertexBuffer(
      TKN_INSTANCE_BINDING_DESCRIPTION,
      state.instanceBuffer.gpuBuffer,
      state.instanceBufferOffset
    );
  }

  // 执行绘制
  passEncoder.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

/**
 * 执行索引绘制
 * 对应 C API: tknDrawIndexed
 *
 * @param {GPURenderPassEncoder} passEncoder - 渲染通道编码器
 * @param {TknBindingState} state - 绑定状态
 * @param {number} indexCount - 索引数
 * @param {number} instanceCount - 实例数（默认 1）
 * @param {number} firstIndex - 第一个索引位置（默认 0）
 * @param {number} baseVertex - 顶点偏移（默认 0）
 * @param {number} firstInstance - 第一个实例索引（默认 0）
 */
export function tknDrawIndexed(
  passEncoder,
  state,
  indexCount,
  instanceCount = 1,
  firstIndex = 0,
  baseVertex = 0,
  firstInstance = 0
) {
  if (!passEncoder) throw new Error("Pass encoder cannot be null");
  if (!state) throw new Error("Binding state cannot be null");
  if (indexCount <= 0) throw new Error("Index count must be greater than 0");
  if (!state.indexBuffer) throw new Error("Index buffer not bound");

  // 绑定 vertex buffer
  if (state.vertexBuffer) {
    passEncoder.setVertexBuffer(
      TKN_VERTEX_BINDING_DESCRIPTION,
      state.vertexBuffer.gpuBuffer,
      state.vertexBufferOffset
    );
  }

  // 绑定 instance buffer
  if (state.instanceBuffer) {
    passEncoder.setVertexBuffer(
      TKN_INSTANCE_BINDING_DESCRIPTION,
      state.instanceBuffer.gpuBuffer,
      state.instanceBufferOffset
    );
  }

  // 绑定索引 buffer
  passEncoder.setIndexBuffer(state.indexBuffer.gpuBuffer, state.indexType, state.indexBufferOffset);

  // 执行绘制
  passEncoder.drawIndexed(indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
}

/**
 * 导出转换函数和 Binding Group 函数供 Lua 或其他客户端使用
 */
export {
  convertVulkanBufferUsageToWebGPU,
  convertVulkanIndexTypeToWebGPU,
  VK_BUFFER_USAGE_FLAGS,
  VK_INDEX_TYPE_MAP,
  tknCreateBindingGroup,
  tknDestroyBindingGroup,
  tknUpdateBindingGroup,
};

// ============================================================================
// WGSL 反射工具（保留）
// ============================================================================

const _WGSL_TYPE_TO_VERTEX_FORMAT = {
  f32: { format: "float32", size: 4 },
  vec2f: { format: "float32x2", size: 8 },
  "vec2<f32>": { format: "float32x2", size: 8 },
  vec3f: { format: "float32x3", size: 12 },
  "vec3<f32>": { format: "float32x3", size: 12 },
  vec4f: { format: "float32x4", size: 16 },
  "vec4<f32>": { format: "float32x4", size: 16 },
  u32: { format: "uint32", size: 4 },
  vec2u: { format: "uint32x2", size: 8 },
  "vec2<u32>": { format: "uint32x2", size: 8 },
  vec3u: { format: "uint32x3", size: 12 },
  "vec3<u32>": { format: "uint32x3", size: 12 },
  vec4u: { format: "uint32x4", size: 16 },
  "vec4<u32>": { format: "uint32x4", size: 16 },
  i32: { format: "sint32", size: 4 },
  vec2i: { format: "sint32x2", size: 8 },
  "vec2<i32>": { format: "sint32x2", size: 8 },
  vec3i: { format: "sint32x3", size: 12 },
  "vec3<i32>": { format: "sint32x3", size: 12 },
  vec4i: { format: "sint32x4", size: 16 },
  "vec4<i32>": { format: "sint32x4", size: 16 },
};

/**
 * 从 WGSL 源码反射出 GPUVertexBufferLayout
 *
 * @param {string} wgslCode - WGSL 着色器源码
 * @param {string} [entryPoint] - vertex entry point 名称（默认 'vs_main'）
 * @returns {GPUVertexBufferLayout}
 */
export function reflectWGSLVertexInput(wgslCode, entryPoint = "vs_main") {
  // 1. 找到 @vertex entry function 的参数列表
  const fnRe = new RegExp(`@vertex\\s+fn\\s+${entryPoint}\\s*\\(([^)]*)\\)`, "s");
  const fnMatch = wgslCode.match(fnRe);
  if (!fnMatch) throw new Error(`@vertex fn "${entryPoint}" not found in WGSL`);

  // 2. 从参数列表中找到有 @location 字段的 struct 类型
  const paramList = fnMatch[1];
  const paramTypeRe = /:\s*([\w]+)/g;
  let structName = null;
  let m;
  while ((m = paramTypeRe.exec(paramList)) !== null) {
    const typeName = m[1];
    const structBodyRe = new RegExp(`struct\\s+${typeName}\\s*\\{([^}]*)\\}`, "s");
    const structMatch = wgslCode.match(structBodyRe);
    if (structMatch && /@location/.test(structMatch[1])) {
      structName = typeName;
      break;
    }
  }
  if (!structName) throw new Error(`No @location-annotated input struct found for "${entryPoint}"`);

  // 3. 解析 struct 字段
  const structBodyRe = new RegExp(`struct\\s+${structName}\\s*\\{([^}]*)\\}`, "s");
  const structBody = wgslCode.match(structBodyRe)[1];
  const fieldRe = /@location\((\d+)\)\s+\w+\s*:\s*([\w<>]+)/g;
  const fields = [];
  while ((m = fieldRe.exec(structBody)) !== null) {
    fields.push({ location: parseInt(m[1], 10), type: m[2].trim() });
  }
  if (fields.length === 0) throw new Error(`No @location fields in struct ${structName}`);

  // 4. 按 location 排序，计算 offset 和 stride
  fields.sort((a, b) => a.location - b.location);
  let offset = 0;
  const attributes = fields.map(({ location, type }) => {
    const entry = _WGSL_TYPE_TO_VERTEX_FORMAT[type];
    if (!entry) throw new Error(`Unsupported WGSL vertex type: "${type}"`);
    const attr = { shaderLocation: location, offset, format: entry.format };
    offset += entry.size;
    return attr;
  });

  return { arrayStride: offset, stepMode: "vertex", attributes };
}