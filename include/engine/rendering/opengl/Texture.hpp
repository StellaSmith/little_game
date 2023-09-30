

#pragma once
#include <glad/glad.h>

#include <engine/rendering/opengl/Object.hpp>

namespace engine::rendering::opengl {
    class TextureContext
        : public engine::rendering::opengl::BoundContext<&glBindTexture, GLenum> {
    public:
        using base_type = engine::rendering::opengl::BoundContext<&glBindTexture, GLenum>;
        using base_type::base_type;

        GLenum target()
        {
            return std::get<0>(base_type::arguments());
        }

        TextureContext &setParameter(GLenum parameter, GLfloat value) &
        {
            glTexParameterf(this->target(), parameter, value);
            return *this;
        }

        TextureContext setParameter(GLenum parameter, GLfloat value) &&
        {
            glTexParameterf(this->target(), parameter, value);
            return std::move(*this);
        }

        TextureContext &setParamater(GLenum parameter, GLint value) &
        {
            glTexParameteri(this->target(), parameter, value);
            return *this;
        }

        TextureContext setParamater(GLenum parameter, GLint value) &&
        {
            glTexParameteri(this->target(), parameter, value);
            return std::move(*this);
        }
    };

    class Texture
        : public engine::rendering::opengl::Object<Texture, TextureContext, &glGenTextures, &glDeleteTextures> {
    public:
        using base_type = engine::rendering::opengl::Object<Texture, TextureContext, &glGenTextures, &glDeleteTextures>;
        using base_type::base_type;
    };
} // namespace engine::rendering::opengl
