#include <iostream>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <map>
#include <cstddef>
#include <cstring>
#ifdef _WIN32
#include <Windows.h>
#undef min
#undef max
#endif

namespace
{
	struct Indent
	{
		unsigned int level = 0u;

		Indent operator+(unsigned int amount)
		{
			return Indent{ level + amount };
		}
	};

	std::ostream& operator<<(std::ostream& out, Indent indent)
	{
		for (; indent.level != 0u; --indent.level)
		{
			out << '\t';
		}
		return out;
	}

	std::string& operator<<(std::string& out, Indent indent)
	{
		for (; indent.level != 0u; --indent.level)
		{
			out += '\t';
		}
		return out;
	}

	template<class T>
	std::string& operator<<(std::string& out, const T& value)
	{
		out += value;
		return out;
	}

	extern "C" typedef bool(*ImportResourceType)(void* context, const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath);

	extern "C" typedef bool(*ImportResourcePtrType)(const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath, void* importResourceContext,
		ImportResourceType importResource, bool forceReImport) noexcept;

	class Importer
	{
#ifdef _WIN32
		HMODULE library;
#endif // _WIN32
		ImportResourcePtrType importResourcePtr;

		bool forceReImport;
	public:
#ifdef _WIN32
		Importer(const std::filesystem::path& path, bool forceReImport1) : 
			library{ LoadLibrary(path.c_str()) },
			forceReImport(forceReImport1)
		{
			if (library == NULL)
			{
				throw std::runtime_error{"Failed to load resource importer " + path.string() };
			}
			auto result = GetProcAddress(library, "importResource");
			if (importResourcePtr == NULL)
			{
				FreeLibrary(library);
				throw std::runtime_error{ "Failed to load the import function from " + path.string() };
			}
			importResourcePtr = reinterpret_cast<ImportResourcePtrType>(result);
		}

		Importer(Importer&& other) :
			library(other.library),
			importResourcePtr(other.importResourcePtr),
			forceReImport(other.forceReImport)
		{
			other.library = NULL;
		}

		~Importer()
		{
			if (library != NULL)
			{
				FreeLibrary(library);
			}
		}
#endif // _WIN32

		bool importResource(const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath, void* importResourceContext,
			ImportResourceType importResource1)
		{
			return importResourcePtr(baseInputPath, baseOutputPath, relativeInputPath, importResourceContext, importResource1, forceReImport);
		}
	};
}

static bool importDirectory(const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath, void* importResourceContext,
	ImportResourceType importResource)
{
	namespace fs = std::filesystem;

	auto inputPath = baseInputPath / relativeInputPath;
	for (const auto& entry : fs::directory_iterator(inputPath))
	{
		const auto fileName = entry.path().filename();
		if (entry.is_directory())
		{
			bool succeeded = importDirectory(baseInputPath, baseOutputPath, relativeInputPath / fileName, importResourceContext, importResource);
			if (!succeeded)
			{
				return false;
			}
		}
		else if (entry.is_regular_file())
		{
			auto relativeInputFileName = relativeInputPath / fileName;
			bool succeeded = importResource(importResourceContext, baseInputPath, baseOutputPath, relativeInputFileName);
			if (!succeeded)
			{
				return false;
			}
		}
		else
		{
			std::cerr << "Error resource file that isn't a regular file or a directory\n";
			return false;
		}
	}
	return true;
}

static bool endsWith(std::string_view fullString, std::string_view ending)
{
	if (fullString.size() >= ending.size())
	{
		return (0 == fullString.compare(fullString.size() - ending.size(), ending.size(), ending));
	}
	else
	{
		return false;
	}
}

static std::unordered_map<std::string, Importer> getFileImporters(const std::filesystem::path& importerPath, std::filesystem::file_time_type lastRebuildTime)
{
	std::unordered_map<std::string, Importer> fileImporters{};
	for (const auto& entry : std::filesystem::directory_iterator(importerPath))
	{
		const auto& fileName = entry.path().filename().string();
		if (entry.is_regular_file() && endsWith(fileName, "ResourceImporter.dll"))
		{
			bool forceReImport = entry.last_write_time() >= lastRebuildTime;
			fileImporters.emplace("." + fileName.substr(0u, fileName.size() - 20u), Importer{ entry.path(), forceReImport });
		}
	}
	return fileImporters;
}

extern "C"
{
	bool importResource(void* context, const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath)
	{
		auto& fileImporters = *static_cast<std::unordered_map<std::string, Importer>*>(context);
		const auto& extension = relativeInputPath.extension().string();
		auto fileImporter = fileImporters.find(extension);
		if (fileImporter == fileImporters.end())
		{
			std::cerr << "No importer for " << extension << " files\n";
			return false;
		}
		return fileImporter->second.importResource(baseInputPath, baseOutputPath, relativeInputPath, &fileImporters, importResource);
	}
}

namespace
{
	struct ResourceNode
	{
		bool isResourceLocation;
	};

	struct ResourceLocation : public ResourceNode
	{
		unsigned long long start;
		unsigned long long end;

		ResourceLocation(unsigned long long start1, unsigned long long end1) : ResourceNode{ true }, start(start1), end(end1) {}
	};

	struct ResourceNamespace : public ResourceNode
	{
		std::unordered_map<std::string, std::unique_ptr<ResourceNode>> children;

		ResourceNamespace() : ResourceNode{ false } {}

		void addResource(const std::filesystem::path& path, unsigned long long start, unsigned long long end)
		{
			const auto pathNoExtentionsEnd = path.end();
			ResourceNamespace* currentNamespace = this;
			for (auto it = path.begin();;)
			{
				std::string value = it->string();
				++it;
				if (it == pathNoExtentionsEnd)
				{
					currentNamespace->children.insert({std::move(value), std::unique_ptr<ResourceNode>{new ResourceLocation{ start, end }}});
					break;
				}
				else
				{
					auto& result = currentNamespace->children[std::move(value)];
					if (result.get() == nullptr)
					{
						result.reset(new ResourceNamespace{});
					}
					currentNamespace = static_cast<ResourceNamespace*>(result.get());
				}
			}
		}
	private:
		template<class Iterator>
		const ResourceNode* findHelper(Iterator& begin, const Iterator& end, const ResourceNode* current) const
		{
			if (begin == end)
			{
				return current;
			}
			auto tokenToGet = begin->string();
			++begin;
			if (tokenToGet == "..")
			{
				return nullptr;
			}
			auto next = static_cast<const ResourceNamespace*>(current)->children.find(tokenToGet)->second.get();
			
			auto result = findHelper(begin, end, next);
			if (result == nullptr)
			{
				return findHelper(begin, end, current);
			}
			else
			{
				return result;
			}
		}
	public:
		const ResourceLocation& find(const std::filesystem::path& path) const
		{
			auto begin = path.begin();
			const ResourceNode* current = findHelper(begin, path.end(), this);
			return *static_cast<const ResourceLocation*>(current);
		}

		template<class Iterator>
		const ResourceLocation& find(Iterator begin, const Iterator& end) const
		{
			const ResourceNode* current = findHelper(begin, end, this);
			return *static_cast<const ResourceLocation*>(current);
		}
	};

	void createResourcesHeaderFile(ResourceNamespace& currentNamespace, std::ostream& headerFile, Indent indent)
	{
		for (const auto& entry : currentNamespace.children)
		{
			const auto& name = entry.first;
			auto infoPtr = entry.second.get();
			if (infoPtr->isResourceLocation)
			{
				auto& info = *static_cast<ResourceLocation*>(infoPtr);
				headerFile << indent << "inline constexpr ResourceLocation " << name << " = {" << info.start << ", " << info.end << "};\n";
			}
			else
			{
				headerFile << indent << "namespace " << name << "\n";
				headerFile << indent << "{\n";
				createResourcesHeaderFile(*static_cast<ResourceNamespace*>(infoPtr), headerFile, indent + 1u);
				headerFile << indent << "};\n";
			}
		}
	}

	void combineResourcesIntoOneFile(const std::filesystem::path& inputPath, const std::filesystem::path& resourcesFilePath, const std::filesystem::path& headerFilePath)
	{
		bool filesExists = std::filesystem::exists(headerFilePath) && std::filesystem::exists(resourcesFilePath);
		auto filesLastModifiedTime = filesExists ? std::filesystem::last_write_time(headerFilePath) : std::filesystem::file_time_type{};
		const auto inputResourcesPath = inputPath / "Resources";

		bool hasChanged = false;
		for (const auto& entry : std::filesystem::recursive_directory_iterator(inputResourcesPath))
		{
			if (!entry.is_regular_file())
			{
				continue;
			}
			if (entry.last_write_time() >= filesLastModifiedTime)
			{
				hasChanged = true;
				break;
			}
		}
		if (hasChanged)
		{
			std::multimap<unsigned long long, std::filesystem::path, std::greater<>> filesBySize;
			for (const auto& entry : std::filesystem::recursive_directory_iterator(inputResourcesPath))
			{
				if (!entry.is_regular_file())
				{
					continue;
				}
				const auto& path = entry.path();
				unsigned long long fileSize = static_cast<unsigned long long>(std::filesystem::file_size(path));
				filesBySize.insert({ fileSize, path });
			}
			ResourceNamespace resourceNamespace{};
			std::ofstream resourcesFile{ resourcesFilePath, std::ios::binary };
			unsigned long long currentResourcesFileLength = 0u;
			constexpr static unsigned long long pageSize = 4u * 1024u;
			for (auto it = filesBySize.cbegin(); it != filesBySize.cend();)
			{
				if (it->first < pageSize)
				{
					break;
				}
				auto alignedResourcesLength = (currentResourcesFileLength + pageSize - 1u) & ~(pageSize - 1u);
				for (auto i = currentResourcesFileLength; i != alignedResourcesLength; ++i)
				{
					resourcesFile << '\0';
				}
				std::ifstream resourceFile(it->second, std::ios::binary);
				resourcesFile << resourceFile.rdbuf();
				currentResourcesFileLength = alignedResourcesLength + it->first;
				resourceNamespace.addResource(it->second.lexically_relative(inputResourcesPath), alignedResourcesLength, currentResourcesFileLength);

				it = filesBySize.erase(it);
			}
			if (!filesBySize.empty())
			{
				auto alignedLength = (currentResourcesFileLength + pageSize - 1u) & ~(pageSize - 1u);
				for (auto i = currentResourcesFileLength; i != alignedLength; ++i)
				{
					resourcesFile << '\0';
				}
				currentResourcesFileLength = alignedLength;
				unsigned long long remainingPageCapacity = 0u;
				do
				{
					auto filePtr = filesBySize.lower_bound(remainingPageCapacity);
					if (filePtr == filesBySize.end())
					{
						currentResourcesFileLength += remainingPageCapacity;
						for (auto i = remainingPageCapacity; i != 0u; --i)
						{
							resourcesFile << '\0';
						}
						remainingPageCapacity = pageSize;
						filePtr = filesBySize.begin();
					}
					std::ifstream resourceFile(filePtr->second, std::ios::binary);
					resourcesFile << resourceFile.rdbuf();
					unsigned long long alignedSize = (filePtr->first + unsigned long long{ 7u }) & ~((unsigned long long)7u);
					for (auto i = filePtr->first; i != alignedSize; ++i)
					{
						resourcesFile << '\0';
					}
					remainingPageCapacity -= alignedSize;
					resourceNamespace.addResource(filePtr->second.lexically_relative(inputResourcesPath), currentResourcesFileLength, currentResourcesFileLength + alignedSize);
					currentResourcesFileLength += alignedSize;
					filesBySize.erase(filePtr);
				} while (!filesBySize.empty());
			}
			{
				//link references to resources from other resources
				const auto inputResourceLinkingPath = inputPath / "Linking" / "Resources";
				for (const auto& entry : std::filesystem::recursive_directory_iterator(inputResourceLinkingPath))
				{
					if (!entry.is_regular_file())
					{
						continue;
					}
					const auto& path = entry.path();
					const auto relativePath = path.lexically_relative(inputResourceLinkingPath);
					const ResourceLocation& resourceLocation = resourceNamespace.find(relativePath);
					auto linkFileLength = std::filesystem::file_size(path);
					std::unique_ptr<char[]> linkData{ new char[linkFileLength] };
					std::ifstream linkFile{ path, std::ios::binary };
					linkFile.read(linkData.get(), linkFileLength);
					const auto linkDataEnd = linkData.get() + linkFileLength;
					for (char* i = linkData.get(); i != linkDataEnd;)
					{
						char* resourceName = i;
						auto resourceNameLength = std::strlen(resourceName);
						uint64_t offsetInResource = *reinterpret_cast<uint64_t*>(i + resourceNameLength + 1u);
						i += resourceNameLength + 1u + sizeof(uint64_t);
						resourcesFile.seekp(resourceLocation.start + offsetInResource);
						const auto resourcePath = std::filesystem::path{ resourceName };
						auto begin = resourcePath.begin();
						++begin;
						const ResourceLocation& resourceLocationToLinkIn = resourceNamespace.find(begin, resourcePath.end());
						uint64_t resourceLocation = resourceLocationToLinkIn.start;
						resourcesFile.write(reinterpret_cast<char*>(&resourceLocation), sizeof(resourceLocation));
						resourceLocation = resourceLocationToLinkIn.end;
						resourcesFile.write(reinterpret_cast<char*>(&resourceLocation), sizeof(resourceLocation));
					}
				}
			}
			{
				//create Resources.h
				std::ofstream headerFile{ headerFilePath };
				headerFile << "#pragma once\n";
				headerFile << "#include <ResourceLocation.h>\n";
				headerFile << "\n";
				headerFile << "namespace Resources\n";
				headerFile << "{\n";
				createResourcesHeaderFile(resourceNamespace, headerFile, Indent{ 1u });
				headerFile << "};\n";
			}
		}
	}
}

int main(int argc, char** argv)
{
	try
	{
		if (argc < 4)
		{
			std::cerr << "Needs an input, output and importer directory\n";
			return 1;
		}
		auto inputDir = std::filesystem::path{ argv[1] };
		auto intermidiateDir = std::filesystem::path{ argv[2] };
		auto outputDir = std::filesystem::path{ argv[3] };
		auto importerDir = std::filesystem::path{ argv[4] };

		auto resourcesHeaderPath = inputDir / "Resources.h";
		auto lastBuildTime = std::filesystem::exists(resourcesHeaderPath) ? std::filesystem::last_write_time(resourcesHeaderPath) : std::filesystem::file_time_type{};
		std::unordered_map<std::string, Importer> fileImporters = getFileImporters(importerDir, lastBuildTime);

		bool succeeded = importDirectory(inputDir, intermidiateDir, std::filesystem::path{ "Resources" }, &fileImporters, importResource);
		if (!succeeded)
		{
			return 1;
		}

		combineResourcesIntoOneFile(intermidiateDir, outputDir / "Resources.data", resourcesHeaderPath);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << "\n";
		return 1;
	}
	catch (...)
	{
		std::cerr << "Failed\n";
		return 1;
	}
}