#ifndef ENGINE_NAMED_REGISTRY_HPP
#define ENGINE_NAMED_REGISTRY_HPP

#include <engine/errors/AlreadyRegistered.hpp>

#include <entt/container/dense_map.hpp>
#include <entt/entity/storage.hpp>

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace engine {

    template <typename T>
    struct named_storage {

    public:
        using size_type = std::size_t;
        using value_type = T;

    private:
        using table_type = entt::storage<value_type>;

    public:
        using entity_type = typename table_type::entity_type;

    private:
        struct StringHasher : public std::hash<std::string_view> {
            using is_transparent = std::string_view;
        };
        using index_type = entt::dense_map<std::string, entity_type, StringHasher, std::equal_to<>>;

    public:
        template <typename... Args>
        [[nodiscard]] std::pair<entity_type, value_type &> registre(std::string name, Args &&...args)
        {
            if (auto it = m_names.lower_bound(name); it == m_names.end() || it->first != name) {
                auto const index = m_storage.size();
                m_names.emplace_hint(it, std::make_pair(std::move(name), index));
                return { index, m_storage.emplace_back(std::forward<Args>(args)...) };
            } else {
                throw engine::errors::AlreadyRegistered {};
            }
        }

        [[nodiscard]] entity_type index(std::string_view name) const noexcept
        {
            auto const it = m_names.find(name);
            if (it == m_names.end())
                return entt::null;
            else
                return it->second;
        }

        [[nodiscard]] value_type &get(entity_type idx) noexcept
        {
            return m_storage.get(idx);
        }

        [[nodiscard]] value_type const &get(entity_type idx) const noexcept
        {
            return m_storage.get(idx);
        }

        auto const &storage() const
        {
            return m_storage;
        }

        auto const &names() const
        {
            return m_names;
        }

        template <typename F>
        void each(F &&func)
        {
            if constexpr (std::is_invocable_v<F, value_type &>) {
                for (auto &[idx, value] : m_storage.each())
                    func(value);
            } else if constexpr (std::is_invocable_v<F, entity_type, value_type &>) {
                for (auto &[idx, value] : m_storage.each())
                    func(idx, value);
            } else {
                for (auto const &[name, idx] : m_names)
                    func(std::string_view { name }, idx, m_storage[idx]);
            }
        }

        template <typename F>
        void each(F &&func) const
        {
            if constexpr (std::is_invocable_v<F, value_type const &>) {
                for (auto const &[idx, value] : m_storage.each())
                    func(value);
            } else if constexpr (std::is_invocable_v<F, entity_type, value_type const &>) {
                for (auto const &[idx, value] : m_storage.each())
                    func(idx, value);
            } else {
                for (auto const &[name, idx] : m_names)
                    func(std::string_view { name }, idx, m_storage.get(idx));
            }
        }

        [[nodiscard]] size_type size() const noexcept
        {
            return m_storage.size();
        }

    private:
        table_type m_storage;
        index_type m_names;
    };

}
#endif