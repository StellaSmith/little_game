#include <engine/Sides.hpp>
#include <engine/Stream.hpp>
#include <engine/assets/BlockMesh.hpp>
#include <engine/errors/UnsupportedFileType.hpp>
#include <utils/strings.hpp>

#include <boost/container/flat_set.hpp>
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

static decltype(auto) string_path(std::filesystem::path const &p) noexcept
{
    if constexpr (std::is_same_v<std::filesystem::path::value_type, char>)
        return p.native();
    else
        return p.string();
}

struct ModelVertex {
    float x, y, z, u, v;

    constexpr engine::rendering::Vertex to_vertex(std::uint32_t texture_index) const noexcept
    {
        return { glm::vec3 { x, y, z }, glm::vec3 { u, v, texture_index } };
    }
};

struct ModelFace {
    std::uint32_t texture_index;
    std::uint32_t vertex_indices[4];

    engine::Sides sides;
    unsigned char solid : 1;
    unsigned char quad : 1;
};

static rapidjson::SchemaDocument const s_model_schema = []() {
    resources::BaseResource const *schema = engine::open_resource("schemas/ModelSchema.json");
    if (schema == nullptr)
        throw std::runtime_error("Can't open resources://schemas/ModelSchema.json");
    if (schema->type != resources::ResourceType::FILE_RESOURCE)
        throw std::runtime_error("resources://schemas/ModelSchema.json is not a file");

    rapidjson::Document doc;
    doc.Parse(reinterpret_cast<char const *>(static_cast<resources::FileResource const *>(schema)->data), schema->size);
    return rapidjson::SchemaDocument(std::move(doc));
}();

using namespace std::literals;

engine::assets::BlockMesh engine::assets::BlockMesh::load(std::filesystem::path const &path)
{
    if (utils::ends_with(path.native(), TEXT(".json"sv)) || utils::ends_with(path.native(), TEXT(".cjson"sv)))
        return load_json(path);
    throw engine::errors::UnsupportedFileType();
}

engine::assets::BlockMesh engine::assets::BlockMesh::load_json(std::filesystem::path const &path)
{

    spdlog::info("Loading model from file {}"sv, string_path(path));

    std::vector<ModelVertex> vertices;
    std::vector<ModelFace> faces;
    boost::container::flat_set<std::uint32_t> textures;

    {
        auto fp = engine::open_file(path, "r");

        char buf[1024 * 8] {}; // 8KiB

        rapidjson::FileReadStream stream(fp.get(), buf, sizeof(buf));
        rapidjson::SchemaValidatingReader<rapidjson::kParseDefaultFlags, rapidjson::FileReadStream, rapidjson::UTF8<>> reader(stream, s_model_schema);
        rapidjson::Document doc;
        doc.Populate(reader);
        fp.reset();

        if (auto const result = reader.GetParseResult(); !result) {
            if (result.Code() == rapidjson::kParseErrorTermination) {
                if (!reader.IsValid()) {
                    spdlog::error("Model {} is invalid according to schema", string_path(path));
                    rapidjson::StringBuffer sb;
                    reader.GetInvalidDocumentPointer().StringifyUriFragment(sb);
                    spdlog::error("\tpath    : {}", std::string_view { sb.GetString(), sb.GetSize() });
                    spdlog::error("\tkeyword : {}", reader.GetInvalidSchemaKeyword());
                    sb.Clear();
                    reader.GetInvalidDocumentPointer().StringifyUriFragment(sb);
                    spdlog::error("\tdocument: {}", std::string_view { sb.GetString(), sb.GetSize() });
                    throw /* invalid according to schema */;
                } else {
                    spdlog::error("Error reading {}", string_path(path));
                    throw /* io error */;
                }
            } else {
                spdlog::error("{} is not valid json", string_path(path));
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

            face.texture_index = face_object["texture"].GetUint(); // we normalize these later
            textures.emplace(face.texture_index);

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

    spdlog::info("Compiling block model from file {}"sv, string_path(path));
    engine::assets::BlockMesh result;
    result.m_textures = std::vector<std::uint32_t> { textures.cbegin(), textures.cend() };
    std::for_each(faces.begin(), faces.end(), [&textures](ModelFace &face) {
        face.texture_index = static_cast<std::uint32_t>(textures.index_of(textures.find(face.texture_index)));
    });

    auto const append_face = [](engine::assets::BlockMesh &model, ModelFace const &face, std::vector<ModelVertex> const &vertices) {
        std::size_t const i = face.sides + (!face.solid * 64);

        auto &current = model.get_or_emplace(i);
        std::uint32_t const current_index = current.indices.size();
        if (face.quad) {
            current.vertices.insert(
                current.vertices.end(),
                {
                    vertices[face.vertex_indices[0]].to_vertex(face.texture_index),
                    vertices[face.vertex_indices[1]].to_vertex(face.texture_index),
                    vertices[face.vertex_indices[2]].to_vertex(face.texture_index),
                    vertices[face.vertex_indices[3]].to_vertex(face.texture_index),
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
                    vertices[face.vertex_indices[0]].to_vertex(face.texture_index),
                    vertices[face.vertex_indices[1]].to_vertex(face.texture_index),
                    vertices[face.vertex_indices[2]].to_vertex(face.texture_index),
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
                append_face(result, face, vertices);
            }
        }
    }

    // TODO: deduplicate the vertices

    return result;
}
