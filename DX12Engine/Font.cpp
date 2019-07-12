#include "Font.h"
#include "GraphicsEngine.h"
#include "TextureManager.h"
#include <fstream>

float Font::getKerning(const wchar_t first, const wchar_t second)
{
	uint64_t searchId = ((uint64_t)first << 32u) | (uint64_t)second;
	auto i = std::lower_bound(&kerningsList[0u], &kerningsList[numKernings], searchId, [](const FontKerning& lhs, uint64_t rhs) {return lhs.firstIdAndSecoundId < rhs; });
	if (i->firstIdAndSecoundId == searchId)
	{
		return i->amount;
	}
	return 0.0f;
}

FontChar* Font::getChar(const wchar_t c)
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

void Font::create(const wchar_t* const filename, unsigned int windowWidth, unsigned int windowHeight)
{
	std::wifstream fs;
	fs.open(filename);

	std::wstring tmp;
	size_t startpos;

	// extract font name
	fs >> tmp >> tmp; // info face="Arial"
	startpos = tmp.find(L"\"") + 1;
	name = tmp.substr(startpos, tmp.size() - startpos - 1);

	// get font size
	fs >> tmp; // size=73
	startpos = tmp.find(L"=") + 1;
	size = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// bold, italic, charset, unicode, stretchH, smooth, aa, padding, spacing
	fs >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp; // bold=0 italic=0 charset="" unicode=0 stretchH=100 smooth=1 aa=1 

														 // get padding
	fs >> tmp; // padding=5,5,5,5 
	startpos = tmp.find(L"=") + 1;
	tmp = tmp.substr(startpos, tmp.size() - startpos); // 5,5,5,5

													   // get up padding
	startpos = tmp.find(L",") + 1;
	toppadding = std::stoi(tmp.substr(0, startpos)) / (float)windowWidth;

	// get right padding
	tmp = tmp.substr(startpos, tmp.size() - startpos);
	startpos = tmp.find(L",") + 1;
	rightpadding = std::stoi(tmp.substr(0, startpos)) / (float)windowWidth;

	// get down padding
	tmp = tmp.substr(startpos, tmp.size() - startpos);
	startpos = tmp.find(L",") + 1;
	bottompadding = std::stoi(tmp.substr(0, startpos)) / (float)windowWidth;

	// get left padding
	tmp = tmp.substr(startpos, tmp.size() - startpos);
	leftpadding = std::stoi(tmp) / (float)windowWidth;

	fs >> tmp; // spacing=0,0

			   // get lineheight (how much to move down for each line), and normalize (between 0.0 and 1.0 based on size of font)
	fs >> tmp >> tmp; // common lineHeight=95
	startpos = tmp.find(L"=") + 1;
	lineHeight = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowHeight;

	// get base height (height of all characters), and normalize (between 0.0 and 1.0 based on size of font)
	fs >> tmp; // base=68
	startpos = tmp.find(L"=") + 1;
	baseHeight = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowHeight;

	// get texture width
	fs >> tmp; // scaleW=512
	startpos = tmp.find(L"=") + 1;
	textureWidth = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// get texture height
	fs >> tmp; // scaleH=512
	startpos = tmp.find(L"=") + 1;
	textureHeight = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

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

	for (auto i = 0u; i < numCharacters; ++i)
	{
		// get unicode id
		fs >> tmp >> tmp; // char id=0
		startpos = tmp.find(L"=") + 1;
		charList[i].id = (wchar_t)std::stoi(tmp.substr(startpos, tmp.size() - startpos));

		// get x
		fs >> tmp; // x=392
		startpos = tmp.find(L"=") + 1;
		charList[i].u = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)textureWidth;

		// get y
		fs >> tmp; // y=340
		startpos = tmp.find(L"=") + 1;
		charList[i].v = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)textureHeight;

		// get width
		fs >> tmp; // width=47
		startpos = tmp.find(L"=") + 1;
		tmp = tmp.substr(startpos, tmp.size() - startpos);
		charList[i].width = (float)std::stoi(tmp) / (float)windowWidth;
		charList[i].twidth = (float)std::stoi(tmp) / (float)textureWidth;

		// get height
		fs >> tmp; // height=57
		startpos = tmp.find(L"=") + 1;
		tmp = tmp.substr(startpos, tmp.size() - startpos);
		charList[i].height = (float)std::stoi(tmp) / (float)windowHeight;
		charList[i].theight = (float)std::stoi(tmp) / (float)textureHeight;

		// get xoffset
		fs >> tmp; // xoffset=-6
		startpos = tmp.find(L"=") + 1;
		charList[i].xoffset = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowWidth;

		// get yoffset
		fs >> tmp; // yoffset=16
		startpos = tmp.find(L"=") + 1;
		charList[i].yoffset = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowHeight;

		// get xadvance
		fs >> tmp; // xadvance=65
		startpos = tmp.find(L"=") + 1;
		charList[i].xadvance = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowWidth;

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

	for (auto i = 0u; i < numKernings; ++i)
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
		float t = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos));
		kerningsList[i].amount = t / (float)windowWidth;
	}
	std::sort(&kerningsList[0u], &kerningsList[numKernings], [](const FontKerning& lhs, const FontKerning& rhs) {return lhs.firstIdAndSecoundId < rhs.firstIdAndSecoundId; });
}