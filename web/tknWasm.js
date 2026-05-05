/**
 * WASM 导入对象和加载器
 */

export async function tknWasm() {
    // 创建内存 (256 pages = 16MB)
    const wasmMemory = new WebAssembly.Memory({ initial: 256, maximum: 512 });

    // 创建导入对象
    const importObject = createImportObject(wasmMemory);

    // 加载 WASM 二进制文件
    console.log('Fetching WASM from: /build/TickernelWeb.wasm');
    const response = await fetch('/build/TickernelWeb.wasm');
    if (!response.ok) {
        throw new Error(`Failed to load WASM: ${response.status}`);
    }

    const buffer = await response.arrayBuffer();
    console.log(`WASM file size: ${buffer.byteLength} bytes`);

    // 实例化 WASM 模块
    console.log('Instantiating WASM module...');
    const wasmModuleObj = await WebAssembly.instantiate(buffer, importObject);
    const wasmModule = wasmModuleObj.module;
    const wasmExports = wasmModuleObj.pInstance.exports;

    console.log('WASM exports:', Object.keys(wasmExports));

    return {
        wasmMemory,
        wasmModule,
        wasmExports,
        importObject
    };
}

// 解析 WGSL 代码提取 binding 信息
function parseWgslBindings(shaderCode) {
    const bindings = {};

    // 提取所有 @group 和 @binding 的声明
    // 例如: @group(0) @binding(0) var<uniform> myUniform: MyStruct;
    const bindingRegex = /@group\((\d+)\)\s+@binding\((\d+)\)\s+var(?:<(\w+)>)?\s+(\w+)\s*:\s*([^;]+);/g;

    let match;
    while ((match = bindingRegex.exec(shaderCode)) !== null) {
        const group = parseInt(match[1]);
        const binding = parseInt(match[2]);
        const storageType = match[3] || 'read';
        const varName = match[4];
        const varType = match[5].trim();

        const key = `${group}_${binding}`;
        bindings[key] = {
            group,
            binding,
            name: varName,
            type: varType,
            storageType
        };
    }

    return bindings;
}

function createImportObject(wasmMemory) {
    let heapPtr = 1024;
    const allocations = new Map();

    const importObject = {
        env: {
            memory: wasmMemory,

            // ===== 内存管理 =====
            tknMalloc: (size) => {
                // 对齐到 4 字节边界
                const alignment = 4;
                const mask = alignment - 1;
                heapPtr = (heapPtr + mask) & ~mask;

                const ptr = heapPtr;
                heapPtr += size;
                allocations.set(ptr, size);
                console.log(`[tknMalloc] ptr=${ptr}, size=${size}`);
                return ptr;
            },

            tknFree: (ptr) => {
                if (allocations.has(ptr)) {
                    const size = allocations.get(ptr);
                    allocations.delete(ptr);
                    console.log(`[tknFree] ptr=${ptr}`);
                }
            },

            // ===== 内存操作 =====
            tknMemcmp: (s1, s2, n) => {
                const bytes1 = new Uint8Array(wasmMemory.buffer, s1, n);
                const bytes2 = new Uint8Array(wasmMemory.buffer, s2, n);
                for (let i = 0; i < n; i++) {
                    if (bytes1[i] !== bytes2[i]) {
                        return bytes1[i] - bytes2[i];
                    }
                }
                return 0;
            },

            tknMemcpy: (dest, src, n) => {
                const destBytes = new Uint8Array(wasmMemory.buffer, dest, n);
                const srcBytes = new Uint8Array(wasmMemory.buffer, src, n);
                destBytes.set(srcBytes);
                return dest;
            },

            tknMemmove: (dest, src, n) => {
                const destBytes = new Uint8Array(wasmMemory.buffer, dest, n);
                const srcBytes = new Uint8Array(wasmMemory.buffer, src, n);
                destBytes.set(srcBytes);
                return dest;
            },

            tknMemset: (ptr, value, n) => {
                const bytes = new Uint8Array(wasmMemory.buffer, ptr, n);
                bytes.fill(value);
                return ptr;
            },

            // ===== 日志和控制 =====
            tknLog: (ptr) => {
                const bytes = new Uint8Array(wasmMemory.buffer, ptr);
                let str = '';
                for (let i = 0; bytes[i] !== 0; i++) {
                    str += String.fromCharCode(bytes[i]);
                }
                console.log('[C]', str);
            },

            tknAbort: () => {
                console.error('[C] tknAbort called');
                throw new Error('C code called tknAbort()');
            },

            tknCreateGfxContextPtr: (
                pInstance,
                pSurface,
                width,
                height,
                globalShaderPathCount,
                globalShaderPathsPtr
            ) => {
                // 创建一个空的实例对象，稍后会被填充
                const gfxContext = {};

                // 异步初始化 WebGPU，返回 Promise
                gfxContext.initPromise = (async () => {
                    try {
                        if (!navigator.gpu) {
                            throw new Error('WebGPU not supported');
                        }

                        let device, context, format;
                        
                        // 检查是否已有全局的 WebGPU 资源（由 app.js 提供）
                        if (window.gpuDevice && window.gpuContext) {
                            console.log('[tknCreateGfxContextPtr] Using existing WebGPU resources from app.js');
                            device = window.gpuDevice;
                            context = window.gpuContext;
                            format = window.gpuFormat || navigator.gpu.getPreferredCanvasFormat();
                        } else {
                            // 否则创建新资源
                            console.log('[tknCreateGfxContextPtr] Creating new WebGPU resources');
                            const adapter = await navigator.gpu.requestAdapter();
                            device = await adapter.requestDevice();

                            const canvas = document.getElementById('canvas');
                            context = canvas.getContext('webgpu');
                            if (!context) {
                                throw new Error('Failed to get WebGPU context from canvas');
                            }
                            format = navigator.gpu.getPreferredCanvasFormat();

                            context.configure({
                                device,
                                format,
                                alphaMode: 'opaque'
                            });
                        }

                        console.log(`[tknCreateGfxContextPtr] Canvas: ${width}x${height}`);
                        console.log('[tknCreateGfxContextPtr] Canvas format:', format);

                        // 读取 WGSL 文件路径
                        let shaderCode = null;
                        let bindings = {};

                        if (globalShaderPathCount > 0 && globalShaderPathsPtr) {
                            const pathPtrs = new Uint32Array(wasmMemory.buffer, globalShaderPathsPtr, globalShaderPathCount);
                            const shaderPaths = [];

                            // 从 WASM 内存读取路径字符串
                            for (let i = 0; i < globalShaderPathCount; i++) {
                                const ptr = pathPtrs[i];
                                if (ptr) {
                                    const bytes = new Uint8Array(wasmMemory.buffer, ptr, 256);
                                    let path = '';
                                    for (let j = 0; j < bytes.length && bytes[j] !== 0; j++) {
                                        path += String.fromCharCode(bytes[j]);
                                    }
                                    shaderPaths.push(path);
                                }
                            }

                            // 读取所有 WGSL 文件
                            const shaderCodes = [];
                            for (const path of shaderPaths) {
                                try {
                                    const response = await fetch(path);
                                    if (!response.ok) {
                                        console.warn(`Failed to load shader: ${path}`);
                                        continue;
                                    }
                                    const code = await response.text();
                                    shaderCodes.push(code);
                                } catch (e) {
                                    console.warn(`Error loading shader ${path}:`, e);
                                }
                            }

                            // 合并所有着色器代码
                            shaderCode = shaderCodes.join('\n\n');

                            // 解析 binding 信息
                            bindings = parseWgslBindings(shaderCode);
                            console.log('Extracted bindings:', bindings);
                        }

                        // 如果没有指定着色器，使用默认着色器

                        shaderCode = `
                                @vertex
                                fn vertexMain(@builtin(vertex_index) idx : u32) -> @builtin(position) vec4f {
                                    let pos = array<vec2f, 3>(
                                        vec2f(0.0, 0.5),
                                        vec2f(-0.5, -0.5),
                                        vec2f(0.5, -0.5)
                                    );
                                    return vec4f(pos[idx], 0.0, 1.0);
                                }

                                @fragment
                                fn fragmentMain() -> @location(0) vec4f {
                                    return vec4f(0.2, 0.8, 0.4, 1.0);
                                }
                            `;


                        // 创建 shader module 和 pipeline
                        const shaderModule = device.createShaderModule({ code: shaderCode });

                        // 创建 BindGroup layouts
                        const bindGroupLayouts = {};
                        const groupCounts = {};
                        for (const key of Object.keys(bindings)) {
                            const binding = bindings[key];
                            const group = binding.group;
                            if (!groupCounts[group]) {
                                groupCounts[group] = 0;
                            }
                            groupCounts[group]++;
                        }

                        // 创建每个 group 的 layout entries
                        for (const group of Object.keys(groupCounts).map(Number)) {
                            const entries = [];
                            for (const key of Object.keys(bindings)) {
                                const binding = bindings[key];
                                if (binding.group === group) {
                                    // 根据类型推断 binding resource 类型
                                    let bindingType = { buffer: { type: 'uniform' } };
                                    if (binding.type.includes('texture')) {
                                        bindingType = { texture: {} };
                                    } else if (binding.type.includes('sampler')) {
                                        bindingType = { sampler: {} };
                                    }

                                    entries.push({
                                        binding: binding.binding,
                                        visibility: GPUShaderStage.VERTEX | GPUShaderStage.FRAGMENT,
                                        ...bindingType
                                    });
                                }
                            }

                            if (entries.length > 0) {
                                bindGroupLayouts[group] = device.createBindGroupLayout({
                                    entries
                                });
                            }
                        }

                        // 创建 pipeline layout
                        const layoutEntries = Object.entries(bindGroupLayouts).map(([group, layout]) => ({
                            index: parseInt(group),
                            layout
                        }));
                        layoutEntries.sort((a, b) => a.index - b.index);

                        let pipelineLayout = null;
                        if (layoutEntries.length > 0) {
                            pipelineLayout = device.createPipelineLayout({
                                bindGroupLayouts: layoutEntries.map(e => e.layout)
                            });
                        }

                        const pipeline = await device.createRenderPipeline({
                            layout: pipelineLayout || 'auto',
                            vertex: {
                                module: shaderModule,
                                entryPoint: 'vertexMain',
                                buffers: []
                            },
                            fragment: {
                                module: shaderModule,
                                entryPoint: 'fragmentMain',
                                targets: [{ format }]
                            },
                            primitive: {
                                topology: 'triangle-list'
                            }
                        });

                        // 初始化完成，填充实例对象
                        gfxContext.device = device;
                        gfxContext.context = context;
                        gfxContext.pipeline = pipeline;

                        console.log('[tknCreateGfxContextPtr] WebGPU initialized');
                        document.getElementById('status').textContent = 'WebGPU Ready';
                        return gfxContext;  // 初始化完成，返回实例
                    } catch (error) {
                        console.error('[tknCreateGfxContextPtr]', error);
                        const el = document.getElementById('error');
                        if (el) {
                            el.textContent = `Error: ${error.message}`;
                            el.style.display = 'block';
                        }
                        throw error;  // 抛出错误以便 Promise reject
                    }
                })();

                console.log(`[tknCreateGfxContextPtr] Created context (${width}x${height})`);
                return gfxContext;
            },

            // tknRenderFrame 接收 gfxContext 作为参数
            tknRenderFrame: (gfxContext) => {
                if (!gfxContext || !gfxContext.device || !gfxContext.pipeline) {
                    console.warn('[tknRenderFrame] Context not initialized');
                    return;
                }

                // 从保存的 context 获取 texture
                const context = gfxContext.context;
                if (!context) {
                    console.error('[tknRenderFrame] Context is null');
                    return;
                }

                const canvas = document.getElementById('canvas');
                const format = navigator.gpu.getPreferredCanvasFormat();
                
                // 配置 context
                context.configure({
                    device: gfxContext.device,
                    format,
                    alphaMode: 'opaque'
                });

                const commandEncoder = gfxContext.device.createCommandEncoder();
                const texture = context.getCurrentTexture();

                const renderPass = commandEncoder.beginRenderPass({
                    colorAttachments: [{
                        view: texture.createView(),
                        loadOp: 'clear',
                        clearValue: [0.0, 0.0, 0.0, 1.0],  // 黑色背景
                        storeOp: 'store'
                    }]
                });

                renderPass.setPipeline(gfxContext.pipeline);
                renderPass.draw(3, 1, 0, 0);
                renderPass.end();

                gfxContext.device.queue.submit([commandEncoder.finish()]);
            }
        }
    };

    return importObject;
}
