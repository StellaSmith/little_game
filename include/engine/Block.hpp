#ifndef ENGINE_BLOCK_T_HPP
#define ENGINE_BLOCK_T_HPP

#include <cstdint>

namespace engine {

    struct Block {
        std::uint32_t id = 0;
        std::uint32_t subid = 0;

        union data_t {
            std::uint64_t u64;
            double f64;
            void *ptr;
        } data { 0ull };
    };

} // namespace engine

#endif
