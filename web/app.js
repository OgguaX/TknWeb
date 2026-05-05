/**
 * Tickernel Web - WebGPU Application with WASM
 */

let fpsCounter = null;
let wasmExports = null;
let wasmImportObject = null;
let gfxContext = null;

async function initApp() {
    console.log('[app.js] Starting WebGPU + WASM');

    if (!navigator.gpu) {
        console.error('WebGPU not supported');
        return;
    }

    try {
        const canvas = document.getElementById('canvas');
        const dpr = window.devicePixelRatio || 1;
        
        canvas.width = window.innerWidth * dpr;
        canvas.height = window.innerHeight * dpr;
        canvas.style.width = window.innerWidth + 'px';
        canvas.style.height = window.innerHeight + 'px';

        console.log(`[app.js] Canvas: ${canvas.width}x${canvas.height}`);

        const adapter = await navigator.gpu.requestAdapter();
        const device = await adapter.requestDevice();
        const context = canvas.getContext('webgpu');
        const format = navigator.gpu.getPreferredCanvasFormat();

        context.configure({
            device,
            format,
            alphaMode: 'opaque'
        });

        console.log('[app.js] WebGPU initialized');
        document.getElementById('status').textContent = 'WebGPU Ready';

        // 保存 WebGPU 资源到全局变量，供 WASM 使用
        window.gpuDevice = device;
        window.gpuContext = context;
        window.gpuFormat = format;
        console.log('[app.js] WebGPU resources saved to window');

        const shaderCode = `
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

        const shaderModule = device.createShaderModule({ code: shaderCode });
        const pipeline = await device.createRenderPipeline({
            layout: 'auto',
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

        console.log('[app.js] Pipeline created');

        fpsCounter = setupFPSCounter();
        
        // 尝试加载 WASM，但如果失败也不中断渲染
        try {
            console.log('[app.js] Attempting to load WASM...');
            const { tknWasm } = await import('./tknWasm.js');
            const wasm = await tknWasm();
            wasmExports = wasm.wasmExports;
            wasmImportObject = wasm.importObject;
            console.log('[app.js] WASM loaded successfully');
            console.log('[app.js] importObject.env.tknCreateGfxContextPtr exists:', typeof wasmImportObject.env.tknCreateGfxContextPtr);
            
            // 获取 canvas 信息
            const canvas = document.getElementById('canvas');
            const canvasWidth = canvas.width;
            const canvasHeight = canvas.height;
            console.log('[app.js] About to call tknCreateGfxContextPtr');
            
            // 调用 tknCreateGfxContextPtr（从 importObject.env 获取）
            const gfxContextObj = wasmImportObject.env.tknCreateGfxContextPtr(
                null,                    // pInstance
                canvas,                  // pSurface
                canvasWidth,            // width
                canvasHeight,           // height
                0,                      // globalShaderPathCount
                0                       // globalShaderPathsPtr
            );
            
            console.log('[app.js] tknCreateGfxContextPtr returned:', gfxContextObj);
            console.log('[app.js] gfxContextObj.initPromise:', gfxContextObj.initPromise);
            
            // 等待 WASM 的异步初始化完成
            gfxContext = await gfxContextObj.initPromise;
            console.log('[app.js] WASM graphics context initialized');
            document.getElementById('status').textContent = 'WASM + WebGPU Ready';
        } catch (wasmError) {
            console.warn('[app.js] WASM loading failed, continuing with WebGPU only:', wasmError);
            // 如果 WASM 加载失败，gfxContext 保持 null，用纯 JavaScript 渲染
        }
        
        startRenderLoop(device, context, pipeline);

    } catch (error) {
        console.error('[app.js] Error:', error);
        document.getElementById('error').textContent = `Error: ${error.message}`;
        document.getElementById('error').style.display = 'block';
    }
}

function startRenderLoop(device, context, pipeline) {
    console.log('[startRenderLoop] Starting render loop');

    let frameCount = 0;
    const render = () => {
        frameCount++;

        // 调用 WASM 的 tknRenderFrame
        if (gfxContext && wasmImportObject && wasmImportObject.env.tknRenderFrame) {
            wasmImportObject.env.tknRenderFrame(gfxContext);
        } else {
            console.warn('[startRenderLoop] WASM not ready, skipping frame');
        }

        fpsCounter.update();
        requestAnimationFrame(render);
    };

    console.log('[startRenderLoop] Calling first render');
    render();
}

function setupFPSCounter() {
    let frameCount = 0;
    let lastTime = performance.now();

    return {
        update: () => {
            frameCount++;
            const now = performance.now();
            if (now - lastTime >= 1000) {
                document.getElementById('fps').textContent = `FPS: ${frameCount}`;
                frameCount = 0;
                lastTime = now;
            }
        }
    };
}

window.addEventListener('load', initApp);
