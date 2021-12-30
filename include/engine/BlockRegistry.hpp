#ifndef ENGINE_BLOCKREGISTRY_HPP
#define ENGINE_BLOCKREGISTRY_HPP

#include <engine/errors/AlreadyRegistered.hpp>

#include <absl/container/flat_hash_map.h>

#include <memory>
#include <string>
#include <string_view>

namespace engine {

    struct BlockType;
    class Game;

    class BlockRegistry
        : private absl::flat_hash_map<std::string, std::unique_ptr<engine::BlockType>> {
        using super = absl::flat_hash_map<std::string, std::unique_ptr<engine::BlockType>>;

    public:
        template <typename T>
        auto Register(std::string_view name) -> std::enable_if_t<std::is_base_of_v<engine::BlockType, T>, std::remove_cv_t<T> *>
        {
            auto it = this->find(name);
            if (it != this->end())
                throw engine::errors::AlreadyRegistered { "BlockType" };
            it = this->emplace(it, std::string(name), std::make_unique<std::remove_cv_t<T>>());
            return it->second.get();
        }

        template <typename T>
        auto Get(std::string_view name) -> std::enable_if_t<std::is_base_of_v<engine::BlockType, T>, T *>
        {
            auto *p = Get(name);
            if constexpr (std::is_same_v<std::remove_cv_t<T>, engine::BlockType>)
                return p;
            else
                return dynamic_cast<T *>(p);
        }

        engine::BlockType *Get(std::string_view name)
        {
            if (auto it = find(name); it != end())
                return it->second.get();
            else
                return nullptr;
        }

        using super::at;
        using super::begin;
        using super::cbegin;
        using super::cend;
        using super::empty;
        using super::end;
        using super::find;
        using super::size;

        engine::BlockType *operator[](std::size_t idx) const noexcept
        {
            if (auto it = super::iterator_at(idx); it == this->end())
                return nullptr;
            else
                return it->second.get();
        }

        engine::BlockType *operator[](std::string_view name)
        {
            if (auto it = this->find(name); it == this->end())
                return nullptr;
            else
                return it->second.get();
        }
    };

}
#endif