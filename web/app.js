/**
 * @typedef {Object} TknGfxContext
 * @property {HTMLCanvasElement} canvas
 * @property {GPUDevice | null} device
 * @property {GPUCanvasContext | null} context
 * @property {GPUTextureFormat | null} format
 */
const GPUTextureUsage = globalThis.GPUTextureUsage;
const GPUShaderStage = globalThis.GPUShaderStage;
const GPUBufferUsage = globalThis.GPUBufferUsage;

class TknGfxContext {
    /**
     * @param {HTMLCanvasElement} canvas
     */
    constructor(canvas) {
        this.canvas = canvas;
        /** @type {GPUDevice | null} */
        this.device = null;
        /** @type {GPUCanvasContext | null} */
        this.context = null;
        /** @type {GPUTextureFormat | null} */
        this.format = null;
    }

    /**
     * @returns {Promise<void>}
     */
    async create() {
        this.canvas.width = window.innerWidth;
        this.canvas.height = window.innerHeight;

        const adapter = await navigator.gpu.requestAdapter();
        if (!adapter) throw new Error('No GPU adapter found');

        this.device = await adapter.requestDevice();
        this.context = this.canvas.getContext('webgpu');
        if (!this.context) throw new Error('WebGPU context not available');

        this.format = navigator.gpu.getPreferredCanvasFormat();

        this.context.configure({
            device: this.device,
            format: this.format,
            alphaMode: 'opaque'
        });
    }

    /**
     * @param {Image|Canvas|OffscreenCanvas|ImageBitmap} imageSource
     * @param {number} width
     * @param {number} height
     * @returns {GPUTexture}
     */
    loadImageAsTexture(imageSource, width, height) {
        const texture = this.device.createTexture({
            size: { width, height, depthOrArrayLayers: 1 },
            format: 'rgba8unorm',
            usage: GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_DST,
            mipLevelCount: 1
        });

        this.device.queue.copyExternalImageToTexture(
            { source: imageSource },
            { texture, mipLevel: 0, origin: [0, 0, 0] },
            { width, height, depthOrArrayLayers: 1 }
        );

        return texture;
    }
}

// 光源结构体（必须与WGSL对齐）
class Light {
    constructor(position, color, intensity, radius) {
        this.position = position;
        this.color = color;
        this.intensity = intensity;
        this.radius = radius;
    }
}

class DeferredRenderer {
    /**
     * @param {TknGfxContext} gfxContext
     */
    constructor(gfxContext) {
        this.gfx = gfxContext;
        
        // 几何体缓冲区
        this.geometries = new Map();
        
        // 管线
        this.pipeline = null;
        
        // 采样器
        this.sampler = null;
    }

    /**
     * @returns {Promise<void>}
     */
    async init() {
        console.log('初始化渲染器...');
        this.setupSampler();
        this.setupPipeline();
        this.createGeometry();
        
        console.log('✓ 渲染器已初始化');
        document.getElementById('status').textContent = '就绪';
    }

    setupSampler() {
        this.sampler = this.gfx.device.createSampler({
            magFilter: 'linear',
            minFilter: 'linear',
            mipmapFilter: 'linear'
        });
    }

    /**
     * 创建矩阵均匀缓冲区
     * @returns {void}
     */
    setupMatrixBuffer() {
        const matrixData = new Float32Array(32);
        
        const aspect = this.gfx.canvas.width / this.gfx.canvas.height;
        const fov = Math.PI / 4;
        const near = 0.1;
        const far = 100.0;
        
        const f = 1.0 / Math.tan(fov / 2.0);
        const nf = 1.0 / (near - far);
        
        matrixData[0] = f / aspect;
        matrixData[5] = f;
        matrixData[10] = (far + near) * nf;
        matrixData[11] = -1;
        matrixData[14] = 2 * far * near * nf;
        
        const eye = [0, 0, 1.5];
        const center = [0, 0, 0];
        const up = [0, 1, 0];
        
        const f2 = [center[0] - eye[0], center[1] - eye[1], center[2] - eye[2]];
        const len = Math.sqrt(f2[0] * f2[0] + f2[1] * f2[1] + f2[2] * f2[2]);
        f2[0] /= len; f2[1] /= len; f2[2] /= len;
        
        const s = [up[1] * f2[2] - up[2] * f2[1], up[2] * f2[0] - up[0] * f2[2], up[0] * f2[1] - up[1] * f2[0]];
        const slen = Math.sqrt(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
        s[0] /= slen; s[1] /= slen; s[2] /= slen;
        
        const u = [f2[1] * s[2] - f2[2] * s[1], f2[2] * s[0] - f2[0] * s[2], f2[0] * s[1] - f2[1] * s[0]];
        
        matrixData[16] = s[0]; matrixData[17] = u[0]; matrixData[18] = -f2[0];
        matrixData[20] = s[1]; matrixData[21] = u[1]; matrixData[22] = -f2[1];
        matrixData[24] = s[2]; matrixData[25] = u[2]; matrixData[26] = -f2[2];
        matrixData[28] = -(s[0] * eye[0] + s[1] * eye[1] + s[2] * eye[2]);
        matrixData[29] = -(u[0] * eye[0] + u[1] * eye[1] + u[2] * eye[2]);
        matrixData[30] = (f2[0] * eye[0] + f2[1] * eye[1] + f2[2] * eye[2]);
        matrixData[31] = 1;
        
        this.matrixBuffer = this.gfx.device.createBuffer({
            size: matrixData.byteLength,
            usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
            mappedAtCreation: true
        });
        new Float32Array(this.matrixBuffer.getMappedRange()).set(matrixData);
        this.matrixBuffer.unmap();
    }

    /**
     * 初始化G-Buffer纹理
     * @returns {void}
     */
    setupGBuffer() {
        const size = { width: this.gfx.canvas.width, height: this.gfx.canvas.height };

        // 位置纹理（RGB32F）
        this.gBuffer.position = this.gfx.device.createTexture({
            size,
            format: 'rgba32float',
            usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING
        });
        this.gBufferViews.position = this.gBuffer.position.createView();

        // 法线纹理（RGBA8）
        this.gBuffer.normal = this.gfx.device.createTexture({
            size,
            format: 'rgba8unorm',
            usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING
        });
        this.gBufferViews.normal = this.gBuffer.normal.createView();

        // 反射率/材质纹理（RGBA8）
        this.gBuffer.albedo = this.gfx.device.createTexture({
            size,
            format: 'rgba8unorm',
            usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING
        });
        this.gBufferViews.albedo = this.gBuffer.albedo.createView();

        // 专项属性纹理（粗糙度、金属度等）
        this.gBuffer.attributes = this.gfx.device.createTexture({
            size,
            format: 'rgba8unorm',
            usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING
        });
        this.gBufferViews.attributes = this.gBuffer.attributes.createView();

        // 深度纹理
        this.depthTexture = this.gfx.device.createTexture({
            size,
            format: 'depth32float',
            usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING
        });
        this.depthView = this.depthTexture.createView();
    }

    /**
     * 初始化管线
     * @returns {void}
     */
    setupPipelines() {
        this.setupGBufferPipeline();
        this.setupLightingPipeline();
    }

    /**
     * 初始化G-Buffer管线
     * @returns {void}
     */
    setupGBufferPipeline() {
        const gBufferShader = this.gfx.device.createShaderModule({
            code: `
                struct VertexInput {
                    @location(0) position: vec3f,
                    @location(1) normal: vec3f,
                    @location(2) color: vec3f,
                }

                struct VertexOutput {
                    @builtin(position) position: vec4f,
                    @location(0) worldPos: vec3f,
                    @location(1) normal: vec3f,
                    @location(2) color: vec3f,
                }

                @vertex
                fn vs_main(input: VertexInput) -> VertexOutput {
                    var output: VertexOutput;
                    output.position = vec4f(input.position, 1.0);
                    output.worldPos = input.position;
                    output.normal = normalize(input.normal);
                    output.color = input.color;
                    return output;
                }

                struct FragmentOutput {
                    @location(0) position: vec4f,
                    @location(1) normal: vec4f,
                    @location(2) albedo: vec4f,
                    @location(3) attributes: vec4f,
                }

                @fragment
                fn fs_main(input: VertexOutput) -> FragmentOutput {
                    var output: FragmentOutput;
                    
                    output.position = vec4f(input.worldPos, 1.0);
                    output.normal = vec4f(input.normal * 0.5 + 0.5, 1.0);
                    output.albedo = vec4f(input.color, 1.0);
                    output.attributes = vec4f(0.5, 0.0, 0.0, 1.0);
                    
                    return output;
                }
            `
        });

        const layout = this.gfx.device.createPipelineLayout({
            bindGroupLayouts: []
        });

        this.gBufferPipeline = this.gfx.device.createRenderPipeline({
            layout,
            vertex: {
                module: gBufferShader,
                entryPoint: 'vs_main',
                buffers: [{
                    arrayStride: 36,
                    attributes: [
                        { shaderLocation: 0, offset: 0, format: 'float32x3' },
                        { shaderLocation: 1, offset: 12, format: 'float32x3' },
                        { shaderLocation: 2, offset: 24, format: 'float32x3' }
                    ]
                }]
            },
            fragment: {
                module: gBufferShader,
                entryPoint: 'fs_main',
                targets: [
                    { format: 'rgba32float' },
                    { format: 'rgba8unorm' },
                    { format: 'rgba8unorm' },
                    { format: 'rgba8unorm' }
                ]
            },
            primitive: { topology: 'triangle-list', frontFace: 'ccw', cullMode: 'back' },
            depthStencil: {
                format: 'depth32float',
                depthWriteEnabled: true,
                depthCompare: 'less'
            }
        });
    }

    setupLightingPipeline() {
        const lightingShader = this.gfx.device.createShaderModule({
            code: `
                @group(0) @binding(0) var gPosition: texture_2d<f32>;
                @group(0) @binding(1) var gNormal: texture_2d<f32>;
                @group(0) @binding(2) var gAlbedo: texture_2d<f32>;
                @group(0) @binding(3) var gAttributes: texture_2d<f32>;
                @group(0) @binding(4) var texSampler: sampler;

                @vertex
                fn vs_main(@builtin(vertex_index) idx: u32) -> @builtin(position) vec4f {
                    let pos = array<vec2f, 6>(
                        vec2f(-1, 1), vec2f(1, 1), vec2f(-1, -1),
                        vec2f(1, 1), vec2f(1, -1), vec2f(-1, -1)
                    );
                    return vec4f(pos[idx], 0, 1);
                }

                @fragment
                fn fs_main(@builtin(position) fragCoord: vec4f) -> @location(0) vec4f {
                    let uv = fragCoord.xy / vec2f(textureDimensions(gPosition));
                    
                    let posData = textureSample(gPosition, texSampler, uv);
                    let normalData = textureSample(gNormal, texSampler, uv);
                    let albedoData = textureSample(gAlbedo, texSampler, uv);
                    
                    let albedo = albedoData.rgb;
                    let normal = normalize(normalData.rgb * 2.0 - 1.0);
                    
                    let lightDir = normalize(vec3f(1.0, 1.0, 1.0));
                    let diffuse = max(dot(normal, lightDir), 0.0);
                    let ambient = 0.3;
                    
                    let finalColor = albedo * (ambient + diffuse * 0.7);
                    
                    return vec4f(finalColor, 1.0);
                }
            `
        });

        const bindGroupLayout = this.gfx.device.createBindGroupLayout({
            entries: [
                { binding: 0, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float', viewDimension: '2d' } },
                { binding: 1, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float', viewDimension: '2d' } },
                { binding: 2, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float', viewDimension: '2d' } },
                { binding: 3, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float', viewDimension: '2d' } },
                { binding: 4, visibility: GPUShaderStage.FRAGMENT, sampler: {} }
            ]
        });

        const layout = this.gfx.device.createPipelineLayout({
            bindGroupLayouts: [bindGroupLayout]
        });

        this.lightingPipeline = this.gfx.device.createRenderPipeline({
            layout,
            vertex: {
                module: lightingShader,
                entryPoint: 'vs_main'
            },
            fragment: {
                module: lightingShader,
                entryPoint: 'fs_main',
                targets: [{ format: this.gfx.format }]
            },
            primitive: { topology: 'triangle-list' }
        });
    }

    /**
     * 创建几何体
     * @returns {void}
     */
    createGeometry() {
        // 创建立方体
        this.createCube();
        this.createPyramid();
    }

    /**
     * 创建立方体几何体
     * @returns {void}
     */
    createCube() {
        const vertices = new Float32Array([
            // Front
            -0.5, -0.5, 0.5, 0, 0, 1, 1, 0, 0,
            0.5, -0.5, 0.5, 0, 0, 1, 1, 0, 0,
            0.5, 0.5, 0.5, 0, 0, 1, 1, 0, 0,
            -0.5, 0.5, 0.5, 0, 0, 1, 1, 0, 0,
            
            // Right
            0.5, -0.5, 0.5, 1, 0, 0, 0, 1, 0,
            0.5, -0.5, -0.5, 1, 0, 0, 0, 1, 0,
            0.5, 0.5, -0.5, 1, 0, 0, 0, 1, 0,
            0.5, 0.5, 0.5, 1, 0, 0, 0, 1, 0,
            
            // Back
            0.5, -0.5, -0.5, 0, 0, -1, 0, 0, 1,
            -0.5, -0.5, -0.5, 0, 0, -1, 0, 0, 1,
            -0.5, 0.5, -0.5, 0, 0, -1, 0, 0, 1,
            0.5, 0.5, -0.5, 0, 0, -1, 0, 0, 1,
            
            // Left
            -0.5, -0.5, -0.5, -1, 0, 0, 1, 1, 0,
            -0.5, -0.5, 0.5, -1, 0, 0, 1, 1, 0,
            -0.5, 0.5, 0.5, -1, 0, 0, 1, 1, 0,
            -0.5, 0.5, -0.5, -1, 0, 0, 1, 1, 0,
            
            // Top
            -0.5, 0.5, 0.5, 0, 1, 0, 1, 0, 1,
            0.5, 0.5, 0.5, 0, 1, 0, 1, 0, 1,
            0.5, 0.5, -0.5, 0, 1, 0, 1, 0, 1,
            -0.5, 0.5, -0.5, 0, 1, 0, 1, 0, 1,
            
            // Bottom
            -0.5, -0.5, -0.5, 0, -1, 0, 0.5, 0.5, 0.5,
            0.5, -0.5, -0.5, 0, -1, 0, 0.5, 0.5, 0.5,
            0.5, -0.5, 0.5, 0, -1, 0, 0.5, 0.5, 0.5,
            -0.5, -0.5, 0.5, 0, -1, 0, 0.5, 0.5, 0.5,
        ]);

        const indices = new Uint32Array([
            0, 1, 2, 0, 2, 3,
            4, 5, 6, 4, 6, 7,
            8, 9, 10, 8, 10, 11,
            12, 13, 14, 12, 14, 15,
            16, 17, 18, 16, 18, 19,
            20, 21, 22, 20, 22, 23,
        ]);

        this.createGeometryBuffers('cube', vertices, indices);
    }

    /**
     * 创建金字塔几何体
     * @returns {void}
     */
    createPyramid() {
        const vertices = new Float32Array([
            // Base
            -0.5, -0.5, -0.5, 0, -1, 0, 0.2, 0.8, 0.2,
            0.5, -0.5, -0.5, 0, -1, 0, 0.2, 0.8, 0.2,
            0.5, -0.5, 0.5, 0, -1, 0, 0.2, 0.8, 0.2,
            -0.5, -0.5, 0.5, 0, -1, 0, 0.2, 0.8, 0.2,
            
            // Apex
            0, 0.5, 0, 0.577, 0.577, 0.577, 0.8, 0.2, 0.2,
        ]);

        const indices = new Uint32Array([
            0, 1, 2,
            0, 2, 3,
            0, 4, 1,
            1, 4, 2,
            2, 4, 3,
            3, 4, 0,
        ]);

        this.createGeometryBuffers('pyramid', vertices, indices);
    }

    /**
     * 创建几何体缓冲区
     * @param {string} name
     * @param {Float32Array} vertices
     * @param {Uint32Array} indices
     */
    createGeometryBuffers(name, vertices, indices) {
        const vertexBuffer = this.gfx.device.createBuffer({
            size: vertices.byteLength,
            mappedAtCreation: true,
            usage: GPUBufferUsage.VERTEX
        });
        new Float32Array(vertexBuffer.getMappedRange()).set(vertices);
        vertexBuffer.unmap();

        const indexBuffer = this.gfx.device.createBuffer({
            size: indices.byteLength,
            mappedAtCreation: true,
            usage: GPUBufferUsage.INDEX
        });
        new Uint32Array(indexBuffer.getMappedRange()).set(indices);
        indexBuffer.unmap();

        this.geometries.set(name, {
            vertexBuffer,
            indexBuffer,
            indexCount: indices.length
        });
    }

    /**
     * 初始化光源
     * @returns {void}
     */
    setupLights() {
        this.lights = [
            new Light([0.5, 0.3, 0.5], [1.0, 0.5, 0.5], 2.0, 1.0),
            new Light([-0.5, 0.3, 0.5], [0.5, 0.5, 1.0], 2.0, 1.0),
            new Light([0.0, -0.2, 0.0], [0.5, 1.0, 0.5], 1.5, 1.0),
        ];

        this.updateLightBuffer();
    }

    /**
     * 更新光源缓冲区
     * @returns {void}
     */
    updateLightBuffer() {
        // 创建光照绑定组
        const bindGroupLayout = this.lightingPipeline.getBindGroupLayout(0);
        this.lightingBindGroup = this.gfx.device.createBindGroup({
            layout: bindGroupLayout,
            entries: [
                { binding: 0, resource: this.gBufferViews.position },
                { binding: 1, resource: this.gBufferViews.normal },
                { binding: 2, resource: this.gBufferViews.albedo },
                { binding: 3, resource: this.gBufferViews.attributes },
                { binding: 4, resource: this.sampler }
            ]
        });
    }

    /**
     * 渲染一帧
     * @returns {void}
     */
    render() {
        const encoder = this.gfx.device.createCommandEncoder();

        // G-Buffer Pass
        {
            const pass = encoder.beginRenderPass({
                colorAttachments: [
                    {
                        view: this.gBufferViews.position,
                        clearValue: [0, 0, 0, 1],
                        loadOp: 'clear',
                        storeOp: 'store'
                    },
                    {
                        view: this.gBufferViews.normal,
                        clearValue: [0.5, 0.5, 1, 1],
                        loadOp: 'clear',
                        storeOp: 'store'
                    },
                    {
                        view: this.gBufferViews.albedo,
                        clearValue: [0, 0, 0, 1],
                        loadOp: 'clear',
                        storeOp: 'store'
                    },
                    {
                        view: this.gBufferViews.attributes,
                        clearValue: [0.5, 0, 0, 1],
                        loadOp: 'clear',
                        storeOp: 'store'
                    }
                ],
                depthStencilAttachment: {
                    view: this.depthView,
                    depthClearValue: 1.0,
                    depthLoadOp: 'clear',
                    depthStoreOp: 'store'
                }
            });

            pass.setPipeline(this.gBufferPipeline);

            // 绘制多个几何体
            let posX = -0.5;
            for (const [name, geo] of this.geometries) {
                pass.setVertexBuffer(0, geo.vertexBuffer);
                pass.setIndexBuffer(geo.indexBuffer, 'uint32');
                pass.drawIndexed(geo.indexCount);
                posX += 0.5;
            }

            pass.end();
        }

        // Lighting Pass
        {
            const texture = this.gfx.context.getCurrentTexture();
            const pass = encoder.beginRenderPass({
                colorAttachments: [{
                    view: texture.createView(),
                    clearValue: [0.0, 0.0, 0.0, 1],
                    loadOp: 'clear',
                    storeOp: 'store'
                }]
            });

            pass.setPipeline(this.lightingPipeline);
            pass.setBindGroup(0, this.lightingBindGroup);
            pass.draw(6);
            pass.end();
        }

        this.gfx.device.queue.submit([encoder.finish()]);
    }
}



/**
 * 主程序入口
 * @returns {Promise<void>}
 */
async function main() {
    try {
        console.log('🚀 启动WebGPU延迟渲染应用...');

        const canvas = document.getElementById('canvas');
        const gfxContext = new TknGfxContext(canvas);
        await gfxContext.create();
        console.log('✓ GPU上下文已创建');

        // 初始化渲染器
        const renderer = new DeferredRenderer(gfxContext);
        await renderer.init();

        // 渲染循环
        let frameCount = 0;
        let lastTime = performance.now();

        /**
         * 帧更新函数
         * @returns {void}
         */
        function frame() {
            renderer.render();

            frameCount++;
            const now = performance.now();
            if (now - lastTime >= 1000) {
                document.getElementById('fps').textContent = `FPS: ${frameCount}`;
                frameCount = 0;
                lastTime = now;
            }

            requestAnimationFrame(frame);
        }

        frame();
    } catch (error) {
        console.error('❌ 错误:', error);
        const el = document.getElementById('error');
        el.textContent = `错误: ${error.message}`;
        el.style.display = 'block';
    }
}

/**
 * DOM加载完成时启动
 * @param {Event} event
 * @returns {void}
 */
window.addEventListener('DOMContentLoaded', main);

