#ifndef ENGINE_BLOCK_HPP
#define ENGINE_BLOCK_HPP

namespace engine {

    struct BlockType;

    struct Block {
        engine::BlockType *type;
        void *data;
    };

} // namespace engine

#endif