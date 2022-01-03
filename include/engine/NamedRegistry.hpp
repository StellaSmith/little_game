#ifndef ENGINE_BLOCKREGISTRY_HPP
#define ENGINE_BLOCKREGISTRY_HPP

#include "errors/AlreadyRegistered.hpp"
#include <absl/container/flat_hash_map.h>

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace engine {

    template <typename T, typename U>
    auto &const_as(U &obj) noexcept
    {
        if (std::is_const_v<T> && std::is_volatile_v<T>)
            return static_cast<U const volatile &>(obj);
        else if (std::is_const_v<T>)
            return static_cast<U const &>(obj);
        else if (std::is_volatile_v<T>)
            return static_cast<U volatile &>(obj);
        else
            return obj;
    }

    template <typename T>
    struct NamedRegistry {

    public:
        using size_type = std::uint32_t;
        using value_type = std::remove_cv_t<T>;

    private:
        using table_type = std::vector<value_type>;
        using index_type = absl::flat_hash_map<std::string, size_type>;

    public:
        template <typename U, typename... Args, std::enable_if_t<std::is_base_of_v<value_type, U>, int> SFINAE = 0>
        [[nodiscard]] auto &registre(std::string_view name, Args &&...args)
        {
            if (auto [start, stop] = m_index.equal_range(absl::string_view { name.data(), name.size() }); start == stop || start->first != name) {
                m_index.emplace_hint(start, std::make_pair(std::string { name }, m_table.size()));
                return const_as<value_type>(m_table.emplace_back(std::forward<Args>(args)...));
            } else {
                throw engine::errors::AlreadyRegistered {};
            }
        }

        template <typename... Args>
        [[nodiscard]] value_type &registre(std::string_view name, Args &&...args)
        {
            return registre<value_type>(name, std::forward<Args>(args)...);
        }

        [[nodiscard]] size_type index(std::string_view name) const noexcept
        {
            auto const it = m_index.find(absl::string_view { name.data(), name.size() });
            assert(it != m_index.end());
            return it->second;
        }

        [[nodiscard]] value_type &operator[](std::uint32_t idx) const noexcept
        {
            assert(idx < m_table.size());
            return at(idx);
        }

        [[nodiscard]] value_type &operator[](std::string_view name) const noexcept
        {
            return at(index(name));
        }

        [[nodiscard]] value_type &at(std::string_view name) const noexcept
        {
            return at(index(name));
        }

        template <typename U,
            std::enable_if_t<std::is_base_of_v<value_type, U>, int> SFINAE = 0>
        [[nodiscard]] auto &at(std::string_view name) const noexcept
        {
            return const_as<value_type>(dynamic_cast<T &>(at(name)));
        }

        [[nodiscard]] value_type &at(std::uint32_t idx) noexcept
        {
            assert(idx < m_table.size());
            return m_table[idx];
        }

        [[nodiscard]] value_type const &at(std::uint32_t idx) const noexcept
        {
            assert(idx < m_table.size());
            return m_table[idx];
        }

        struct iterator {
            explicit iterator(typename table_type::const_iterator it) noexcept
                : m_base(it)
            {
            }

            [[nodiscard]] value_type &operator*() const noexcept
            {
                return *m_base;
            }

            [[nodiscard]] value_type *const &operator->() const noexcept
            {
                return &*m_base;
            }

            iterator &operator++() noexcept
            {
                ++m_base;
                return *this;
            }

            iterator operator++(int) noexcept
            {

                return iterator { m_base++ };
            }

            iterator &operator--() noexcept
            {
                --m_base;
                return *this;
            }

            iterator operator--(int) noexcept
            {

                return iterator { m_base-- };
            }

            [[nodiscard]] bool operator!=(iterator const &rhs) const noexcept
            {
                return m_base != rhs.m_base;
            }

            [[nodiscard]] bool operator==(iterator const &rhs) const noexcept
            {
                return m_base == rhs.m_base;
            }

            [[nodiscard]] typename table_type::const_iterator base() const noexcept { return m_base; }

        private:
            typename table_type::const_iterator m_base;
        };

        using const_iterator = iterator;

        [[nodiscard]] iterator begin() const noexcept
        {
            return iterator { m_table.begin() };
        }

        [[nodiscard]] iterator end() const noexcept
        {
            return iterator { m_table.begin() };
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return m_table.size();
        }

        [[nodiscard]] std::size_t empty() const noexcept
        {
            return m_table.empty();
        }

    private:
        table_type m_table;
        index_type m_index;
    };

}
#endif