#ifndef ENGINE_ASSETS_IMAGE_HPP
#define ENGINE_ASSETS_IMAGE_HPP

#include <utils/memory.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <span>

namespace engine {

    struct ImageFormat {
        enum permutations {
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

        enum types {
            uint,
            floating,
        };

        enum widths {
            b1,
            b2,
            b4,
            b8
        };

        ImageFormat(types type, permutations permutation, widths channel_width, unsigned channels) noexcept
            : m_type { type }
            , m_permutation(permutation)
            , m_channel_width(channel_width)
            , m_channels(channels)
        {
        }

        types type() const noexcept { return static_cast<types>(m_type); }
        permutations permutation() const noexcept { return static_cast<permutations>(m_permutation); }
        widths channel_width() const noexcept { return static_cast<widths>(m_channel_width); }
        unsigned channels() const noexcept { return static_cast<unsigned>(m_channel_width); }
        unsigned pixel_bit_width() const noexcept { return bit_width(channel_width()) * channels(); }

    private:
        static unsigned bit_width(widths width) noexcept
        {
            switch (width) {
            case b1:
                return 1;
            case b2:
                return 2;
            case b4:
                return 4;
            case b8:
                return 8;
            }
        }

    private:
        // float, or unsigned
        unsigned m_type : 1 = 1;
        unsigned m_permutation : 5 = 0;

        // 1, 2, 3, or 4 channels
        unsigned m_channels : 2 = 3;

        // 1, 2, 4, or 8 bytes per channel
        unsigned m_channel_width : 2 = 0;
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
        static Image load(std::filesystem::path const &path, ImageFormat desired_format = { ImageFormat::uint, ImageFormat::rgba, ImageFormat::b1, 4 });
        static Image load(std::span<std::byte const> data, ImageFormat desired_format = { ImageFormat::uint, ImageFormat::rgba, ImageFormat::b1, 4 });

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