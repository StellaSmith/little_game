#ifndef UTILS_MEMORY_HPP
#define UTILS_MEMORY_HPP

#include <cstddef>
#include <memory>

namespace utils {

    struct VoidDeleter {
        void operator()(void *p) const noexcept
        {
            ::operator delete(p);
        }
    };

    struct FreeDeleter {
        void operator()(void *p) const noexcept
        {
            std::free(p);
        }
    };

    inline static std::unique_ptr<void, VoidDeleter> allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t))
    {
        return std::unique_ptr<void, VoidDeleter> { ::operator new (bytes, std::align_val_t { alignment }) };
    }

}

#endif