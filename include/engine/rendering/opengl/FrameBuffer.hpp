#pragma once

#include <engine/rendering/opengl/Object.hpp>

namespace engine::rendering::opengl {
    class FrameBufferContext
        : public engine::rendering::opengl::BoundContext<&glBindFramebuffer> {
    public:
        using base_type = engine::rendering::opengl::BoundContext<&glBindFramebuffer>;
        using base_type::base_type;
    };

    class FrameBuffer : public engine::rendering::opengl::Object<FrameBuffer, FrameBufferContext, &glGenFramebuffers, &glDeleteFramebuffers> {
    public:
        using base_type = engine::rendering::opengl::Object<FrameBuffer, FrameBufferContext, &glGenFramebuffers, &glDeleteFramebuffers>;
        using base_type::base_type;
    };

} // namespace engine::rendering::opengl
