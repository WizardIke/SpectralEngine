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

	std::ostream& operator<<(std::ostream& out, Indent indent)
	{
		for (; indent.level != 0u; --indent.level)
		{
			out << '\t';
		}
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

static bool processDirectory(const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath, void* importResourceContext,
	bool(*importResource)(void* context, const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath),
	std::ofstream& headerFile, Indent indent)
{
	namespace fs = std::filesystem;

	auto inputPath = baseInputPath / relativeInputPath;
	for (const auto& entry : fs::directory_iterator(inputPath))
	{
		const auto fileName = entry.path().filename();
		if (entry.is_directory())
		{
			headerFile << indent << "namespace " << fileName.string() << "\n";
			headerFile << indent << "{\n";
			bool succeeded = processDirectory(baseInputPath, baseOutputPath, relativeInputPath / fileName, importResourceContext, importResource, headerFile, indent + 1u);
			headerFile << indent << "};\n";
			if (!succeeded)
			{
				return false;
			}
		}
		else if (entry.is_regular_file())
		{
			auto inputFileName = inputPath / fileName;
			auto relativeOutputFileName = relativeInputPath / fileName;
			relativeOutputFileName += ".data";
			auto outputFileName = baseOutputPath / relativeOutputFileName;
			bool succeeded = importResource(importResourceContext, baseInputPath, baseOutputPath, inputFileName);
			if (!succeeded)
			{
				return false;
			}
			headerFile << indent << "static constexpr " << fileName.stem().string() << " = L\"" << relativeOutputFileName.string() << "\";\n";
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
			fileImporters.emplace(fileName.substr(0u, fileName.size() - 21u), entry.path());
		}
	}
	return fileImporters;
}

static bool importResource(void* context, const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath)
{
	auto& fileImporters = *static_cast<std::unordered_map<std::string, Importer>*>(context);
	auto fileImporter = fileImporters.find(relativeInputPath.extension().string());
	if (fileImporter == fileImporters.end())
	{
		std::cerr << "No importer for " << relativeInputPath.extension().string() << " files\n";
		return false;
	}
	return fileImporter->second.importResource(baseInputPath, baseOutputPath, relativeInputPath, &fileImporters, importResource);
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

		std::ofstream headerFile{ fs::path{ inputDir } / "Resources.h" };
		headerFile << "#pragma once\n\n";

		headerFile << "namespace Resources\n";
		headerFile << "{\n";
		bool succeeded = processDirectory(fs::path{ inputDir }, fs::path{ outputDir }, fs::path{ "Resources" }, &fileImporters, importResource, headerFile, Indent{ 1u });
		headerFile << "};\n";
		if (!succeeded)
		{
			return 1;
		}
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