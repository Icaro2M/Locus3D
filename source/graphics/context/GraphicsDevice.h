#pragma once

namespace locus::graphics
{

    class GraphicsDevice
    {
    public:
        GraphicsDevice() = default;
        ~GraphicsDevice() = default;

        GraphicsDevice(const GraphicsDevice&) = delete;
        GraphicsDevice& operator=(const GraphicsDevice&) = delete;

        GraphicsDevice(GraphicsDevice&&) = delete;
        GraphicsDevice& operator=(GraphicsDevice&&) = delete;
    };

}