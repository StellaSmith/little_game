#include <engine/Sides.hpp>
#include <engine/Stream.hpp>
#include <engine/assets/BlockModel.hpp>
#include <engine/errors/UnsupportedFileType.hpp>
#include <utils/strings.hpp>

#include <glm/fwd.hpp>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/reader.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string_view>

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
    unsigned char solid;
    unsigned char quad;
    unsigned char unused;
};

static rapidjson::SchemaDocument const model_schema = []() {
    resources::BaseResource const *schema = engine::open_resource("schemas/ModelSchema.json");
    if (schema == nullptr)
        throw std::runtime_error("Can't open resources://schemas/ModelSchema.json");
    if (schema->type != resources::FILE_RESOURCE)
        throw std::runtime_error("resources://schemas/ModelSchema.json is not a file");

    rapidjson::Document doc;
    doc.Parse(reinterpret_cast<char const *>(static_cast<resources::FileResource const *>(schema)->data), schema->size);
    return rapidjson::SchemaDocument(std::move(doc));
}();

using namespace std::literals;

engine::assets::BlockModel engine::assets::BlockModel::load(std::string_view path)
{
    if (utils::ends_with(path, ".json"sv))
        return load_json(path);
    throw engine::errors::UnsupportedFileType("engine::assets::BlockModel");
}

engine::assets::BlockModel engine::assets::BlockModel::load_json(std::string_view path)
{

    spdlog::info("Loading model from json {}"sv, path);

    std::vector<ModelVertex> vertices;
    std::vector<ModelFace> faces;
    std::vector<std::string> textures;

    {
        std::FILE *fp = std::fopen(path.data(), "r");
        if (!fp)
            throw /* can't open file */;

        char buf[1024 * 8] {}; // 8KiB

        rapidjson::FileReadStream stream(fp, buf, sizeof(buf));
        rapidjson::SchemaValidatingReader<rapidjson::kParseDefaultFlags, rapidjson::FileReadStream, rapidjson::UTF8<>> reader(stream, model_schema);
        rapidjson::Document doc;
        doc.Populate(reader);
        std::fclose(fp);
        fp = nullptr;

        if (auto const result = reader.GetParseResult(); !result) {
            if (result.Code() == rapidjson::kParseErrorTermination) {
                if (!reader.IsValid()) {
                    spdlog::error("Model {} is invalid according to schema", path);
                    rapidjson::StringBuffer sb;
                    reader.GetInvalidDocumentPointer().StringifyUriFragment(sb);
                    spdlog::error("\tpath    : {}", std::string_view { sb.GetString(), sb.GetSize() });
                    spdlog::error("\tkeyword : {}", reader.GetInvalidSchemaKeyword());
                    sb.Clear();
                    reader.GetInvalidDocumentPointer().StringifyUriFragment(sb);
                    spdlog::error("\tdocument: {}", std::string_view { sb.GetString(), sb.GetSize() });
                    throw /* invalid according to schema */;
                } else {
                    spdlog::error("Error reading {}", path);
                    throw /* io error */;
                }
            } else {
                spdlog::error("{} is not valid json", path);
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

            {
                std::string_view const texture { face_object["texture"].GetString(), face_object["texture"].GetStringLength() };
                if (auto it = std::find(textures.begin(), textures.end(), texture); it == textures.end()) {
                    textures.emplace_back(texture);
                    face.texture_index = textures.size() - 1;
                } else {
                    face.texture_index = static_cast<std::uint32_t>(std::distance(textures.begin(), it));
                }
            }

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

    spdlog::info("Compiling block model from json {}"sv, path);
    engine::assets::BlockModel result;
    result.m_textures = std::move(textures);

    auto const append_face = [](engine::assets::BlockModel &model, ModelFace const &face, std::vector<ModelVertex> const &vertices) {
        std::size_t const i = face.sides + (!face.solid * 64);
        std::uint32_t const current = model.m_meshes[i].indices.size();
        if (face.quad) {
            model.m_meshes[i].vertices.insert(
                model.m_meshes[i].vertices.end(),
                {
                    vertices[face.vertex_indices[0]].to_vertex(face.texture_index),
                    vertices[face.vertex_indices[1]].to_vertex(face.texture_index),
                    vertices[face.vertex_indices[2]].to_vertex(face.texture_index),
                    vertices[face.vertex_indices[3]].to_vertex(face.texture_index),
                });
            model.m_meshes[i].indices.insert(
                model.m_meshes[i].indices.end(),
                {
                    current + 0,
                    current + 1,
                    current + 2,
                    current + 1,
                    current + 2,
                    current + 3,
                });
        } else {
            model.m_meshes[i].vertices.insert(
                model.m_meshes[i].vertices.end(),
                {
                    vertices[face.vertex_indices[0]].to_vertex(face.texture_index),
                    vertices[face.vertex_indices[1]].to_vertex(face.texture_index),
                    vertices[face.vertex_indices[2]].to_vertex(face.texture_index),
                });
            model.m_meshes[i].indices.insert(
                model.m_meshes[i].indices.end(),
                {
                    current + 0,
                    current + 1,
                    current + 2,
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
