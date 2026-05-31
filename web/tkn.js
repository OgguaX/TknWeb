/**
 * 更新WebGPU纹理数据
 * @param {GPUDevice} device - GPU设备
 * @param {GPUTexture} texture - 目标纹理
 * @param {Uint8Array} imageData - 像素数据
 * @param {Object} options - 可选参数
 * @param {number} options.width - 宽度（必需）
 * @param {number} options.height - 高度（必需）
 * @param {number} options.bytesPerRow - 每行字节数（默认: width * 4）
 * @param {number} options.mipLevel - mip级别（默认: 0）
 * @param {Array} options.origin - 起始位置[x, y, z]（默认: [0, 0, 0]）
 * @param {string} options.aspect - 纹理方面（默认: "all"）
 * @param {number} options.depthOrArrayLayers - 深度或数组层（默认: 1）
 * @param {string} options.label - 调试标签
 */
async function updateWebGPUTexture(device, texture, imageData, options = {}) {
  // 提取参数并设置默认值
  const {
    width = 256,
    height = 256,
    bytesPerRow = width * 4,
    mipLevel = 0,
    origin = [0, 0, 0],
    aspect = "all",
    depthOrArrayLayers = 1,
    label = "textureUpdate",
  } = options;

  // 验证参数
  if (!device || !texture || !imageData) {
    throw new Error("device, texture, and imageData are required");
  }

  if (bytesPerRow % 256 !== 0) {
    console.warn(`bytesPerRow (${bytesPerRow}) should be a multiple of 256`);
  }

  // 1. 创建staging buffer
  const stagingBuffer = device.createBuffer({
    size: imageData.byteLength,
    usage: GPUBufferUsage.COPY_SRC,
    mappedAtCreation: true,
    label: `${label}_stagingBuffer`,
  });

  // 2. 写入数据
  new Uint8Array(stagingBuffer.getMappedRange()).set(imageData);
  stagingBuffer.unmap();

  // 3. 创建命令编码器
  const commandEncoder = device.createCommandEncoder({
    label: `${label}_encoder`,
  });

  // 4. 复制buffer到texture
  commandEncoder.copyBufferToTexture(
    {
      buffer: stagingBuffer,
      offset: 0,
      bytesPerRow: bytesPerRow,
      rowsPerImage: height,
    },
    {
      texture: texture,
      mipLevel: mipLevel,
      origin: origin,
      aspect: aspect,
    },
    [width, height, depthOrArrayLayers]
  );

  // 5. 提交命令
  device.queue.submit([
    commandEncoder.finish({
      label: `${label}_commands`,
    }),
  ]);

  // 6. 清理
  stagingBuffer.destroy();
}

// ---------------------------------------------------------------------------
// WGSL Vertex Input Reflection
// ---------------------------------------------------------------------------

const _WGSL_TYPE_TO_VERTEX_FORMAT = {
    'f32':       { format: 'float32',   size: 4  },
    'vec2f':     { format: 'float32x2', size: 8  },
    'vec2<f32>': { format: 'float32x2', size: 8  },
    'vec3f':     { format: 'float32x3', size: 12 },
    'vec3<f32>': { format: 'float32x3', size: 12 },
    'vec4f':     { format: 'float32x4', size: 16 },
    'vec4<f32>': { format: 'float32x4', size: 16 },
    'u32':       { format: 'uint32',    size: 4  },
    'vec2u':     { format: 'uint32x2',  size: 8  },
    'vec2<u32>': { format: 'uint32x2',  size: 8  },
    'vec3u':     { format: 'uint32x3',  size: 12 },
    'vec3<u32>': { format: 'uint32x3',  size: 12 },
    'vec4u':     { format: 'uint32x4',  size: 16 },
    'vec4<u32>': { format: 'uint32x4',  size: 16 },
    'i32':       { format: 'sint32',    size: 4  },
    'vec2i':     { format: 'sint32x2',  size: 8  },
    'vec2<i32>': { format: 'sint32x2',  size: 8  },
    'vec3i':     { format: 'sint32x3',  size: 12 },
    'vec3<i32>': { format: 'sint32x3',  size: 12 },
    'vec4i':     { format: 'sint32x4',  size: 16 },
    'vec4<i32>': { format: 'sint32x4',  size: 16 },
};

/**
 * 从 WGSL 源码反射出 GPUVertexBufferLayout，免去手写 attributes/arrayStride。
 *
 * 约束：
 *  - 只处理单个 interleaved vertex buffer（stepMode: 'vertex'）
 *  - 不支持 @builtin 字段混在 vertex input struct 里
 *  - 字段按 @location 值从小到大紧密打包（无 padding）
 *
 * @param {string} wgslCode       - WGSL 着色器源码
 * @param {string} [entryPoint]   - vertex entry point 名称，默认 'vs_main'
 * @returns {GPUVertexBufferLayout}
 */
export function reflectWGSLVertexInput(wgslCode, entryPoint = 'vs_main') {
    // 1. 找到 @vertex entry function 的参数列表
    const fnRe = new RegExp(`@vertex\\s+fn\\s+${entryPoint}\\s*\\(([^)]*)\\)`, 's');
    const fnMatch = wgslCode.match(fnRe);
    if (!fnMatch) throw new Error(`@vertex fn "${entryPoint}" not found in WGSL`);

    // 2. 从参数列表中找到有 @location 字段的 struct 类型
    const paramList = fnMatch[1];
    const paramTypeRe = /:\s*([\w]+)/g;
    let structName = null;
    let m;
    while ((m = paramTypeRe.exec(paramList)) !== null) {
        const typeName = m[1];
        const structBodyRe = new RegExp(`struct\\s+${typeName}\\s*\\{([^}]*)\\}`, 's');
        const structMatch = wgslCode.match(structBodyRe);
        if (structMatch && /@location/.test(structMatch[1])) {
            structName = typeName;
            break;
        }
    }
    if (!structName) throw new Error(`No @location-annotated input struct found for "${entryPoint}"`);

    // 3. 解析 struct 字段
    const structBodyRe = new RegExp(`struct\\s+${structName}\\s*\\{([^}]*)\\}`, 's');
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

    return { arrayStride: offset, stepMode: 'vertex', attributes };
}