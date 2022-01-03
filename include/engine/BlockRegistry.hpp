#ifndef ENGINE_BLOCKREGISTRY_HPP
#define ENGINE_BLOCKREGISTRY_HPP

#include <absl/container/flat_hash_map.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace engine {

    struct BlockType;
    class Game;

    class BlockRegistry {
    private:
        using block_types_vector = std::vector<std::unique_ptr<engine::BlockType>>;
        using name_map = absl::flat_hash_map<std::string, std::uint32_t>;

    public:
        template <typename T>
        [[nodiscard]] auto registre(std::string_view name) -> std::enable_if_t<std::is_base_of_v<engine::BlockType, T>, std::remove_cv_t<T> *>
        {
            if (auto [start, stop] = m_name_to_id.equal_range(absl::string_view { name.data(), name.size() }); start == stop || start->first != name) {
                m_name_to_id.emplace_hint(start, std::make_pair(std::string { name }, m_block_types.size()));
                return m_block_types.emplace_back(std::make_unique<T>()).get();
            } else {
                return nullptr;
            }
        }

        [[nodiscard]] engine::BlockType *operator[](std::uint32_t idx) const noexcept
        {
            assert(idx < m_block_types.size());
            return m_block_types[idx].get();
        }

        [[nodiscard]] engine::BlockType *operator[](std::string_view name) const noexcept
        {
            auto it = m_name_to_id.find(absl::string_view { name.data(), name.size() });
            assert(it != m_name_to_id.end());
            return (*this)[it->second];
        }

        [[nodiscard]] engine::BlockType *at(std::string_view name) const noexcept
        {
            auto it = m_name_to_id.find(absl::string_view { name.data(), name.size() });
            if (it == m_name_to_id.end())
                return nullptr;
            return m_block_types[it->second].get();
        }

        template <typename T,
            std::enable_if_t<std::is_base_of_v<engine::BlockType, T>, int> SFINAE = 0>
        [[nodiscard]] T *at(std::string_view name) const noexcept
        {
            return dynamic_cast<T *>(at(name));
        }

        [[nodiscard]] engine::BlockType *at(std::uint32_t idx) const noexcept
        {
            assert(idx < m_block_types.size());
            return m_block_types[idx].get();
        }

        struct iterator {
            explicit iterator(block_types_vector::const_iterator it) noexcept
                : m_base(it)
            {
            }

            [[nodiscard]] engine::BlockType *operator*() const noexcept
            {
                return m_base->get();
            }

            [[nodiscard]] std::unique_ptr<engine::BlockType> const &operator->() const noexcept
            {
                return *m_base;
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

            [[nodiscard]] block_types_vector::const_iterator base() const noexcept { return m_base; }

        private:
            block_types_vector::const_iterator m_base;
        };

        using const_iterator = iterator;

        [[nodiscard]] iterator begin() const noexcept
        {
            return iterator { m_block_types.begin() };
        }

        [[nodiscard]] iterator end() const noexcept
        {
            return iterator { m_block_types.begin() };
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return m_block_types.size();
        }

        [[nodiscard]] std::size_t empty() const noexcept
        {
            return m_block_types.empty();
        }

    private:
        name_map m_name_to_id;
        block_types_vector m_block_types;
    };

}
#endif