#include <engine/File.hpp>
#include <engine/Sides.hpp>
#include <engine/assets/BlockMesh.hpp>
#include <engine/errors/UnsupportedFileType.hpp>
#include <engine/resources.hpp>

#include <boost/container/flat_set.hpp>
#include <fmt/std.h>
#include <glm/fwd.hpp>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/reader.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#if defined(_WIN32)
#define TEXT(s) L##s
#else
#define TEXT(s) s
#endif

namespace {
    struct ModelVertex {
        float x, y, z, u, v;

        constexpr engine::rendering::Vertex to_vertex(std::uint32_t texture_index, std::uint32_t color_mask_index) const noexcept
        {
            return {
                .position = { x, y, z },
                .uv = { u, v },
                .textures = {
                    texture_index,
                    color_mask_index,
                }
            };
        }
    };

    struct ModelFace {
        std::uint32_t texture;
        std::uint32_t color_mask;
        std::uint32_t vertex_indices[4];

        engine::Sides sides;
        unsigned char solid : 1;
        unsigned char quad : 1;
    };
}

static rapidjson::SchemaDocument const s_model_schema = []() {
    resources::BaseResource const *schema = MUST(engine::open_resource("schemas/ModelSchema.json"));
    if (schema->type != resources::ResourceType::FILE_RESOURCE)
        throw std::runtime_error("resources://schemas/ModelSchema.json is not a file");

    rapidjson::Document doc;
    doc.Parse(reinterpret_cast<char const *>(static_cast<resources::FileResource const *>(schema)->data), schema->size);
    return rapidjson::SchemaDocument(std::move(doc));
}();

using namespace std::literals;

engine::assets::BlockMesh::BlockMesh(std::string_view name)
    : IAsset(name)
{
}

void engine::assets::BlockMesh::load(std::filesystem::path const &path)
{
    if (path.native().ends_with(TEXT(".json"sv)) || path.native().ends_with(TEXT(".cjson"sv)))
        return load_json(path);
    throw engine::errors::UnsupportedFileType();
}

void engine::assets::BlockMesh::load_json(std::filesystem::path const &path)
{
    SPDLOG_INFO("Loading model from file {:?}", path);

    std::vector<ModelVertex> vertices;
    std::vector<ModelFace> faces;
    boost::container::flat_set<std::uint32_t> textures;
    boost::container::flat_set<std::uint32_t> color_masks;

    {

        auto fp = engine::File::open(path, "r").value();

        char buf[1024 * 8] {}; // 8KiB

        rapidjson::FileReadStream stream(fp.get(), buf, sizeof(buf));
        rapidjson::SchemaValidatingReader<rapidjson::kParseDefaultFlags, rapidjson::FileReadStream, rapidjson::UTF8<>> reader(stream, s_model_schema);
        rapidjson::Document doc;
        doc.Populate(reader);
        fp.reset();

        if (auto const result = reader.GetParseResult(); !result) {
            if (result.Code() == rapidjson::kParseErrorTermination) {
                if (!reader.IsValid()) {
                    SPDLOG_ERROR("Model {} is invalid according to schema", path);
                    rapidjson::StringBuffer sb;
                    reader.GetInvalidDocumentPointer().StringifyUriFragment(sb);
                    SPDLOG_ERROR("\tpath    : {}", std::string_view { sb.GetString(), sb.GetSize() });
                    SPDLOG_ERROR("\tkeyword : {}", reader.GetInvalidSchemaKeyword());
                    sb.Clear();
                    reader.GetInvalidDocumentPointer().StringifyUriFragment(sb);
                    SPDLOG_ERROR("\tdocument: {}", std::string_view { sb.GetString(), sb.GetSize() });
                    throw /* invalid according to schema */;
                } else {
                    SPDLOG_ERROR("Error reading {}", path);
                    throw /* io error */;
                }
            } else {
                SPDLOG_ERROR("{} is not valid json", path);
                throw /* invalid json */;
            }
        }

        auto const root = doc.GetObject();

        vertices.reserve(root["vertices"].GetArray().Size());
        for (auto const &vertex_object_ref : root["vertices"].GetArray()) {
            auto const vertex_object = vertex_object_ref.GetObject();

            ModelVertex vertex {};

            {
                auto const position_array = vertex_object["position"].GetArray();
                vertex.x = position_array[0].GetFloat();
                vertex.y = position_array[1].GetFloat();
                if (position_array.Size() > 3)
                    vertex.z = position_array[2].GetFloat();
            }
            {
                auto const uv_array = vertex_object["uv"].GetArray();
                vertex.u = uv_array[0].GetFloat();
                vertex.v = uv_array[1].GetFloat();
            }

            vertices.push_back(vertex);
        }

        faces.reserve(root["faces"].GetArray().Size());
        for (auto const &face_object_ref : root["faces"].GetArray()) {
            auto const face_object = face_object_ref.GetObject();

            ModelFace face {};

            {
                auto const indices_array = face_object["indices"].GetArray();
                face.vertex_indices[0] = indices_array[0].GetInt64();
                face.vertex_indices[1] = indices_array[1].GetInt64();
                face.vertex_indices[2] = indices_array[2].GetInt64();
                if (indices_array.Size() > 3) {
                    face.quad = true;
                    face.vertex_indices[3] = indices_array[3].GetInt64();
                }

                if (face.vertex_indices[0] >= vertices.size()
                    || face.vertex_indices[1] >= vertices.size()
                    || face.vertex_indices[2] >= vertices.size()
                    || face.vertex_indices[3] >= vertices.size())
                    throw /* index out of range */;
            }

            face.texture = face_object["texture"].GetUint(); // we normalize these later
            face.color_mask = face_object["color_mask"].GetUint();
            textures.emplace(face.texture);
            color_masks.emplace(face.color_mask);
            {
                auto const sides_array = face_object["sides"].GetArray();
                std::vector<std::string_view> sides;
                sides.reserve(6);
                for (auto const &side_string_ref : sides_array)
                    sides.emplace_back(side_string_ref.GetString(), side_string_ref.GetStringLength());

                for (auto const side : sides) {
                    if (side == "north"sv)
                        face.sides = static_cast<engine::Sides>(face.sides | engine::Sides::NORTH);
                    else if (side == "south"sv)
                        face.sides = static_cast<engine::Sides>(face.sides | engine::Sides::SOUTH);
                    else if (side == "east"sv)
                        face.sides = static_cast<engine::Sides>(face.sides | engine::Sides::EAST);
                    else if (side == "west"sv)
                        face.sides = static_cast<engine::Sides>(face.sides | engine::Sides::WEST);
                    else if (side == "top"sv)
                        face.sides = static_cast<engine::Sides>(face.sides | engine::Sides::TOP);
                    else if (side == "bottom"sv)
                        face.sides = static_cast<engine::Sides>(face.sides | engine::Sides::BOTTOM);
                }
            }

            face.solid = true;
            if (auto it = face_object.FindMember("solid"); it != face_object.end())
                face.solid = it->value.GetBool();

            faces.push_back(face);
        }
    }

    SPDLOG_INFO("Compiling block model from file {}", path);
    m_textures.insert(m_textures.end(), textures.cbegin(), textures.cend());
    std::for_each(faces.begin(), faces.end(), [&](ModelFace &face) {
        face.texture = static_cast<std::uint32_t>(textures.index_of(textures.find(face.texture)));
        face.color_mask = static_cast<std::uint32_t>(color_masks.index_of(color_masks.find(face.color_mask)));
    });

    auto const append_face = [&](ModelFace const &face, std::vector<ModelVertex> const &vertices) {
        std::size_t const i = face.sides + (!face.solid * 64);

        auto &current = get_or_emplace(i);
        std::uint32_t const current_index = current.indices.size();
        if (face.quad) {
            current.vertices.insert(
                current.vertices.end(),
                {
                    vertices[face.vertex_indices[0]].to_vertex(face.texture, face.color_mask),
                    vertices[face.vertex_indices[1]].to_vertex(face.texture, face.color_mask),
                    vertices[face.vertex_indices[2]].to_vertex(face.texture, face.color_mask),
                    vertices[face.vertex_indices[3]].to_vertex(face.texture, face.color_mask),
                });
            current.indices.insert(
                current.indices.end(),
                {
                    current_index + 0,
                    current_index + 1,
                    current_index + 2,
                    current_index + 1,
                    current_index + 2,
                    current_index + 3,
                });
        } else {
            current.vertices.insert(
                current.vertices.end(),
                {
                    vertices[face.vertex_indices[0]].to_vertex(face.texture, face.color_mask),
                    vertices[face.vertex_indices[1]].to_vertex(face.texture, face.color_mask),
                    vertices[face.vertex_indices[2]].to_vertex(face.texture, face.color_mask),
                });
            current.indices.insert(
                current.indices.end(),
                {
                    current_index + 0,
                    current_index + 1,
                    current_index + 2,
                });
        }
    };

    for (std::uint_fast16_t side_mask = 1; side_mask <= 64; ++side_mask) {
        for (auto const &face : faces) {
            if (!(face.sides & side_mask)) continue;
            for (bool solid : { true, false }) {
                if (face.solid != solid) continue;
                append_face(face, vertices);
            }
        }
    }

    // TODO: deduplicate the vertices
}
