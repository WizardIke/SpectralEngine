#include <iostream>
#include <filesystem>
#include <fstream>
#include <unordered_map>
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

	class Importer
	{
#ifdef _WIN32
		HMODULE library;
#endif // _WIN32
		bool(*importResourcePtr)(const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath, void* importResourceContext,
			bool(*importResource)(void* context, const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath));
	public:
#ifdef _WIN32
		Importer(const std::filesystem::path& path) : library{ LoadLibrary(path.c_str()) }
		{
			if (library == NULL)
			{
				throw std::runtime_error{"Failed to load resource importer " + path.string() };
			}
			importResourcePtr = reinterpret_cast<decltype(importResourcePtr)>(GetProcAddress(library, "importResource"));
			if (importResourcePtr == NULL)
			{
				FreeLibrary(library);
				throw std::runtime_error{ "Failed to load the import function from " + path.string() };
			}
		}

		~Importer()
		{
			FreeLibrary(library);
		}
#endif // _WIN32

		bool importResource(const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath, void* importResourceContext,
			bool(*importResource)(void* context, const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath))
		{
			return importResourcePtr(baseInputPath, baseOutputPath, relativeInputPath, importResourceContext, importResource);
		}
	};
}

static bool importDirectory(const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath, void* importResourceContext,
	bool(*importResource)(void* context, const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath))
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

static std::unordered_map<std::string, Importer> getFileImporters(const std::filesystem::path& importerPath)
{
	std::unordered_map<std::string, Importer> fileImporters{};
	for (const auto& entry : std::filesystem::directory_iterator(importerPath))
	{
		const auto& fileName = entry.path().filename().string();
		if (entry.is_regular_file() && endsWith(fileName, "ResourceImporter.dll"))
		{
			fileImporters.emplace("." + fileName.substr(0u, fileName.size() - 20u), entry.path());
		}
	}
	return fileImporters;
}

static bool importResource(void* context, const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath)
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

static std::filesystem::path removeExtensions(const std::filesystem::path& path)
{
	auto result = path.stem();
	while (!result.extension().empty())
	{
		result = result.stem();
	}
	return result;
}

static bool createResourcesHeaderFile(const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeOutputFileName, std::string& headerFile, Indent indent,
	std::filesystem::file_time_type headerFileLastModifiedTime)
{
	bool hasChanged = false;
	for (const auto& entry : std::filesystem::directory_iterator(baseOutputPath / relativeOutputFileName))
	{
		const auto fileName = entry.path().filename();
		if (entry.is_directory())
		{
			headerFile << indent << "namespace " << fileName.string() << "\n";
			headerFile << indent << "{\n";
			hasChanged |= createResourcesHeaderFile(baseOutputPath, relativeOutputFileName / fileName, headerFile, indent + 1u, headerFileLastModifiedTime);
			headerFile << indent << "};\n";
		}
		else if (entry.is_regular_file())
		{
			if (entry.last_write_time() >= headerFileLastModifiedTime)
			{
				hasChanged = true;
			}
			headerFile << indent << "inline constexpr const wchar_t* " << removeExtensions(fileName).string() << " = LR\"/:*?<>|(" << (relativeOutputFileName / fileName).string() << ")/:*?<>|\";\n";
		}
	}
	return hasChanged;
}

static void createResourcesHeaderFile(const std::filesystem::path& outputPath, const std::filesystem::path& headerFilePath)
{
	bool headerFileExists = std::filesystem::exists(headerFilePath);
	auto headerFileLastModifiedTime = headerFileExists ? std::filesystem::last_write_time(headerFilePath) : std::filesystem::file_time_type{};
	std::string header{};
	header << "#pragma once\n";
	header << "\n";
	header << "namespace Resources\n";
	header << "{\n";
	bool hasChanged = createResourcesHeaderFile(outputPath, "Resources", header, Indent{ 1u }, headerFileLastModifiedTime);
	header << "};\n";
	if (hasChanged || !headerFileExists)
	{
		std::ofstream headerFile{ headerFilePath };
		headerFile << header;
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
		const char* inputDir = argv[1];
		const char* outputDir = argv[2];
		const char* importerDir = argv[3];


		std::unordered_map<std::string, Importer> fileImporters = getFileImporters(importerDir);

		namespace fs = std::filesystem;
		bool succeeded = importDirectory(fs::path{ inputDir }, fs::path{ outputDir }, fs::path{ "Resources" }, &fileImporters, importResource);
		if (!succeeded)
		{
			return 1;
		}

		createResourcesHeaderFile(fs::path{ outputDir }, fs::path{ inputDir } / "Resources.h" );
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