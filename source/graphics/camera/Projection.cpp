/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/camera/Projection.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

namespace locus::graphics
{
    void Projection::set_perspective(float verticalFovRadians, float aspectRatio, float nearPlane, float farPlane)
    {
        type_ = ProjectionType::Perspective;
        verticalFovRadians_ = verticalFovRadians;
        // Avoid degenerate projection matrices when the viewport is minimized.
        aspectRatio_ = std::max(aspectRatio, 0.0001f);
        nearPlane_ = nearPlane;
        farPlane_ = farPlane;
    }

    void Projection::set_orthographic(float height, float aspectRatio, float nearPlane, float farPlane)
    {
        type_ = ProjectionType::Orthographic;
        // Keep the view volume positive so glm::ortho receives valid bounds.
        orthographicHeight_ = std::max(height, 0.0001f);
        aspectRatio_ = std::max(aspectRatio, 0.0001f);
        nearPlane_ = nearPlane;
        farPlane_ = farPlane;
    }

    void Projection::set_aspect_ratio(float aspectRatio)
    {
        aspectRatio_ = std::max(aspectRatio, 0.0001f);
    }

    ProjectionType Projection::type() const
    {
        return type_;
    }

    float Projection::aspect_ratio() const
    {
        return aspectRatio_;
    }

    float Projection::near_plane() const
    {
        return nearPlane_;
    }

    float Projection::far_plane() const
    {
        return farPlane_;
    }

    float Projection::vertical_fov_radians() const
    {
        return verticalFovRadians_;
    }

    float Projection::orthographic_height() const
    {
        return orthographicHeight_;
    }

    glm::mat4 Projection::matrix() const
    {
        if (type_ == ProjectionType::Perspective)
        {
            return glm::perspective(verticalFovRadians_, aspectRatio_, nearPlane_, farPlane_);
        }

        const float halfHeight = orthographicHeight_ * 0.5f;
        const float halfWidth = halfHeight * aspectRatio_;

        // Build symmetric bounds around the view center.
        return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane_, farPlane_);
    }
}   
