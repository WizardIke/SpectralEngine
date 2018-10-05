#pragma once

class FontChar
{
public:
	wchar_t id;

	// these need to be converted to texture coordinates 
	// (where 0.0 is 0 and 1.0 is textureWidth of the font)
	float u; // u texture coordinate
	float v; // v texture coordinate
	float twidth; // width of character on texture
	float theight; // height of character on texture

	float width; // width of character in screen coords
	float height; // height of character in screen coords
	
	float xoffset; // offset from current cursor pos to left side of character in screen coords
	float yoffset; // offset from top of line to top of character in screen coords
	float xadvance; // how far to move to right for next character in screen coords
};
