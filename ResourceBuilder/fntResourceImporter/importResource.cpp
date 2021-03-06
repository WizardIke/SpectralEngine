#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string_view>
#include <string>
#include <algorithm>

namespace
{
	struct CharDescriptor
	{
		uint32_t id;
		float x;
		float y;
		float width;
		float height;
		float offsetX;
		float offsetY;
		float advanceX;
	};

	struct Kerning
	{
		uint64_t firstAndSecondChar;
		float amount;
	};

	struct FontDescriptor
	{
		std::string textureFileName;
		float width;
		float height;
		float scaleW;
		float scaleH;
		std::vector<CharDescriptor> charDescriptors;
		std::vector<Kerning> kernings;
	};

	static std::vector<std::string_view> splitByCharIngoringDuplicateSeparators(std::string_view str, char token)
	{
		std::vector<std::string_view> results;
		std::size_t startIndex = 0u;
		const auto strSize = str.size();
		for (std::size_t i = 0u; i != strSize; ++i)
		{
			if (str[i] == token)
			{
				if (startIndex != i)
				{
					results.push_back(str.substr(startIndex, i - startIndex));
				}
				startIndex = i + 1u;
			}
			if (i + 1u == strSize)
			{
				results.push_back(str.substr(startIndex, i - startIndex + 1));
			}
		}

		return results;
	}

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

	static long stringToLong(std::string_view str)
	{
		return std::stol(std::string(str));
	}

	static bool passLine(std::string_view line, unsigned long lineNumber, FontDescriptor& font, const std::filesystem::path& baseInputDirectory, const std::filesystem::path& currentDirectory)
	{
		auto trimmedLine = trim(line, " \t\r");
		if (trimmedLine.empty()) { return true; }
		auto tokens = splitByCharIngoringDuplicateSeparators(trimmedLine, ' ');
		if (tokens[0] == "common")
		{
			auto end = tokens.end();
			for (auto current = tokens.begin() + 1u; current != end; ++current)
			{
				auto info = trim(*current);


				auto indexOfNameEnd = info.find_first_of("=");
				if (indexOfNameEnd == std::string_view::npos || indexOfNameEnd == 0)
				{
					std::cerr << "error on line: " << lineNumber << std::endl;
					return false;
				}
				if (indexOfNameEnd == info.size())
				{
					std::cerr << "error on line: " << lineNumber << std::endl;
					return false;
				}
				std::string_view name = info.substr(0, indexOfNameEnd);
				std::string_view data = info.substr(indexOfNameEnd + 1u, info.size() - indexOfNameEnd - 1u);

				if (name == "lineHeight")
				{
					try
					{
						font.height = static_cast<float>(stringToLong(data));
						font.width = font.height;
					}
					catch (...)
					{
						std::cerr << "invalid lineHeight of " << data << " on line " << lineNumber << std::endl;
						return false;
					}
				}
				else if (name == "scaleW")
				{
					try
					{
						font.scaleW = static_cast<float>(stringToLong(data));
					}
					catch (...)
					{
						std::cerr << "invalid scaleW of " << data << " on line " << lineNumber << std::endl;
						return false;
					}
				}
				else if (name == "scaleH")
				{
					try
					{
						font.scaleH = static_cast<float>(stringToLong(data));
					}
					catch (...)
					{
						std::cerr << "invalid scaleH of " << data << " on line " << lineNumber << std::endl;
						return false;
					}
				}
				else if (name == "pages")
				{
					if (data != "1")
					{
						std::cerr << "error on line " << lineNumber << " fonts that use more than one texture aren't supported" << std::endl;
						return false;
					}
				}
			}
		}
		else if (tokens[0] == "page")
		{
			auto end = tokens.end();
			for (auto current = tokens.begin() + 1u; current != end; ++current)
			{
				auto info = trim(*current);


				auto indexOfNameEnd = info.find_first_of("=");
				if (indexOfNameEnd == std::string_view::npos || indexOfNameEnd == 0)
				{
					std::cerr << "error on line: " << lineNumber << std::endl;
					return false;
				}
				if (indexOfNameEnd == info.size())
				{
					std::cerr << "error on line: " << lineNumber << std::endl;
					return false;
				}
				std::string_view name = info.substr(0, indexOfNameEnd);
				std::string_view data = info.substr(indexOfNameEnd + 1u, info.size() - indexOfNameEnd - 1u);

				if (name == "file")
				{
					std::filesystem::path path{ trim(data, "\"") };
					if (!path.is_absolute())
					{
						path = currentDirectory / path;
					}
					while (path.has_extension())
					{
						path.replace_extension();
					}
					font.textureFileName = path.lexically_relative(baseInputDirectory).string();
				}
			}
		}
		else if (tokens[0] == "chars")
		{
			auto end = tokens.end();
			for (auto current = tokens.begin() + 1u; current != end; ++current)
			{
				auto info = trim(*current);


				auto indexOfNameEnd = info.find_first_of("=");
				if (indexOfNameEnd == std::string_view::npos || indexOfNameEnd == 0)
				{
					std::cerr << "error on line: " << lineNumber << std::endl;
					return false;
				}
				if (indexOfNameEnd == info.size())
				{
					std::cerr << "error on line: " << lineNumber << std::endl;
					return false;
				}
				std::string_view name = info.substr(0, indexOfNameEnd);
				std::string_view data = info.substr(indexOfNameEnd + 1u, info.size() - indexOfNameEnd - 1u);

				if (name == "count")
				{
					font.charDescriptors.reserve(std::stoul(std::string(data)));
				}
			}
		}
		else if (tokens[0] == "char")
		{
			CharDescriptor charDesc{};
			auto end = tokens.end();
			for (auto current = tokens.begin() + 1u; current != end; ++current)
			{
				auto info = trim(*current);


				auto indexOfNameEnd = info.find_first_of("=");
				if (indexOfNameEnd == std::string_view::npos || indexOfNameEnd == 0)
				{
					std::cerr << info << " is not given a value on line " << lineNumber << std::endl;
					return false;
				}
				std::string_view name = info.substr(0, indexOfNameEnd);
				std::string_view data = info.substr(indexOfNameEnd + 1u, info.size() - indexOfNameEnd - 1u);

				if (name == "id")
				{
					try
					{
						charDesc.id = static_cast<uint32_t>(std::stoul(std::string(data)));
					}
					catch (...)
					{
						std::cerr << "invalid id of " << data << " on line " << lineNumber << std::endl;
						return false;
					}
				}
				else if (name == "x")
				{
					try
					{
						charDesc.x = static_cast<float>(stringToLong(data));
					}
					catch (...)
					{
						std::cerr << "invalid x of " << data << " on line " << lineNumber << std::endl;
						return false;
					}
				}
				else if (name == "y")
				{
					try
					{
						charDesc.y = static_cast<float>(stringToLong(data));
					}
					catch (...)
					{
						std::cerr << "invalid y of " << data << " on line " << lineNumber << std::endl;
						return false;
					}
				}
				else if (name == "width")
				{
					try
					{
						charDesc.width = static_cast<float>(stringToLong(data));
					}
					catch (...)
					{
						std::cerr << "invalid width of " << data << " on line " << lineNumber << std::endl;
						return false;
					}
				}
				else if (name == "height")
				{
					try
					{
						charDesc.height = static_cast<float>(stringToLong(data));
					}
					catch (...)
					{
						std::cerr << "invalid height of " << data << " on line " << lineNumber << std::endl;
						return false;
					}
				}
				else if (name == "xoffset")
				{
					try
					{
						charDesc.offsetX = static_cast<float>(stringToLong(data));
					}
					catch (...)
					{
						std::cerr << "invalid xoffset of " << data << " on line " << lineNumber << std::endl;
						return false;
					}
				}
				else if (name == "yoffset")
				{
					try
					{
						charDesc.offsetY = static_cast<float>(stringToLong(data));
					}
					catch (...)
					{
						std::cerr << "invalid yoffset of " << data << " on line " << lineNumber << std::endl;
						return false;
					}
				}
				else if (name == "xadvance")
				{
					try
					{
						charDesc.advanceX = static_cast<float>(stringToLong(data));
					}
					catch (...)
					{
						std::cerr << "invalid xadvance of " << data << " on line " << lineNumber << std::endl;
						return false;
					}
				}
			}

			font.charDescriptors.push_back(std::move(charDesc));
		}
		else if (tokens[0] == "kernings")
		{
			auto end = tokens.end();
			for (auto current = tokens.begin() + 1u; current != end; ++current)
			{
				auto info = trim(*current);


				auto indexOfNameEnd = info.find_first_of("=");
				if (indexOfNameEnd == std::string_view::npos || indexOfNameEnd == 0)
				{
					std::cerr << "error on line: " << lineNumber << std::endl;
					return false;
				}
				if (indexOfNameEnd == info.size())
				{
					std::cerr << "error on line: " << lineNumber << std::endl;
					return false;
				}
				std::string_view name = info.substr(0, indexOfNameEnd);
				std::string_view data = info.substr(indexOfNameEnd + 1u, info.size() - indexOfNameEnd - 1u);

				if (name == "count")
				{
					font.kernings.reserve(std::stoul(std::string(data)));
				}
			}
		}
		else if (tokens[0] == "kerning")
		{
			Kerning kerning{ 0u, 0.0f };
			auto end = tokens.end();
			for (auto current = tokens.begin() + 1u; current != end; ++current)
			{
				auto info = trim(*current);


				auto indexOfNameEnd = info.find_first_of("=");
				if (indexOfNameEnd == std::string_view::npos || indexOfNameEnd == 0)
				{
					std::cerr << "error on line: " << lineNumber << std::endl;
					return false;
				}
				if (indexOfNameEnd == info.size())
				{
					std::cerr << "error on line: " << lineNumber << std::endl;
					return false;
				}
				std::string_view name = info.substr(0, indexOfNameEnd);
				std::string_view data = info.substr(indexOfNameEnd + 1u, info.size() - indexOfNameEnd - 1u);

				if (name == "first")
				{
					kerning.firstAndSecondChar |= static_cast<uint64_t>(std::stoul(std::string(data))) << 32u;
				}
				else if (name == "second")
				{
					kerning.firstAndSecondChar |= static_cast<uint64_t>(std::stoul(std::string(data)));
				}
				else if (name == "amount")
				{
					kerning.amount = static_cast<float>(stringToLong(data));
				}
			}

			font.kernings.push_back(std::move(kerning));
		}
		return true;
	}

	static void normalizeFontScale(FontDescriptor& font)
	{
		const float oneOverScaleX = 1.0f / font.scaleW;
		const float oneOverScaleY = 1.0f / font.scaleH;
		font.scaleW = 1.0f;
		font.scaleH = 1.0f;
		font.width *= oneOverScaleX;
		font.height *= oneOverScaleY;
		for (auto& charDesc : font.charDescriptors)
		{
			charDesc.x *= oneOverScaleX;
			charDesc.y *= oneOverScaleY;
			charDesc.width *= oneOverScaleX;
			charDesc.height *= oneOverScaleY;
			charDesc.offsetX *= oneOverScaleX;
			charDesc.offsetY *= oneOverScaleY;
			charDesc.advanceX *= oneOverScaleX;
		}
		for (auto& kerning : font.kernings)
		{
			kerning.amount *= oneOverScaleX;
		}
	}

	static void sortFont(FontDescriptor& font)
	{
		std::sort(font.charDescriptors.begin(), font.charDescriptors.end(),
			[](const CharDescriptor& lhs, const CharDescriptor& rhs) {return lhs.id < rhs.id; });

		std::sort(font.kernings.begin(), font.kernings.end(),
			[](const Kerning& lhs, const Kerning& rhs) {return lhs.firstAndSecondChar < rhs.firstAndSecondChar; });
	}

	static constexpr std::size_t alignedLength(std::size_t length, std::size_t alignment) noexcept
	{
		return (length + (alignment - 1u)) & ~(alignment - 1u);
	}

	static void writeToFontFile(std::ofstream& outFile, std::ofstream& linkFile, const FontDescriptor& font)
	{
		auto fileNameLengthBytes = (font.textureFileName.size() + 1u) * sizeof(char); //include null char
		linkFile.write(font.textureFileName.c_str(), fileNameLengthBytes);
		uint64_t textureResourceLocation = 0u;
		linkFile.write(reinterpret_cast<const char*>(&textureResourceLocation), sizeof(textureResourceLocation));


		uint64_t textureResourcePadding = 0u;
		outFile.write(reinterpret_cast<const char*>(&textureResourcePadding), sizeof(textureResourcePadding));
		outFile.write(reinterpret_cast<const char*>(&textureResourcePadding), sizeof(textureResourcePadding));

		outFile.write(reinterpret_cast<const char*>(&font.width), sizeof(float));
		outFile.write(reinterpret_cast<const char*>(&font.height), sizeof(float));

		uint32_t charCount = static_cast<uint32_t>(font.charDescriptors.size());
		outFile.write(reinterpret_cast<const char*>(&charCount), sizeof(uint32_t));
		if (font.charDescriptors.size() != 0u)
		{
			outFile.write(reinterpret_cast<const char*>(font.charDescriptors.data()), font.charDescriptors.size() * sizeof(CharDescriptor));
		}

		uint32_t kerningCount = static_cast<uint32_t>(font.kernings.size());
		outFile.write(reinterpret_cast<const char*>(&kerningCount), sizeof(uint32_t));
		auto lengthWritten = 2u * sizeof(uint64_t) + sizeof(float) + sizeof(float) + sizeof(uint32_t)
			+ font.charDescriptors.size() * sizeof(CharDescriptor) + sizeof(uint32_t);
		auto paddedLength = alignedLength(lengthWritten, alignof(Kerning));
		for (auto i = paddedLength - lengthWritten; i != 0u; --i)
		{
			constexpr char padding = '\0';
			outFile.write(&padding, sizeof(char));
		}
		if (font.kernings.size() != 0u)
		{
			outFile.write(reinterpret_cast<const char*>(font.kernings.data()), font.kernings.size() * sizeof(Kerning));
		}
	}
}

extern "C"
{
	typedef bool(*ImportResourceType)(void* context, const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath);

#ifdef _WIN32
		__declspec(dllexport)
#endif
		bool importResource(const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath, void* importResourceContext,
			ImportResourceType importResource, bool forceReImport) noexcept
	{
		try
		{
			auto inputPath = baseInputPath / relativeInputPath;
			auto outputPath = baseOutputPath / relativeInputPath;
			while (outputPath.has_extension())
			{
				outputPath.replace_extension();
			}
			auto linkingOutputPath = baseOutputPath / "Linking" / relativeInputPath;
			while (linkingOutputPath.has_extension())
			{
				linkingOutputPath.replace_extension();
			}

			if (!forceReImport && std::filesystem::exists(linkingOutputPath) && std::filesystem::exists(outputPath) && std::filesystem::last_write_time(outputPath) > std::filesystem::last_write_time(inputPath))
			{
				return true;
			}

			std::cout << "importing " << inputPath.string() << "\n";
			auto inDirectory = inputPath.parent_path();
			std::ifstream inFile{ inputPath, std::ios::binary };

			FontDescriptor font{};
			std::string line;
			unsigned long lineNumber = 1u;
			while (std::getline(inFile, line))
			{
				bool succeeded = passLine(line, lineNumber, font, baseInputPath, inDirectory);
				if (!succeeded)
				{
					return 1;
				}
				++lineNumber;
			}
			inFile.close();

			normalizeFontScale(font);
			sortFont(font);

			const auto& outputDirectory = outputPath.parent_path();
			if (!std::filesystem::exists(outputDirectory))
			{
				std::filesystem::create_directories(outputDirectory);
			}
			std::ofstream outFile{ outputPath, std::ios::binary };
			const auto& linkingOutputDirectory = linkingOutputPath.parent_path();
			if (!std::filesystem::exists(linkingOutputDirectory))
			{
				std::filesystem::create_directories(linkingOutputDirectory);
			}
			std::ofstream linkFile{ linkingOutputPath, std::ios::binary };
			writeToFontFile(outFile, linkFile, font);
			outFile.close();
		}
		catch (std::exception& e)
		{
			std::cerr << "failed: " << e.what() << "\n";
			return false;
		}
		catch (...)
		{
			std::cerr << "failed\n";
			return false;
		}
		return true;
	}
}