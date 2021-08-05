#ifndef ENGINE_BLOCKREGISTRY_HPP
#define ENGINE_BLOCKREGISTRY_HPP

#include <engine/errors/AlreadyRegistered.hpp>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace engine {

    struct BlockType;
    struct Game;

    class BlockRegistry {

        struct Item {
            std::string name; // must be unique withing the registry
            std::unique_ptr<engine::BlockType> blockType;
        };

    public:
        template <typename T>
        std::enable_if_t<std::is_base_of_v<engine::BlockType, T>, std::remove_cv_t<T>> *Register(std::string_view name)
        {
            auto it = std::lower_bound(m_registeredBlocks.begin(), m_registeredBlocks.end(), name, [](Item const &item, std::string_view name) {
                return item.name < name;
            });
            if (it != m_registeredBlocks.end() && it->name == name)
                throw engine::errors::AlreadyRegistered { "BlockType" };
            it = m_registeredBlocks.emplace(it, std::string(name), std::make_unique<std::remove_cv_t<T>>());
            return it->blockType.get();
        }

        template <typename T>
        std::enable_if_t<std::is_base_of_v<engine::BlockType, T>, T> *Get(std::string_view name)
        {
            auto *p = Get(name);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, engine::BlockType>)
                return p;
            else
                return dynamic_cast<T *>(p);
        }

        BlockType *Get(std::string_view name);

        struct const_iterator;

        struct iterator {
            friend const_iterator;
            using value_type = std::pair<std::string_view, BlockType *>;

            explicit iterator(Item *p) noexcept
                : m_cur(p)
            {
            }

            bool operator!=(iterator const &other) const noexcept
            {
                return m_cur != other.m_cur;
            }

            value_type operator*() const noexcept
            {
                return { m_cur->name, m_cur->blockType.get() };
            }

            iterator &operator++() noexcept
            {
                ++m_cur;
                return *this;
            }

            operator const_iterator() const noexcept
            {
                return const_iterator(m_cur);
            }

        private:
            Item *m_cur = nullptr;
        };

        struct const_iterator {
            using value_type = std::pair<std::string_view, BlockType const *>;

            explicit const_iterator(Item const *p) noexcept
                : m_cur(p)
            {
            }

            value_type operator*() const noexcept
            {
                return { m_cur->name, m_cur->blockType.get() };
            }

            const_iterator &operator++() noexcept
            {
                ++m_cur;
                return *this;
            }

        private:
            Item const *m_cur = nullptr;
        };

        iterator begin() noexcept
        {
            return iterator(m_registeredBlocks.data());
        }

        const_iterator begin() const noexcept
        {
            return const_iterator(m_registeredBlocks.data());
        }

        const_iterator cbegin() const noexcept
        {
            return const_iterator(m_registeredBlocks.data());
        }

        iterator end() noexcept
        {
            return iterator(m_registeredBlocks.data() + m_registeredBlocks.size());
        }

        const_iterator end() const noexcept
        {
            return const_iterator(m_registeredBlocks.data() + m_registeredBlocks.size());
        }

        const_iterator cend() const noexcept
        {
            return const_iterator(m_registeredBlocks.data() + m_registeredBlocks.size());
        }

        [[nodiscard]] bool empty() const noexcept { return m_registeredBlocks.empty(); }
        std::size_t size() const noexcept { return m_registeredBlocks.size(); }

        iterator find(std::string_view name) noexcept;
        const_iterator find(std::string_view name) const noexcept;

        iterator::value_type at(std::size_t i)
        {
            auto &ref = m_registeredBlocks.at(i);
            return { ref.name, ref.blockType.get() };
        }

        const_iterator::value_type at(std::size_t i) const
        {
            auto &ref = m_registeredBlocks.at(i);
            return { ref.name, ref.blockType.get() };
        }

        iterator::value_type operator[](std::size_t i) noexcept
        {
            auto &ref = m_registeredBlocks.data()[i];
            return { ref.name, ref.blockType.get() };
        }

        const_iterator::value_type operator[](std::size_t i) const noexcept
        {
            auto &ref = m_registeredBlocks.data()[i];
            return { ref.name, ref.blockType.get() };
        }

    private:
        std::vector<Item> m_registeredBlocks;
    };

}
#endif