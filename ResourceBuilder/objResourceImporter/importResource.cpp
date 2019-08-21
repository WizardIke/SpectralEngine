#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>
#include <string_view>
#include <array>
#include <cstdint>
#include <unordered_map>
#include <utility> //std::make_pair
#include <cstring> //std::memcopy
#include <string> //std::stof
#include <type_traits> //std::is_trivially_copyable_v, std::enable_if_t
#include <optional>

namespace
{
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

	struct Mesh
	{
		std::vector<std::array<float, 3>> positions;
		std::vector<std::array<float, 2>> textureCoordinates;
		std::vector<std::array<float, 3>> normals;
		std::vector<std::array<std::array<unsigned long, 3>, 3>> faces;
	};

	struct ConvertedMesh
	{
		uint32_t compressedVertexType;
		uint32_t unpackedVertexType;
		uint32_t vertexCount;
		uint32_t indexCount;
		unsigned long verticesSizeInBytes;
		std::unique_ptr<float[]> vertices;
		unsigned long indicesSizeInBytes;
		std::unique_ptr<char[]> indices;
	};

	static constexpr std::string_view trim(std::string_view str,
		std::string_view whitespace = " \t") noexcept
	{
		const auto strBegin = str.find_first_not_of(whitespace);
		if (strBegin == std::string::npos)
			return ""; // no content

		const auto strEnd = str.find_last_not_of(whitespace);
		const auto strRange = strEnd - strBegin + 1;

		return str.substr(strBegin, strRange);
	}

	static std::vector<std::string_view> splitByCharIngoringDuplicateSeparators(
		std::string_view str, char token)
	{
		std::vector<std::string_view> results;
		std::size_t startIndex = 0u;
		const auto strSize = str.size();
		if (strSize == 0u)
		{
			results.emplace_back();
			return results;
		}
		for (std::size_t i = 0u;;)
		{
			if (str[i] == token)
			{
				if (startIndex != i)
				{
					results.push_back(str.substr(startIndex, i - startIndex));
				}
				startIndex = i + 1u;
			}
			++i;
			if (i == strSize)
			{
				results.push_back(str.substr(startIndex, i - startIndex));
				break;
			}
		}

		return results;
	}

	static std::vector<std::string_view> splitByChar(std::string_view str, char token)
	{
		std::vector<std::string_view> results;
		std::size_t startIndex = 0u;
		const auto strSize = str.size();
		if (strSize == 0u)
		{
			results.emplace_back();
			return results;
		}
		for (std::size_t i = 0u;;)
		{
			if (str[i] == token)
			{
				results.push_back(str.substr(startIndex, i - startIndex));
				startIndex = i + 1u;
			}
			++i;
			if (i == strSize)
			{
				results.push_back(str.substr(startIndex, i - startIndex));
				break;
			}
		}

		return results;
	}

	static float stringToFloat(std::string_view str)
	{
		return std::stof(std::string(str));
	}

	static unsigned long stringToUnsignedLong(std::string_view str)
	{
		return std::stoul(std::string{ str });
	}

	static bool readTripleIndex(std::string_view str,
		std::array<unsigned long, 3> componentCounts,
		std::array<unsigned long, 3>& result) noexcept
	{
		try
		{
			auto indices = splitByChar(str, '/');
			auto i = 0u;
			for (auto index : indices)
			{
				if (i == 3u)
				{
					return false;
				}
				auto trimmedIndex = trim(index);
				if (trimmedIndex[0] == '-')
				{
					result[i] = componentCounts[i] + 1u - stringToUnsignedLong(
						std::string_view{ trimmedIndex.data() + 1u, trimmedIndex.size() - 1u });
				}
				else
				{
					result[i] = stringToUnsignedLong(trimmedIndex);
				}
				++i;
			}
			for (; i != 3u; ++i)
			{
				result[i] = 0u;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}

	static bool passLine(std::string_view line, unsigned long long lineNumber, Mesh& mesh)
	{
		std::string_view trimmedLine = trim(line);
		if (line.empty())
		{
			return true;
		}
		if (line[0] == '#')
		{
			return true;
		}
		auto tokens = splitByCharIngoringDuplicateSeparators(trimmedLine, ' ');
		auto current = tokens.begin();
		const auto end = tokens.end();
		if (*current == "v")
		{
			auto index = 0u;
			std::array<float, 3> position;
			++current;
			for (; current != end; ++current)
			{
				if (index == 3u)
				{
					std::cout << "too many components in vertex position on line " << lineNumber << "\n";
					return false;
				}
				position[index] = stringToFloat(*current);
				++index;
			}
			mesh.positions.push_back(std::move(position));
		}
		else if (*current == "vt")
		{
			auto index = 0u;
			std::array<float, 2> textureCoordinate;
			++current;
			for (; current != end; ++current)
			{
				if (index == 2u)
				{
					std::cout << "too many components in vertex texture coordinate on line " << lineNumber << "\n";
					return false;
				}
				textureCoordinate[index] = stringToFloat(*current);
				++index;
			}
			mesh.textureCoordinates.push_back(std::move(textureCoordinate));
		}
		else if (*current == "vn")
		{
			auto index = 0u;
			std::array<float, 3> normal;
			++current;
			for (; current != end; ++current)
			{
				if (index == 3u)
				{
					std::cout << "too many components in vertex normal on line " << lineNumber << "\n";
					return false;
				}
				normal[index] = stringToFloat(*current);
				++index;
			}
			mesh.normals.push_back(std::move(normal));
		}
		else if (*current == "f")
		{
			auto index = 0u;
			std::array<std::array<unsigned long, 3>, 3> face;
			++current;
			for (; current != end; ++current)
			{
				if (index == 3u)
				{
					std::cout << "too many vertices in face on line " << lineNumber << "\n";
					return false;
				}
				bool result = readTripleIndex(*current,
					{ static_cast<unsigned long>(mesh.positions.size()),
						static_cast<unsigned long>(mesh.textureCoordinates.size()),
						static_cast<unsigned long>(mesh.normals.size()) },
					face[index]);
				if (!result)
				{
					std::cout << "invalid set of three indices on line " << lineNumber << "\n";
					return false;
				}
				++index;
			}
			mesh.faces.push_back(std::move(face));
		}
		return true;
	}

	template<class T>
	static std::enable_if_t<std::is_trivially_copyable_v<T>, std::array<char, sizeof(T)>>
		toBytes(const T& value)
	{
		std::array<char, sizeof(T)> bytes;
		std::memcpy(&bytes[0], &value, sizeof(T));
		return bytes;
	}

	static uint32_t getFormat(bool hasPositions, bool hasTextureCoordinates,
		bool hasNormals)
	{
		if (hasPositions)
		{
			if (hasTextureCoordinates)
			{
				if (hasNormals)
				{
					return VertexType::position3f_texCoords2f_normal3f;
				}
				else
				{
					return VertexType::position3f_texCoords2f;
				}
			}
			else
			{
				if (hasNormals)
				{
					return VertexType::position3f_normal3f;
				}
				else
				{
					return VertexType::position3f;
				}
			}
		}
		else
		{
			if (hasTextureCoordinates)
			{
				if (hasNormals)
				{
					return VertexType::texCoords2f_normal3f;
				}
				else
				{
					return VertexType::texCoords2f;
				}
			}
			else
			{
				if (hasNormals)
				{
					return VertexType::normal3f;
				}
				else
				{
					return VertexType::none;
				}
			}
		}
	}

	static unsigned long getVertexSizeInFloats(bool hasPositions,
		bool hasTextureCoordinates, bool hasNormals)
	{
		unsigned long vertexFloatCount = 0u;
		if (hasPositions)
		{
			vertexFloatCount += 3u;
		}
		if (hasTextureCoordinates)
		{
			vertexFloatCount += 2u;
		}
		if (hasNormals)
		{
			vertexFloatCount += 3u;
		}
		return vertexFloatCount;
	}

	static void resize(std::unique_ptr<float[]>& array, unsigned long oldSize,
		unsigned long newSize)
	{
		float* newArray = new float[newSize];
		float* oldArray = array.get();
		for (unsigned long i = 0u; i != oldSize; ++i)
		{
			newArray[i] = oldArray[i];
		}
		array.reset(newArray);
	}

	template<class T, std::size_t n, class Hash = std::hash<T>>
	struct ArrayHash : private Hash
	{
		std::size_t operator() (const std::array<T, n>& key) const
		{
			if constexpr (n == 0u)
			{
				return 0u;
			}
			const Hash& hasher = static_cast<const Hash&>(*this);
			std::size_t result = hasher(key[0]);
			for (std::size_t i = 1u; i != n; ++i) {
				result = result * 31 + hasher(key[i]);
			}
			return result;
		}
	};

	static ConvertedMesh convert(const Mesh& mesh)
	{
		bool hasPositions = !mesh.positions.empty();
		bool hasTextureCoordinates = !mesh.textureCoordinates.empty();
		bool hasNormals = !mesh.normals.empty();
		const unsigned long vertexFloatCount = getVertexSizeInFloats(
			hasPositions, hasTextureCoordinates, hasNormals);
		unsigned long vertexCapacity = 0u;
		unsigned long vertexCount = 0u;
		std::unique_ptr<float[]> vertices;
		using Hash = ArrayHash<unsigned long, 3>;
		std::unordered_map<std::array<unsigned long, 3>, unsigned long, Hash> indexMap;
		const unsigned long faceCount = static_cast<unsigned long>(mesh.faces.size());
		const unsigned long indexCount = faceCount * 3u;
		std::unique_ptr<char[]> indices{
			new char[indexCount <= 65535u ? indexCount * 2u : indexCount * 4u] };
		for (unsigned long i = 0u; i != faceCount; ++i)
		{
			const auto& face = mesh.faces[i];
			for (unsigned long j = 0u; j != 3u; ++j)
			{
				auto result = indexMap.insert(std::make_pair(face[j], vertexCount));
				unsigned long newIndex;
				if (result.second)
				{
					newIndex = vertexCount;
					unsigned long vertexIndex = vertexCount * vertexFloatCount;
					if ((vertexIndex + vertexFloatCount) > vertexCapacity)
					{
						auto newVertexCapacity = vertexCapacity < 64u ? 128u : vertexCapacity * 2u;
						resize(vertices, vertexCapacity, newVertexCapacity);
						vertexCapacity = newVertexCapacity;
					}
					if (hasPositions)
					{
						const auto& position = mesh.positions[face[j][0] - 1u];
						vertices[vertexIndex] = position[0];
						++vertexIndex;
						vertices[vertexIndex] = position[1];
						++vertexIndex;
						vertices[vertexIndex] = position[2];
						++vertexIndex;
					}
					if (hasTextureCoordinates)
					{
						const auto& textureCoordinate =
							mesh.textureCoordinates[face[j][1] - 1u];
						vertices[vertexIndex] = textureCoordinate[0];
						++vertexIndex;
						vertices[vertexIndex] = textureCoordinate[1];
						++vertexIndex;
					}
					if (hasNormals)
					{
						const auto& normal = mesh.normals[face[j][2] - 1u];
						vertices[vertexIndex] = normal[0];
						++vertexIndex;
						vertices[vertexIndex] = normal[1];
						++vertexIndex;
						vertices[vertexIndex] = normal[2];
						++vertexIndex;
					}
					++vertexCount;
				}
				else
				{
					newIndex = result.first->second;
				}
				if (indexCount <= 65535u)
				{
					unsigned long indexInIndices = (i * 3 + j) * 2u;
					auto bytes = toBytes(static_cast<uint16_t>(newIndex));
					indices[indexInIndices] = bytes[0];
					indices[indexInIndices + 1u] = bytes[1];
				}
				else
				{
					unsigned long indexInIndices = (i * 3 + j) * 4u;
					auto bytes = toBytes(static_cast<uint32_t>(newIndex));
					indices[indexInIndices] = bytes[0];
					indices[indexInIndices + 1u] = bytes[1];
					indices[indexInIndices + 2u] = bytes[2];
					indices[indexInIndices + 3u] = bytes[3];
				}
			}
		}

		uint32_t format = getFormat(hasPositions, hasTextureCoordinates, hasNormals);
		unsigned long sizeOfIndex = faceCount <= 65535u ? sizeof(uint16_t) : sizeof(uint32_t);
		return ConvertedMesh{ format, format,
			vertexCount, faceCount,
			static_cast<unsigned long>(vertexCount * vertexFloatCount * sizeof(float)), std::move(vertices),
			faceCount * sizeOfIndex, std::move(indices) };
	}

	static void writeOutputFile(std::ofstream& outFile, const ConvertedMesh& mesh)
	{
		outFile.write(reinterpret_cast<const char*>(&mesh), sizeof(uint32_t) * 4u);
		outFile.write(reinterpret_cast<const char*>(mesh.vertices.get()), mesh.verticesSizeInBytes);
		outFile.write(reinterpret_cast<const char*>(mesh.indices.get()), mesh.indicesSizeInBytes);
	}

	static std::optional<ConvertedMesh> readAndConvertMesh(std::ifstream& inFile)
	{
		Mesh mesh{};
		std::string line;
		unsigned long long lineNumber = 1u;
		while (std::getline(inFile, line))
		{
			bool succeeded = passLine(line, lineNumber, mesh);
			if (!succeeded)
			{
				return std::nullopt;
			}
			++lineNumber;
		}
		inFile.close();

		return convert(mesh);
	}
}

extern "C"
#ifdef _WIN32
__declspec(dllexport)
#endif
bool importResource(const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath, void* importResourceContext,
	bool(*importResource)(void* context, const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath))
{
	try
	{
		std::ifstream inFile{ baseInputPath / relativeInputPath };
		auto convertedMesh = readAndConvertMesh(inFile);
		if (!convertedMesh)
		{
			return 1;
		}

		auto outputPath = baseOutputPath / relativeInputPath;
		outputPath += ".data";
		const auto& outputDirectory = outputPath.parent_path();
		if (!std::filesystem::exists(outputDirectory))
		{
			std::filesystem::create_directories(outputDirectory);
		}
		std::ofstream outFile{ outputPath, std::ios::binary };
		if (!outFile)
		{
			std::cout << "failed to create output file\n";
			return false;
		}
		writeOutputFile(outFile, *convertedMesh);
		outFile.close();

		std::cout << "done\n";
	}
	catch (std::exception& e)
	{
		std::cout << "failed: " << e.what() << "\n";
		return false;
	}
	catch (...)
	{
		std::cout << "failed\n";
		return false;
	}
	return true;
}