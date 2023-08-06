#ifndef ENGINE_SYSTEM_PAGE_SIZE_HPP
#define ENGINE_SYSTEM_PAGE_SIZE_HPP

#include <cstddef>

namespace engine::system {
    /**
     * obtain the page size of the running system
     */
    [[nodiscard, gnu::pure]] std::size_t page_size() noexcept;
} // namespace engine

#endif