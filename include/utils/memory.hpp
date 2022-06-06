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
    requires(!std::allocator_traits<Alloc>::is_always_equal::value) auto new_object(Alloc &allocator, Args &&...args) -> typename std::allocator_traits<Alloc>::pointer
    {
        auto ptr = std::allocator_traits<Alloc>::allocate(allocator, 1);
        try {
            std::allocator_traits<Alloc>::construct(allocator, ptr, std::forward<Args>(args)...);
        } catch (...) {
            std::allocator_traits<Alloc>::deallocate(allocator, ptr, 1);
            throw;
        }
        return ptr;
    }

    template <typename Alloc>
    requires(!std::allocator_traits<Alloc>::is_always_equal::value) void delete_object(Alloc &allocator, typename std::allocator_traits<Alloc>::pointer ptr) noexcept
    {
        std::allocator_traits<Alloc>::destroy(allocator, ptr);
        std::allocator_traits<Alloc>::deallocate(allocator, ptr, 1);
    }

    template <typename Alloc, typename... Args>
    requires(std::allocator_traits<Alloc>::is_always_equal::value) auto new_object(Alloc allocator, Args &&...args) -> typename std::allocator_traits<Alloc>::pointer
    {

        return new_object(allocator, std::forward<Args>(args)...);
    }

    template <typename Alloc>
    requires(std::allocator_traits<Alloc>::is_always_equal::value) void delete_object(Alloc allocator, typename std::allocator_traits<Alloc>::pointer ptr) noexcept
    {
        delete_object(allocator, ptr);
    }

}

#endif