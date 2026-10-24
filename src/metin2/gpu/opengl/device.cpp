#include "device.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstring>

#include <metin2/log/log.h>

namespace metin2::gpu::opengl {

    namespace {
        constexpr size_t USER_VERTEX_CAPACITY = 8u << 20;
        constexpr size_t USER_INDEX_CAPACITY = 2u << 20;

        void color_to_floats(uint32_t argb, float *out) {
            out[0] = static_cast<float>((argb >> 16) & 255) / 255.0f;
            out[1] = static_cast<float>((argb >> 8) & 255) / 255.0f;
            out[2] = static_cast<float>(argb & 255) / 255.0f;
            out[3] = static_cast<float>((argb >> 24) & 255) / 255.0f;
        }

        auto blend_factor(uint32_t b) -> GLenum {
            switch (b) {
            case blend::ZERO:
                return GL_ZERO;
            case blend::ONE:
                return GL_ONE;
            case blend::SRC_COLOR:
                return GL_SRC_COLOR;
            case blend::INV_SRC_COLOR:
                return GL_ONE_MINUS_SRC_COLOR;
            case blend::SRC_ALPHA:
                return GL_SRC_ALPHA;
            case blend::INV_SRC_ALPHA:
                return GL_ONE_MINUS_SRC_ALPHA;
            case blend::DEST_ALPHA:
                return GL_DST_ALPHA;
            case blend::INV_DEST_ALPHA:
                return GL_ONE_MINUS_DST_ALPHA;
            case blend::DEST_COLOR:
                return GL_DST_COLOR;
            case blend::INV_DEST_COLOR:
                return GL_ONE_MINUS_DST_COLOR;
            case blend::SRC_ALPHA_SAT:
                return GL_SRC_ALPHA_SATURATE;
            default:
                return GL_ONE;
            }
        }

        auto compare_function(uint32_t c) -> GLenum {
            switch (c) {
            case compare::NEVER:
                return GL_NEVER;
            case compare::LESS:
                return GL_LESS;
            case compare::EQUAL:
                return GL_EQUAL;
            case compare::LESS_EQUAL:
                return GL_LEQUAL;
            case compare::GREATER:
                return GL_GREATER;
            case compare::NOT_EQUAL:
                return GL_NOTEQUAL;
            case compare::GREATER_EQUAL:
                return GL_GEQUAL;
            default:
                return GL_ALWAYS;
            }
        }

        auto primitive_mode(primitive type) -> GLenum {
            switch (type) {
            case primitive::POINT_LIST:
                return GL_POINTS;
            case primitive::LINE_LIST:
                return GL_LINES;
            case primitive::LINE_STRIP:
                return GL_LINE_STRIP;
            case primitive::TRIANGLE_STRIP:
                return GL_TRIANGLE_STRIP;
            case primitive::TRIANGLE_FAN:
                return GL_TRIANGLE_FAN;
            default:
                return GL_TRIANGLES;
            }
        }

        auto address_mode(uint32_t a) -> GLint {
            switch (a) {
            case address::MIRROR:
                return GL_MIRRORED_REPEAT;
            case address::CLAMP:
                return GL_CLAMP_TO_EDGE;
            case address::BORDER:
                return GL_CLAMP_TO_BORDER;
            default:
                return GL_REPEAT;
            }
        }

        // The scene texture to the back buffer: one triangle covering the screen, gamma, and FXAA.
        // The texture was rendered flipped (row 0 is the top, as in Direct3D), hence 0.5 - y.
        const char *const BLIT_VERTEX_SHADER = R"GLSL(
#version 330 core
out vec2 vUV;
void main() {
    vec2 pos = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2)) * 2.0 - 1.0;
    gl_Position = vec4(pos, 0.0, 1.0);
    vUV = vec2(pos.x * 0.5 + 0.5, 0.5 - pos.y * 0.5);
}
)GLSL";

        const char *const BLIT_FRAGMENT_SHADER = R"GLSL(
#version 330 core
uniform sampler2D uScene;
uniform float uGamma;
uniform int uAntialias;
uniform vec2 uTexel;
in vec2 vUV;
out vec4 fragColor;

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

vec3 fxaa(vec2 uv) {
    const float SPAN_MAX = 8.0;
    const float REDUCE_MUL = 1.0 / 8.0;
    const float REDUCE_MIN = 1.0 / 128.0;
    vec3 rgbNW = texture(uScene, uv + vec2(-1.0, -1.0) * uTexel).rgb;
    vec3 rgbNE = texture(uScene, uv + vec2(1.0, -1.0) * uTexel).rgb;
    vec3 rgbSW = texture(uScene, uv + vec2(-1.0, 1.0) * uTexel).rgb;
    vec3 rgbSE = texture(uScene, uv + vec2(1.0, 1.0) * uTexel).rgb;
    vec3 rgbM = texture(uScene, uv).rgb;
    float lumaNW = luma(rgbNW), lumaNE = luma(rgbNE), lumaSW = luma(rgbSW), lumaSE = luma(rgbSE), lumaM = luma(rgbM);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    vec2 dir = vec2(-((lumaNW + lumaNE) - (lumaSW + lumaSE)), (lumaNW + lumaSW) - (lumaNE + lumaSE));
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * REDUCE_MUL), REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-SPAN_MAX), vec2(SPAN_MAX)) * uTexel;
    vec3 rgbA = 0.5 * (texture(uScene, uv + dir * (1.0 / 3.0 - 0.5)).rgb + texture(uScene, uv + dir * (2.0 / 3.0 - 0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(uScene, uv + dir * -0.5).rgb + texture(uScene, uv + dir * 0.5).rgb);
    float lumaB = luma(rgbB);
    return (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;
}

void main() {
    vec3 c = uAntialias != 0 ? fxaa(vUV) : texture(uScene, vUV).rgb;
    if (uGamma != 1.0)
        c = pow(max(c, vec3(0.0)), vec3(uGamma));
    fragColor = vec4(c, 1.0);
}
)GLSL";

        auto compile_shader(GLenum type, const char *source) -> GLuint {
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);
            GLint ok = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char log[4096];
                glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
                fprintf(stderr, "[opengl] shader compile failed:\n%s\n", log);
                glDeleteShader(shader);
                return 0;
            }
            return shader;
        }
    }

    gl_device::gl_device(const device_settings &settings) : settings_(settings) {
        for (bool &enabled : state_.light_enabled)
            enabled = false;
        for (light &l : state_.lights)
            l = {};
    }

    gl_device::~gl_device() {
        for (auto &t : textures_)
            t.reset();
        for (stream &s : streams_)
            s.buffer.reset();
        indices_.reset();
        target_.reset();
        depth_target_.reset();
        if (program_)
            glDeleteProgram(program_);
        if (uniform_buffer_)
            glDeleteBuffers(1, &uniform_buffer_);
        if (vertex_array_)
            glDeleteVertexArrays(1, &vertex_array_);
        if (user_vertices_)
            glDeleteBuffers(1, &user_vertices_);
        if (user_indices_)
            glDeleteBuffers(1, &user_indices_);
        if (samplers_[0])
            glDeleteSamplers(2, samplers_);
        if (white_texture_)
            glDeleteTextures(1, &white_texture_);
        if (blit_program_)
            glDeleteProgram(blit_program_);
        if (blit_sampler_)
            glDeleteSamplers(1, &blit_sampler_);
        if (blit_vertex_array_)
            glDeleteVertexArrays(1, &blit_vertex_array_);
    }

    auto gl_device::initialize() -> bool {
        if (!IsWindowReady()) {
            metin2::log::error("Cannot create the OpenGL device: no window");
            return false;
        }
        if (!create_program() || !create_blit_program())
            return false;
        glGenVertexArrays(1, &vertex_array_);
        glBindVertexArray(vertex_array_);
        glGenBuffers(1, &user_vertices_);
        glBindBuffer(GL_ARRAY_BUFFER, user_vertices_);
        glBufferData(GL_ARRAY_BUFFER, USER_VERTEX_CAPACITY, nullptr, GL_STREAM_DRAW);
        glGenBuffers(1, &user_indices_);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, user_indices_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, USER_INDEX_CAPACITY, nullptr, GL_STREAM_DRAW);
        glGenSamplers(2, samplers_);
        glGenTextures(1, &white_texture_);
        glBindTexture(GL_TEXTURE_2D, white_texture_);
        const uint32_t white = 0xffffffffu;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        const extensions &ext = get_extensions();
        GLint max_texture = 2048;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture);
        caps_.max_texture_size = static_cast<uint32_t>(max_texture);
        caps_.compressed_textures = ext.s3tc;
        caps_.anisotropic_filter = ext.anisotropic;
        if (ext.anisotropic) {
            GLfloat anisotropy = 1.0f;
            glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropy);
            caps_.max_anisotropy = static_cast<uint32_t>(anisotropy);
        }
        caps_.max_texture_stages = fixed_function_state::EVALUATED_STAGES;
        caps_.max_lights = fixed_function_state::LIGHTS;

        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_DITHER);
        if (!reset(settings_))
            return false;
        return true;
    }

    auto gl_device::create_program() -> bool {
        GLuint vs = compile_shader(GL_VERTEX_SHADER, FIXED_FUNCTION_VERTEX_SHADER);
        GLuint fs = compile_shader(GL_FRAGMENT_SHADER, FIXED_FUNCTION_FRAGMENT_SHADER);
        if (!vs || !fs) {
            if (vs)
                glDeleteShader(vs);
            if (fs)
                glDeleteShader(fs);
            return false;
        }
        program_ = glCreateProgram();
        glAttachShader(program_, vs);
        glAttachShader(program_, fs);
        glLinkProgram(program_);
        glDeleteShader(vs);
        glDeleteShader(fs);
        GLint ok = 0;
        glGetProgramiv(program_, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[4096];
            glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
            fprintf(stderr, "[opengl] program link failed:\n%s\n", log);
            glDeleteProgram(program_);
            program_ = 0;
            return false;
        }
        const GLuint block = glGetUniformBlockIndex(program_, "FFP");
        glUniformBlockBinding(program_, block, 0);
        GLint size = 0;
        glGetActiveUniformBlockiv(program_, block, GL_UNIFORM_BLOCK_DATA_SIZE, &size);
        if (size != static_cast<GLint>(sizeof(fixed_function_uniforms)))
            fprintf(stderr, "[opengl] FFP block is %d bytes, expected %zu\n", size, sizeof(fixed_function_uniforms));
        glGenBuffers(1, &uniform_buffer_);
        glBindBuffer(GL_UNIFORM_BUFFER, uniform_buffer_);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(fixed_function_uniforms), nullptr, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniform_buffer_);
        glUseProgram(program_);
        glUniform1i(glGetUniformLocation(program_, "uSampler0"), 0);
        glUniform1i(glGetUniformLocation(program_, "uSampler1"), 1);
        return true;
    }

    auto gl_device::adapter_name() const -> std::string {
        const auto *renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
        return renderer ? renderer : "OpenGL";
    }

    auto gl_device::check_format(format fmt, uint32_t usage) const -> bool {
        const format_info *info = get_format_info(fmt);
        if (!info || !get_gl_format(fmt))
            return false;
        if (info->compressed && !caps_.compressed_textures)
            return false;
        if (usage & usage::DEPTH_STENCIL)
            return info->depth;
        if (usage & usage::RENDER_TARGET)
            return format_is_renderable(fmt);
        return !info->depth;
    }

    auto gl_device::available_texture_memory() const -> uint32_t {
        return 1024u * 1024u * 1024u;
    }

    auto gl_device::create_texture(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, format fmt) -> std::shared_ptr<texture> {
        const format_info *info = get_format_info(fmt);
        if (!info || info->depth || !get_gl_format(fmt) || width == 0 || height == 0) {
            metin2::log::error("Cannot create a texture of format {} and size {}x{}", static_cast<uint32_t>(fmt), width, height);
            return nullptr;
        }
        if (info->compressed && !caps_.compressed_textures)
            return nullptr;
        if ((usage & usage::RENDER_TARGET) && !format_is_renderable(fmt))
            return nullptr;
        return std::make_shared<gl_texture>(width, height, levels, usage, fmt);
    }

    auto gl_device::create_vertex_buffer(uint32_t size, uint32_t usage) -> std::shared_ptr<vertex_buffer> {
        if (size == 0)
            return nullptr;
        return std::make_shared<gl_vertex_buffer>(size, usage);
    }

    auto gl_device::create_index_buffer(uint32_t size, uint32_t usage, format fmt) -> std::shared_ptr<index_buffer> {
        if (size == 0 || (fmt != format::INDEX16 && fmt != format::INDEX32))
            return nullptr;
        return std::make_shared<gl_index_buffer>(size, usage, fmt);
    }

    auto gl_device::create_depth_buffer(uint32_t width, uint32_t height, format fmt) -> std::shared_ptr<depth_buffer> {
        const format_info *info = get_format_info(fmt);
        if (!info || !info->depth)
            return nullptr;
        return std::make_shared<gl_depth_buffer>(width, height, fmt);
    }

    auto gl_device::create_vertex_layout(std::span<const vertex_element> elements) -> uint32_t {
        auto layout = std::make_unique<vertex_layout>();
        if (!layout_from_elements(elements, *layout))
            return 0;
        for (size_t i = 0; i < layouts_.size(); ++i)
            if (!layouts_[i]) {
                layouts_[i] = std::move(layout);
                return VERTEX_LAYOUT_HANDLE | static_cast<uint32_t>(i);
            }
        layouts_.push_back(std::move(layout));
        return VERTEX_LAYOUT_HANDLE | static_cast<uint32_t>(layouts_.size() - 1);
    }

    void gl_device::destroy_vertex_layout(uint32_t handle) {
        if (!(handle & VERTEX_LAYOUT_HANDLE))
            return;
        const size_t index = handle & ~VERTEX_LAYOUT_HANDLE;
        if (index >= layouts_.size() || !layouts_[index])
            return;
        if (uniform_layout_ == layouts_[index].get())
            uniform_layout_ = nullptr;
        layouts_[index].reset();
        if (vertex_format_ == handle)
            vertex_format_ = 0;
    }

    auto gl_device::reset(const device_settings &settings) -> bool {
        settings_ = settings;
        if (settings_.width == 0)
            settings_.width = static_cast<uint32_t>(GetScreenWidth());
        if (settings_.height == 0)
            settings_.height = static_cast<uint32_t>(GetScreenHeight());
        if (static_cast<uint32_t>(GetScreenWidth()) != settings_.width || static_cast<uint32_t>(GetScreenHeight()) != settings_.height)
            SetWindowSize(static_cast<int>(settings_.width), static_cast<int>(settings_.height));
        for (auto &t : textures_)
            t.reset();
        target_.reset();
        depth_target_.reset();
        target_framebuffer_ = 0;
        flip_y_ = false;
        target_width_ = settings_.width;
        target_height_ = settings_.height;
        state_.reset(target_width_, target_height_, settings_.depth_buffer);
        vertex_format_ = 0;
        uniforms_dirty_ = pipeline_dirty_ = viewport_dirty_ = true;
        sampler_dirty_[0] = sampler_dirty_[1] = true;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return true;
    }

    void gl_device::present() {
        if (before_swap_)
            before_swap_();
        SwapScreenBuffer();
        if (after_swap_)
            after_swap_();
    }

    auto gl_device::read_back_buffer(std::vector<uint8_t> &bgra) -> bool {
        // The framebuffer may already have the size of a window resize the game has not handled yet.
        const auto w = static_cast<uint32_t>(GetRenderWidth());
        const auto h = static_cast<uint32_t>(GetRenderHeight());
        if (w == 0 || h == 0)
            return false;
        bgra.assign(static_cast<size_t>(w) * h * 4, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glReadBuffer(GL_BACK);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
        glReadPixels(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, bgra.data());
        glBindFramebuffer(GL_READ_FRAMEBUFFER, target_framebuffer_);
        // bottom-up to top-down
        std::vector<uint8_t> row(static_cast<size_t>(w) * 4);
        for (uint32_t y = 0; y < h / 2; ++y) {
            uint8_t *a = bgra.data() + static_cast<size_t>(y) * w * 4;
            uint8_t *b = bgra.data() + static_cast<size_t>(h - 1 - y) * w * 4;
            memcpy(row.data(), a, row.size());
            memcpy(a, b, row.size());
            memcpy(b, row.data(), row.size());
        }
        return true;
    }

    void gl_device::set_render_state(state s, uint32_t value) {
        const auto index = static_cast<uint32_t>(s);
        if (index >= 256 || state_.render_states[index] == value)
            return;
        state_.render_states[index] = value;
        if (is_pipeline_state(s))
            pipeline_dirty_ = true;
        else
            uniforms_dirty_ = true;
    }

    void gl_device::set_texture_stage_state(uint32_t stage, stage_state s, uint32_t value) {
        const auto index = static_cast<uint32_t>(s);
        if (stage >= fixed_function_state::STAGES || index >= 32 || state_.stage_states[stage][index] == value)
            return;
        state_.stage_states[stage][index] = value;
        if (stage >= fixed_function_state::EVALUATED_STAGES)
            return;
        if (is_sampler_state(s))
            sampler_dirty_[stage] = true;
        else
            uniforms_dirty_ = true;
    }

    auto gl_device::texture_stage_state(uint32_t stage, stage_state s) const -> uint32_t {
        const auto index = static_cast<uint32_t>(s);
        if (stage >= fixed_function_state::STAGES || index >= 32)
            return 0;
        return state_.stage_states[stage][index];
    }

    void gl_device::set_texture(uint32_t stage, texture *tex) {
        if (stage >= fixed_function_state::STAGES || textures_[stage].get() == tex)
            return;
        textures_[stage] = tex ? std::static_pointer_cast<gl_texture>(tex->shared_from_this()) : nullptr;
        state_.texture_bound[stage] = tex != nullptr;
        if (stage < fixed_function_state::EVALUATED_STAGES)
            uniforms_dirty_ = true;
    }

    void gl_device::set_transform(transform t, const math::matrix &m) {
        if (math::matrix *slot = state_.transform_slot(t)) {
            *slot = m;
            uniforms_dirty_ = true;
        }
    }

    auto gl_device::get_transform(transform t) const -> math::matrix {
        if (const math::matrix *slot = state_.transform_slot(t))
            return *slot;
        return math::matrix_identity();
    }

    void gl_device::set_material(const material &m) {
        state_.mat = m;
        uniforms_dirty_ = true;
    }

    void gl_device::set_light(uint32_t index, const light &l) {
        if (index >= fixed_function_state::LIGHTS)
            return;
        state_.lights[index] = l;
        uniforms_dirty_ = true;
    }

    void gl_device::enable_light(uint32_t index, bool enable) {
        if (index >= fixed_function_state::LIGHTS)
            return;
        state_.light_enabled[index] = enable;
        uniforms_dirty_ = true;
    }

    void gl_device::set_viewport(const viewport &vp) {
        state_.view_port = vp;
        viewport_dirty_ = true;
        uniforms_dirty_ = true;
    }

    void gl_device::set_vertex_format(uint32_t format_or_handle) {
        vertex_format_ = format_or_handle;
    }

    void gl_device::set_stream(uint32_t index, vertex_buffer *buffer, uint32_t stride) {
        if (index >= 4)
            return;
        streams_[index].buffer = buffer ? std::static_pointer_cast<gl_vertex_buffer>(buffer->shared_from_this()) : nullptr;
        streams_[index].stride = stride;
    }

    void gl_device::set_indices(index_buffer *buffer, uint32_t base_vertex) {
        indices_ = buffer ? std::static_pointer_cast<gl_index_buffer>(buffer->shared_from_this()) : nullptr;
        base_vertex_ = base_vertex;
    }

    auto gl_device::current_layout() -> const vertex_layout * {
        if (vertex_format_ & VERTEX_LAYOUT_HANDLE) {
            const size_t index = vertex_format_ & ~VERTEX_LAYOUT_HANDLE;
            return index < layouts_.size() ? layouts_[index].get() : nullptr;
        }
        if (vertex_format_ == 0)
            return nullptr;
        if (fvf_layout_code_ != vertex_format_) {
            if (!layout_from_fvf(vertex_format_, fvf_layout_))
                return nullptr;
            fvf_layout_code_ = vertex_format_;
            // same object, new contents
            if (uniform_layout_ == &fvf_layout_)
                uniform_layout_ = nullptr;
        }
        return &fvf_layout_;
    }

    auto gl_device::set_render_target(texture *color, depth_buffer *depth) -> bool {
        auto *gl_color = static_cast<gl_texture *>(color);
        if (gl_color && !(gl_color->usage() & usage::RENDER_TARGET))
            return false;
        target_ = gl_color ? std::static_pointer_cast<gl_texture>(gl_color->shared_from_this()) : nullptr;
        depth_target_ = depth ? std::static_pointer_cast<gl_depth_buffer>(depth->shared_from_this()) : nullptr;
        if (!target_) {
            target_framebuffer_ = 0;
            flip_y_ = false;
            target_width_ = settings_.width;
            target_height_ = settings_.height;
        } else {
            target_framebuffer_ = target_->framebuffer();
            glBindFramebuffer(GL_FRAMEBUFFER, target_framebuffer_);
            const GLuint depth_name = depth_target_ ? depth_target_->name : 0;
            const bool stencil = depth_target_ && depth_target_->stencil;
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_name);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencil ? depth_name : 0);
            const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE)
                metin2::log::error("Render target incomplete: 0x{:x}", status);
            // GL textures are stored bottom-up: draw flipped so the texture reads like Direct3D's.
            flip_y_ = true;
            target_width_ = target_->width();
            target_height_ = target_->height();
        }
        state_.view_port = {0, 0, target_width_, target_height_, 0.0f, 1.0f};
        viewport_dirty_ = pipeline_dirty_ = uniforms_dirty_ = true;
        bind_target();
        return true;
    }

    void gl_device::bind_target() {
        glBindFramebuffer(GL_FRAMEBUFFER, target_framebuffer_);
    }

    void gl_device::clear(std::span<const rectangle> areas, uint32_t flags, uint32_t color, float z, uint32_t stencil) {
        bind_target();
        GLbitfield mask = 0;
        if (flags & clear::TARGET) {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            float c[4];
            color_to_floats(color, c);
            glClearColor(c[0], c[1], c[2], c[3]);
            mask |= GL_COLOR_BUFFER_BIT;
        }
        if (flags & clear::Z_BUFFER) {
            glDepthMask(GL_TRUE);
            glClearDepth(z);
            mask |= GL_DEPTH_BUFFER_BIT;
        }
        if (flags & clear::STENCIL) {
            glStencilMask(0xffffffff);
            glClearStencil(static_cast<GLint>(stencil));
            mask |= GL_STENCIL_BUFFER_BIT;
        }
        // Clears are limited to the viewport, as in Direct3D.
        const viewport &vp = state_.view_port;
        auto scissor = [&](int32_t x1, int32_t y1, int32_t x2, int32_t y2) {
            x1 = std::max(x1, static_cast<int32_t>(vp.x));
            y1 = std::max(y1, static_cast<int32_t>(vp.y));
            x2 = std::min(x2, static_cast<int32_t>(vp.x + vp.width));
            y2 = std::min(y2, static_cast<int32_t>(vp.y + vp.height));
            if (x2 <= x1 || y2 <= y1)
                return false;
            const GLint y = flip_y_ ? y1 : static_cast<GLint>(target_height_) - y2;
            glScissor(x1, y, x2 - x1, y2 - y1);
            return true;
        };
        glEnable(GL_SCISSOR_TEST);
        if (areas.empty()) {
            if (scissor(0, 0, static_cast<int32_t>(target_width_), static_cast<int32_t>(target_height_)))
                glClear(mask);
        } else {
            for (const rectangle &r : areas)
                if (scissor(r.left, r.top, r.right, r.bottom))
                    glClear(mask);
        }
        glDisable(GL_SCISSOR_TEST);
        pipeline_dirty_ = true; // the colour and depth masks were touched
    }

    void gl_device::apply_viewport() {
        const viewport &vp = state_.view_port;
        const GLint y = flip_y_ ? static_cast<GLint>(vp.y) : static_cast<GLint>(target_height_) - static_cast<GLint>(vp.y) - static_cast<GLint>(vp.height);
        glViewport(static_cast<GLint>(vp.x), y, static_cast<GLsizei>(vp.width), static_cast<GLsizei>(vp.height));
        glDepthRange(vp.min_z, vp.max_z);
        viewport_dirty_ = false;
    }

    void gl_device::apply_pipeline_state() {
        if (state_.render_state(state::Z_ENABLE)) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(compare_function(state_.render_state(state::Z_FUNC)));
        } else {
            glDisable(GL_DEPTH_TEST);
        }
        glDepthMask(state_.render_state(state::Z_WRITE_ENABLE) ? GL_TRUE : GL_FALSE);
        if (state_.render_state(state::ALPHA_BLEND_ENABLE)) {
            glEnable(GL_BLEND);
            uint32_t src = state_.render_state(state::SRC_BLEND);
            uint32_t dest = state_.render_state(state::DEST_BLEND);
            if (src == blend::BOTH_SRC_ALPHA) {
                src = blend::SRC_ALPHA;
                dest = blend::INV_SRC_ALPHA;
            } else if (src == blend::BOTH_INV_SRC_ALPHA) {
                src = blend::INV_SRC_ALPHA;
                dest = blend::SRC_ALPHA;
            }
            glBlendFunc(blend_factor(src), blend_factor(dest));
            switch (state_.render_state(state::BLEND_OP)) {
            case blend_op::SUBTRACT:
                glBlendEquation(GL_FUNC_SUBTRACT);
                break;
            case blend_op::REV_SUBTRACT:
                glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
                break;
            case blend_op::MIN:
                glBlendEquation(GL_MIN);
                break;
            case blend_op::MAX:
                glBlendEquation(GL_MAX);
                break;
            default:
                glBlendEquation(GL_FUNC_ADD);
                break;
            }
        } else {
            glDisable(GL_BLEND);
        }
        const uint32_t cull_mode = state_.render_state(state::CULL_MODE);
        if (cull_mode == cull::NONE) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            // CW culls clockwise triangles, so the front faces are counter-clockwise; drawing into a
            // texture flips y and with it the winding.
            bool front_ccw = cull_mode == cull::CW;
            if (flip_y_)
                front_ccw = !front_ccw;
            glFrontFace(front_ccw ? GL_CCW : GL_CW);
        }
        const uint32_t fill_mode = state_.render_state(state::FILL_MODE);
        glPolygonMode(GL_FRONT_AND_BACK, fill_mode == fill::WIREFRAME ? GL_LINE : (fill_mode == fill::POINT ? GL_POINT : GL_FILL));
        const uint32_t write = state_.render_state(state::COLOR_WRITE_ENABLE);
        glColorMask((write & 1) != 0, (write & 2) != 0, (write & 4) != 0, (write & 8) != 0);
        if (const uint32_t bias = state_.render_state(state::Z_BIAS)) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(0.0f, -static_cast<float>(bias) * 2.0f);
        } else {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
        pipeline_dirty_ = false;
    }

    void gl_device::apply_sampler(int stage) {
        const GLuint sampler = samplers_[stage];
        const uint32_t mag = state_.stage_state(stage, stage_state::MAG_FILTER);
        const uint32_t min = state_.stage_state(stage, stage_state::MIN_FILTER);
        const uint32_t mip = state_.stage_state(stage, stage_state::MIP_FILTER);
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, mag == filter::POINT || mag == filter::NONE ? GL_NEAREST : GL_LINEAR);
        const bool min_linear = !(min == filter::POINT || min == filter::NONE);
        GLint min_filter;
        if (mip == filter::NONE)
            min_filter = min_linear ? GL_LINEAR : GL_NEAREST;
        else if (mip == filter::POINT)
            min_filter = min_linear ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
        else
            min_filter = min_linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
        glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, min_filter);
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, address_mode(state_.stage_state(stage, stage_state::ADDRESS_U)));
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, address_mode(state_.stage_state(stage, stage_state::ADDRESS_V)));
        float border[4];
        color_to_floats(state_.stage_state(stage, stage_state::BORDER_COLOR), border);
        glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, border);
        if (caps_.anisotropic_filter) {
            const bool anisotropic = mag == filter::ANISOTROPIC || min == filter::ANISOTROPIC;
            const float amount = anisotropic ? static_cast<float>(std::max(1u, state_.stage_state(stage, stage_state::MAX_ANISOTROPY))) : 1.0f;
            glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, amount);
        }
        float bias;
        const uint32_t bias_bits = state_.stage_state(stage, stage_state::MIPMAP_LOD_BIAS);
        memcpy(&bias, &bias_bits, sizeof(bias));
        glSamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, bias);
        sampler_dirty_[stage] = false;
    }

    void gl_device::prepare_draw(const vertex_layout &layout, const GLuint *buffers, const uint32_t *strides, const uint32_t *offsets) {
        bind_target();
        glUseProgram(program_);
        glBindVertexArray(vertex_array_);
        if (viewport_dirty_)
            apply_viewport();
        if (pipeline_dirty_)
            apply_pipeline_state();
        for (int s = 0; s < fixed_function_state::EVALUATED_STAGES; ++s)
            if (sampler_dirty_[s])
                apply_sampler(s);
        // The block depends on the vertex layout too (pre-transformed, colours, texture dimensions).
        if (uniforms_dirty_ || uniform_layout_ != &layout) {
            pack_uniforms(state_, layout, {.flip_y = flip_y_, .half_pixel = true}, uniforms_);
            glBindBuffer(GL_UNIFORM_BUFFER, uniform_buffer_);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(fixed_function_uniforms), &uniforms_);
            uniforms_dirty_ = false;
            uniform_layout_ = &layout;
        }
        for (int s = 0; s < fixed_function_state::EVALUATED_STAGES; ++s) {
            glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(s));
            glBindTexture(GL_TEXTURE_2D, textures_[s] ? textures_[s]->name : white_texture_);
            glBindSampler(static_cast<GLuint>(s), samplers_[s]);
        }
        bool used[7] = {};
        for (const layout_attribute &a : layout.attributes) {
            const GLuint buffer = buffers[a.stream];
            if (!buffer)
                continue;
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            const auto slot = static_cast<GLuint>(a.slot);
            const auto *pointer = reinterpret_cast<const void *>(static_cast<uintptr_t>(offsets[a.stream] + static_cast<uint32_t>(a.offset)));
            const auto stride = static_cast<GLsizei>(strides[a.stream]);
            glEnableVertexAttribArray(slot);
            switch (a.data) {
            case layout_attribute::kind::FLOAT:
                glVertexAttribPointer(slot, a.components, GL_FLOAT, GL_FALSE, stride, pointer);
                break;
            case layout_attribute::kind::COLOR:
                glVertexAttribPointer(slot, GL_BGRA, GL_UNSIGNED_BYTE, GL_TRUE, stride, pointer);
                break;
            case layout_attribute::kind::UBYTE:
                glVertexAttribPointer(slot, a.components, GL_UNSIGNED_BYTE, GL_FALSE, stride, pointer);
                break;
            case layout_attribute::kind::SHORT:
                glVertexAttribPointer(slot, a.components, GL_SHORT, GL_FALSE, stride, pointer);
                break;
            }
            used[a.slot] = true;
        }
        for (GLuint i = 0; i < 7; ++i) {
            if (used[i])
                continue;
            glDisableVertexAttribArray(i);
            if (i == 2)
                glVertexAttrib4f(i, 1.0f, 1.0f, 1.0f, 1.0f);
            else if (i == 1)
                glVertexAttrib4f(i, 0.0f, 0.0f, 1.0f, 0.0f);
            else
                glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    void gl_device::submit(primitive type, uint32_t primitive_count, GLenum index_type, GLuint indices, size_t index_offset, int32_t base_vertex, uint32_t first_vertex) {
        const uint32_t count = primitive_vertex_count(type, primitive_count);
        if (count == 0)
            return;
        if (indices) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);
            glDrawElementsBaseVertex(primitive_mode(type), static_cast<GLsizei>(count), index_type, reinterpret_cast<const void *>(index_offset), base_vertex);
        } else {
            glDrawArrays(primitive_mode(type), static_cast<GLint>(first_vertex), static_cast<GLsizei>(count));
        }
    }

    auto gl_device::draw(primitive type, uint32_t start_vertex, uint32_t primitive_count) -> bool {
        const vertex_layout *layout = current_layout();
        if (!layout || !streams_[0].buffer)
            return false;
        GLuint buffers[4];
        uint32_t strides[4];
        const uint32_t offsets[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4; ++i) {
            buffers[i] = streams_[i].buffer ? streams_[i].buffer->name : 0;
            strides[i] = streams_[i].stride ? streams_[i].stride : layout->stream_stride[i];
        }
        prepare_draw(*layout, buffers, strides, offsets);
        submit(type, primitive_count, 0, 0, 0, 0, start_vertex);
        return true;
    }

    auto gl_device::draw_indexed(primitive type, uint32_t, uint32_t, uint32_t start_index, uint32_t primitive_count) -> bool {
        const vertex_layout *layout = current_layout();
        if (!layout || !streams_[0].buffer || !indices_)
            return false;
        GLuint buffers[4];
        uint32_t strides[4];
        const uint32_t offsets[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4; ++i) {
            buffers[i] = streams_[i].buffer ? streams_[i].buffer->name : 0;
            strides[i] = streams_[i].stride ? streams_[i].stride : layout->stream_stride[i];
        }
        prepare_draw(*layout, buffers, strides, offsets);
        const bool wide = indices_->index_format() == format::INDEX32;
        submit(type, primitive_count, wide ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, indices_->name, static_cast<size_t>(start_index) * (wide ? 4 : 2), static_cast<int32_t>(base_vertex_), 0);
        return true;
    }

    // Appends to a ring buffer, orphaning it when full; returns the offset of the data.
    auto gl_device::stream_user_data(GLenum target, GLuint buffer, size_t capacity, size_t &offset, const void *data, size_t size) -> size_t {
        glBindBuffer(target, buffer);
        if (offset + size > capacity) {
            glBufferData(target, static_cast<GLsizeiptr>(capacity), nullptr, GL_STREAM_DRAW);
            offset = 0;
        }
        const size_t at = offset;
        glBufferSubData(target, static_cast<GLintptr>(at), static_cast<GLsizeiptr>(size), data);
        offset += (size + 15) & ~static_cast<size_t>(15);
        return at;
    }

    auto gl_device::draw_user(primitive type, uint32_t primitive_count, const void *vertices, uint32_t stride) -> bool {
        const vertex_layout *layout = current_layout();
        if (!layout || !vertices || stride == 0)
            return false;
        const size_t bytes = static_cast<size_t>(primitive_vertex_count(type, primitive_count)) * stride;
        if (bytes > USER_VERTEX_CAPACITY)
            return false;
        const size_t at = stream_user_data(GL_ARRAY_BUFFER, user_vertices_, USER_VERTEX_CAPACITY, user_vertices_offset_, vertices, bytes);
        const GLuint buffers[4] = {user_vertices_, 0, 0, 0};
        const uint32_t strides[4] = {stride, 0, 0, 0};
        const uint32_t offsets[4] = {static_cast<uint32_t>(at), 0, 0, 0};
        prepare_draw(*layout, buffers, strides, offsets);
        submit(type, primitive_count, 0, 0, 0, 0, 0);
        // as in Direct3D, stream 0 is unset afterwards
        streams_[0].buffer.reset();
        return true;
    }

    auto gl_device::create_blit_program() -> bool {
        GLuint vs = compile_shader(GL_VERTEX_SHADER, BLIT_VERTEX_SHADER);
        GLuint fs = compile_shader(GL_FRAGMENT_SHADER, BLIT_FRAGMENT_SHADER);
        if (!vs || !fs) {
            if (vs)
                glDeleteShader(vs);
            if (fs)
                glDeleteShader(fs);
            return false;
        }
        blit_program_ = glCreateProgram();
        glAttachShader(blit_program_, vs);
        glAttachShader(blit_program_, fs);
        glLinkProgram(blit_program_);
        glDeleteShader(vs);
        glDeleteShader(fs);
        GLint ok = 0;
        glGetProgramiv(blit_program_, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[4096];
            glGetProgramInfoLog(blit_program_, sizeof(log), nullptr, log);
            fprintf(stderr, "[opengl] blit program link failed:\n%s\n", log);
            glDeleteProgram(blit_program_);
            blit_program_ = 0;
            return false;
        }
        glUseProgram(blit_program_);
        glUniform1i(glGetUniformLocation(blit_program_, "uScene"), 0);
        blit_gamma_location_ = glGetUniformLocation(blit_program_, "uGamma");
        blit_antialias_location_ = glGetUniformLocation(blit_program_, "uAntialias");
        blit_texel_location_ = glGetUniformLocation(blit_program_, "uTexel");
        glGenSamplers(1, &blit_sampler_);
        glSamplerParameteri(blit_sampler_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glSamplerParameteri(blit_sampler_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glSamplerParameteri(blit_sampler_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glSamplerParameteri(blit_sampler_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glGenVertexArrays(1, &blit_vertex_array_);
        return true;
    }

    void gl_device::blit(texture *source, const blit_params &params) {
        auto *gl_source = static_cast<gl_texture *>(source);
        if (!gl_source || !blit_program_)
            return;

        bind_target();
        if (viewport_dirty_)
            apply_viewport();
        glUseProgram(blit_program_);
        glBindVertexArray(blit_vertex_array_);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gl_source->name);
        glBindSampler(0, blit_sampler_);
        glUniform1f(blit_gamma_location_, params.gamma);
        glUniform1i(blit_antialias_location_, params.antialias ? 1 : 0);
        glUniform2f(blit_texel_location_, 1.0f / static_cast<float>(gl_source->width()), 1.0f / static_cast<float>(gl_source->height()));
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // The fixed-function path puts its own state back on its next draw.
        pipeline_dirty_ = true;
        sampler_dirty_[0] = true;
    }

    auto gl_device::create_occlusion_query() -> uint32_t {
        GLuint query = 0;
        glGenQueries(1, &query);
        return query;
    }

    void gl_device::destroy_occlusion_query(uint32_t query) {
        if (query)
            glDeleteQueries(1, &query);
    }

    void gl_device::begin_occlusion_query(uint32_t query) {
        glBeginQuery(GL_SAMPLES_PASSED, query);
    }

    void gl_device::end_occlusion_query() {
        glEndQuery(GL_SAMPLES_PASSED);
    }

    auto gl_device::occlusion_query_result(uint32_t query, uint32_t &samples) -> bool {
        GLuint available = 0;
        glGetQueryObjectuiv(query, GL_QUERY_RESULT_AVAILABLE, &available);
        if (!available)
            return false;
        GLuint result = 0;
        glGetQueryObjectuiv(query, GL_QUERY_RESULT, &result);
        samples = result;
        return true;
    }

    auto gl_device::draw_indexed_user(primitive type, uint32_t min_index, uint32_t vertex_count, uint32_t primitive_count, const void *indices, format index_format, const void *vertices, uint32_t stride) -> bool {
        const vertex_layout *layout = current_layout();
        if (!layout || !vertices || !indices || stride == 0)
            return false;
        const bool wide = index_format == format::INDEX32;
        const size_t vertex_bytes = static_cast<size_t>(min_index + vertex_count) * stride;
        const size_t index_bytes = static_cast<size_t>(primitive_vertex_count(type, primitive_count)) * (wide ? 4 : 2);
        if (vertex_bytes > USER_VERTEX_CAPACITY || index_bytes > USER_INDEX_CAPACITY)
            return false;
        const size_t vertex_at = stream_user_data(GL_ARRAY_BUFFER, user_vertices_, USER_VERTEX_CAPACITY, user_vertices_offset_, vertices, vertex_bytes);
        const size_t index_at = stream_user_data(GL_ELEMENT_ARRAY_BUFFER, user_indices_, USER_INDEX_CAPACITY, user_indices_offset_, indices, index_bytes);
        const GLuint buffers[4] = {user_vertices_, 0, 0, 0};
        const uint32_t strides[4] = {stride, 0, 0, 0};
        const uint32_t offsets[4] = {static_cast<uint32_t>(vertex_at), 0, 0, 0};
        prepare_draw(*layout, buffers, strides, offsets);
        submit(type, primitive_count, wide ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT, user_indices_, index_at, 0, 0);
        streams_[0].buffer.reset();
        indices_.reset();
        return true;
    }
}
