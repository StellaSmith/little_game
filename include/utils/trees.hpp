#ifndef UTILS_TREES_HPP
#define UTILS_TREES_HPP

#include <functional>
#include <utils/compressed_pair.hpp>
#include <utils/memory.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <utility>

namespace utils {

    template <typename T>
    struct intrusive_dynamic_tree {
        struct node_type {
            T value;
            std::vector<node_type *> children;
        };

        using value_type = T;

        node_type *head = nullptr;
    };

    template <typename T, std::size_t N>
    struct intrusive_tree {
        struct node_type {
            constexpr node_type() noexcept = default;

            template <typename... Args>
            constexpr node_type(std::in_place_t, Args &&...args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
                : value(std::forward<Args>(args)...)
            {
            }

            T value;
            std::array<node_type *, N> children = {};
        };

        using value_type = T;

        node_type *head = nullptr;
    };

    namespace impl {
        template <typename T>
        struct is_intrusive_tree : std::false_type {
        };

        template <typename T, std::size_t N>
        struct is_intrusive_tree<utils::intrusive_tree<T, N>> : std::true_type {
        };

        template <typename T>
        struct is_dynamic_intrusive_tree : std::false_type {
        };

        template <typename T>
        struct is_dynamic_intrusive_tree<utils::intrusive_dynamic_tree<T>> : std::true_type {
        };
    }

    struct depth_first {
        static constexpr inline struct pre_order_t {
            explicit pre_order_t() = default;
        } pre_order {};

        static constexpr inline struct post_order_t {
            explicit post_order_t() = default;
        } post_order {};

        static constexpr inline struct in_order_t {
            explicit in_order_t() = default;
        } in_order {};

        static constexpr inline struct reverse_pre_order_t {
            explicit reverse_pre_order_t() = default;
        } reverse_pre_order {};

        static constexpr inline struct reverse_post_order_t {
            explicit reverse_post_order_t() = default;
        } reverse_post_order {};

        static constexpr inline struct reverse_in_order_t {
            explicit reverse_in_order_t() = default;
        } reverse_in_order {};
    };

    template <typename F, typename Node>
    constexpr void each(depth_first::pre_order_t, Node *node, F &&func)
    {
        if (node == nullptr) return;
        func(node);
        auto children = node->children;
        std::for_each(std::begin(children), std::end(children), [&func](Node *node) {
            each(depth_first::pre_order, node, func);
        });
    }

    template <typename F, typename Node>
    constexpr void each(depth_first::post_order_t, Node *node, F &&func)
    {
        if (node == nullptr) return;
        auto children = node->children;
        std::for_each(std::begin(children), std::end(children), [&func](Node *node) {
            each(depth_first::post_order, node, func);
        });
        func(node);
    }

    template <typename F, typename Node>
    constexpr void each(depth_first::in_order_t, Node *node, F &&func)
    {
        if (node == nullptr) return;
        auto children = node->children;

        for (std::size_t i = 0; i < std::size(children); ++i) {
            each(depth_first::in_order, node, func);
            if (i == std::size(children) / 2) {
                func(node);
            }
        }
    }

    template <typename F, typename Node>
    constexpr void each(depth_first::reverse_pre_order_t, Node *node, F &&func)
    {
        if (node == nullptr) return;
        func(node);
        auto children = node->children;
        std::for_each(std::rbegin(children), std::rend(children), [&func](Node *node) {
            each(depth_first::reverse_pre_order, node, func);
        });
    }

    template <typename F, typename Node>
    constexpr void each(depth_first::reverse_post_order_t, Node *node, F &&func)
    {
        if (node == nullptr) return;
        auto children = node->children;
        std::for_each(std::rbegin(children), std::rend(children), [&func](Node *node) {
            each(depth_first::reverse_post_order, node, func);
        });
        func(node);
    }

    template <typename F, typename Node>
    constexpr void each(depth_first::reverse_in_order_t, Node *node, F &&func)
    {
        if (node == nullptr) return;
        auto children = node->children;

        for (std::size_t i = std::size(children) - 1;; --i) {
            each(depth_first::reverse_in_order, node, func);
            if (i == (std::size(children) - 1) / 2 + 1) {
                func(node);
            }
            if (i == 0) break;
        }
    }

    struct VectorCompare {
        template <typename T, std::size_t N>
        constexpr std::size_t operator()(std::array<T, N> const &lhs, std::array<T, N> const &rhs) noexcept
        {
            // similar to the _mm(|256|512)_cmple_(.*) Intel intrinsics
            // element wise operator<, returning a bitmask

            std::size_t index = 0;
            std::size_t bit = 1 << N;
            for (std::size_t i = 0; i < N; ++i) {
                index |= (lhs[i] < rhs[i]) << bit;
                bit >>= 1;
            }
            return index;
        }
    };

    template <std::size_t Dimensions, typename K, typename V, typename Compare, typename Allocator>
    requires(Dimensions >= 1 && Dimensions <= CHAR_BIT * sizeof(std::size_t)) struct spatial_tree {
        static constexpr std::size_t dimensions = Dimensions;
        static constexpr std::size_t children = 1 << dimensions;

        using key_type = std::array<K, dimensions>;
        using key_compare = Compare;
        using map_type = V;
        using tree_type = intrusive_tree<std::pair<key_type const, map_type>, children>;
        using node_type = typename tree_type::node_type;
        using value_type = typename tree_type::value_type;
        using allocator_type = typename std::allocator_traits<Allocator>::template rebind_alloc<node_type>;

        constexpr spatial_tree() noexcept(noexcept(Compare()) && (noexcept(Allocator())))
            : spatial_tree(Compare(), Allocator())
        {
        }

        constexpr explicit spatial_tree(Allocator const &alloc) noexcept(noexcept(Compare()))
            : spatial_tree(Compare(), alloc)
        {
        }

        constexpr explicit spatial_tree(Compare const &comp) noexcept(noexcept(Allocator()))
            : spatial_tree(comp, Allocator())
        {
        }

        constexpr explicit spatial_tree(Compare const &comp, const Allocator &alloc) requires(std::is_empty_v<Compare> &&std::is_empty_v<Allocator>)
        {
        }

        constexpr explicit spatial_tree(Compare const &comp, const Allocator &alloc) requires(!std::is_empty_v<Compare> && std::is_empty_v<Allocator>)
            : m_impl(tree_type(), { comp })
        {
        }

        constexpr explicit spatial_tree(Compare const &comp, const Allocator &alloc) requires(std::is_empty_v<Compare> && !std::is_empty_v<Allocator>)
            : m_impl(tree_type(), { alloc })
        {
        }

        constexpr explicit spatial_tree(Compare const &comp, const Allocator &alloc) requires(!std::is_empty_v<Compare> && !std::is_empty_v<Allocator>)
            : m_impl(tree_type(), { alloc, comp })
        {
        }

        template <typename It>
        spatial_tree(It start, It end, Compare const &comp = Compare(), const Allocator &alloc = Allocator())
            : spatial_tree(comp, alloc)
        {
            for (; start != end; ++start)
                emplace(*start);
        }

        template <typename It>
        spatial_tree(It start, std::default_sentinel_t, Compare const &comp = Compare(), const Allocator &alloc = Allocator())
            : spatial_tree(comp, alloc)
        {
            for (; start != std::default_sentinel; ++start)
                emplace(*start);
        }

        node_type *find(node_type *node, key_type const &key) noexcept
        {
            auto const [parent, index] = lower_bound(node, key);
            if (parent->children[index] && equal(parent->children[index]->value.first, key))
                return parent->children[index];
            return nullptr;
        }

        node_type *find(key_type const &key) noexcept
        {
            return find(head(), key);
        }

        node_type const *find(node_type const *node, key_type const &key) const noexcept
        {
            return const_cast<spatial_tree *>(this)->find(node, key);
        }

        node_type const *find(key_type const &key) const noexcept
        {
            return const_cast<spatial_tree *>(this)->find(key);
        }

        std::pair<node_type *, std::size_t> lower_bound(node_type *node, key_type const &key) noexcept
        {
            assert(node != nullptr);
            for (;;) {
                std::size_t const index = compare(node->value.first, key);
                if (node->children[index] == nullptr)
                    return std::make_pair(node, index);
                node = node->children[index];
            }
        }

        std::pair<node_type *, std::size_t> lower_bound(key_type const &key) noexcept
        {
            return lower_bound(head(), key);
        }

        template <typename Algorithm, typename F>
        decltype(auto) each(Algorithm algorithm, F &&func)
        {
            return utils::each(algorithm, head(), [&func](node_type *node) {
                func(node->value);
            });
        }

        template <typename... Args>
        node_type *emplace(Args &&...args)
        {
            auto *node = utils::new_object(get_allocator(), std::in_place, std::forward<Args>(args)...);
            insert(node);
            return node;
        }

        template <typename... Args>
        node_type *emplace_hint(node_type *parent, Args &&...args)
        {
            auto *node = utils::new_object(get_allocator(), std::in_place, std::forward<Args>(args)...);
            insert(parent, node);
            return node;
        }

        void insert(node_type *parent, node_type *node) noexcept
        {
            auto const [real_parent, index] = lower_bound(parent, node->value.first);
            if (real_parent == nullptr)
                head() = node;
            else
                real_parent->children[index] = node;
        }

        void insert(node_type *node) noexcept
        {
            return insert(head(), node);
        }

        node_type *insert(node_type *parent, value_type const &value)
        {
            return emplace_hint(parent, value);
        }

        node_type *insert(node_type *parent, value_type &&value)
        {
            return emplace_hint(parent, static_cast<value_type &&>(value));
        }

        bool erase(key_type const &key) noexcept
        {
            auto const [parent, index] = lower_bound(key);
            if (parent->children[index]->value.first == key) {
                node_type *children[spatial_tree::children];
                for (std::size_t i = 0; i < spatial_tree::children; ++i)
                    children[i] = parent->children[index]->children[i];
                utils::delete_object(get_allocator(), parent->children[index]);
                for (std::size_t i = 0; i < spatial_tree::children; ++i) {
                    if (children[i] == nullptr) continue;
                    insert(parent, children[i]);
                }
                return true;
            } else {
                return false;
            }
        }

        constexpr allocator_type get_allocator() const noexcept
        {
            if constexpr (std::is_empty_v<allocator_type>) {
                return allocator_type();
            } else {
                return m_impl.second.first;
            }
        }

        void clear() noexcept
        {
            utils::each(depth_first::pre_order, head(), [this](node_type *node) {
                utils::delete_object(get_allocator(), node);
            });
        }

        ~spatial_tree()
        {
            clear();
        }

    private:
        constexpr std::size_t compare(key_type const &lhs, key_type const &rhs) noexcept
        {
            if constexpr (std::is_empty_v<key_compare>)
                return key_compare {}(lhs, rhs);
            else
                return m_impl.second.second(lhs, rhs);
        }

        constexpr bool equal(key_type const &lhs, key_type const &rhs) noexcept
        {
            return !compare(lhs, rhs) && !compare(rhs, lhs);
        }

        node_type *&head() noexcept
        {
            return m_impl.first.head;
        }

        node_type const *const &head() const noexcept
        {
            return m_impl.first.head;
        }

    private:
        utils::compressed_pair<tree_type, utils::compressed_pair<allocator_type, key_compare>> m_impl;
    };

    template <typename K, typename V, typename Alloc = std::allocator<std::byte>>
    using bitree = spatial_tree<1, K, V, VectorCompare, Alloc>;

    template <typename K, typename V, typename Alloc = std::allocator<std::byte>>
    using quadtree = spatial_tree<2, K, V, VectorCompare, Alloc>;

    template <typename K, typename V, typename Alloc = std::allocator<std::byte>>
    using octtree = spatial_tree<3, K, V, VectorCompare, Alloc>;

}

#endif
