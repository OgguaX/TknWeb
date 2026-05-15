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