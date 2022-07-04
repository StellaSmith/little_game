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

    template <typename Alloc, typename... Args>
    auto new_object(Alloc &&allocator, Args &&...args)
    {
        if constexpr (std::allocator_traits<Alloc>::is_always_equal::value && std::is_const_v<std::remove_reference_t<Alloc>>) {
            // we need a mutable reference
            // since the allocator is empty copying should be pretty cheap
            auto copy = allocator;
            return new_object(copy, std::forward<Args>(args)...);
        } else {
            auto ptr = std::allocator_traits<Alloc>::allocate(allocator, 1);
            try {
                std::allocator_traits<Alloc>::construct(allocator, ptr, std::forward<Args>(args)...);
            } catch (...) {
                std::allocator_traits<Alloc>::deallocate(allocator, ptr, 1);
                throw;
            }
            return ptr;
        }
    }

    template <typename Alloc>
    void delete_object(Alloc &&allocator, typename std::allocator_traits<Alloc>::pointer ptr) noexcept
    {
        if constexpr (std::allocator_traits<Alloc>::is_always_equal::value && std::is_const_v<std::remove_reference_t<Alloc>>) {
            // we need a mutable reference
            // since the allocator is empty copying should be pretty cheap
            auto copy = allocator;
            return delete_object(copy, ptr);
        } else {
            std::allocator_traits<Alloc>::destroy(allocator, ptr);
            std::allocator_traits<Alloc>::deallocate(allocator, ptr, 1);
        }
    }
}

#endif