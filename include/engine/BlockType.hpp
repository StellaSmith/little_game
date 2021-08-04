#ifndef ENGINE_BLOCKTYPE_HPP
#define ENGINE_BLOCKTYPE_HPP

#include <engine/Sides.hpp>
#include <engine/rendering/Mesh.hpp>
#include <glm/fwd.hpp>

namespace engine {
    class Game;
    struct Block;

    struct BlockType {
        virtual void Initialize(engine::Game &);
        virtual engine::rendering::Mesh GetSolidMesh(engine::Block const &, engine::Sides) const;
        virtual engine::rendering::Mesh GetTranslucentMesh(engine::Block const &, engine::Sides) const;
        virtual engine::Sides GetSolidSides(engine::Block const &) const;
        virtual glm::u8vec4 GetProducedLight(engine::Block const &) const;

        virtual ~BlockType();
    };

} // namespace engine

#endif