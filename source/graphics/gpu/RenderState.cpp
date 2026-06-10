/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/gpu/RenderState.h"

#include <glad/glad.h>

namespace locus::graphics
{
    void RenderState::set_depth_test(bool enabled)
    {
        if (enabled)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }
    }

    void RenderState::set_depth_write(bool enabled)
    {
        glDepthMask(enabled ? GL_TRUE : GL_FALSE);
    }

    void RenderState::set_depth_func(DepthFunc func)
    {
        glDepthFunc(gl_depth_func(func));
    }

    void RenderState::set_blend(bool enabled)
    {
        if (enabled)
        {
            glEnable(GL_BLEND);
        }
        else
        {
            glDisable(GL_BLEND);
        }
    }

    void RenderState::set_blend_func(BlendFactor source, BlendFactor destination)
    {
        glBlendFunc(
            gl_blend_factor(source),
            gl_blend_factor(destination)
        );
    }

    void RenderState::set_cull_face(bool enabled)
    {
        if (enabled)
        {
            glEnable(GL_CULL_FACE);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }
    }

    void RenderState::set_cull_face_mode(CullFace face)
    {
        glCullFace(gl_cull_face(face));
    }

    void RenderState::set_front_face(FrontFace face)
    {
        glFrontFace(gl_front_face(face));
    }

    void RenderState::set_polygon_mode(RenderPolygonMode mode)
    {
        glPolygonMode(GL_FRONT_AND_BACK, gl_polygon_mode(mode));
    }

    void RenderState::set_line_width(float width)
    {
        glLineWidth(width);
    }

    void RenderState::set_point_size(float size)
    {
        glPointSize(size);
    }

    void RenderState::set_viewport(i32 x, i32 y, i32 width, i32 height)
    {
        glViewport(x, y, width, height);
    }

    void RenderState::set_scissor_test(bool enabled)
    {
        if (enabled)
        {
            glEnable(GL_SCISSOR_TEST);
        }
        else
        {
            glDisable(GL_SCISSOR_TEST);
        }
    }

    void RenderState::set_scissor(i32 x, i32 y, i32 width, i32 height)
    {
        glScissor(x, y, width, height);
    }

    void RenderState::set_color_write(bool red, bool green, bool blue, bool alpha)
    {
        glColorMask(
            red ? GL_TRUE : GL_FALSE,
            green ? GL_TRUE : GL_FALSE,
            blue ? GL_TRUE : GL_FALSE,
            alpha ? GL_TRUE : GL_FALSE
        );
    }

    void RenderState::reset_default()
    {
        // Keep this baseline explicit so passes can recover from specialized state.
        set_depth_test(true);
        set_depth_write(true);
        set_depth_func(DepthFunc::Less);

        set_blend(false);
        set_blend_func(
            BlendFactor::SourceAlpha,
            BlendFactor::OneMinusSourceAlpha
        );

        set_cull_face(false);
        set_cull_face_mode(CullFace::Back);
        set_front_face(FrontFace::CounterClockwise);

        set_polygon_mode(RenderPolygonMode::Fill);
        set_line_width(1.0f);
        set_point_size(1.0f);

        set_scissor_test(false);
        set_color_write(true, true, true, true);
    }

    u32 RenderState::gl_depth_func(DepthFunc func)
    {
        switch (func)
        {
        case DepthFunc::Never:
            return GL_NEVER;

        case DepthFunc::Less:
            return GL_LESS;

        case DepthFunc::Equal:
            return GL_EQUAL;

        case DepthFunc::LessEqual:
            return GL_LEQUAL;

        case DepthFunc::Greater:
            return GL_GREATER;

        case DepthFunc::NotEqual:
            return GL_NOTEQUAL;

        case DepthFunc::GreaterEqual:
            return GL_GEQUAL;

        case DepthFunc::Always:
            return GL_ALWAYS;
        }

        return GL_LESS;
    }

    u32 RenderState::gl_blend_factor(BlendFactor factor)
    {
        switch (factor)
        {
        case BlendFactor::Zero:
            return GL_ZERO;

        case BlendFactor::One:
            return GL_ONE;

        case BlendFactor::SourceAlpha:
            return GL_SRC_ALPHA;

        case BlendFactor::OneMinusSourceAlpha:
            return GL_ONE_MINUS_SRC_ALPHA;

        case BlendFactor::DestinationAlpha:
            return GL_DST_ALPHA;

        case BlendFactor::OneMinusDestinationAlpha:
            return GL_ONE_MINUS_DST_ALPHA;

        case BlendFactor::SourceColor:
            return GL_SRC_COLOR;

        case BlendFactor::OneMinusSourceColor:
            return GL_ONE_MINUS_SRC_COLOR;

        case BlendFactor::DestinationColor:
            return GL_DST_COLOR;

        case BlendFactor::OneMinusDestinationColor:
            return GL_ONE_MINUS_DST_COLOR;
        }

        return GL_ONE;
    }

    u32 RenderState::gl_cull_face(CullFace face)
    {
        switch (face)
        {
        case CullFace::Back:
            return GL_BACK;

        case CullFace::Front:
            return GL_FRONT;

        case CullFace::FrontAndBack:
            return GL_FRONT_AND_BACK;
        }

        return GL_BACK;
    }

    u32 RenderState::gl_front_face(FrontFace face)
    {
        switch (face)
        {
        case FrontFace::CounterClockwise:
            return GL_CCW;

        case FrontFace::Clockwise:
            return GL_CW;
        }

        return GL_CCW;
    }

    u32 RenderState::gl_polygon_mode(RenderPolygonMode mode)
    {
        switch (mode)
        {
        case RenderPolygonMode::Fill:
            return GL_FILL;

        case RenderPolygonMode::Line:
            return GL_LINE;

        case RenderPolygonMode::Point:
            return GL_POINT;
        }

        return GL_FILL;
    }
}
