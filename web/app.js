/**
 * @typedef {Object} TknGfxContext
 * @property {HTMLCanvasElement} canvas
 * @property {GPUDevice | null} device
 * @property {GPUCanvasContext | null} context
 * @property {GPUTextureFormat | null} format
 */
const GPUTextureUsage = globalThis.GPUTextureUsage;

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
     * 从图像加载纹理到GPU
     * @see copyExternalImageToTexture https://gpuweb.github.io/gpuweb/#dom-gpuqueue-copyexternalimagetotexture
     * @param {Image|Canvas|OffscreenCanvas|ImageBitmap} imageSource - 图像源
     * @param {number} width - 纹理宽度
     * @param {number} height - 纹理高度
     * @returns {GPUTexture}
     * 
     * @example
     * // 用法示例
     * const img = new Image();
     * img.src = 'texture.png';
     * img.onload = () => {
     *     const texture = gfxContext.loadImageAsTexture(img, img.width, img.height);
     * };
     * 
     * // copyExternalImageToTexture 内部实现:
     * // device.queue.copyExternalImageToTexture(
     * //     { source: imageBitmap },
     * //     { texture },
     * //     { width, height }
     * // );
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
class DeferredRenderer {
    /**
     * @param {TknGfxContext} gfxContext
     */
    constructor(gfxContext) {
        /** @type {TknGfxContext} */
        this.gfx = gfxContext;
        /** @type {GPUBuffer | null} */
        this.triangleBuffer = null;
        /** @type {GPURenderPipeline | null} */
        this.gBufferPipeline = null;
        /** @type {GPURenderPipeline | null} */
        this.lightingPipeline = null;
        /** @type {Object<string, GPUTexture>} */
        this.gBuffer = {};
        /** @type {Object<string, GPUTextureView>} */
        this.gBufferViews = {};
        /** @type {GPUTextureView | null} */
        this.depthView = null;
        /** @type {GPUBindGroup | null} */
        this.lightingBindGroup = null;
    }

    /**
     * @returns {Promise<void>}
     */
    async init() {
        this.setupGBuffer();
        this.setupPipelines();
        this.createGeometry();

        console.log('✓ DeferredRenderer initialized');
        document.getElementById('status').textContent = 'Ready';
    }

    /**
     * @returns {void}
     */
    setupGBuffer() {
        const size = { width: this.gfx.canvas.width, height: this.gfx.canvas.height };

        // Albedo (RGBA8)
        this.gBuffer.albedo = this.gfx.device.createTexture({
            size,
            format: 'rgba8unorm',
            usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING
        });
        this.gBufferViews.albedo = this.gBuffer.albedo.createView();

        // Normal (RGBA8)
        this.gBuffer.normal = this.gfx.device.createTexture({
            size,
            format: 'rgba8unorm',
            usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING
        });
        this.gBufferViews.normal = this.gBuffer.normal.createView();

        // Depth
        const depthTexture = this.gfx.device.createTexture({
            size,
            format: 'depth32float',
            usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.TEXTURE_BINDING
        });
        this.depthView = depthTexture.createView();
    }

    /**
     * @returns {void}
     */
    setupPipelines() {
        // G-Buffer Pipeline
        const gBufferShader = this.gfx.device.createShaderModule({
            code: `
                struct VertexInput {
                    @location(0) position: vec3f,
                    @location(1) normal: vec3f,
                    @location(2) color: vec3f,
                }

                struct VertexOutput {
                    @builtin(position) position: vec4f,
                    @location(0) normal: vec3f,
                    @location(1) color: vec3f,
                }

                @vertex
                fn vs_main(input: VertexInput) -> VertexOutput {
                    var output: VertexOutput;
                    output.position = vec4f(input.position, 1.0);
                    output.normal = input.normal;
                    output.color = input.color;
                    return output;
                }

                struct FragmentOutput {
                    @location(0) albedo: vec4f,
                    @location(1) normal: vec4f,
                }

                @fragment
                fn fs_main(input: VertexOutput) -> FragmentOutput {
                    var output: FragmentOutput;
                    output.albedo = vec4f(input.color, 1.0);
                    output.normal = vec4f(normalize(input.normal) * 0.5 + 0.5, 1.0);
                    return output;
                }
            `
        });

        let layout = this.gfx.device.createPipelineLayout({
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
                    { format: 'rgba8unorm' },
                    { format: 'rgba8unorm' }
                ]
            },
            primitive: { topology: 'triangle-list' },
            depthStencil: {
                format: 'depth32float',
                depthWriteEnabled: true,
                depthCompare: 'less'
            }
        });

        // Lighting Pipeline
        const lightingShader = this.gfx.device.createShaderModule({
            code: `
                @group(0) @binding(0) var gAlbedo: texture_2d<f32>;
                @group(0) @binding(1) var gNormal: texture_2d<f32>;
                @group(0) @binding(2) var texSampler: sampler;

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
                    let uv = fragCoord.xy / vec2f(textureDimensions(gAlbedo));
                    let albedo = textureSample(gAlbedo, texSampler, uv);
                    let normal = textureSample(gNormal, texSampler, uv).rgb * 2.0 - 1.0;
                    
                    let lightDir = normalize(vec3f(1.0, 1.0, 1.0));
                    let diffuse = max(dot(normal, lightDir), 0.0);
                    let ambient = 0.2;
                    
                    return vec4f(albedo.rgb * (ambient + diffuse), 1.0);
                }
            `
        });

        let samplerLayout = this.gfx.device.createBindGroupLayout({
            entries: [
                { binding: 0, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float' } },
                { binding: 1, visibility: GPUShaderStage.FRAGMENT, texture: { sampleType: 'float' } },
                { binding: 2, visibility: GPUShaderStage.FRAGMENT, sampler: {} }
            ]
        });

        let lightingLayout = this.gfx.device.createPipelineLayout({
            bindGroupLayouts: [samplerLayout]
        });

        this.lightingPipeline = this.gfx.device.createRenderPipeline({
            layout: lightingLayout,
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

        const sampler = this.gfx.device.createSampler();
        this.lightingBindGroup = this.gfx.device.createBindGroup({
            layout: samplerLayout,
            entries: [
                { binding: 0, resource: this.gBufferViews.albedo },
                { binding: 1, resource: this.gBufferViews.normal },
                { binding: 2, resource: sampler }
            ]
        });
    }

    /**
     * @returns {void}
     */
    createGeometry() {
        const vertices = new Float32Array([
            // Position (3) + Normal (3) + Color (3)
            -0.5, -0.5, 0, 0, 0, 1, 1, 0.2, 0.2,
            0.5, -0.5, 0, 0, 0, 1, 0.2, 1, 0.2,
            0.0, 0.5, 0, 0, 0, 1, 0.2, 0.2, 1,
        ]);

        this.triangleBuffer = this.gfx.device.createBuffer({
            size: vertices.byteLength,
            mappedAtCreation: true,
            usage: GPUBufferUsage.VERTEX
        });
        new Float32Array(this.triangleBuffer.getMappedRange()).set(vertices);
        this.triangleBuffer.unmap();
    }

    /**
     * @returns {void}
     */
    render() {
        const encoder = this.gfx.device.createCommandEncoder();

        // G-Buffer Pass
        {
            const pass = encoder.beginRenderPass({
                colorAttachments: [
                    {
                        view: this.gBufferViews.albedo,
                        clearValue: [0, 0, 0, 1],
                        loadOp: 'clear',
                        storeOp: 'store'
                    },
                    {
                        view: this.gBufferViews.normal,
                        clearValue: [0.5, 0.5, 1, 1],
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
            pass.setVertexBuffer(0, this.triangleBuffer);
            pass.draw(3);
            pass.end();
        }

        // Lighting Pass
        {
            const texture = this.gfx.context.getCurrentTexture();
            const pass = encoder.beginRenderPass({
                colorAttachments: [{
                    view: texture.createView(),
                    clearValue: [0.1, 0.1, 0.1, 1],
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
 * @returns {Promise<void>}
 */
async function main() {
    try {
        console.log('🚀 Starting...');

        /** @type {HTMLCanvasElement} */
        const gfxContext = new TknGfxContext(canvas);
        await gfxContext.init();

        // Initialize renderer
        const renderer = new DeferredRenderer(gfxContext);
        await renderer.init();

        // Render loop
        let frameCount = 0;
        let lastTime = performance.now();

        /**
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
        console.error('❌ Error:', error);
        /** @type {HTMLElement} */
        const el = document.getElementById('error');
        el.textContent = `Error: ${error.message}`;
        el.style.display = 'block';
    }
}

/**
 * @param {Event} event
 * @returns {void}
 */
window.addEventListener('DOMContentLoaded', main);
