// Simple test shader with binding for uniforms

struct GlobalUniform {
    view: mat4x4f,
    proj: mat4x4f,
    near: f32,
    far: f32,
    fov: f32,
    time: f32,
    frameCount: u32,
    screenWidth: u32,
    screenHeight: u32,
}

@group(0) @binding(0) var<uniform> globalUniform: GlobalUniform;

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
