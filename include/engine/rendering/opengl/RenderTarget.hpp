#pragma once

#include <engine/rendering/IRenderTarget.hpp>
#include <engine/rendering/opengl/FrameBuffer.hpp>
#include <engine/rendering/opengl/Renderer.hpp>

namespace engine::rendering::opengl {

    class RenderTarget
        : public virtual engine::rendering::IRenderTarget,
          public engine::rendering::opengl::FrameBuffer {
        using engine::rendering::opengl::FrameBuffer::FrameBuffer;
    };
} // namespace engine::rendering::opengl
