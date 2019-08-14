#include <cstdint>

enum VertexType : uint32_t
{
	position3f = 0u,
	position3f_texCoords2f = 1u,
	position3f_texCoords2f_normal3f = 2u,
	position3f_texCoords2f_normal3f_tangent3f_bitangent3f = 3u,
	position3f_color3f = 4u,
	position3f_color4f = 5u,
	position3f_normal3f = 6u,
	texCoords2f = 7u,
	texCoords2f_normal3f = 8u,
	normal3f = 9u,
	none = 10u,
};

struct Vertex_position3f
{
	constexpr static uint32_t vertexType = VertexType::position3f;
	float position[3u];
};

struct Vertex_position3f_texCoords2f
{
	constexpr static uint32_t vertexType = VertexType::position3f_texCoords2f;
	float position[3u];
	float texCoords[2u];
};

struct Vertex_position3f_texCoords2f_normal3f
{
	constexpr static uint32_t vertexType = VertexType::position3f_texCoords2f_normal3f;
	float position[3u];
	float texCoords[2u];
	float normal[3u];
};

struct Vertex_position3f_texCoords2f_normal3f_tangent3f_bitangent3f
{
	constexpr static uint32_t vertexType = VertexType::position3f_texCoords2f_normal3f_tangent3f_bitangent3f;
	float position[3u];
	float texCoords[2u];
	float normal[3u];
	float tangent[3u];
	float bitangent[3u];
};

struct Vertex_position3f_color3f
{
	constexpr static uint32_t vertexType = VertexType::position3f_color3f;
	float position[3u];
	float texCoords[2u];
	float color[3u];
};

struct Vertex_position3f_color4f
{
	constexpr static uint32_t vertexType = VertexType::position3f_color3f;
	float position[3u];
	float texCoords[2u];
	float color[4u];
};