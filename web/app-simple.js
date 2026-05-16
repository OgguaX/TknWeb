const GPUTextureUsage = globalThis.GPUTextureUsage;
const GPUShaderStage = globalThis.GPUShaderStage;
const GPUBufferUsage = globalThis.GPUBufferUsage;

class TknGfxContext {
    constructor(canvas) {
        this.canvas = canvas;
        this.device = null;
        this.context = null;
        this.format = null;
    }

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
}

// ============================================================================
// 延迟渲染器 - WebGPU实现（简化版）
// ============================================================================

class DeferredRenderer {
    constructor(gfxContext) {
        this.gfx = gfxContext;
        this.geometries = new Map();
        
        // G-Buffer
        this.gBufferPosition = null;
        this.gBufferNormal = null;
        this.gBufferAlbedo = null;
        this.gBufferDepth = null;
        
        this.gBufferPositionView = null;
        this.gBufferNormalView = null;
        this.gBufferAlbedoView = null;
        this.gBufferDepthView = null;
        
        // 管线
        this.gBufferPipeline = null;
        this.lightingPipeline = null;
        this.lightingBindGroup = null;
        this.sampler = null;
        
        // Swapchain 管理
        this.canvasWidth = 0;
        this.canvasHeight = 0;
        this.resizeObserver = null;
    }

    async init() {
        console.log('🔧 初始化延迟渲染器...');
        this.createGBuffer();
        this.createSampler();
        this.createGBufferPipeline();
        this.createLightingPipeline();
        this.createGeometry();
        this.setupResizeObserver();
        console.log('✓ 延迟渲染器已初始化');
        document.getElementById('status').textContent = '就绪 (延迟管线)';
    }

    setupResizeObserver() {
        this.canvasWidth = this.gfx.canvas.width;
        this.canvasHeight = this.gfx.canvas.height;
        
        // 监听canvas尺寸变化
        this.resizeObserver = new ResizeObserver(() => {
            const newWidth = this.gfx.canvas.clientWidth;
            const newHeight = this.gfx.canvas.clientHeight;
            
            if (newWidth !== this.canvasWidth || newHeight !== this.canvasHeight) {
                this.canvasWidth = newWidth;
                this.canvasHeight = newHeight;
                
                // 重新配置canvas
                this.gfx.canvas.width = newWidth;
                this.gfx.canvas.height = newHeight;
                this.gfx.context.configure({
                    device: this.gfx.device,
                    format: this.gfx.format,
                    alphaMode: 'opaque'
                });
                
                console.log(`📐 Canvas大小已改变: ${newWidth}x${newHeight}, 重建G-Buffer`);
                
                // 重建G-Buffer（尺寸改变时需要）
                this.createGBuffer();
                this.createLightingBindGroup();
            }
        });
        
        this.resizeObserver.observe(this.gfx.canvas);
    }

    createGBuffer() {
        const width = this.gfx.canvas.width;
        const height = this.gfx.canvas.height;
        
        // Position - 改用RGBA16Float (8字节，可采样)
        this.gBufferPosition = this.gfx.device.createTexture({
            size: [width, height],
            format: 'rgba16float',
            usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING,
        });
        this.gBufferPositionView = this.gBufferPosition.createView();
        
        // Normal - 改用RGBA16Float (8字节，可采样)
        this.gBufferNormal = this.gfx.device.createTexture({
            size: [width, height],
            format: 'rgba16float',
            usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING,
        });
        this.gBufferNormalView = this.gBufferNormal.createView();
        
        // Albedo - 保持RGBA8 (4字节)
        this.gBufferAlbedo = this.gfx.device.createTexture({
            size: [width, height],
            format: 'rgba8unorm',
            usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING,
        });
        this.gBufferAlbedoView = this.gBufferAlbedo.createView();
        
        // Depth
        this.gBufferDepth = this.gfx.device.createTexture({
            size: [width, height],
            format: 'depth24plus',
            usage: GPUTextureUsage.RENDER_ATTACHMENT,
        });
        this.gBufferDepthView = this.gBufferDepth.createView();
        
        console.log(`✓ G-Buffer已创建 (${width}x${height})`);
    }

    createSampler() {
        this.sampler = this.gfx.device.createSampler({
            magFilter: 'linear',
            minFilter: 'linear',
        });
    }

    createGBufferPipeline() {
        const shaderCode = `
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
            }
            
            @fragment
            fn fs_main(input: VertexOutput) -> FragmentOutput {
                var output: FragmentOutput;
                output.position = vec4f(input.worldPos, 1.0);
                output.normal = vec4f(normalize(input.normal), 1.0);
                output.albedo = vec4f(input.color, 1.0);
                return output;
            }
        `;

        const shaderModule = this.gfx.device.createShaderModule({ code: shaderCode });

        const pipelineLayout = this.gfx.device.createPipelineLayout({
            bindGroupLayouts: []
        });

        this.gBufferPipeline = this.gfx.device.createRenderPipeline({
            layout: pipelineLayout,
            vertex: {
                module: shaderModule,
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
                module: shaderModule,
                entryPoint: 'fs_main',
                targets: [
                    { format: 'rgba16float' },  // position
                    { format: 'rgba16float' },  // normal
                    { format: 'rgba8unorm' }    // albedo
                ]
            },
            primitive: {
                topology: 'triangle-list',
                frontFace: 'ccw',
                cullMode: 'none'
            },
            depthStencil: {
                format: 'depth24plus',
                depthWriteEnabled: true,
                depthCompare: 'less'
            }
        });

        console.log('✓ G-Buffer管线已创建');
    }

    createLightingPipeline() {
        const shaderCode = `
            @group(0) @binding(0) var gPosition: texture_2d<f32>;
            @group(0) @binding(1) var gNormal: texture_2d<f32>;
            @group(0) @binding(2) var gAlbedo: texture_2d<f32>;
            @group(0) @binding(3) var texSampler: sampler;
            
            struct VertexOutput {
                @builtin(position) position: vec4f,
                @location(0) uv: vec2f,
            }
            
            @vertex
            fn vs_main(@builtin(vertex_index) idx: u32) -> VertexOutput {
                var output: VertexOutput;
                
                // 全屏四边形
                let uv = vec2f(f32(idx & 1u), f32((idx >> 1u) & 1u));
                output.position = vec4f(uv * 2.0 - 1.0, 0.0, 1.0);
                output.uv = uv;
                
                return output;
            }
            
            @fragment
            fn fs_main(input: VertexOutput) -> @location(0) vec4f {
                let uv = input.uv;
                
                // 从G-Buffer读取数据
                let posData = textureSample(gPosition, texSampler, uv);
                let normalData = textureSample(gNormal, texSampler, uv);
                let albedoData = textureSample(gAlbedo, texSampler, uv);
                
                let worldPos = posData.xyz;
                let normal = normalize(normalData.xyz);
                let albedo = albedoData.xyz;
                
                // 简单多光源光照计算
                var finalColor = vec3f(0.2);  // 环境光
                
                // 红光 - 右上
                let light1Pos = vec3f(2.0, 1.0, 0.0);
                let light1Dir = normalize(light1Pos - worldPos);
                let light1Dist = length(light1Pos - worldPos);
                let light1Att = max(0.0, 1.0 - light1Dist / 10.0);
                let light1Diff = max(dot(normal, light1Dir), 0.0) * 1.5;
                finalColor += vec3f(1.0, 0.0, 0.0) * light1Diff * light1Att;
                
                // 绿光 - 左上
                let light2Pos = vec3f(-2.0, 1.0, 0.0);
                let light2Dir = normalize(light2Pos - worldPos);
                let light2Dist = length(light2Pos - worldPos);
                let light2Att = max(0.0, 1.0 - light2Dist / 10.0);
                let light2Diff = max(dot(normal, light2Dir), 0.0) * 1.5;
                finalColor += vec3f(0.0, 1.0, 0.0) * light2Diff * light2Att;
                
                // 蓝光 - 前方
                let light3Pos = vec3f(0.0, 1.0, 2.0);
                let light3Dir = normalize(light3Pos - worldPos);
                let light3Dist = length(light3Pos - worldPos);
                let light3Att = max(0.0, 1.0 - light3Dist / 10.0);
                let light3Diff = max(dot(normal, light3Dir), 0.0) * 1.5;
                finalColor += vec3f(0.0, 0.0, 1.0) * light3Diff * light3Att;
                
                return vec4f(albedo * finalColor, 1.0);
            }
        `;
        
        const shaderModule = this.gfx.device.createShaderModule({ code: shaderCode });
        
        const bindGroupLayout = this.gfx.device.createBindGroupLayout({
            entries: [
                { binding: 0, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float' } },
                { binding: 1, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float' } },
                { binding: 2, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float' } },
                { binding: 3, visibility: GPUShaderStage.FRAGMENT, sampler: { type: 'filtering' } }
            ]
        });
        
        const pipelineLayout = this.gfx.device.createPipelineLayout({
            bindGroupLayouts: [bindGroupLayout]
        });
        
        this.lightingPipeline = this.gfx.device.createRenderPipeline({
            layout: pipelineLayout,
            vertex: {
                module: shaderModule,
                entryPoint: 'vs_main'
            },
            fragment: {
                module: shaderModule,
                entryPoint: 'fs_main',
                targets: [{ format: this.gfx.format }]
            },
            primitive: {
                topology: 'triangle-list',
                frontFace: 'ccw'
            }
        });
        
        this.createLightingBindGroup();
        
        console.log('✓ 光照管线已创建');
    }

    createLightingBindGroup() {
        const bindGroupLayout = this.lightingPipeline.getBindGroupLayout(0);
        
        // 创建bind group
        this.lightingBindGroup = this.gfx.device.createBindGroup({
            layout: bindGroupLayout,
            entries: [
                { binding: 0, resource: this.gBufferPositionView },
                { binding: 1, resource: this.gBufferNormalView },
                { binding: 2, resource: this.gBufferAlbedoView },
                { binding: 3, resource: this.sampler }
            ]
        });
    }

    createGeometry() {
        const vertices = new Float32Array([
            // Front face (红)
            -0.3, -0.3, 0.3,  0, 0, 1,  1, 0, 0,
             0.3, -0.3, 0.3,  0, 0, 1,  1, 0, 0,
             0.3,  0.3, 0.3,  0, 0, 1,  1, 0, 0,
            -0.3,  0.3, 0.3,  0, 0, 1,  1, 0, 0,

            // Right face (绿)
             0.3, -0.3, 0.3,  1, 0, 0,  0, 1, 0,
             0.3, -0.3, -0.3, 1, 0, 0,  0, 1, 0,
             0.3,  0.3, -0.3, 1, 0, 0,  0, 1, 0,
             0.3,  0.3, 0.3,  1, 0, 0,  0, 1, 0,

            // Back face (蓝)
             0.3, -0.3, -0.3, 0, 0, -1, 0, 0, 1,
            -0.3, -0.3, -0.3, 0, 0, -1, 0, 0, 1,
            -0.3,  0.3, -0.3, 0, 0, -1, 0, 0, 1,
             0.3,  0.3, -0.3, 0, 0, -1, 0, 0, 1,

            // Left face (黄)
            -0.3, -0.3, -0.3, -1, 0, 0, 1, 1, 0,
            -0.3, -0.3,  0.3, -1, 0, 0, 1, 1, 0,
            -0.3,  0.3,  0.3, -1, 0, 0, 1, 1, 0,
            -0.3,  0.3, -0.3, -1, 0, 0, 1, 1, 0,

            // Top face (品红)
            -0.3,  0.3,  0.3, 0, 1, 0, 1, 0, 1,
             0.3,  0.3,  0.3, 0, 1, 0, 1, 0, 1,
             0.3,  0.3, -0.3, 0, 1, 0, 1, 0, 1,
            -0.3,  0.3, -0.3, 0, 1, 0, 1, 0, 1,

            // Bottom face (青)
            -0.3, -0.3, -0.3, 0, -1, 0, 0, 1, 1,
             0.3, -0.3, -0.3, 0, -1, 0, 0, 1, 1,
             0.3, -0.3,  0.3, 0, -1, 0, 0, 1, 1,
            -0.3, -0.3,  0.3, 0, -1, 0, 0, 1, 1,
        ]);

        const indices = new Uint32Array([
            0,1,2, 0,2,3,
            4,5,6, 4,6,7,
            8,9,10, 8,10,11,
            12,13,14, 12,14,15,
            16,17,18, 16,18,19,
            20,21,22, 20,22,23,
        ]);

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

        this.geometries.set('cube', {
            vertexBuffer,
            indexBuffer,
            indexCount: indices.length
        });
    }

    render() {
        // 获取本帧的swapchain纹理和view（必须每帧获取）
        const screenTexture = this.gfx.context.getCurrentTexture();
        const screenView = screenTexture.createView();
        
        const encoder = this.gfx.device.createCommandEncoder();

        // ============================================================
        // Pass 1: G-Buffer
        // ============================================================
        const gBufferPass = encoder.beginRenderPass({
            colorAttachments: [
                {
                    view: this.gBufferPositionView,
                    clearValue: [0, 0, 0, 1],
                    loadOp: 'clear',
                    storeOp: 'store'
                },
                {
                    view: this.gBufferNormalView,
                    clearValue: [0, 0, 0, 1],
                    loadOp: 'clear',
                    storeOp: 'store'
                },
                {
                    view: this.gBufferAlbedoView,
                    clearValue: [0, 0, 0, 1],
                    loadOp: 'clear',
                    storeOp: 'store'
                }
            ],
            depthStencilAttachment: {
                view: this.gBufferDepthView,
                depthClearValue: 1.0,
                depthLoadOp: 'clear',
                depthStoreOp: 'store'
            }
        });

        gBufferPass.setPipeline(this.gBufferPipeline);
        const geo = this.geometries.get('cube');
        gBufferPass.setVertexBuffer(0, geo.vertexBuffer);
        gBufferPass.setIndexBuffer(geo.indexBuffer, 'uint32');
        gBufferPass.drawIndexed(geo.indexCount);
        gBufferPass.end();

        // ============================================================
        // Pass 2: 光照合成
        // ============================================================
        const lightingPass = encoder.beginRenderPass({
            colorAttachments: [{
                view: screenView,
                clearValue: [0, 0, 0, 1],
                loadOp: 'clear',
                storeOp: 'store'
            }]
        });

        lightingPass.setPipeline(this.lightingPipeline);
        lightingPass.setBindGroup(0, this.lightingBindGroup);
        lightingPass.draw(6);  // 全屏四边形（2个三角形）
        lightingPass.end();

        this.gfx.device.queue.submit([encoder.finish()]);
    }

    destroy() {
        if (this.resizeObserver) {
            this.resizeObserver.disconnect();
        }
    }
}

async function main() {
    try {
        console.log('🚀 启动延迟渲染引擎...\n');

        const canvas = document.getElementById('canvas');
        const gfxContext = new TknGfxContext(canvas);
        await gfxContext.create();
        console.log('✓ GPU上下文已创建\n');

        const renderer = new DeferredRenderer(gfxContext);
        await renderer.init();

        let frameCount = 0;
        let lastTime = performance.now();

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
        console.log('\n✓ 渲染循环已启动');
    } catch (error) {
        console.error('❌ 错误:', error);
        const el = document.getElementById('error');
        el.textContent = `错误: ${error.message}`;
        el.style.display = 'block';
    }
}

window.addEventListener('DOMContentLoaded', main);
