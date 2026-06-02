/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::graphics
{
    /**
     * @brief Placeholder for backend device-level resource management.
     *
     * The first graphics layer keeps context ownership separate from future
     * GPU resource orchestration.
     */
    class GraphicsDevice
    {
    public:
        /**
         * @brief Creates an empty graphics device placeholder.
         */
        GraphicsDevice() = default;

        /**
         * @brief Destroys the graphics device placeholder.
         */
        ~GraphicsDevice() = default;

        GraphicsDevice(const GraphicsDevice&) = delete;
        GraphicsDevice& operator=(const GraphicsDevice&) = delete;

        GraphicsDevice(GraphicsDevice&&) = delete;
        GraphicsDevice& operator=(GraphicsDevice&&) = delete;
    };

}
