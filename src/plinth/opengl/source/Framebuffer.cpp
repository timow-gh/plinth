#include "OpenGL/Framebuffer.hpp"
#include "OpenGL/ErrorReporting.hpp"
#include <array>
#include <utility>

namespace opengl {

Framebuffer::Framebuffer(GLuint framebuffer, GLuint colorTexture, GLuint colorRenderbuffer,
                         GLuint depthStencilRenderbuffer, GLuint depthTexture, bool hasDepthTexture,
                         int width, int height, int samples, bool srgb)
    : m_framebuffer(framebuffer)
    , m_colorTexture(colorTexture)
    , m_colorRenderbuffer(colorRenderbuffer)
    , m_depthStencilRenderbuffer(depthStencilRenderbuffer)
    , m_depthTexture(depthTexture)
    , m_hasDepthTexture(hasDepthTexture)
    , m_width(width)
    , m_height(height)
    , m_samples(samples)
    , m_srgb(srgb) {
}

Framebuffer::Framebuffer(Framebuffer&& other) noexcept
    : m_framebuffer(std::exchange(other.m_framebuffer, 0))
    , m_colorTexture(std::exchange(other.m_colorTexture, 0))
    , m_colorRenderbuffer(std::exchange(other.m_colorRenderbuffer, 0))
    , m_depthStencilRenderbuffer(std::exchange(other.m_depthStencilRenderbuffer, 0))
    , m_depthTexture(std::exchange(other.m_depthTexture, 0))
    , m_hasDepthTexture(std::exchange(other.m_hasDepthTexture, false))
    , m_width(std::exchange(other.m_width, 0))
    , m_height(std::exchange(other.m_height, 0))
    , m_samples(std::exchange(other.m_samples, 0))
    , m_srgb(std::exchange(other.m_srgb, false)) {
}

Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept {
    if (this != &other) {
        reset();
        m_framebuffer = std::exchange(other.m_framebuffer, 0);
        m_colorTexture = std::exchange(other.m_colorTexture, 0);
        m_colorRenderbuffer = std::exchange(other.m_colorRenderbuffer, 0);
        m_depthStencilRenderbuffer = std::exchange(other.m_depthStencilRenderbuffer, 0);
        m_depthTexture = std::exchange(other.m_depthTexture, 0);
        m_hasDepthTexture = std::exchange(other.m_hasDepthTexture, false);
        m_width = std::exchange(other.m_width, 0);
        m_height = std::exchange(other.m_height, 0);
        m_samples = std::exchange(other.m_samples, 0);
        m_srgb = std::exchange(other.m_srgb, false);
    }
    return *this;
}

Framebuffer::~Framebuffer() {
    reset();
}

void Framebuffer::reset() noexcept {
    if (m_colorRenderbuffer != 0) {
        glDeleteRenderbuffers(1, &m_colorRenderbuffer);
        m_colorRenderbuffer = 0;
    }
    if (m_colorTexture != 0) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    if (m_depthStencilRenderbuffer != 0) {
        glDeleteRenderbuffers(1, &m_depthStencilRenderbuffer);
        m_depthStencilRenderbuffer = 0;
    }
    if (m_depthTexture != 0) {
        glDeleteTextures(1, &m_depthTexture);
        m_depthTexture = 0;
    }
    if (m_framebuffer != 0) {
        glDeleteFramebuffers(1, &m_framebuffer);
        m_framebuffer = 0;
    }
    m_width = 0;
    m_height = 0;
    m_samples = 0;
    m_srgb = false;
    m_hasDepthTexture = false;
}

std::optional<Framebuffer> Framebuffer::create(int width, int height, int samples, bool srgb) {
    if (width <= 0 || height <= 0 || samples < 1) {
        report_error("Framebuffer::create: invalid dimensions or sample count");
        return std::nullopt;
    }

    GLuint framebuffer{0};
    GLuint colorTexture{0};
    GLuint colorRenderbuffer{0};
    GLuint depthStencilRenderbuffer{0};

    glGenFramebuffers(1, &framebuffer);
    if (framebuffer == 0) {
        report_error("Framebuffer::create: glGenFramebuffers failed");
        return std::nullopt;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    const GLenum colorInternalFormat = srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;

    if (samples == 1) {
        glGenTextures(1, &colorTexture);
        if (colorTexture == 0) {
            report_error("Framebuffer::create: glGenTextures failed");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &framebuffer);
            return std::nullopt;
        }
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(colorInternalFormat), width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    } else {
        glGenRenderbuffers(1, &colorRenderbuffer);
        if (colorRenderbuffer == 0) {
            report_error("Framebuffer::create: glGenRenderbuffers failed");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &framebuffer);
            return std::nullopt;
        }
        glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, colorInternalFormat, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    glGenRenderbuffers(1, &depthStencilRenderbuffer);
    if (depthStencilRenderbuffer == 0) {
        report_error("Framebuffer::create: failed to create depth/stencil renderbuffer");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (colorTexture != 0) {
            glDeleteTextures(1, &colorTexture);
        }
        if (colorRenderbuffer != 0) {
            glDeleteRenderbuffers(1, &colorRenderbuffer);
        }
        glDeleteFramebuffers(1, &framebuffer);
        return std::nullopt;
    }
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRenderbuffer);
    if (samples == 1) {
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    } else {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
    }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (colorTexture != 0) {
            glDeleteTextures(1, &colorTexture);
        }
        if (colorRenderbuffer != 0) {
            glDeleteRenderbuffers(1, &colorRenderbuffer);
        }
        glDeleteRenderbuffers(1, &depthStencilRenderbuffer);
        glDeleteFramebuffers(1, &framebuffer);
        report_error("Framebuffer::create: framebuffer incomplete");
        return std::nullopt;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Framebuffer result{framebuffer, colorTexture, colorRenderbuffer, depthStencilRenderbuffer,
                       0, false,
                       width, height, samples, srgb};
    return std::optional<Framebuffer>{std::move(result)};
}

std::optional<Framebuffer> Framebuffer::create_hdr(const HdrConfig& config) {
    if (config.width <= 0 || config.height <= 0 || config.samples < 1) {
        report_error("Framebuffer::create_hdr: invalid dimensions or sample count");
        return std::nullopt;
    }

    GLuint framebuffer{0};
    GLuint colorTexture{0};
    GLuint colorRenderbuffer{0};
    GLuint depthTexture{0};
    GLuint depthStencilRenderbuffer{0};
    bool hasDepthTexture = config.useDepthTexture;

    glGenFramebuffers(1, &framebuffer);
    if (framebuffer == 0) {
        report_error("Framebuffer::create_hdr: glGenFramebuffers failed");
        return std::nullopt;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    if (config.samples == 1) {
        glGenTextures(1, &colorTexture);
        if (colorTexture == 0) {
            report_error("Framebuffer::create_hdr: glGenTextures failed for color");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &framebuffer);
            return std::nullopt;
        }
        glBindTexture(GL_TEXTURE_2D, colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, config.width, config.height, 0,
                     GL_RGBA, GL_HALF_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

        if (config.useDepthTexture) {
            glGenTextures(1, &depthTexture);
            if (depthTexture == 0) {
                report_error("Framebuffer::create_hdr: glGenTextures failed for depth");
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDeleteTextures(1, &colorTexture);
                glDeleteFramebuffers(1, &framebuffer);
                return std::nullopt;
            }
            glBindTexture(GL_TEXTURE_2D, depthTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, config.width, config.height, 0,
                         GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
        } else {
            glGenRenderbuffers(1, &depthStencilRenderbuffer);
            if (depthStencilRenderbuffer == 0) {
                report_error("Framebuffer::create_hdr: failed to create depth/stencil renderbuffer");
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDeleteTextures(1, &colorTexture);
                glDeleteFramebuffers(1, &framebuffer);
                return std::nullopt;
            }
            glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRenderbuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, config.width, config.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }
    } else {
        glGenTextures(1, &colorTexture);
        if (colorTexture == 0) {
            report_error("Framebuffer::create_hdr: glGenTextures failed for color");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &framebuffer);
            return std::nullopt;
        }
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorTexture);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.samples, GL_RGBA16F,
                                config.width, config.height, GL_TRUE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, colorTexture, 0);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);

        if (config.useDepthTexture) {
            glGenTextures(1, &depthTexture);
            if (depthTexture == 0) {
                report_error("Framebuffer::create_hdr: glGenTextures failed for depth");
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDeleteTextures(1, &colorTexture);
                glDeleteFramebuffers(1, &framebuffer);
                return std::nullopt;
            }
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, depthTexture);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, config.samples, GL_DEPTH_COMPONENT24,
                                    config.width, config.height, GL_TRUE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depthTexture, 0);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        } else {
            glGenRenderbuffers(1, &depthStencilRenderbuffer);
            if (depthStencilRenderbuffer == 0) {
                report_error("Framebuffer::create_hdr: failed to create depth/stencil renderbuffer");
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glDeleteTextures(1, &colorTexture);
                glDeleteFramebuffers(1, &framebuffer);
                return std::nullopt;
            }
            glBindRenderbuffer(GL_RENDERBUFFER, depthStencilRenderbuffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, config.samples, GL_DEPTH24_STENCIL8,
                                             config.width, config.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }
    }

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (colorTexture != 0) {
            glDeleteTextures(1, &colorTexture);
        }
        if (colorRenderbuffer != 0) {
            glDeleteRenderbuffers(1, &colorRenderbuffer);
        }
        if (depthTexture != 0) {
            glDeleteTextures(1, &depthTexture);
        }
        if (depthStencilRenderbuffer != 0) {
            glDeleteRenderbuffers(1, &depthStencilRenderbuffer);
        }
        glDeleteFramebuffers(1, &framebuffer);
        report_error("Framebuffer::create_hdr: framebuffer incomplete");
        return std::nullopt;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Framebuffer result{framebuffer, colorTexture, colorRenderbuffer, depthStencilRenderbuffer,
                       depthTexture, hasDepthTexture,
                       config.width, config.height, config.samples, false};
    return std::optional<Framebuffer>{std::move(result)};
}

std::optional<Framebuffer> Framebuffer::create_ldr_intermediate(int width, int height) {
    if (width <= 0 || height <= 0) {
        report_error("Framebuffer::create_ldr_intermediate: invalid dimensions");
        return std::nullopt;
    }

    GLuint framebuffer{0};
    GLuint colorTexture{0};

    glGenFramebuffers(1, &framebuffer);
    if (framebuffer == 0) {
        report_error("Framebuffer::create_ldr_intermediate: glGenFramebuffers failed");
        return std::nullopt;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &colorTexture);
    if (colorTexture == 0) {
        report_error("Framebuffer::create_ldr_intermediate: glGenTextures failed");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &framebuffer);
        return std::nullopt;
    }
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteTextures(1, &colorTexture);
        glDeleteFramebuffers(1, &framebuffer);
        report_error("Framebuffer::create_ldr_intermediate: framebuffer incomplete");
        return std::nullopt;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Framebuffer result{framebuffer, colorTexture, 0, 0, 0, false,
                       width, height, 1, false};
    return std::optional<Framebuffer>{std::move(result)};
}

bool Framebuffer::resize(int width, int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    auto temp = create(width, height, m_samples, m_srgb);
    if (!temp.has_value()) {
        return false;
    }

    *this = std::move(*temp);
    return true;
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
}

void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool Framebuffer::resolve_to(Framebuffer& destination, GLbitfield mask) const {
    if (!is_valid() || m_samples <= 1) {
        return false;
    }
    if (!destination.is_valid() || destination.m_samples != 1) {
        return false;
    }
    if (m_width != destination.m_width || m_height != destination.m_height) {
        return false;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destination.m_framebuffer);
    glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, destination.m_width, destination.m_height,
                      mask, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return !check_gl_errors("Framebuffer::resolve_to");
}

bool Framebuffer::is_valid() const noexcept {
    return m_framebuffer != 0;
}

GLuint Framebuffer::get_id() const noexcept {
    return m_framebuffer;
}

GLuint Framebuffer::get_color_texture() const noexcept {
    return m_colorTexture;
}

GLuint Framebuffer::get_depth_texture() const noexcept {
    return m_depthTexture;
}

int Framebuffer::get_width() const noexcept {
    return m_width;
}

int Framebuffer::get_height() const noexcept {
    return m_height;
}

int Framebuffer::get_samples() const noexcept {
    return m_samples;
}

} // namespace opengl
