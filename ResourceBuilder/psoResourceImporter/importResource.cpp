#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <ios>

namespace
{
	struct SoDeclaration
	{
		unsigned long Stream;
		const char* SemanticName;
		unsigned long SemanticIndex;
		unsigned char StartComponent;
		unsigned char ComponentCount;
		unsigned char OutputSlot;
	};

	struct StreamOutputDesc
	{
		std::vector<SoDeclaration> soDeclarations;
		std::vector<unsigned long> bufferStrides;
		unsigned long rasterizedStream = 0u;
	};

	enum class Blend
	{
		zero = 1,
		one = 2,
		srcColor = 3,
		invSrcColor = 4,
		srcAlpha = 5,
		invSrcAlpha = 6,
		destAlpha = 7,
		invDescAlpha = 8,
		destColor = 9,
		invDestColor = 10,
		srcAlphaSat = 11,
		blendFactor = 14,
		invBlendFactor = 15,
		src1Color = 16,
		invSrc1Color = 17,
		src1Alpha = 18,
		invSrc1Alpha = 19
	};

	enum class BlendOp
	{
		add = 1,
		subtract = 2,
		revSubtract = 3,
		min = 4,
		max = 5
	};

	enum class LogicOp
	{
		clear = 0,
		set = (clear + 1),
		copy = (set + 1),
		copyInverted = (copy + 1),
		noOp = (copyInverted + 1),
		invert = (noOp + 1),
		andOp = (invert + 1),
		nand = (andOp + 1),
		orOp = (nand + 1),
		nor = (orOp + 1),
		xorOp = (nor + 1),
		equiv = (xorOp + 1),
		andReverse = (equiv + 1),
		andInverted = (andReverse + 1),
		orReverse = (andInverted + 1),
		orInverted = (orReverse + 1)
	};

	enum class ColorWriteEnable
	{
		red,
		green,
		blue,
		alpha,
		all
	};

	struct RenderTargetBlendDesc
	{
		bool blendEnable = false;
		bool logicOpEnable = false;
		Blend srcBlend = Blend::srcColor;
		Blend destBlend = Blend::invSrcAlpha;
		BlendOp blendOp = BlendOp::add;
		Blend srcBlendAlpha = Blend::one;
		Blend destBlendAlpha = Blend::invSrcAlpha;
		BlendOp blendOpAlpha = BlendOp::add;
		LogicOp logicOp = LogicOp::clear;
		ColorWriteEnable renderTargetWriteMask = ColorWriteEnable::all;
	};

	enum class FillMode
	{
		wireframe = 2,
		solid = 3
	};

	enum class CullMode
	{
		none = 1,
		front = 2,
		back = 3
	};

	struct RasterizerDesc
	{
		FillMode fillMode = FillMode::solid;
		CullMode cullMode = CullMode::back;
		bool frontCounterClockwise = false;
		long depthBias = 0;
		float depthBiasClamp = 0.0f;
		float slopeScaledDepthBias = 0.0f;
		bool depthClipEnable = true;
		bool multisampleEnable = false;
		bool antialiasedLineEnable = false;
		unsigned long forcedSampleCount = 0u;
		bool conservativeRaster = false;
	};

	enum class DepthWriteMask
	{
		zero = 0,
		all = 1
	};

	enum class ComparisonFunc
	{
		never = 1,
		less = 2,
		equal = 3,
		lessEqual = 4,
		greater = 5,
		notEqual = 6,
		greaterEqual = 7,
		always = 8
	};

	enum class StencilOp
	{
		keep = 1,
		zero = 2,
		replace = 3,
		incrSat = 4,
		decrSat = 5,
		invert = 6,
		incr = 7,
		decr = 8
	};

	struct DepthStencilOpDesc
	{
		StencilOp stencilFailOp = StencilOp::keep;
		StencilOp stencilDepthFailOp = StencilOp::keep;
		StencilOp stencilPassOp = StencilOp::keep;
		ComparisonFunc stencilFunc = ComparisonFunc::always;
	};

	struct DepthSencilDesc
	{
		bool depthEnable = true;
		DepthWriteMask depthWriteMask = DepthWriteMask::all;
		ComparisonFunc depthFunc = ComparisonFunc::less;
		bool stencilEnable = false;
		unsigned short stencilReadMask = 0u;
		unsigned short stencilWriteMask = 0u;
		DepthStencilOpDesc frontFace;
		DepthStencilOpDesc backFace;
	};

	struct BlendDesc
	{
		bool alphaToCoverageEnable = false;
		bool independentBlendEnable = false;
		RenderTargetBlendDesc renderTarget[8];

	};

	enum class Format
	{
		UNKNOWN = 0,
		R32G32B32A32_TYPELESS = 1,
		R32G32B32A32_FLOAT = 2,
		R32G32B32A32_UINT = 3,
		R32G32B32A32_SINT = 4,
		R32G32B32_TYPELESS = 5,
		R32G32B32_FLOAT = 6,
		R32G32B32_UINT = 7,
		R32G32B32_SINT = 8,
		R16G16B16A16_TYPELESS = 9,
		R16G16B16A16_FLOAT = 10,
		R16G16B16A16_UNORM = 11,
		R16G16B16A16_UINT = 12,
		R16G16B16A16_SNORM = 13,
		R16G16B16A16_SINT = 14,
		R32G32_TYPELESS = 15,
		R32G32_FLOAT = 16,
		R32G32_UINT = 17,
		R32G32_SINT = 18,
		R32G8X24_TYPELESS = 19,
		D32_FLOAT_S8X24_UINT = 20,
		R32_FLOAT_X8X24_TYPELESS = 21,
		X32_TYPELESS_G8X24_UINT = 22,
		R10G10B10A2_TYPELESS = 23,
		R10G10B10A2_UNORM = 24,
		R10G10B10A2_UINT = 25,
		R11G11B10_FLOAT = 26,
		R8G8B8A8_TYPELESS = 27,
		R8G8B8A8_UNORM = 28,
		R8G8B8A8_UNORM_SRGB = 29,
		R8G8B8A8_UINT = 30,
		R8G8B8A8_SNORM = 31,
		R8G8B8A8_SINT = 32,
		R16G16_TYPELESS = 33,
		R16G16_FLOAT = 34,
		R16G16_UNORM = 35,
		R16G16_UINT = 36,
		R16G16_SNORM = 37,
		R16G16_SINT = 38,
		R32_TYPELESS = 39,
		D32_FLOAT = 40,
		R32_FLOAT = 41,
		R32_UINT = 42,
		R32_SINT = 43,
		R24G8_TYPELESS = 44,
		D24_UNORM_S8_UINT = 45,
		R24_UNORM_X8_TYPELESS = 46,
		X24_TYPELESS_G8_UINT = 47,
		R8G8_TYPELESS = 48,
		R8G8_UNORM = 49,
		R8G8_UINT = 50,
		R8G8_SNORM = 51,
		R8G8_SINT = 52,
		R16_TYPELESS = 53,
		R16_FLOAT = 54,
		D16_UNORM = 55,
		R16_UNORM = 56,
		R16_UINT = 57,
		R16_SNORM = 58,
		R16_SINT = 59,
		R8_TYPELESS = 60,
		R8_UNORM = 61,
		R8_UINT = 62,
		R8_SNORM = 63,
		R8_SINT = 64,
		A8_UNORM = 65,
		R1_UNORM = 66,
		R9G9B9E5_SHAREDEXP = 67,
		R8G8_B8G8_UNORM = 68,
		G8R8_G8B8_UNORM = 69,
		BC1_TYPELESS = 70,
		BC1_UNORM = 71,
		BC1_UNORM_SRGB = 72,
		BC2_TYPELESS = 73,
		BC2_UNORM = 74,
		BC2_UNORM_SRGB = 75,
		BC3_TYPELESS = 76,
		BC3_UNORM = 77,
		BC3_UNORM_SRGB = 78,
		BC4_TYPELESS = 79,
		BC4_UNORM = 80,
		BC4_SNORM = 81,
		BC5_TYPELESS = 82,
		BC5_UNORM = 83,
		BC5_SNORM = 84,
		B5G6R5_UNORM = 85,
		B5G5R5A1_UNORM = 86,
		B8G8R8A8_UNORM = 87,
		B8G8R8X8_UNORM = 88,
		R10G10B10_XR_BIAS_A2_UNORM = 89,
		B8G8R8A8_TYPELESS = 90,
		B8G8R8A8_UNORM_SRGB = 91,
		B8G8R8X8_TYPELESS = 92,
		B8G8R8X8_UNORM_SRGB = 93,
		BC6H_TYPELESS = 94,
		BC6H_UF16 = 95,
		BC6H_SF16 = 96,
		BC7_TYPELESS = 97,
		BC7_UNORM = 98,
		BC7_UNORM_SRGB = 99,
		AYUV = 100,
		Y410 = 101,
		Y416 = 102,
		NV12 = 103,
		P010 = 104,
		P016 = 105,
		FORMAT420_OPAQUE = 106,
		YUY2 = 107,
		Y210 = 108,
		Y216 = 109,
		NV11 = 110,
		AI44 = 111,
		IA44 = 112,
		P8 = 113,
		A8P8 = 114,
		B4G4R4A4_UNORM = 115,

		P208 = 130,
		V208 = 131,
		V408 = 132
	};

	enum class InputClassification
	{
		perVertex = 0,
		perInstance = 1
	};

	struct InputElementDesc
	{
		const char* semanticName;
		unsigned long semanticIndex;
		Format format;
		unsigned long inputSlot;
		unsigned long alignedByteOffset;
		InputClassification inputSlotClass;
		unsigned long instanceDataStepRate;
	};

	struct InputLayoutDesc
	{
		std::vector<InputElementDesc> inputElementDescs;
	};

	enum class IndexBufferStripCutValue
	{
		disabled = 0,
		Oxffff = 1,
		Oxffffffff = 2
	};

	enum class PrimitiveTopologyType
	{
		undefined = 0,
		point = 1,
		line = 2,
		triangle = 3,
		patch = 4
	};

	struct SampleDesc
	{
		unsigned long count = 1u;
		unsigned long quality = 0u;
	};

	struct GraphicsPipelineStateDesc
	{
		std::string vertexShader;
		std::string pixelShader;
		StreamOutputDesc streamOutput;
		BlendDesc blendState;
		unsigned long sampleMask = 0xffffffff;
		RasterizerDesc rasterizerState;
		DepthSencilDesc depthStencilState;
		InputLayoutDesc inputLayout;
		IndexBufferStripCutValue ibStripCutValue = IndexBufferStripCutValue::disabled;
		PrimitiveTopologyType primitiveTopologyType = PrimitiveTopologyType::triangle;
		unsigned long numRenderTargets = 0u;
		Format rtvFormats[8] = { Format::UNKNOWN, Format::UNKNOWN, Format::UNKNOWN, Format::UNKNOWN, Format::UNKNOWN, Format::UNKNOWN, Format::UNKNOWN, Format::UNKNOWN };
		Format dsvFormat = Format::UNKNOWN;
		SampleDesc sampleDesc;
	};
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

static constexpr void updateOpenBraces(std::string_view str, unsigned int& numOpenBraces) noexcept
{
	for (auto c : str)
	{
		if (c == '{')
		{
			++numOpenBraces;
		}
		else if (c == '}')
		{
			--numOpenBraces;
		}
	}
}

static bool getBracedValue(std::ifstream& infile, std::string_view data, int& lineNumber, std::string& newData)
{
	unsigned int numberOfOpenBraces = 1u;
	auto indexOfFirstBrace = data.find_first_of("{");
	if (indexOfFirstBrace == std::string::npos)
	{
		std::cerr << "error on line: " << lineNumber << std::endl;
		return false;
	}
	auto indexOfNewDataStart = data.find_first_not_of(" \t", indexOfFirstBrace + 1u);
	if (indexOfNewDataStart == std::string::npos)
	{
		indexOfNewDataStart = indexOfFirstBrace + 1u;
	}
	newData = data.substr(indexOfNewDataStart, std::string::npos);
	updateOpenBraces(newData, numberOfOpenBraces);
	while (numberOfOpenBraces != 0u)
	{
		std::string line;
		bool succeeded = (bool)getline(infile, line);
		++lineNumber;
		if (!succeeded)
		{
			std::cerr << "error on line: " << lineNumber << std::endl;
			return false;
		}
		updateOpenBraces(line, numberOfOpenBraces);
		newData += line;
	}
	auto indexOfEndBrace = newData.find_last_of("}");
	newData = trim(newData.substr(0u, indexOfEndBrace));
	return true;
}

static unsigned long stringToUnsignedLong(std::string_view str)
{
	if (str.size() >= 2 && str[0] == '0' && str[1] == 'x')
	{
		return std::stoul(std::string(str), nullptr, 16);
	}
	else
	{
		return std::stoul(std::string(str), nullptr, 10);
	}
}

static std::vector<std::string_view> splitByCommaIgnoringTrailingComma(std::string_view str)
{
	std::vector<std::string_view> results;
	std::size_t startIndex = 0u;
	const auto strSize = str.size();
	for (std::size_t i = 0u; i != strSize; ++i)
	{
		if (str[i] == ',')
		{
			results.push_back(str.substr(startIndex, i - startIndex));
			startIndex = i + 1u;
		}
		else if (i + 1u == strSize)
		{
			results.push_back(str.substr(startIndex, i - startIndex + 1));
		}
	}

	return results;
}

static std::vector<std::string_view> splitByOuterCommaIgnoringTrailingComma(std::string_view str)
{
	std::vector<std::string_view> results;
	std::size_t startIndex = 0u;
	unsigned long openBraceCount = 0;
	const auto strSize = str.size();
	for (std::size_t i = 0u; i != strSize; ++i)
	{
		if (str[i] == '{')
		{
			++openBraceCount;
		}
		else if (str[i] == '}')
		{
			--openBraceCount;
		}
		else if (openBraceCount == 0u && str[i] == ',')
		{
			results.push_back(str.substr(startIndex, i - startIndex));
			startIndex = i + 1u;
		}

		if (str[i] != ',' && i + 1u == strSize)
		{
			results.push_back(str.substr(startIndex, i - startIndex + 1));
		}
	}

	return results;
}

static std::string replaceFileExtension(std::string_view str, std::string_view extension)
{
	auto index = str.find_last_of(".");
	auto result = std::string(str.substr(0u, index));
	result += extension;
	return result;
}

static constexpr StencilOp stringToStencilOp(std::string_view data, bool& succeeded) noexcept
{
	StencilOp op = StencilOp::keep;
	if (data == "keep")
	{
		op = StencilOp::keep;
	}
	else if (data == "zero")
	{
		op = StencilOp::zero;
	}
	else if (data == "replace")
	{
		op = StencilOp::replace;
	}
	else if (data == "incrSat")
	{
		op = StencilOp::incrSat;
	}
	else if (data == "decrSat")
	{
		op = StencilOp::decrSat;
	}
	else if (data == "invert")
	{
		op = StencilOp::invert;
	}
	else if (data == "incr")
	{
		op = StencilOp::incr;
	}
	else if (data == "decr")
	{
		op = StencilOp::decr;
	}
	else
	{
		succeeded = false;
		return StencilOp::keep;
	}
	succeeded = true;
	return op;
}

static constexpr ComparisonFunc stringToComparisonFunc(std::string_view data, bool& succeeded) noexcept
{
	ComparisonFunc comparisonFunc = ComparisonFunc::never;
	if (data == "never")
	{
		comparisonFunc = ComparisonFunc::never;
	}
	else if (data == "less")
	{
		comparisonFunc = ComparisonFunc::less;
	}
	else if (data == "equal")
	{
		comparisonFunc = ComparisonFunc::equal;
	}
	else if (data == "lessEqual")
	{
		comparisonFunc = ComparisonFunc::lessEqual;
	}
	else if (data == "greater")
	{
		comparisonFunc = ComparisonFunc::greater;
	}
	else if (data == "notEqual")
	{
		comparisonFunc = ComparisonFunc::notEqual;
	}
	else if (data == "greaterEqual")
	{
		comparisonFunc = ComparisonFunc::greaterEqual;
	}
	else if (data == "always")
	{
		comparisonFunc = ComparisonFunc::always;
	}
	else
	{
		succeeded = false;
		return ComparisonFunc::never;
	}
	succeeded = true;
	return comparisonFunc;
}

static constexpr Blend stringToBlend(std::string_view str, bool& succeeded) noexcept
{
	succeeded = true;
	if (str == "zero")
	{
		return Blend::zero;
	}
	else if (str == "one")
	{
		return Blend::one;
	}
	else if (str == "srcColor")
	{
		return Blend::srcColor;
	}
	else if (str == "invSrcColor")
	{
		return Blend::invSrcColor;
	}
	else if (str == "srcAlpha")
	{
		return Blend::srcAlpha;
	}
	else if (str == "invSrcAlpha")
	{
		return Blend::invSrcAlpha;
	}
	else if (str == "destAlpha")
	{
		return Blend::destAlpha;
	}
	else if (str == "invDescAlpha")
	{
		return Blend::invDescAlpha;
	}
	else if (str == "destColor")
	{
		return Blend::destColor;
	}
	else if (str == "invDestColor")
	{
		return Blend::invDestColor;
	}
	else if (str == "srcAlphaSat")
	{
		return Blend::srcAlphaSat;
	}
	else if (str == "blendFactor")
	{
		return Blend::blendFactor;
	}
	else if (str == "invBlendFactor")
	{
		return Blend::invBlendFactor;
	}
	else if (str == "src1Color")
	{
		return Blend::src1Color;
	}
	else if (str == "invSrc1Color")
	{
		return Blend::invSrc1Color;
	}
	else if (str == "src1Alpha")
	{
		return Blend::src1Alpha;
	}
	else if (str == "invSrc1Alpha")
	{
		return Blend::invSrc1Alpha;
	}
	succeeded = false;
	return Blend::zero;
}

static constexpr BlendOp stringToBlendOp(std::string_view str, bool& succeeded) noexcept
{
	succeeded = true;
	if (str == "add")
	{
		return BlendOp::add;
	}
	else if (str == "subtract")
	{
		return BlendOp::subtract;
	}
	else if (str == "revSubtract")
	{
		return BlendOp::revSubtract;
	}
	else if (str == "min")
	{
		return BlendOp::min;
	}
	else if (str == "max")
	{
		return BlendOp::max;
	}
	succeeded = false;
	return BlendOp::add;
}

static constexpr LogicOp stringToLogicOp(std::string_view str, bool& succeeded) noexcept
{
	succeeded = true;
	if (str == "clear")
	{
		return LogicOp::clear;
	}
	else if (str == "set")
	{
		return LogicOp::set;
	}
	else if (str == "copy")
	{
		return LogicOp::copy;
	}
	else if (str == "copyInverted")
	{
		return LogicOp::copyInverted;
	}
	else if (str == "noOp")
	{
		return LogicOp::noOp;
	}
	else if (str == "invert")
	{
		return LogicOp::invert;
	}
	else if (str == "and")
	{
		return LogicOp::andOp;
	}
	else if (str == "nand")
	{
		return LogicOp::nand;
	}
	else if (str == "or")
	{
		return LogicOp::orOp;
	}
	else if (str == "nor")
	{
		return LogicOp::nor;
	}
	else if (str == "xor")
	{
		return LogicOp::xorOp;
	}
	else if (str == "equiv")
	{
		return LogicOp::equiv;
	}
	else if (str == "andReverse")
	{
		return LogicOp::andReverse;
	}
	else if (str == "andInverted")
	{
		return LogicOp::andInverted;
	}
	else if (str == "orReverse")
	{
		return LogicOp::orReverse;
	}
	else if (str == "orInverted")
	{
		return LogicOp::orInverted;
	}
	succeeded = false;
	return LogicOp::clear;
}

static std::string convertShaderName(const std::filesystem::path& oldName, const std::filesystem::path& baseInputPath, const std::filesystem::path& inputPath)
{
	auto absolutePath = inputPath.parent_path() / oldName;
	absolutePath = std::filesystem::canonical(absolutePath);
	auto vertexShaderPath = absolutePath.lexically_relative(baseInputPath);
	vertexShaderPath.replace_extension();
	std::string vertexShaderName{};
	const auto vertexShaderPathEnd = vertexShaderPath.end();
	auto current = vertexShaderPath.begin();
	if (current != vertexShaderPathEnd)
	{
		while (true)
		{
			vertexShaderName += current->string();
			++current;
			if (current == vertexShaderPathEnd)
			{
				break;
			}
			else
			{
				vertexShaderName += "::";
			}
		}
	}
	return vertexShaderName;
}

static GraphicsPipelineStateDesc readFromTextFile(std::ifstream& inFile, const std::filesystem::path& baseInputPath, const std::filesystem::path& inputPath, bool& succeeded)
{
	GraphicsPipelineStateDesc psoDesc = {};
	int lineNumber = 1;
	std::string line;
	succeeded = false;
	while (getline(inFile, line))
	{
		std::string_view trimmedLine = trim(line);
		if (trimmedLine == "") continue;
		auto indexOfNameEnd = trimmedLine.find_first_of(" \t=");
		if (indexOfNameEnd == std::string_view::npos || indexOfNameEnd == 0)
		{
			std::cerr << "error on line: " << lineNumber << std::endl;
			return psoDesc;
		}
		std::string_view name = trimmedLine.substr(0, indexOfNameEnd);
		auto indexOfDataStart = trimmedLine.find_first_of("=", indexOfNameEnd);
		indexOfDataStart = trimmedLine.find_first_not_of(" \t", indexOfDataStart + 1);
		if (indexOfDataStart == std::string_view::npos)
		{
			std::cerr << "error on line: " << lineNumber << std::endl;
			return psoDesc;
		}
		std::string_view data = trimmedLine.substr(indexOfDataStart, std::string_view::npos);


		if (name == "VertexShader")
		{
			psoDesc.vertexShader = convertShaderName(std::filesystem::path{ data }, baseInputPath, inputPath);
		}
		else if (name == "PixelShader")
		{
			psoDesc.pixelShader = convertShaderName(std::filesystem::path{ data }, baseInputPath, inputPath);
		}
		else if (name == "StreamOutput.SODeclaration")
		{
			std::string newData;
			bool succeeded1 = getBracedValue(inFile, data, lineNumber, newData);
			if (!succeeded1)
			{
				return psoDesc;
			}
			newData = trim(newData);
			if (newData != "")
			{
				std::cerr << "not yet implemented on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "StreamOutput.BufferStrides")
		{
			std::string newData;
			bool succeeded1 = getBracedValue(inFile, data, lineNumber, newData);
			if (!succeeded1)
			{
				return psoDesc;
			}
			newData = trim(newData);
			if (newData != "")
			{
				std::cerr << "not yet implemented on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "StreamOutput.RasterizedStream")
		{
			psoDesc.streamOutput.rasterizedStream = stringToUnsignedLong(data);
		}
		else if (name == "BlendState.AlphaToCoverageEnable")
		{
			if (data == "true")
			{
				psoDesc.blendState.alphaToCoverageEnable = true;
			}
			else if (data == "false")
			{
				psoDesc.blendState.alphaToCoverageEnable = false;
			}
			else
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "BlendState.RenderTargets")
		{
			std::string newData1;
			bool succeeded1 = getBracedValue(inFile, data, lineNumber, newData1);
			if (!succeeded1)
			{
				return psoDesc;
			}
			std::string_view newData = trim(newData1);
			if (newData != "")
			{
				auto renderTargetStrings = splitByOuterCommaIgnoringTrailingComma(newData);
				unsigned int i = 0u;
				for (const auto renderTargetStr : renderTargetStrings)
				{
					if (i == 8u)
					{
						std::cerr << "more than 8 render target blend states on line " << lineNumber << std::endl;
						return psoDesc;
					}
					auto values = splitByCommaIgnoringTrailingComma(trim(renderTargetStr, " \t{}"));
					if (values.size() != 10)
					{
						std::cerr << "incorrect number of arguments for a render target blend state on line " << lineNumber << std::endl;
						return psoDesc;
					}
					auto blendEnableStr = trim(values[0]);
					bool blendEnable;
					if (blendEnableStr == "true")
					{
						blendEnable = true;
					}
					else if (blendEnableStr == "false")
					{
						blendEnable = false;
					}
					else
					{
						std::cerr << "blend enabled wasn't true or false on line " << lineNumber << std::endl;
						return psoDesc;
					}

					auto logicOpEnableStr = trim(values[1]);
					bool logicOpEnable;
					if (logicOpEnableStr == "true")
					{
						logicOpEnable = true;
					}
					else if (logicOpEnableStr == "false")
					{
						logicOpEnable = false;
					}
					else
					{
						std::cerr << "logic op enabled wasn't true or false on line " << lineNumber << std::endl;
						return psoDesc;
					}

					bool succeeded;
					Blend srcBlend = stringToBlend(trim(values[2]), succeeded);
					if (!succeeded)
					{
						std::cerr << "unknown src blend \"" << trim(values[2]) << "\" on line " << lineNumber << std::endl;
						return psoDesc;
					}

					Blend destBlend = stringToBlend(trim(values[3]), succeeded);
					if (!succeeded)
					{
						std::cerr << "unknown dest blend \"" << trim(values[3]) << "\" on line " << lineNumber << std::endl;
						return psoDesc;
					}

					BlendOp blendOp = stringToBlendOp(trim(values[4]), succeeded);
					if (!succeeded)
					{
						std::cerr << "unknown blend op \"" << trim(values[4]) << "\" on line " << lineNumber << std::endl;
						return psoDesc;
					}

					Blend srcAlphaBlend = stringToBlend(trim(values[5]), succeeded);
					if (!succeeded)
					{
						std::cerr << "unknown src blend alpha \"" << trim(values[5]) << "\" on line " << lineNumber << std::endl;
						return psoDesc;
					}

					Blend destAlphaBlend = stringToBlend(trim(values[6]), succeeded);
					if (!succeeded)
					{
						std::cerr << "unknown dest blend alpha \"" << trim(values[6]) << "\" on line " << lineNumber << std::endl;
						return psoDesc;
					}

					BlendOp blendOpAlpha = stringToBlendOp(trim(values[7]), succeeded);
					if (!succeeded)
					{
						std::cerr << "unknown blend op alpha \"" << trim(values[7]) << "\" on line " << lineNumber << std::endl;
						return psoDesc;
					}

					LogicOp logicOp = stringToLogicOp(trim(values[8]), succeeded);
					if (!succeeded)
					{
						std::cerr << "unknown logic op \"" << trim(values[8]) << "\" on line " << lineNumber << std::endl;
						return psoDesc;
					}

					auto renderTargetWriteMaskStr = trim(values[9]);
					ColorWriteEnable renderTargetWriteMask;
					if (renderTargetWriteMaskStr == "red")
					{
						renderTargetWriteMask = ColorWriteEnable::red;
					}
					else if (renderTargetWriteMaskStr == "green")
					{
						renderTargetWriteMask = ColorWriteEnable::green;
					}
					else if (renderTargetWriteMaskStr == "blue")
					{
						renderTargetWriteMask = ColorWriteEnable::blue;
					}
					else if (renderTargetWriteMaskStr == "alpha")
					{
						renderTargetWriteMask = ColorWriteEnable::alpha;
					}
					else if (renderTargetWriteMaskStr == "all")
					{
						renderTargetWriteMask = ColorWriteEnable::all;
					}
					else
					{
						std::cerr << "unknown render target write mask \"" << renderTargetWriteMaskStr << "\" on line " << lineNumber << std::endl;
						return psoDesc;
					}


					auto& renderTarget = psoDesc.blendState.renderTarget[i];
					renderTarget.blendEnable = blendEnable;
					renderTarget.logicOpEnable = logicOpEnable;
					renderTarget.srcBlend = srcBlend;
					renderTarget.destBlend = destBlend;
					renderTarget.blendOp = blendOp;
					renderTarget.srcBlendAlpha = srcAlphaBlend;
					renderTarget.destBlendAlpha = destAlphaBlend;
					renderTarget.blendOpAlpha = blendOpAlpha;
					renderTarget.logicOp = logicOp;
					renderTarget.renderTargetWriteMask = renderTargetWriteMask;

					++i;
				}
			}
		}
		else if (name == "SampleMask")
		{
			psoDesc.sampleMask = stringToUnsignedLong(data);
		}
		else if (name == "RasterizerState.FillMode")
		{
			if (data == "solid")
			{
				psoDesc.rasterizerState.fillMode = FillMode::solid;
			}
			else if (data == "wireframe")
			{
				psoDesc.rasterizerState.fillMode = FillMode::wireframe;
			}
			else
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "RasterizerState.CullMode")
		{
			if (data == "none")
			{
				psoDesc.rasterizerState.cullMode = CullMode::none;
			}
			else if (data == "front")
			{
				psoDesc.rasterizerState.cullMode = CullMode::front;
			}
			else if (data == "back")
			{
				psoDesc.rasterizerState.cullMode = CullMode::back;
			}
			else
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "RasterizerState.FrontCounterClockwise")
		{
			if (data == "true")
			{
				psoDesc.rasterizerState.frontCounterClockwise = true;
			}
			else if (data == "false")
			{
				psoDesc.rasterizerState.frontCounterClockwise = false;
			}
			else
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "RasterizerState.DepthBias")
		{
			psoDesc.rasterizerState.depthBias = std::stol(std::string(data));
		}
		else if (name == "RasterizerState.DepthBiasClamp")
		{
			psoDesc.rasterizerState.depthBiasClamp = std::stof(std::string(data));
		}
		else if (name == "RasterizerState.SlopeScaledDepthBias")
		{
			psoDesc.rasterizerState.slopeScaledDepthBias = std::stof(std::string(data));
		}
		else if (name == "RasterizerState.DepthClipEnable")
		{
			if (data == "true")
			{
				psoDesc.rasterizerState.depthClipEnable = true;
			}
			else if (data == "false")
			{
				psoDesc.rasterizerState.depthClipEnable = false;
			}
			else
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "RasterizerState.LineAntialiasingMode")
		{
			if (data == "aliased")
			{
				psoDesc.rasterizerState.multisampleEnable = false;
				psoDesc.rasterizerState.antialiasedLineEnable = false;
			}
			else if (data == "alpha")
			{
				psoDesc.rasterizerState.multisampleEnable = false;
				psoDesc.rasterizerState.antialiasedLineEnable = true;
			}
			else if (data == "quad")
			{
				psoDesc.rasterizerState.multisampleEnable = true;
				psoDesc.rasterizerState.antialiasedLineEnable = false;
			}
			else
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "RasterizerState.ForcedSampleCount")
		{
			psoDesc.rasterizerState.forcedSampleCount = stringToUnsignedLong(data);
		}
		else if (name == "RasterizerState.ConservativeRaster")
		{
			if (data == "true")
			{
				psoDesc.rasterizerState.conservativeRaster = true;
			}
			else if (data == "false")
			{
				psoDesc.rasterizerState.conservativeRaster = false;
			}
			else
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "DepthStencilState.DepthEnable")
		{
			if (data == "true")
			{
				psoDesc.depthStencilState.depthEnable = true;
			}
			else if (data == "false")
			{
				psoDesc.depthStencilState.depthEnable = false;
			}
			else
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "DepthStencilState.DepthWriteMask")
		{
			if (data == "zero")
			{
				psoDesc.depthStencilState.depthWriteMask = DepthWriteMask::zero;
			}
			else if (data == "all")
			{
				psoDesc.depthStencilState.depthWriteMask = DepthWriteMask::all;
			}
			else
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "DepthStencilState.DepthFunc")
		{
			bool succeeded1;
			psoDesc.depthStencilState.depthFunc = stringToComparisonFunc(data, succeeded1);
			if (!succeeded1)
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "DepthStencilState.StencilEnable")
		{
			if (data == "true")
			{
				psoDesc.depthStencilState.stencilEnable = true;
			}
			else if (data == "false")
			{
				psoDesc.depthStencilState.stencilEnable = false;
			}
			else
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "DepthStencilState.StencilReadMask")
		{
			auto stencilReadMask = stringToUnsignedLong(data);
			if (stencilReadMask > 255u)
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
			psoDesc.depthStencilState.stencilReadMask = (unsigned short)stencilReadMask;
		}
		else if (name == "DepthStencilState.StencilWriteMask")
		{
			auto stencilWriteMask = stringToUnsignedLong(data);
			if (stencilWriteMask > 255u)
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
			psoDesc.depthStencilState.stencilWriteMask = (unsigned short)stencilWriteMask;
		}
		else if (name == "DepthStencilState.FrontFace.StencilFailOp")
		{
			bool succeeded1;
			psoDesc.depthStencilState.frontFace.stencilFailOp = stringToStencilOp(data, succeeded1);
			if (!succeeded1)
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "DepthStencilState.FrontFace.StencilDepthFailOp")
		{
			bool succeeded1;
			psoDesc.depthStencilState.frontFace.stencilDepthFailOp = stringToStencilOp(data, succeeded1);
			if (!succeeded1)
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "DepthStencilState.FrontFace.StencilPassOp")
		{
			bool succeeded1;
			psoDesc.depthStencilState.frontFace.stencilPassOp = stringToStencilOp(data, succeeded1);
			if (!succeeded1)
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "DepthStencilState.FrontFace.StencilFunc")
		{
			bool succeeded1;
			psoDesc.depthStencilState.frontFace.stencilFunc = stringToComparisonFunc(data, succeeded1);
			if (!succeeded1)
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "DepthStencilState.BackFace.StencilFailOp")
		{
			bool succeeded1;
			psoDesc.depthStencilState.backFace.stencilFailOp = stringToStencilOp(data, succeeded1);
			if (!succeeded1)
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "DepthStencilState.BackFace.StencilDepthFailOp")
		{
			bool succeeded1;
			psoDesc.depthStencilState.backFace.stencilDepthFailOp = stringToStencilOp(data, succeeded1);
			if (!succeeded1)
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "DepthStencilState.BackFace.StencilPassOp")
		{
			bool succeeded1;
			psoDesc.depthStencilState.backFace.stencilPassOp = stringToStencilOp(data, succeeded1);
			if (!succeeded1)
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "DepthStencilState.BackFace.StencilFunc")
		{
			bool succeeded1;
			psoDesc.depthStencilState.backFace.stencilFunc = stringToComparisonFunc(data, succeeded1);
			if (!succeeded1)
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "InputLayout.InputElementDescs")
		{
			std::string newData;
			auto oldLineNumber = lineNumber;
			bool succeeded1 = getBracedValue(inFile, data, lineNumber, newData);
			if (!succeeded1)
			{
				std::cerr << "error { on line " << oldLineNumber << " doesn't have a closing } by line " << lineNumber << std::endl;
				return psoDesc;
			}
			if (newData != "")
			{
				auto inputElementStrings = splitByOuterCommaIgnoringTrailingComma(newData);
				unsigned long i = 0u;
				for (const auto& elementString : inputElementStrings)
				{
					auto values = splitByCommaIgnoringTrailingComma(trim(elementString, " \t{}"));
					if (values.size() != 7)
					{
						std::cerr << "incorrect number of arguments for a input element on line: " << lineNumber << std::endl;
						return psoDesc;
					}

					std::string_view SemanticNameStr = trim(values[0]);
					const char* semanticName;
					if (SemanticNameStr == "BINORMAL")
					{
						semanticName = "BINORMAL";
					}
					else if (SemanticNameStr == "BLENDINDICES")
					{
						semanticName = "BLENDINDICES";
					}
					else if (SemanticNameStr == "BLENDWEIGHT")
					{
						semanticName = "BLENDWEIGHT";
					}
					else if (SemanticNameStr == "COLOR")
					{
						semanticName = "COLOR";
					}
					else if (SemanticNameStr == "NORMAL")
					{
						semanticName = "NORMAL";
					}
					else if (SemanticNameStr == "POSITION")
					{
						semanticName = "POSITION";
					}
					else if (SemanticNameStr == "PSIZE")
					{
						semanticName = "PSIZE";
					}
					else if (SemanticNameStr == "TANGENT")
					{
						semanticName = "TANGENT";
					}
					else if (SemanticNameStr == "TEXCOORD")
					{
						semanticName = "TEXCOORD";
					}
					else if (SemanticNameStr == "FOG")
					{
						semanticName = "FOG";
					}
					else if (SemanticNameStr == "TESSFACTOR")
					{
						semanticName = "TESSFACTOR";
					}
					else
					{
						std::cerr << "unknown sematic name on line " << lineNumber << std::endl;
						return psoDesc;
					}

					auto semanticIndex = stringToUnsignedLong(trim(values[1]));

					auto formatStr = trim(values[2]);
					Format format;
					if (formatStr == "R32G32B32A32_FLOAT")
					{
						format = Format::R32G32B32A32_FLOAT;
					}
					else if (formatStr == "R32G32B32_FLOAT")
					{
						format = Format::R32G32B32_FLOAT;
					}
					else if (formatStr == "R32G32_FLOAT")
					{
						format = Format::R32G32_FLOAT;
					}
					else
					{
						std::cerr << "Unknown format on line " << lineNumber << std::endl;
						return psoDesc;
					}

					auto inputSlot = stringToUnsignedLong(trim(values[3]));

					auto alignedByteOffsetStr = trim(values[4]);
					unsigned long alignedByteOffset;
					if (alignedByteOffsetStr == "alignedAppend")
					{
						alignedByteOffset = 0xffffffff;
					}
					else
					{
						alignedByteOffset = stringToUnsignedLong(alignedByteOffsetStr);
					}

					auto inputSlotClassStr = trim(values[5]);
					InputClassification inputSlotClass;
					if (inputSlotClassStr == "perInstance")
					{
						inputSlotClass = InputClassification::perInstance;
					}
					else if (inputSlotClassStr == "perVertex")
					{
						inputSlotClass = InputClassification::perVertex;
					}
					else
					{
						std::cerr << "unknown input slot class on line " << lineNumber << std::endl;
						return psoDesc;
					}

					auto instanceDataStepRate = stringToUnsignedLong(trim(values[6]));

					psoDesc.inputLayout.inputElementDescs.push_back({ semanticName, semanticIndex, format,
						inputSlot, alignedByteOffset, inputSlotClass, instanceDataStepRate });

					++i;
				}
			}
		}
		else if (name == "IBStripCutValue")
		{
			if (data == "disabled")
			{
				psoDesc.ibStripCutValue = IndexBufferStripCutValue::disabled;
			}
			else if (data == "0xffff")
			{
				psoDesc.ibStripCutValue = IndexBufferStripCutValue::Oxffff;
			}
			else if (data == "0xffffffff")
			{
				psoDesc.ibStripCutValue = IndexBufferStripCutValue::Oxffffffff;
			}
			else
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "PrimitiveTopologyType")
		{
			if (data == "undefined")
			{
				psoDesc.primitiveTopologyType = PrimitiveTopologyType::undefined;
			}
			else if (data == "point")
			{
				psoDesc.primitiveTopologyType = PrimitiveTopologyType::point;
			}
			else if (data == "line")
			{
				psoDesc.primitiveTopologyType = PrimitiveTopologyType::line;
			}
			else if (data == "triangle")
			{
				psoDesc.primitiveTopologyType = PrimitiveTopologyType::triangle;
			}
			else if (data == "patch")
			{
				psoDesc.primitiveTopologyType = PrimitiveTopologyType::patch;
			}
			else
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "RTVFormats")
		{
			std::string newData;
			bool succeeded1 = getBracedValue(inFile, data, lineNumber, newData);
			if (!succeeded1)
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
			if (newData != "")
			{
				auto formatStrings = splitByCommaIgnoringTrailingComma(newData);
				unsigned int i = 0u;
				for (const auto& formatString : formatStrings)
				{
					auto trimmedFormatStr = trim(formatString);
					if (trimmedFormatStr == "R8G8B8A8_UNORM")
					{
						psoDesc.rtvFormats[i] = Format::R8G8B8A8_UNORM;
					}
					else if (trimmedFormatStr == "R16G16B16A16_FLOAT")
					{
						psoDesc.rtvFormats[i] = Format::R16G16B16A16_FLOAT;
					}
					else if (trimmedFormatStr == "R32G32B32A32_FLOAT")
					{
						psoDesc.rtvFormats[i] = Format::R32G32B32A32_FLOAT;
					}
					else if (trimmedFormatStr == "R16G16B16A16_UINT")
					{
						psoDesc.rtvFormats[i] = Format::R16G16B16A16_UINT;
					}
					else
					{
						std::cerr << "error on line: " << lineNumber << std::endl;
						return psoDesc;
					}

					++i;
					if (i == 8)
					{
						std::cerr << "error on line: " << lineNumber << std::endl;
						return psoDesc;
					}
				}
				psoDesc.numRenderTargets = i;
			}
		}
		else if (name == "DSVFormat")
		{
			if (data == "D32_FLOAT_S8X24_UINT")
			{
				psoDesc.dsvFormat = Format::D32_FLOAT_S8X24_UINT;
			}
			else if (data == "D32_FLOAT")
			{
				psoDesc.dsvFormat = Format::D32_FLOAT;
			}
			else if (data == "D24_UNORM_S8_UINT")
			{
				psoDesc.dsvFormat = Format::D24_UNORM_S8_UINT;
			}
			else if (data == "D16_UNORM")
			{
				psoDesc.dsvFormat = Format::D16_UNORM;
			}
			else
			{
				std::cerr << "error on line: " << lineNumber << std::endl;
				return psoDesc;
			}
		}
		else if (name == "SampleDesc.Count")
		{
			psoDesc.sampleDesc.count = stringToUnsignedLong(data);
		}
		else if (name == "SampleDesc.Quality")
		{
			psoDesc.sampleDesc.quality = stringToUnsignedLong(data);
		}
		else
		{
			std::cerr << "error on line: " << lineNumber << std::endl;
			return psoDesc;
		}

		++lineNumber;
	}

	succeeded = true;
	return psoDesc;
}

static constexpr const char* d3d12BlendName(Blend blend) noexcept
{
	constexpr const char* const names[20] = { nullptr, "ZERO", "ONE", "SRC_COLOR", "INV_SRC_COLOR", "SRC_ALPHA", "INV_SRC_ALPHA", "DEST_ALPHA",
		"INV_DEST_ALPHA", "DEST_COLOR", "INV_DEST_COLOR", "SRC_ALPHA_SAT", nullptr, nullptr, "BLEND_FACTOR", "INV_BLEND_FACTOR",
		"SRC1_COLOR", "INV_SRC1_COLOR", "SRC1_ALPHA", "INV_SRC1_ALPHA"
	};
	return names[(unsigned int)blend];
}

static constexpr const char* d3d12BlendOpName(BlendOp blendOp) noexcept
{
	constexpr const char* const names[6] = { nullptr, "ADD", "SUBTRACT", "REV_SUBTRACT", "MIN", "MAX" };
	return names[(unsigned int)blendOp];
}

static constexpr const char* d3d12LogicOpName(LogicOp logicOp) noexcept
{
	constexpr const char* const names[16] = { "CLEAR", "SET", "COPY", "COPY_INVERTED", "NOOP", "INVERT", "AND", "NAND", "OR", "NOR", "XOR", "EQUIV", "AND_REVERSE", "AND_INVERTED", "OR_REVERSE", "OR_INVERTED" };
	return names[(unsigned int)logicOp];
}

static constexpr const char* d3d12FillModeName(FillMode fillMode) noexcept
{
	if (fillMode == FillMode::wireframe)
	{
		return "WIREFRAME";
	}
	else
	{
		return "SOLID";
	}
}

static constexpr const char* d3d12CullModeName(CullMode cullMode) noexcept
{
	constexpr const char* const names[4] = { nullptr, "NONE", "FRONT", "BACK" };
	return names[(unsigned int)cullMode];
}

static constexpr const char* d3d12DepthWriteMaskName(DepthWriteMask depthWriteMask) noexcept
{
	if (depthWriteMask == DepthWriteMask::all)
	{
		return "ALL";
	}
	else
	{
		return "ZERO";
	}
}

static constexpr const char* d3d12ComparisonFuncName(ComparisonFunc comparisonFunc) noexcept
{
	constexpr const char* const names[9] = { nullptr, "NEVER", "LESS", "EQUAL", "LESS_EQUAL", "GREATER", "NOT_EQUAL", "GREATER_EQUAL", "ALWAYS" };
	return names[(unsigned int)comparisonFunc];
}

static constexpr const char* d3d12StencilOpName(StencilOp stencilOp) noexcept
{
	constexpr const char* const names[9] = { nullptr, "KEEP", "ZERO", "REPLACE", "INCR_SAT", "DECR_SAT", "INVERT", "INCR", "DECR" };
	return names[(unsigned int)stencilOp];
}

static constexpr const char* d3d12FormatName(Format format) noexcept
{
	constexpr const char* const names[133] = { "UNKNOWN", "R32G32B32A32_TYPELESS", "R32G32B32A32_FLOAT", "R32G32B32A32_UINT", "R32G32B32A32_SINT", "R32G32B32_TYPELESS", "R32G32B32_FLOAT", "R32G32B32_UINT", "R32G32B32_SINT", "R16G16B16A16_TYPELESS",
		"R16G16B16A16_FLOAT", "R16G16B16A16_UNORM", "R16G16B16A16_UINT", "R16G16B16A16_SNORM", "R16G16B16A16_SINT", "R32G32_TYPELESS", "R32G32_FLOAT", "R32G32_UINT", "R32G32_SINT", "R32G8X24_TYPELESS", "D32_FLOAT_S8X24_UINT", "R32_FLOAT_X8X24_TYPELESS",
		"X32_TYPELESS_G8X24_UINT", "R10G10B10A2_TYPELESS", "R10G10B10A2_UNORM", "R10G10B10A2_UINT", "R11G11B10_FLOAT", "R8G8B8A8_TYPELESS", "R8G8B8A8_UNORM", "R8G8B8A8_UNORM_SRGB", "R8G8B8A8_UINT", "R8G8B8A8_SNORM", "R8G8B8A8_SINT", "R16G16_TYPELESS",
		"R16G16_FLOAT", "R16G16_UNORM", "R16G16_UINT", "R16G16_SNORM", "R16G16_SINT", "R32_TYPELESS", "D32_FLOAT", "R32_FLOAT", "R32_UINT", "R32_SINT", "R24G8_TYPELESS", "D24_UNORM_S8_UINT", "R24_UNORM_X8_TYPELESS", "X24_TYPELESS_G8_UINT", "R8G8_TYPELESS",
		"R8G8_UNORM", "R8G8_UINT", "R8G8_SNORM", "R8G8_SINT", "R16_TYPELESS", "R16_FLOAT", "D16_UNORM", "R16_UNORM", "R16_UINT", "R16_SNORM", "R16_SINT", "R8_TYPELESS", "R8_UNORM", "R8_UINT", "R8_SNORM", "R8_SINT", "A8_UNORM", "R1_UNORM", "R9G9B9E5_SHAREDEXP",
		"R8G8_B8G8_UNORM", "G8R8_G8B8_UNORM", "BC1_TYPELESS", "BC1_UNORM", "BC1_UNORM_SRGB", "BC2_TYPELESS", "BC2_UNORM", "BC2_UNORM_SRGB", "BC3_TYPELESS", "BC3_UNORM", "BC3_UNORM_SRGB", "BC4_TYPELESS", "BC4_UNORM", "BC4_SNORM", "BC5_TYPELESS",
		"BC5_UNORM", "BC5_SNORM", "B5G6R5_UNORM", "B5G5R5A1_UNORM", "B8G8R8A8_UNORM", "B8G8R8X8_UNORM", "R10G10B10_XR_BIAS_A2_UNORM", "B8G8R8A8_TYPELESS", "B8G8R8A8_UNORM_SRGB", "B8G8R8X8_TYPELESS", "B8G8R8X8_UNORM_SRGB", "BC6H_TYPELESS", "BC6H_UF16",
		"BC6H_SF16", "BC7_TYPELESS", "BC7_UNORM", "BC7_UNORM_SRGB", "AYUV", "Y410", "Y416", "NV12", "P010", "P016", "FORMAT420_OPAQUE", "YUY2", "Y210", "Y216", "NV11", "AI44", "IA44", "P8", "A8P8", "B4G4R4A4_UNORM", nullptr, nullptr, nullptr, nullptr, nullptr,
		nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "P208", "V208", "V408"
	};
	return names[(unsigned int)format];
}

static constexpr const char* d3d12InputClassification(InputClassification inputClassification) noexcept
{
	if (inputClassification == InputClassification::perVertex)
	{
		return "PER_VERTEX_DATA";
	}
	else
	{
		return "PER_INSTANCE_DATA";
	}
}

static constexpr const char* d3d12IndexBufferStripCutValueName(IndexBufferStripCutValue value) noexcept
{
	constexpr const char* const names[3] = { "DISABLED", "0xFFFF", "0xFFFFFFFF" };
	return names[(unsigned int)value];
}

static constexpr const char* d3d12PrimitiveTopologyType(PrimitiveTopologyType primitiveTopology) noexcept
{
	constexpr const char* const names[5] = { "UNDEFINED", "POINT", "LINE", "TRIANGLE", "PATCH" };
	return names[(unsigned int)primitiveTopology];
}

static constexpr const char* d3d12ColorWriteEnableName(ColorWriteEnable colorWriteEnable)
{
	constexpr const char* const names[5] = { "RED", "GREEN", "BLUE", "ALPHA", "ALL" };
	return names[(unsigned int)colorWriteEnable];
}

static void writeToCppHeaderFile(const GraphicsPipelineStateDesc& psoDesc, std::ofstream& outFile, std::string_view className, const std::filesystem::path& relativeResourcesFileLocation)
{
	outFile << "#pragma once\n";
	outFile << "#include <GraphicsPipelineStateDesc.h>\n";
	outFile << "#include " << relativeResourcesFileLocation << "\n";
	outFile << "\n";
	outFile << "namespace PipelineStateObjectDescs\n";
	outFile << "{\n";
	outFile << "\tclass " << className << "\n";
	outFile << "\t{\n";
	if (psoDesc.streamOutput.soDeclarations.size() == 0u)
	{
		outFile << "\t\tstatic constexpr D3D12_SO_DECLARATION_ENTRY* soDeclarations = nullptr;\n";
	}
	else
	{
		outFile << "\t\tstatic constexpr D3D12_SO_DECLARATION_ENTRY soDeclarations[" << psoDesc.streamOutput.soDeclarations.size() << "] =\n";
		outFile << "\t\t{\n";
		for (const auto& declaration : psoDesc.streamOutput.soDeclarations)
		{
			outFile << "\t\t\tD3D12_SO_DECLARATION_ENTRY{" << declaration.Stream << ", \"" << declaration.SemanticName << "\", " << declaration.SemanticIndex << ", " <<
				(unsigned short)declaration.StartComponent << ", " << (unsigned short)declaration.ComponentCount << ", " << (unsigned short)declaration.OutputSlot << "},\n";
		}
		outFile << "\t\t};\n";
	}
	outFile << "\t\tstatic constexpr UINT numSoDeclarations = UINT{" << psoDesc.streamOutput.soDeclarations.size() << "ul};\n";
	outFile << "\n";
	if (psoDesc.streamOutput.bufferStrides.size() == 0u)
	{
		outFile << "\t\tstatic constexpr UINT* bufferStrides = nullptr;\n";
	}
	else
	{
		outFile << "\t\tstatic constexpr UINT bufferStrides[" << psoDesc.streamOutput.bufferStrides.size() << "] =\n";
		outFile << "\t\t{\n";
		for (auto stride : psoDesc.streamOutput.bufferStrides)
		{
			outFile << "\t\t\tUINT{" << stride << "ul},\n";
		}
		outFile << "\t\t};\n";
	}
	outFile << "\t\tstatic constexpr UINT numBufferStrides = UINT{" << psoDesc.streamOutput.bufferStrides.size() << "ul};\n";
	outFile << "\n";
	if (psoDesc.inputLayout.inputElementDescs.size() == 0u)
	{
		outFile << "\t\tstatic constexpr D3D12_INPUT_ELEMENT_DESC* inputElementDescs = nullptr;\n";
	}
	else
	{
		outFile << "\t\tstatic constexpr D3D12_INPUT_ELEMENT_DESC inputElementDescs[" << psoDesc.inputLayout.inputElementDescs.size() << "] =\n";
		outFile << "\t\t{\n";
		for (const auto& inputElement : psoDesc.inputLayout.inputElementDescs)
		{
			outFile << "\t\t\tD3D12_INPUT_ELEMENT_DESC{\"" << inputElement.semanticName << "\", " << inputElement.semanticIndex << ", DXGI_FORMAT_" << d3d12FormatName(inputElement.format) <<
				", UINT{" << inputElement.inputSlot << "ul}, ";
			if (inputElement.alignedByteOffset == 0xffffffff)
			{
				outFile << "D3D12_APPEND_ALIGNED_ELEMENT";
			}
			else
			{
				outFile << "UINT{" << inputElement.alignedByteOffset << "ul}";
			}
			outFile << ", D3D12_INPUT_CLASSIFICATION_" << d3d12InputClassification(inputElement.inputSlotClass) << ", UINT{" << inputElement.instanceDataStepRate << "ul}},\n";
		}
		outFile << "\t\t};\n";
	}
	outFile << "\t\tstatic constexpr UINT numInputElementDescs = UINT{" << psoDesc.inputLayout.inputElementDescs.size() << "ul};\n";
	outFile << "\tpublic:\n";
	outFile << "\t\tstatic inline GraphicsPipelineStateDesc desc =\n";
	outFile << "\t\t{\n";
	outFile << "\t\t\t" << psoDesc.vertexShader << ",\n";
	outFile << "\t\t\t" << psoDesc.pixelShader << ",\n";
	outFile << "\t\t\tD3D12_STREAM_OUTPUT_DESC{soDeclarations, numSoDeclarations, bufferStrides, numBufferStrides, " << psoDesc.streamOutput.rasterizedStream << "},\n";
	outFile << "\t\t\tD3D12_BLEND_DESC\n";
	outFile << "\t\t\t{\n";
	outFile << "\t\t\t\t" << (psoDesc.blendState.alphaToCoverageEnable ? "TRUE" : "FALSE") << ",\n";
	outFile << "\t\t\t\t" << (psoDesc.blendState.independentBlendEnable ? "TRUE" : "FALSE") << ",\n";
	outFile << "\t\t\t\t{\n" << std::hex;
	for (auto i = 0u; i != 8u; ++i)
	{
		outFile << "\t\t\t\t\tD3D12_RENDER_TARGET_BLEND_DESC{" << (psoDesc.blendState.renderTarget[i].blendEnable ? "TRUE" : "FALSE") << ", " << (psoDesc.blendState.renderTarget[i].logicOpEnable ? "TRUE" : "FALSE") <<
			", D3D12_BLEND_" << d3d12BlendName(psoDesc.blendState.renderTarget[i].srcBlend) << ", D3D12_BLEND_" << d3d12BlendName(psoDesc.blendState.renderTarget[i].destBlend) <<
			", D3D12_BLEND_OP_" << d3d12BlendOpName(psoDesc.blendState.renderTarget[i].blendOp) << ", D3D12_BLEND_" << d3d12BlendName(psoDesc.blendState.renderTarget[i].srcBlendAlpha) <<
			", D3D12_BLEND_" << d3d12BlendName(psoDesc.blendState.renderTarget[i].destBlendAlpha) << ", D3D12_BLEND_OP_" << d3d12BlendOpName(psoDesc.blendState.renderTarget[i].blendOpAlpha) <<
			", D3D12_LOGIC_OP_" << d3d12LogicOpName(psoDesc.blendState.renderTarget[i].logicOp) << ", D3D12_COLOR_WRITE_ENABLE_" << d3d12ColorWriteEnableName(psoDesc.blendState.renderTarget[i].renderTargetWriteMask) << "},\n";
	}
	outFile << "\t\t\t\t}\n";
	outFile << "\t\t\t},\n";
	outFile << "\t\t\tUINT{0x" << psoDesc.sampleMask << "},\n" << std::dec;
	outFile << "\t\t\tD3D12_RASTERIZER_DESC{D3D12_FILL_MODE_" << d3d12FillModeName(psoDesc.rasterizerState.fillMode) << ", D3D12_CULL_MODE_" << d3d12CullModeName(psoDesc.rasterizerState.cullMode) <<
		", " << (psoDesc.rasterizerState.frontCounterClockwise ? "TRUE" : "FALSE") << ", " << psoDesc.rasterizerState.depthBias << ", " << psoDesc.rasterizerState.depthBiasClamp <<
		", " << psoDesc.rasterizerState.slopeScaledDepthBias << ", " << (psoDesc.rasterizerState.depthClipEnable ? "TRUE" : "FALSE") << ", " << (psoDesc.rasterizerState.multisampleEnable ? "TRUE" : "FALSE") <<
		", " << (psoDesc.rasterizerState.antialiasedLineEnable ? "TRUE" : "FALSE") << ", UINT{" << psoDesc.rasterizerState.forcedSampleCount <<
		"ul}, D3D12_CONSERVATIVE_RASTERIZATION_MODE_" << (psoDesc.rasterizerState.conservativeRaster ? "ON" : "OFF") << "},\n";
	outFile << "\t\t\tD3D12_DEPTH_STENCIL_DESC\n";
	outFile << "\t\t\t{\n";
	outFile << "\t\t\t\t" << (psoDesc.depthStencilState.depthEnable ? "TRUE" : "FALSE") << ",\n";
	outFile << "\t\t\t\tD3D12_DEPTH_WRITE_MASK_" << d3d12DepthWriteMaskName(psoDesc.depthStencilState.depthWriteMask) << ",\n";
	outFile << "\t\t\t\tD3D12_COMPARISON_FUNC_" << d3d12ComparisonFuncName(psoDesc.depthStencilState.depthFunc) << ",\n";
	outFile << "\t\t\t\t" << (psoDesc.depthStencilState.stencilEnable ? "TRUE" : "FALSE") << ",\n";
	outFile << "\t\t\t\t" << std::hex << "UINT8{" << psoDesc.depthStencilState.stencilReadMask << "u},\n";
	outFile << "\t\t\t\t" << "UINT8{" << psoDesc.depthStencilState.stencilWriteMask << "u},\n";
	outFile << "\t\t\t\tD3D12_DEPTH_STENCILOP_DESC{D3D12_STENCIL_OP_" << d3d12StencilOpName(psoDesc.depthStencilState.frontFace.stencilFailOp) <<
		", D3D12_STENCIL_OP_" << d3d12StencilOpName(psoDesc.depthStencilState.frontFace.stencilDepthFailOp) <<
		", D3D12_STENCIL_OP_" << d3d12StencilOpName(psoDesc.depthStencilState.frontFace.stencilPassOp) <<
		", D3D12_COMPARISON_FUNC_" << d3d12ComparisonFuncName(psoDesc.depthStencilState.frontFace.stencilFunc) << "},\n";
	outFile << "\t\t\t\tD3D12_DEPTH_STENCILOP_DESC{D3D12_STENCIL_OP_" << d3d12StencilOpName(psoDesc.depthStencilState.backFace.stencilFailOp) <<
		", D3D12_STENCIL_OP_" << d3d12StencilOpName(psoDesc.depthStencilState.backFace.stencilDepthFailOp) <<
		", D3D12_STENCIL_OP_" << d3d12StencilOpName(psoDesc.depthStencilState.backFace.stencilPassOp) <<
		", D3D12_COMPARISON_FUNC_" << d3d12ComparisonFuncName(psoDesc.depthStencilState.backFace.stencilFunc) << "}\n";
	outFile << "\t\t\t},\n";
	outFile << "\t\t\tD3D12_INPUT_LAYOUT_DESC{inputElementDescs, numInputElementDescs},\n";
	outFile << "\t\t\tD3D12_INDEX_BUFFER_STRIP_CUT_VALUE_" << d3d12IndexBufferStripCutValueName(psoDesc.ibStripCutValue) << ",\n";
	outFile << "\t\t\tD3D12_PRIMITIVE_TOPOLOGY_TYPE_" << d3d12PrimitiveTopologyType(psoDesc.primitiveTopologyType) << ",\n";
	outFile << std::dec << "\t\t\tUINT{" << psoDesc.numRenderTargets << "ul},\n";
	outFile << "\t\t\t{DXGI_FORMAT_" << d3d12FormatName(psoDesc.rtvFormats[0]) << ", DXGI_FORMAT_" << d3d12FormatName(psoDesc.rtvFormats[1]) <<
		", DXGI_FORMAT_" << d3d12FormatName(psoDesc.rtvFormats[2]) << ", DXGI_FORMAT_" << d3d12FormatName(psoDesc.rtvFormats[3]) <<
		", DXGI_FORMAT_" << d3d12FormatName(psoDesc.rtvFormats[4]) << ", DXGI_FORMAT_" << d3d12FormatName(psoDesc.rtvFormats[5]) <<
		", DXGI_FORMAT_" << d3d12FormatName(psoDesc.rtvFormats[6]) << ", DXGI_FORMAT_" << d3d12FormatName(psoDesc.rtvFormats[7]) << "},\n";
	outFile << "\t\t\tDXGI_FORMAT_" << d3d12FormatName(psoDesc.dsvFormat) << ",\n";
	outFile << "\t\t\tDXGI_SAMPLE_DESC{UINT{" << psoDesc.sampleDesc.count << "ul}, UINT{" << psoDesc.sampleDesc.quality << "ul}}\n";
	outFile << "\t\t};\n";
	outFile << "\t};\n";
	outFile << "};\n";
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
		auto inputPath = baseInputPath / relativeInputPath;
		auto outputDirectory = baseInputPath / "Generated" / relativeInputPath.parent_path();
		auto outputPath = outputDirectory / relativeInputPath.stem();
		outputPath += ".h";

		if (std::filesystem::exists(outputPath) && std::filesystem::last_write_time(outputPath) > std::filesystem::last_write_time(inputPath))
		{
			return true;
		}

		std::cout << "importing " << inputPath.string() << "\n";
		std::ifstream infile(inputPath);
		bool succeeded;
		GraphicsPipelineStateDesc psoDesc = readFromTextFile(infile, baseInputPath, inputPath, succeeded);
		infile.close();
		if (!succeeded)
		{
			return false;
		}

		if (!std::filesystem::exists(outputDirectory))
		{
			std::filesystem::create_directories(outputDirectory);
		}
		std::ofstream outFile(outputPath);
		writeToCppHeaderFile(psoDesc, outFile, relativeInputPath.stem().string(), baseInputPath.lexically_relative(outputDirectory) / "Resources.h");
		outFile.close();
	}
	catch (...)
	{
		std::cerr << "failed\n";
		return false;
	}
	return true;
}