#ifndef ENGINE_STREAM_HPP
#define ENGINE_STREAM_HPP

#include <resources_generated.hpp> // auto-generated

#include <engine/result.hpp>
#include <engine/system/block_size.hpp>
#include <engine/system/open_file.hpp>
#include <engine/system/page_size.hpp>
#include <utils/FileHandle.hpp>

#include <boost/outcome/try.hpp>

#include <numeric>
#include <span>

namespace engine {

    [[nodiscard]] inline engine::result<utils::FileHandle, std::errc> open_file(std::filesystem::path const &path, engine::nonnull<char const> mode) noexcept
    {
        auto const lcm = [](auto x, auto... xs) { return ((x = std::lcm(x, xs)), ...); };

        engine::nonnull<std::FILE> fp = BOOST_OUTCOME_TRYX(engine::system::open_file(path, mode));

        std::size_t const page_size = engine::system::page_size();
        auto const maybe_block_size = engine::system::block_size(fp);

        auto const buffer_size = lcm(BUFSIZ, page_size, maybe_block_size ? maybe_block_size.value() : BUFSIZ);
        if (buffer_size > BUFSIZ)
            std::setvbuf(fp, nullptr, _IOFBF, buffer_size);

        return utils::FileHandle { fp };
    }

    engine::result<engine::nonnull<resources::BaseResource const>, std::errc> open_resource(std::string_view path) noexcept;
    engine::result<std::span<std::byte const>, std::errc> load_resource(std::string_view path) noexcept;
}

#endif
