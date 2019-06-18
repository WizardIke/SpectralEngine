#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <d3d12.h>
#undef min
#undef max

std::string trim(const std::string& str,
	const std::string& whitespace = " \t")
{
	const auto strBegin = str.find_first_not_of(whitespace);
	if (strBegin == std::string::npos)
		return ""; // no content

	const auto strEnd = str.find_last_not_of(whitespace);
	const auto strRange = strEnd - strBegin + 1;

	return str.substr(strBegin, strRange);
}

void updateOpenBraces(const std::string& str, unsigned int& numOpenBraces)
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

bool getBracedValue(std::ifstream& infile, const std::string& data, int& lineNumber, std::string& newData)
{
	unsigned int numberOfOpenBraces = 1u;
	auto indexOfFirstBrace = data.find_first_of("{");
	if (indexOfFirstBrace == std::string::npos)
	{
		std::cout << "error on line: " << lineNumber << std::endl;
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
			std::cout << "error on line: " << lineNumber << std::endl;
			return false;
		}
		updateOpenBraces(line, numberOfOpenBraces);
		newData += line;
	}
	auto indexOfEndBrace = newData.find_last_of("}");
	newData = trim(newData.substr(0u, indexOfEndBrace));
	return true;
}

unsigned long stringToUnsignedLong(const std::string& str)
{
	if (str.size() >= 2 && str[0] == '0' && str[1] == 'x')
	{
		return std::stoul(str, nullptr, 16);
	}
	else
	{
		return std::stoul(str, nullptr, 10);
	}
}

std::vector<std::string> splitByCommaIgnoringTrailingComma(const std::string& str)
{
	std::vector<std::string> results;
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

std::vector<std::string> splitByOuterCommaIgnoringTrailingComma(const std::string& str)
{
	std::vector<std::string> results;
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
		else if (i + 1u == strSize)
		{
			results.push_back(str.substr(startIndex, i - startIndex + 1));
		}
	}

	return results;
}

std::string replaceFileExtension(const std::string& str, const std::string& extension)
{
	auto index = str.find_last_of(".");
	auto result = str.substr(0u, index);
	result += extension;
	return result;
}

constexpr std::size_t alignPadding(std::size_t length, std::size_t alignment)
{
	auto paddedLength = (length + (alignment - 1u)) & ~(alignment - 1u);
	return paddedLength - length;
}

int main(int argc, char** argv)
{
	if (argc < 2) return 1;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	std::vector<D3D12_SO_DECLARATION_ENTRY> soDeclaration;
	std::vector<UINT> bufferStrides;
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;

	std::string inFileName = argv[1];
	std::ifstream infile(argv[1]);
	int lineNumber = 1;
	std::string line;
	while (getline(infile, line))
	{
		line = trim(line);
		if (line == "") continue;
		auto indexOfNameEnd = line.find_first_of(" \t=");
		if (indexOfNameEnd == std::string::npos || indexOfNameEnd == 0)
		{
			std::cout << "error on line: " << lineNumber << std::endl;
			return 1;
		}
		std::string name = line.substr(0, indexOfNameEnd);
		auto indexOfDataStart = line.find_first_of("=", indexOfNameEnd);
		indexOfDataStart = line.find_first_not_of(" \t", indexOfDataStart + 1);
		if (indexOfDataStart == std::string::npos)
		{
			std::cout << "error on line: " << lineNumber << std::endl;
			return 1;
		}
		std::string data = line.substr(indexOfDataStart, std::string::npos);


		if (name == "StreamOutput.SODeclaration")
		{
			std::string newData;
			bool succeeded = getBracedValue(infile, data, lineNumber, newData);
			if (!succeeded)
			{
				return 1;
			}
			newData = trim(newData);
			if (newData != "")
			{
				std::cout << "not yet implemented on line: " << lineNumber << std::endl;
				return 2;
			}
		}
		else if (name == "StreamOutput.BufferStrides")
		{
			std::string newData;
			bool succeeded = getBracedValue(infile, data, lineNumber, newData);
			if (!succeeded)
			{
				return 1;
			}
			newData = trim(newData);
			if (newData != "")
			{
				std::cout << "not yet implemented on line: " << lineNumber << std::endl;
				return 2;
			}
		}
		else if (name == "StreamOutput.RasterizedStream")
		{
			psoDesc.StreamOutput.RasterizedStream = stringToUnsignedLong(data);
		}
		else if (name == "BlendState.AlphaToCoverageEnable")
		{
			if (data == "true")
			{
				psoDesc.BlendState.AlphaToCoverageEnable = TRUE;
			}
			else if (data == "false")
			{
				psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "BlendState.RenderTargets")
		{
			std::string newData;
			bool succeeded = getBracedValue(infile, data, lineNumber, newData);
			if (!succeeded)
			{
				return 1;
			}
			newData = trim(newData);
			if (newData != "")
			{
				std::cout << "not yet implemented on line: " << lineNumber << std::endl;
				return 2;
			}
		}
		else if (name == "SampleMask")
		{
			psoDesc.SampleMask = stringToUnsignedLong(data);
		}
		else if (name == "RasterizerState.FillMode")
		{
			if (data == "solid")
			{
				psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			}
			else if (data == "wireframe")
			{
				psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "RasterizerState.CullMode")
		{
			if (data == "none")
			{
				psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
			}
			else if (data == "front")
			{
				psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
			}
			else if (data == "back")
			{
				psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "RasterizerState.FrontCounterClockwise")
		{
			if (data == "true")
			{
				psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
			}
			else if (data == "false")
			{
				psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "RasterizerState.DepthBias")
		{
			psoDesc.RasterizerState.DepthBias = std::stol(data);
		}
		else if (name == "RasterizerState.DepthBiasClamp")
		{
			psoDesc.RasterizerState.DepthBiasClamp = std::stof(data);
		}
		else if (name == "RasterizerState.SlopeScaledDepthBias")
		{
			psoDesc.RasterizerState.SlopeScaledDepthBias = std::stof(data);
		}
		else if (name == "RasterizerState.DepthClipEnable")
		{
			if (data == "true")
			{
				psoDesc.RasterizerState.DepthClipEnable = TRUE;
			}
			else if (data == "false")
			{
				psoDesc.RasterizerState.DepthClipEnable = FALSE;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "RasterizerState.LineAntialiasingMode")
		{
			if (data == "aliased")
			{
				psoDesc.RasterizerState.MultisampleEnable = FALSE;
				psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
			}
			else if (data == "alpha")
			{
				psoDesc.RasterizerState.MultisampleEnable = FALSE;
				psoDesc.RasterizerState.AntialiasedLineEnable = TRUE;
			}
			else if (data == "quad")
			{
				psoDesc.RasterizerState.MultisampleEnable = TRUE;
				psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "RasterizerState.ForcedSampleCount")
		{
			psoDesc.RasterizerState.ForcedSampleCount = stringToUnsignedLong(data);
		}
		else if (name == "RasterizerState.ConservativeRaster")
		{
			if (data == "true")
			{
				psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON;
			}
			else if (data == "false")
			{
				psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "DepthStencilState.DepthEnable")
		{
			if (data == "true")
			{
				psoDesc.DepthStencilState.DepthEnable = TRUE;
			}
			else if (data == "false")
			{
				psoDesc.DepthStencilState.DepthEnable = FALSE;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "DepthStencilState.DepthWriteMask")
		{
			if (data == "zero")
			{
				psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
			}
			else if (data == "all")
			{
				psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "DepthStencilState.DepthFunc")
		{
			if (data == "never")
			{
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
			}
			else if (data == "less")
			{
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			}
			else if (data == "equal")
			{
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
			}
			else if (data == "lessEqual")
			{
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			}
			else if (data == "greater")
			{
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
			}
			else if (data == "notEqual")
			{
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
			}
			else if (data == "greaterEqual")
			{
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			}
			else if (data == "always")
			{
				psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "DepthStencilState.StencilEnable")
		{
			if (data == "true")
			{
				psoDesc.DepthStencilState.StencilEnable = TRUE;
			}
			else if (data == "false")
			{
				psoDesc.DepthStencilState.StencilEnable = FALSE;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "DepthStencilState.StencilReadMask")
		{
			auto stencilReadMask = stringToUnsignedLong(data);
			if (stencilReadMask > std::numeric_limits<UINT8>::max())
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
			psoDesc.DepthStencilState.StencilReadMask = (UINT8)stringToUnsignedLong(data);
		}
		else if (name == "DepthStencilState.StencilWriteMask")
		{
			auto stencilWriteMask = stringToUnsignedLong(data);
			if (stencilWriteMask > std::numeric_limits<UINT8>::max())
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
			psoDesc.DepthStencilState.StencilWriteMask = (UINT8)stencilWriteMask;
		}
		else if (name == "DepthStencilState.FrontFace.StencilFailOp")
		{
			if (data == "keep")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			}
			else if (data == "zero")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_ZERO;
			}
			else if (data == "replace")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_REPLACE;
			}
			else if (data == "incrSat")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_INCR_SAT;
			}
			else if (data == "decrSat")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_DECR_SAT;
			}
			else if (data == "invert")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_INVERT;
			}
			else if (data == "incr")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_INCR;
			}
			else if (data == "decr")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_DECR;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "DepthStencilState.FrontFace.StencilDepthFailOp")
		{
			if (data == "keep")
			{
				psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			}
			else if (data == "zero")
			{
				psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_ZERO;
			}
			else if (data == "replace")
			{
				psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE;
			}
			else if (data == "incrSat")
			{
				psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR_SAT;
			}
			else if (data == "decrSat")
			{
				psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR_SAT;
			}
			else if (data == "invert")
			{
				psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INVERT;
			}
			else if (data == "incr")
			{
				psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
			}
			else if (data == "decr")
			{
				psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "DepthStencilState.FrontFace.StencilPassOp")
		{
			if (data == "keep")
			{
				psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
			}
			else if (data == "zero")
			{
				psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_ZERO;
			}
			else if (data == "replace")
			{
				psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
			}
			else if (data == "incrSat")
			{
				psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR_SAT;
			}
			else if (data == "decrSat")
			{
				psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_DECR_SAT;
			}
			else if (data == "invert")
			{
				psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INVERT;
			}
			else if (data == "incr")
			{
				psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
			}
			else if (data == "decr")
			{
				psoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_DECR;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "DepthStencilState.FrontFace.StencilFunc")
		{
			if (data == "never")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
			}
			else if (data == "less")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS;
			}
			else if (data == "equal")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
			}
			else if (data == "lessEqual")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			}
			else if (data == "greater")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER;
			}
			else if (data == "notEqual")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
			}
			else if (data == "greaterEqual")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			}
			else if (data == "always")
			{
				psoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "DepthStencilState.BackFace.StencilFailOp")
		{
			if (data == "keep")
			{
				psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			}
			else if (data == "zero")
			{
				psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_ZERO;
			}
			else if (data == "replace")
			{
				psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_REPLACE;
			}
			else if (data == "incrSat")
			{
				psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_INCR_SAT;
			}
			else if (data == "decrSat")
			{
				psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_DECR_SAT;
			}
			else if (data == "invert")
			{
				psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_INVERT;
			}
			else if (data == "incr")
			{
				psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_INCR;
			}
			else if (data == "decr")
			{
				psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_DECR;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "DepthStencilState.BackFace.StencilDepthFailOp")
		{
			if (data == "keep")
			{
				psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			}
			else if (data == "zero")
			{
				psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_ZERO;
			}
			else if (data == "replace")
			{
				psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_REPLACE;
			}
			else if (data == "incrSat")
			{
				psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR_SAT;
			}
			else if (data == "decrSat")
			{
				psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR_SAT;
			}
			else if (data == "invert")
			{
				psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_INVERT;
			}
			else if (data == "incr")
			{
				psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
			}
			else if (data == "decr")
			{
				psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "DepthStencilState.BackFace.StencilPassOp")
		{
			if (data == "keep")
			{
				psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
			}
			else if (data == "zero")
			{
				psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_ZERO;
			}
			else if (data == "replace")
			{
				psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
			}
			else if (data == "incrSat")
			{
				psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR_SAT;
			}
			else if (data == "decrSat")
			{
				psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_DECR_SAT;
			}
			else if (data == "invert")
			{
				psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_INVERT;
			}
			else if (data == "incr")
			{
				psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
			}
			else if (data == "decr")
			{
				psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_DECR;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "DepthStencilState.BackFace.StencilFunc")
		{
			if (data == "never")
			{
				psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
			}
			else if (data == "less")
			{
				psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS;
			}
			else if (data == "equal")
			{
				psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
			}
			else if (data == "lessEqual")
			{
				psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
			}
			else if (data == "greater")
			{
				psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER;
			}
			else if (data == "notEqual")
			{
				psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NOT_EQUAL;
			}
			else if (data == "greaterEqual")
			{
				psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			}
			else if (data == "always")
			{
				psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "InputLayout.InputElementDescs")
		{
			std::string newData;
			bool succeeded = getBracedValue(infile, data, lineNumber, newData);
			if (!succeeded)
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
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
						std::cout << "error on line: " << lineNumber << std::endl;
						return 1;
					}

					std::string SemanticNameStr = trim(values[0]);
					char* semanticName;
					if (SemanticNameStr == "BINORMAL")
					{
						semanticName = (char*)(0);
					}
					else if (SemanticNameStr == "BLENDINDICES")
					{
						semanticName = (char*)(1);
					}
					else if (SemanticNameStr == "BLENDWEIGHT")
					{
						semanticName = (char*)(2);
					}
					else if (SemanticNameStr == "COLOR")
					{
						semanticName = (char*)(3);
					}
					else if (SemanticNameStr == "NORMAL")
					{
						semanticName = (char*)(4);
					}
					else if (SemanticNameStr == "POSITION")
					{
						semanticName = (char*)(5);
					}
					else if (SemanticNameStr == "PSIZE")
					{
						semanticName = (char*)(6);
					}
					else if (SemanticNameStr == "TANGENT")
					{
						semanticName = (char*)(7);
					}
					else if (SemanticNameStr == "TEXCOORD")
					{
						semanticName = (char*)(8);
					}
					else if (SemanticNameStr == "FOG")
					{
						semanticName = (char*)(9);
					}
					else if (SemanticNameStr == "TESSFACTOR")
					{
						semanticName = (char*)(10);
					}
					else
					{
						std::cout << "error on line: " << lineNumber << std::endl;
						return 1;
					}

					auto semanticIndex = stringToUnsignedLong(trim(values[1]));

					auto formatStr = trim(values[2]);
					DXGI_FORMAT format;
					if (formatStr == "R32G32B32A32_FLOAT")
					{
						format = DXGI_FORMAT_R32G32B32A32_FLOAT;
					}
					else
					{
						std::cout << "error on line: " << lineNumber << std::endl;
						return 1;
					}

					auto inputSlot = stringToUnsignedLong(trim(values[3]));

					auto alignedByteOffsetStr = trim(values[4]);
					unsigned long alignedByteOffset;
					if (alignedByteOffsetStr == "alignedAppend")
					{
						alignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
					}
					else
					{
						alignedByteOffset = stringToUnsignedLong(alignedByteOffsetStr);
					}

					auto inputSlotClassStr = trim(values[5]);
					D3D12_INPUT_CLASSIFICATION inputSlotClass;
					if (inputSlotClassStr == "perInstance")
					{
						inputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
					}
					else if (inputSlotClassStr == "perVertex")
					{
						inputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
					}
					else
					{
						std::cout << "error on line: " << lineNumber << std::endl;
						return 1;
					}

					auto instanceDataStepRate = stringToUnsignedLong(trim(values[6]));

					inputElementDescs.push_back({ semanticName, semanticIndex, format,
						inputSlot, alignedByteOffset, inputSlotClass, instanceDataStepRate });

					++i;
				}

				psoDesc.InputLayout.NumElements = i;
			}
		}
		else if (name == "IBStripCutValue")
		{
			if (data == "disabled")
			{
				psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
			}
			else if (data == "0xffff")
			{
				psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
			}
			else if (data == "0xffffffff")
			{
				psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "PrimitiveTopologyType")
		{
			if (data == "undefined")
			{
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
			}
			else if (data == "point")
			{
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			}
			else if (data == "line")
			{
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			}
			else if (data == "triangle")
			{
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			}
			else if (data == "patch")
			{
				psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "RTVFormats")
		{
			std::string newData;
			bool succeeded = getBracedValue(infile, data, lineNumber, newData);
			if (!succeeded)
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
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
						psoDesc.RTVFormats[i] = DXGI_FORMAT_R8G8B8A8_UNORM;
					}
					else if (trimmedFormatStr == "R16G16B16A16_FLOAT")
					{
						psoDesc.RTVFormats[i] = DXGI_FORMAT_R16G16B16A16_FLOAT;
					}
					else if (trimmedFormatStr == "R32G32B32A32_FLOAT")
					{
						psoDesc.RTVFormats[i] = DXGI_FORMAT_R32G32B32A32_FLOAT;
					}
					else
					{
						std::cout << "error on line: " << lineNumber << std::endl;
						return 1;
					}

					++i;
					if (i == 8)
					{
						std::cout << "error on line: " << lineNumber << std::endl;
						return 1;
					}
				}
				psoDesc.NumRenderTargets = i;
			}
		}
		else if (name == "DSVFormat")
		{
			if (data == "D32_FLOAT_S8X24_UINT")
			{
				psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			}
			else if (data == "D32_FLOAT")
			{
				psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			}
			else if (data == "D24_UNORM_S8_UINT")
			{
			psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			}
			else if (data == "D16_UNORM")
			{
				psoDesc.DSVFormat = DXGI_FORMAT_D16_UNORM;
			}
			else
			{
				std::cout << "error on line: " << lineNumber << std::endl;
				return 1;
			}
		}
		else if (name == "SampleDesc.Count")
		{
			psoDesc.SampleDesc.Count = stringToUnsignedLong(data);
		}
		else if (name == "SampleDesc.Quality")
		{
			psoDesc.SampleDesc.Quality = stringToUnsignedLong(data);
		}
		else
		{
			std::cout << "error on line: " << lineNumber << std::endl;
			return 1;
		}

		++lineNumber;
	}
	infile.close();

	std::string outFileName = replaceFileExtension(inFileName, ".pso");
	std::ofstream outFile(outFileName, std::ios::binary);
	outFile.write(reinterpret_cast<char*>(&psoDesc), sizeof(psoDesc));

	UINT numSoDeclarations = (UINT)soDeclaration.size();
	outFile.write(reinterpret_cast<char*>(&numSoDeclarations), sizeof(numSoDeclarations));
	constexpr auto paddingLength1 = alignPadding(sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC) + sizeof(UINT), alignof(D3D12_SO_DECLARATION_ENTRY));
	if (paddingLength1 != 0u)
	{
		char padding[paddingLength1] = {};
		outFile.write(padding, paddingLength1);
	}
	for (const auto& declaration : soDeclaration)
	{
		outFile.write(reinterpret_cast<const char*>(&declaration), sizeof(declaration));
	}

	UINT numBufferStrides = (UINT)bufferStrides.size();
	outFile.write(reinterpret_cast<const char*>(&numBufferStrides), sizeof(numBufferStrides));
	for (UINT stride : bufferStrides)
	{
		outFile.write(reinterpret_cast<const char*>(&stride), sizeof(stride));
	}

	UINT numInputElementDescs = (UINT)inputElementDescs.size();
	outFile.write(reinterpret_cast<const char*>(&numInputElementDescs), sizeof(numInputElementDescs));
	auto currentLengthWrittin = sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC) +
		sizeof(UINT) + paddingLength1 + numSoDeclarations * sizeof(D3D12_SO_DECLARATION_ENTRY) +
		sizeof(UINT) + numBufferStrides * sizeof(UINT) +
		sizeof(UINT);
	auto paddingLength2 = alignPadding(currentLengthWrittin, alignof(D3D12_INPUT_ELEMENT_DESC));
	for (auto i = paddingLength2; i != 0u; --i)
	{
		char pad = '\0';
		outFile.write(&pad, sizeof(pad));
	}
	for (const auto& inputElementDesc : inputElementDescs)
	{
		outFile.write(reinterpret_cast<const char*>(&inputElementDesc), sizeof(inputElementDesc));
	}

	outFile.close();

	std::cout << "done" << std::endl;
}