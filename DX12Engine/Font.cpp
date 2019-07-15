#include "Font.h"
#include "GraphicsEngine.h"
#include "TextureManager.h"
#include <fstream>

float Font::getKerning(const wchar_t first, const wchar_t second) const
{
	uint64_t searchId = ((uint64_t)first << 32u) | (uint64_t)second;
	auto i = std::lower_bound(&kerningsList[0u], &kerningsList[numKernings], searchId, [](const FontKerning& lhs, uint64_t rhs) {return lhs.firstIdAndSecoundId < rhs; });
	if (i->firstIdAndSecoundId == searchId)
	{
		return i->amount;
	}
	return 0.0f;
}

const FontChar* Font::getChar(const wchar_t c) const
{
	auto i = std::lower_bound(&charList[0u], &charList[numCharacters], c, [](const FontChar& lhs, wchar_t rhs) {return lhs.id < rhs; });
	if (i->id == c)
	{
		return i;
	}
	return nullptr;
}

void Font::setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, unsigned char*& constantBufferCpuAddress)
{
	constantBufferCpuAddress += psPerObjectConstantBufferSize;
	psPerObjectCBVGpuAddress = constantBufferGpuAddress;
	constantBufferGpuAddress += psPerObjectConstantBufferSize;
}

void Font::create(const wchar_t* const filename, float aspectRatio)
{
	std::wifstream fs(filename);

	std::wstring tmp;
	std::size_t startpos;

	// extract font name
	fs >> tmp >> tmp; // info face="Arial"
	startpos = tmp.find(L"\"") + 1;
	std::wstring name = tmp.substr(startpos, tmp.size() - startpos - 1);

	// get font size
	fs >> tmp; // size=73

	// bold, italic, charset, unicode, stretchH, smooth, aa, padding, spacing
	fs >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp; // bold=0 italic=0 charset="" unicode=0 stretchH=100 smooth=1 aa=1 
	fs >> tmp; // padding=5,5,5,5 
	fs >> tmp; // spacing=0,0

	// lineheight is how much to move down for each line
	fs >> tmp >> tmp; // common lineHeight=95
	startpos = tmp.find(L"=") + 1;
	int lineHeightInPixels = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// baseHeight is the offset from the top of line, to where the base of each character is.
	fs >> tmp; // base=68

	// get texture width
	fs >> tmp; // scaleW=512
	startpos = tmp.find(L"=") + 1;
	int textureWidth = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// get texture height
	fs >> tmp; // scaleH=512
	startpos = tmp.find(L"=") + 1;
	int textureHeight = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	const float oneOverWidth = 1.0f / (float)textureWidth;
	const float oneOverHeight = 1.0f / (float)textureHeight;

	const float widthMultiplier = oneOverWidth * 2.0f;
	const float heightMultiplier = aspectRatio * oneOverHeight * 2.0f;

	lineHeight = lineHeightInPixels * heightMultiplier;

	// get pages, packed, page id
	fs >> tmp >> tmp; // pages=1 packed=0
	fs >> tmp >> tmp; // page id=0

					  // get texture filename
	std::wstring wtmp;
	fs >> wtmp; // file="Arial.png"
	startpos = wtmp.find(L"\"") + 1;
	std::wstring fontImage = wtmp.substr(startpos, wtmp.size() - startpos - 1); //would normally be used to load the texture

																				// get number of characters
	fs >> tmp >> tmp; // chars count=97
	startpos = tmp.find(L"=") + 1;
	numCharacters = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// initialize the character list
	charList.reset(new FontChar[numCharacters]);

	for (auto i = 0u; i != numCharacters; ++i)
	{
		// get unicode id
		fs >> tmp >> tmp; // char id=0
		startpos = tmp.find(L"=") + 1;
		charList[i].id = (wchar_t)std::stoi(tmp.substr(startpos, tmp.size() - startpos));

		// get x
		fs >> tmp; // x=392
		startpos = tmp.find(L"=") + 1;
		int pixelU = std::stoi(tmp.substr(startpos, tmp.size() - startpos));
		charList[i].u = ((float)pixelU + 0.5f) * oneOverWidth;

		// get y
		fs >> tmp; // y=340
		startpos = tmp.find(L"=") + 1;
		int pixelV = std::stoi(tmp.substr(startpos, tmp.size() - startpos));
		charList[i].v = ((float)pixelV + 0.5f) * oneOverHeight;

		// get width
		fs >> tmp; // width=47
		startpos = tmp.find(L"=") + 1;
		tmp = tmp.substr(startpos, tmp.size() - startpos);
		charList[i].width = (float)std::stoi(tmp) * widthMultiplier;
		charList[i].twidth = (float)std::stoi(tmp) * oneOverWidth;

		// get height
		fs >> tmp; // height=57
		startpos = tmp.find(L"=") + 1;
		tmp = tmp.substr(startpos, tmp.size() - startpos);
		charList[i].height = (float)std::stoi(tmp) * heightMultiplier;
		charList[i].theight = (float)std::stoi(tmp) * oneOverHeight;

		// get xoffset
		fs >> tmp; // xoffset=-6
		startpos = tmp.find(L"=") + 1;
		charList[i].xoffset = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) * widthMultiplier;

		// get yoffset
		fs >> tmp; // yoffset=16
		startpos = tmp.find(L"=") + 1;
		charList[i].yoffset = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) * heightMultiplier;

		// get xadvance
		fs >> tmp; // xadvance=65
		startpos = tmp.find(L"=") + 1;
		charList[i].xadvance = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) * widthMultiplier;

		// get page
		// get channel
		fs >> tmp >> tmp; // page=0    chnl=0
	}
	std::sort(&charList[0u], &charList[numCharacters], [](const FontChar& lhs, const FontChar& rhs) {return lhs.id < rhs.id; });

	// get number of kernings
	fs >> tmp >> tmp; // kernings count=96
	startpos = tmp.find(L"=") + 1;
	numKernings = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// initialize the kernings list
	kerningsList.reset(new FontKerning[numKernings]);

	for (auto i = 0u; i != numKernings; ++i)
	{
		// get first character
		fs >> tmp >> tmp; // kerning first=87
		startpos = tmp.find(L"=") + 1;
		kerningsList[i].firstIdAndSecoundId = (uint64_t)(std::stoul(tmp.substr(startpos, tmp.size() - startpos))) << 32u;

		// get second character
		fs >> tmp; // second=45
		startpos = tmp.find(L"=") + 1;
		kerningsList[i].firstIdAndSecoundId |= std::stoul(tmp.substr(startpos, tmp.size() - startpos));

		// get amount
		fs >> tmp; // amount=-1
		startpos = tmp.find(L"=") + 1;
		int amountInPixels = std::stoi(tmp.substr(startpos, tmp.size() - startpos));
		kerningsList[i].amount = (float)amountInPixels * widthMultiplier;
	}
	std::sort(&kerningsList[0u], &kerningsList[numKernings], [](const FontKerning& lhs, const FontKerning& rhs) {return lhs.firstIdAndSecoundId < rhs.firstIdAndSecoundId; });
}