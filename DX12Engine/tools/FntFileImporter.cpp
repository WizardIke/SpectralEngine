#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string_view>
#include <string>
#include <codecvt>
#include <locale>
#include <algorithm>

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
	std::wstring textureFileName;
	float lineHeight;
	float scaleW;
	float scaleH;
	std::vector<CharDescriptor> charDescriptors;
	std::vector<Kerning> kernings;
};

static std::vector<std::string_view> splitByChar(std::string_view str, char token)
{
	std::vector<std::string_view> results;
	std::size_t startIndex = 0u;
	const auto strSize = str.size();
	for (std::size_t i = 0u; i != strSize; ++i)
	{
		if (str[i] == token)
		{
			results.push_back(str.substr(startIndex, i - startIndex));
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

static unsigned long stringToLong(std::string_view str)
{
	return std::stol(std::string(str));
}

static std::string replaceFileExtension(std::string_view str, std::string_view extension)
{
	auto index = str.find_last_of(".");
	auto result = std::string(str.substr(0u, index));
	result += extension;
	return result;
}

static std::wstring toWstring(std::string_view str)
{
	return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(str.data(), str.data() + str.size());
}

static bool passLine(std::string_view line, unsigned long lineNumber, FontDescriptor& font)
{
	auto trimmedLine = trim(line);
	if(trimmedLine.empty()) { return true; }
	auto tokens = splitByChar(trimmedLine, ' ');
	if(tokens[0] == "common")
	{
		auto end = tokens.end();
		for(auto current = tokens.begin() + 1u; current != end; ++current)
		{
			auto info = trim(*current);
			
			
			auto indexOfNameEnd = info.find_first_of("=");
			if (indexOfNameEnd == std::string_view::npos || indexOfNameEnd == 0)
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return false;
			}
			std::string_view name = info.substr(0, indexOfNameEnd);
			std::string_view data = info.substr(indexOfNameEnd, info.size() - indexOfNameEnd);
			
			if(name == "lineHeight")
			{
				font.lineHeight = static_cast<float>(stringToLong(data));
			}
			else if(name == "scaleW")
			{
				font.scaleW = static_cast<float>(stringToLong(data));
			}
			else if(name == "scaleH")
			{
				font.scaleH = static_cast<float>(stringToLong(data));
			}
			else if(name == "pages")
			{
				if(data != "1")
				{
					std::cout << "error on line " << lineNumber << " fonts that use more than one texture aren't supported" << std::endl;
					return false;
				}
			}
		}
	}
	else if(tokens[0] == "page")
	{
		auto end = tokens.end();
		for(auto current = tokens.begin() + 1u; current != end; ++current)
		{
			auto info = trim(*current);
			
			
			auto indexOfNameEnd = info.find_first_of("=");
			if (indexOfNameEnd == std::string_view::npos || indexOfNameEnd == 0)
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return false;
			}
			std::string_view name = info.substr(0, indexOfNameEnd);
			std::string_view data = info.substr(indexOfNameEnd, info.size() - indexOfNameEnd);
			
			if(name == "file")
			{
				font.textureFileName = toWstring(data);
			}
		}
	}
	else if(tokens[0] == "chars")
	{
		auto end = tokens.end();
		for(auto current = tokens.begin() + 1u; current != end; ++current)
		{
			auto info = trim(*current);
			
			
			auto indexOfNameEnd = info.find_first_of("=");
			if (indexOfNameEnd == std::string_view::npos || indexOfNameEnd == 0)
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return false;
			}
			std::string_view name = info.substr(0, indexOfNameEnd);
			std::string_view data = info.substr(indexOfNameEnd, info.size() - indexOfNameEnd);
			
			if(name == "count")
			{
				font.charDescriptors.reserve(std::stoul(std::string(data)));
			}
		}
	}
	else if(tokens[0] == "char")
	{
		CharDescriptor charDesc{};
		auto end = tokens.end();
		for(auto current = tokens.begin() + 1u; current != end; ++current)
		{
			auto info = trim(*current);
			
			
			auto indexOfNameEnd = info.find_first_of("=");
			if (indexOfNameEnd == std::string_view::npos || indexOfNameEnd == 0)
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return false;
			}
			std::string_view name = info.substr(0, indexOfNameEnd);
			std::string_view data = info.substr(indexOfNameEnd, info.size() - indexOfNameEnd);
			
			if(name == "id")
			{
				charDesc.id = static_cast<uint32_t>(std::stoul(std::string(data)));
			}
			else if(name == "x")
			{
				charDesc.x = static_cast<float>(stringToLong(data));
			}
			else if(name == "y")
			{
				charDesc.y = static_cast<float>(stringToLong(data));
			}
			else if(name == "width")
			{
				charDesc.width = static_cast<float>(stringToLong(data));
			}
			else if(name == "height")
			{
				charDesc.height = static_cast<float>(stringToLong(data));
			}
			else if(name == "xoffset")
			{
				charDesc.offsetX = static_cast<float>(stringToLong(data));
			}
			else if(name == "yoffset")
			{
				charDesc.offsetY = static_cast<float>(stringToLong(data));
			}
			else if(name == "xadvance")
			{
				charDesc.advanceX = static_cast<float>(stringToLong(data));
			}
		}
		
		font.charDescriptors.push_back(std::move(charDesc));
	}
	else if(tokens[0] == "kernings")
	{
		auto end = tokens.end();
		for(auto current = tokens.begin() + 1u; current != end; ++current)
		{
			auto info = trim(*current);
			
			
			auto indexOfNameEnd = info.find_first_of("=");
			if (indexOfNameEnd == std::string_view::npos || indexOfNameEnd == 0)
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return false;
			}
			std::string_view name = info.substr(0, indexOfNameEnd);
			std::string_view data = info.substr(indexOfNameEnd, info.size() - indexOfNameEnd);
			
			if(name == "count")
			{
				font.kernings.resize(std::stoul(std::string(data)));
			}
		}
	}
	else if(tokens[0] == "kerning")
	{
		Kerning kerning{0u, 0.0f};
		auto end = tokens.end();
		for(auto current = tokens.begin() + 1u; current != end; ++current)
		{
			auto info = trim(*current);
			
			
			auto indexOfNameEnd = info.find_first_of("=");
			if (indexOfNameEnd == std::string_view::npos || indexOfNameEnd == 0)
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return false;
			}
			std::string_view name = info.substr(0, indexOfNameEnd);
			std::string_view data = info.substr(indexOfNameEnd, info.size() - indexOfNameEnd);
			
			if(name == "first")
			{
				kerning.firstAndSecondChar |= static_cast<uint64_t>(std::stoul(std::string(data))) << 32u;
			}
			else if(name == "second")
			{
				kerning.firstAndSecondChar |= static_cast<uint64_t>(std::stoul(std::string(data)));
			}
			else if(name == "amount")
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
	font.lineHeight *= oneOverScaleY;
	for(auto& charDesc : font.charDescriptors)
	{
		charDesc.x *= oneOverScaleX;
		charDesc.y *= oneOverScaleY;
		charDesc.width *= oneOverScaleX;
		charDesc.height *= oneOverScaleY;
		charDesc.offsetX *= oneOverScaleX;
		charDesc.offsetY *= oneOverScaleY;
		charDesc.advanceX *= oneOverScaleX;
	}
	for(auto& kerning : font.kernings)
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

static void writeToFontFile(std::ofstream& outFile, const FontDescriptor& font)
{
	auto fileNameLengthBytes = (font.textureFileName.size() + 1u) * sizeof(wchar_t); //include null char
	outFile.write(reinterpret_cast<const char*>(font.textureFileName.c_str()), fileNameLengthBytes);
	constexpr char padding = '\0';
	auto paddedLength = alignedLength(fileNameLengthBytes, alignof(uint32_t));
	for(auto i = paddedLength - fileNameLengthBytes; i != 0u; --i)
	{
		outFile.write(&padding, sizeof(char));
	}
	uint32_t charCount = static_cast<uint32_t>(font.charDescriptors.size());
	outFile.write(reinterpret_cast<const char*>(&charCount), sizeof(uint32_t));
	if(font.charDescriptors.size() != 0u)
	{
		outFile.write(reinterpret_cast<const char*>(font.charDescriptors.data()), font.charDescriptors.size() * sizeof(CharDescriptor));
	}
	uint32_t kerningCount = static_cast<uint32_t>(font.kernings.size());
	auto lengthWritten = paddedLength + sizeof(uint32_t) + font.charDescriptors.size() * sizeof(CharDescriptor) + sizeof(uint32_t);
	paddedLength = alignedLength(lengthWritten, alignof(Kerning));
	for(auto i = paddedLength - lengthWritten; i != 0u; --i)
	{
		outFile.write(&padding, sizeof(char));
	}
	if(font.kernings.size() != 0u)
	{
		outFile.write(reinterpret_cast<const char*>(font.kernings.data()), font.kernings.size() * sizeof(Kerning));
	}
}

int main(int argc, char** argv)
{
	if(argc < 2)
	{
		return 1;
	}
	std::ifstream inFile{argv[1]};
	
	FontDescriptor font{};
	std::string line;
	unsigned long lineNumber = 1u;
	while (std::getline(inFile, line))
	{
		bool succeeded = passLine(line, lineNumber, font);
		if(!succeeded)
		{
			return 1;
		}
		++lineNumber;
	}
	inFile.close();
	
	normalizeFontScale(font);
	sortFont(font);
	
	auto outFileName = replaceFileExtension(argv[1], ".font");
	std::ofstream outFile{outFileName, std::ios::binary};
	writeToFontFile(outFile, font);
	outFile.close();
	
	std::cout << "done\n";
}