#pragma once

#include "splinetexture.h"

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QDebug>
#include <array>

namespace XmbSpline
{
inline constexpr const char *gpuVertex = R"GLSL(
#version 330 core
void main() {
    vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

inline constexpr const char *gpuFragment = R"GLSL(
#version 330 core
uniform sampler2D descriptors;
uniform float flow;
layout(location = 0) out float value;
void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    vec2 uv = vec2(pixel) / vec2(1023.0, 127.0);
    float u = uv.x;
    float z = uv.y * 2.0 - 1.0;
    float band = sin(flow * 0.25 + z * 1.7 + u * 6.2) * 0.200;
    float secondary = cos(z * 7.0 + u * 4.8 + flow * 0.09) * 0.025;
    float travel =
        sin((u * 3.14159265358979323846 * 1.3 + z * 0.8) - flow * 0.25) * 0.014 * 0.12 +
        sin((u * 3.14159265358979323846 * 2.8 - z * 1.2) + flow * 0.15) * 0.008;
    float perturb = 0.0998587 * 0.07 *
        sin((u * (4.0 + 0.306001 * 2.0) + z * 4.0 - flow * 0.6) * 4.07658);
    value = 0.45 * (band + secondary + texelFetch(descriptors, pixel, 0).r)
        + 0.55 * (travel + perturb);
}
)GLSL";

class GpuGenerator
{
public:
    bool initialize(QOpenGLFunctions_3_3_Core *gl, GLuint destination)
    {
        m_gl = gl;
        if (!gl || !destination)
            return false;
        if (!m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, gpuVertex)
            || !m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, gpuFragment)
            || !m_program.link())
        {
            qWarning() << "XMB GPU spline:" << m_program.log();
            return false;
        }
        GLint oldUnpackBuffer = 0;
        constexpr std::array<GLenum, 5> unpackNames = {
            GL_UNPACK_ALIGNMENT, GL_UNPACK_ROW_LENGTH, GL_UNPACK_SKIP_ROWS,
            GL_UNPACK_SKIP_PIXELS, GL_UNPACK_SWAP_BYTES};
        std::array<GLint, 5> unpackValues = {};
        gl->glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &oldUnpackBuffer);
        gl->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        for (std::size_t i = 0; i < unpackNames.size(); ++i)
        {
            gl->glGetIntegerv(unpackNames[i], &unpackValues[i]);
            gl->glPixelStorei(unpackNames[i], i == 0 ? 4 : 0);
        }
        GLint oldTexture = 0;
        GLint oldFramebuffer = 0;
        gl->glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
        gl->glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFramebuffer);
        gl->glGenTextures(1, &m_descriptors);
        gl->glBindTexture(GL_TEXTURE_2D, m_descriptors);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0,
                         GL_RED, GL_FLOAT, descriptors().data());
        gl->glGenVertexArrays(1, &m_vao);
        gl->glGenFramebuffers(1, &m_framebuffer);
        gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer);
        gl->glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                  GL_TEXTURE_2D, destination, 0);
        const bool complete = gl->glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(oldFramebuffer));
        gl->glBindTexture(GL_TEXTURE_2D, GLuint(oldTexture));
        for (std::size_t i = 0; i < unpackNames.size(); ++i)
            gl->glPixelStorei(unpackNames[i], unpackValues[i]);
        gl->glBindBuffer(GL_PIXEL_UNPACK_BUFFER, GLuint(oldUnpackBuffer));
        if (!complete)
            cleanup();
        return complete;
    }
    bool render(float flow)
    {
        if (!m_gl || !m_framebuffer || !m_vao)
            return false;
        GLint oldTexture = 0;
        GLint oldFramebuffer = 0;
        GLint oldViewport[4] = {};
        GLboolean oldBlend = m_gl->glIsEnabled(GL_BLEND);
        GLboolean oldDepth = m_gl->glIsEnabled(GL_DEPTH_TEST);
        GLboolean oldScissor = m_gl->glIsEnabled(GL_SCISSOR_TEST);
        GLboolean oldCull = m_gl->glIsEnabled(GL_CULL_FACE);
        GLboolean oldDiscard = m_gl->glIsEnabled(GL_RASTERIZER_DISCARD);
        GLboolean oldStencil = m_gl->glIsEnabled(GL_STENCIL_TEST);
        GLboolean oldLogic = m_gl->glIsEnabled(GL_COLOR_LOGIC_OP);
        GLint oldPolygonMode[2] = {};
        GLint oldSampler = 0;
        GLint oldProgram = 0;
        GLint oldVao = 0;
        GLint oldActiveTexture = 0;
        GLboolean oldColorMask[4] = {};
        m_gl->glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldTexture);
        m_gl->glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFramebuffer);
        m_gl->glGetIntegerv(GL_VIEWPORT, oldViewport);
        m_gl->glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
        m_gl->glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVao);
        m_gl->glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActiveTexture);
        m_gl->glGetIntegerv(GL_SAMPLER_BINDING, &oldSampler);
        m_gl->glGetIntegerv(GL_POLYGON_MODE, oldPolygonMode);
        m_gl->glGetBooleanv(GL_COLOR_WRITEMASK, oldColorMask);
        m_gl->glBindVertexArray(m_vao);
        m_gl->glDisable(GL_BLEND);
        m_gl->glDisable(GL_DEPTH_TEST);
        m_gl->glDisable(GL_SCISSOR_TEST);
        m_gl->glDisable(GL_CULL_FACE);
        m_gl->glDisable(GL_RASTERIZER_DISCARD);
        m_gl->glDisable(GL_STENCIL_TEST);
        m_gl->glDisable(GL_COLOR_LOGIC_OP);
        m_gl->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        m_gl->glBindSampler(GLuint(oldActiveTexture - GL_TEXTURE0), 0);
        m_gl->glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        m_gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer);
        m_gl->glViewport(0, 0, width, height);
        m_program.bind();
        m_program.setUniformValue("flow", flow);
        m_program.setUniformValue("descriptors", oldActiveTexture - GL_TEXTURE0);
        m_gl->glBindTexture(GL_TEXTURE_2D, m_descriptors);
        m_gl->glDrawArrays(GL_TRIANGLES, 0, 3);
        m_program.release();
        m_gl->glBindTexture(GL_TEXTURE_2D, GLuint(oldTexture));
        m_gl->glBindSampler(GLuint(oldActiveTexture - GL_TEXTURE0), GLuint(oldSampler));
        m_gl->glActiveTexture(GLenum(oldActiveTexture));
        m_gl->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GLuint(oldFramebuffer));
        m_gl->glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
        m_gl->glBindVertexArray(GLuint(oldVao));
        if (oldBlend) m_gl->glEnable(GL_BLEND);
        if (oldDepth) m_gl->glEnable(GL_DEPTH_TEST);
        if (oldScissor) m_gl->glEnable(GL_SCISSOR_TEST);
        if (oldCull) m_gl->glEnable(GL_CULL_FACE);
        if (oldDiscard) m_gl->glEnable(GL_RASTERIZER_DISCARD);
        if (oldStencil) m_gl->glEnable(GL_STENCIL_TEST);
        if (oldLogic) m_gl->glEnable(GL_COLOR_LOGIC_OP);
        m_gl->glPolygonMode(GL_FRONT_AND_BACK, GLenum(oldPolygonMode[0]));
        m_gl->glColorMask(oldColorMask[0], oldColorMask[1], oldColorMask[2], oldColorMask[3]);
        m_gl->glUseProgram(GLuint(oldProgram));
        return true;
    }

    void cleanup()
    {
        if (!m_gl)
            return;
        if (m_descriptors)
            m_gl->glDeleteTextures(1, &m_descriptors);
        if (m_vao)
            m_gl->glDeleteVertexArrays(1, &m_vao);
        if (m_framebuffer)
            m_gl->glDeleteFramebuffers(1, &m_framebuffer);
        m_descriptors = m_vao = m_framebuffer = 0;
    }

private:
    QOpenGLFunctions_3_3_Core *m_gl = nullptr;
    QOpenGLShaderProgram m_program;
    GLuint m_descriptors = 0;
    GLuint m_vao = 0;
    GLuint m_framebuffer = 0;
};
}
