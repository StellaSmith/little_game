#ifndef ENGINE_ASSETS_IMAGE_HPP
#define ENGINE_ASSETS_IMAGE_HPP

#include <utils/memory.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <span>

namespace engine {

    struct ImageFormat {
        enum permutation_t {
            rgba,
            rgab,
            rbga,
            rbag,
            ragb,
            rabg,
            grba,
            grab,
            gbra,
            gbar,
            garb,
            gabr,
            brga,
            brag,
            bgra,
            bgar,
            barg,
            bagr,
            argb,
            arbg,
            agrb,
            agbr,
            abrg,
            abgr,
        };

        enum type_t {
            uint,
            floating,
        };

        enum width_t {
            b1,
            b2,
            b4,
            b8,
        };

        enum channels_t {
            c1,
            c2,
            c3,
            c4,
        };

        constexpr ImageFormat(type_t type, permutation_t permutation, width_t channel_width, channels_t channels) noexcept
            : m_packged((type & 0b1) | (permutation & 0b11111) << 1 | (channel_width & 0b11) << 6 | (channels & 0b11) << 8)
        {
        }

        constexpr type_t type() const noexcept
        {
            return static_cast<type_t>(m_packged & 0b1);
        }

        constexpr permutation_t permutation() const noexcept
        {
            return static_cast<permutation_t>((m_packged & 0b111110) >> 1);
        }

        constexpr width_t channel_width() const noexcept
        {
            return static_cast<width_t>((m_packged & 0b11000000) >> 6);
        }

        constexpr channels_t channels() const noexcept
        {
            return static_cast<channels_t>((m_packged & 0b1100000000) >> 8);
        }

        constexpr unsigned pixel_bit_width() const noexcept
        {
            return (1 << static_cast<unsigned>(channel_width())) * (static_cast<unsigned>(channels()) + 1);
        }

    private:
        uint16_t m_packged;
    };

    namespace errors {
        struct UnsupportedImageFormat : public std::exception {
        public:
            explicit UnsupportedImageFormat(ImageFormat) noexcept;

            char const *what() const noexcept override;
            ImageFormat format() const noexcept;

        private:
            ImageFormat m_format;
        };
    }
}

namespace engine::assets {

    class Image {
    public:
        static Image load(std::filesystem::path const &path, ImageFormat desired_format = {
                                                                 ImageFormat::uint,
                                                                 ImageFormat::rgba,
                                                                 ImageFormat::b1,
                                                                 ImageFormat::c4,
                                                             });
        static Image load(std::span<std::byte const> data, ImageFormat desired_format = {
                                                               ImageFormat::uint,
                                                               ImageFormat::rgba,
                                                               ImageFormat::b1,
                                                               ImageFormat::c4,
                                                           });

        ImageFormat format() const noexcept { return m_format; }
        std::uint32_t width() const noexcept { return m_width; }
        std::uint32_t height() const noexcept { return m_height; }

        void *data() noexcept { return m_data.get(); }
        void const *data() const noexcept { return m_data.get(); }

    private:
        explicit Image(ImageFormat fmt) noexcept
            : m_format { fmt }
        {
        }

    private:
        ImageFormat m_format;
        std::uint32_t m_width = 0, m_height = 0;
        std::unique_ptr<void, utils::FreeDeleter> m_data = nullptr;
    };

}

#endif