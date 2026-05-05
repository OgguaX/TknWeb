# Tickernel Web - WebGPU Renderer

这是一个使用 Emscripten 编译的 C/C++ WebGPU 渲染项目。

## 项目结构

```
TknWeb/
├── web/
│   ├── CMakeLists.txt          # CMake 配置文件
│   ├── include/                # C 头文件
│   └── src/                    # C 源文件
├── build/                      # 编译输出目录
│   ├── TickernelWeb.js         # Emscripten 生成的 JavaScript 包装
│   └── TickernelWeb.wasm       # WebAssembly 二进制
├── index.html                  # Web 页面入口
├── app.js                      # 应用程序主逻辑
└── serve.sh                    # 开发服务器脚本
```

## 编译和运行

### 1. 编译项目

```bash
# 激活 Emscripten 环境
source /Users/oggua/Documents/GitHub/emsdk/emsdk_env.sh

# 编译（使用 VS Code 任务更方便）
emcmake cmake -B build -S web
cmake --build build
```

或在 VS Code 中按 `Cmd+Shift+B` 快速编译。

### 2. 启动开发服务器

```bash
# 方式 1: 使用提供的脚本
bash serve.sh

# 方式 2: 直接用 Python
python3 -m http.server 8000

# 方式 3: 用 Node.js
npx http-server
```

### 3. 在浏览器中打开

访问 `http://localhost:8000`

## JavaScript API

`app.js` 提供了以下主要功能：

### 加载 Emscripten 模块
```javascript
const module = await TknWebModule();
```

### 调用 C 函数
```javascript
// cwrap 将 C 函数包装为 JavaScript 可调用的函数
const initWebGPU = module.cwrap('initWebGPU', null, []);
initWebGPU();

const render = module.cwrap('render', null, []);
render();
```

### 访问导出的内存
```javascript
// 可以访问 WASM 线性内存
const memory = module.wasmMemory.buffer;
```

## WebGPU 互操作

C 代码中的 `EM_JS` 宏可以直接调用 JavaScript：

```c
EM_JS(void, js_initWebGPU, (), {
    // JavaScript 代码
    const adapter = await navigator.gpu.requestAdapter();
    // ...
});
```

JavaScript 代码中可以访问 C 导出的全局变量：

```javascript
// 在 tknGfxContext.c 中使用 EM_JS 设置的全局变量
window.gpuDevice
window.gpuContext
window.gpuFormat
```

## 常见问题

### 编译失败？
- 确保 EMSDK 已正确激活：`echo $EMSDK`
- 尝试清除构建目录：`rm -rf build`

### 页面加载失败？
- 检查浏览器控制台（F12）的错误信息
- 确保 `TickernelWeb.js` 和 `TickernelWeb.wasm` 在 `build/` 目录中
- WebGPU 需要现代浏览器支持（Chrome 113+, Firefox 需启用实验功能）

### 黑屏/无法渲染？
- 检查浏览器是否支持 WebGPU：`await navigator.gpu.requestAdapter() !== null`
- 查看浏览器开发工具中的 WebGPU 错误
- 确保 C 代码中的 `initWebGPU()` 和 `render()` 正确实现

## 调试技巧

### 查看 WASM 导出
```javascript
// 在浏览器控制台中
window.app.module
```

### 获取更详细的编译日志
```bash
emcmake cmake -B build -S web -DCMAKE_VERBOSE_MAKEFILE=ON
cmake --build build
```

### 性能分析
Chrome DevTools 中的 Performance 标签可以分析渲染性能。

## 下一步

1. 完善 WebGPU 渲染管道（shader、texture 等）
2. 添加用户交互（键盘、鼠标输入）
3. 优化性能（使用 Rust 重写瓶颈代码）
4. 打包部署（生产构建配置）

## 参考资源

- [Emscripten 官方文档](https://emscripten.org/docs/)
- [WebGPU 规范](https://gpuweb.github.io/gpuweb/)
- [WebGPU 示例](https://github.com/gpuweb/gpuweb/wiki/Implementation-Status)
