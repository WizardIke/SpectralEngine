#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN    // Exclude rarely-used stuff from Windows headers.
#endif

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include <string>
#include <wincodec.h>
#include <iostream>
#include <fstream>

// this will only call release if an object exists (prevents exceptions calling release on non existant objects)
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

using namespace DirectX; // we will be using the directxmath library

						 // Handle to the window
HWND hwnd = NULL;

// name of the window (not the title)
LPCTSTR WindowName = L"BzTutsApp";

// title of the window
LPCTSTR WindowTitle = L"Bz Window";

// width and height of the window
int Width = 800;
int Height = 600;

// is window full screen?
bool FullScreen = false;

// we will exit the program when this becomes false
bool Running = true;

// create a window
bool InitializeWindow(HINSTANCE hInstance,
	int ShowWnd,
	bool fullscreen);

// main application loop
void mainloop();

// callback function for windows messages
LRESULT CALLBACK WndProc(HWND hWnd,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

// direct3d stuff
const int frameBufferCount = 3; // number of buffers we want, 2 for double buffering, 3 for tripple buffering

ID3D12Device* device; // direct3d device

IDXGISwapChain3* swapChain; // swapchain used to switch between render targets

ID3D12CommandQueue* commandQueue; // container for command lists

ID3D12DescriptorHeap* rtvDescriptorHeap; // a descriptor heap to hold resources like the render targets

ID3D12Resource* renderTargets[frameBufferCount]; // number of render targets equal to buffer count

ID3D12CommandAllocator* commandAllocator[frameBufferCount]; // we want enough allocators for each buffer * number of threads (we only have one thread)

ID3D12GraphicsCommandList* commandList; // a command list we can record commands into, then execute them to render the frame

ID3D12Fence* fence[frameBufferCount];    // an object that is locked while our command list is being executed by the gpu. We need as many 
										 //as we have allocators (more if we want to know when the gpu is finished with an asset)

HANDLE fenceEvent; // a handle to an event when our fence is unlocked by the gpu

UINT64 fenceValue[frameBufferCount]; // this value is incremented each frame. each fence will have its own value

int frameIndex; // current rtv we are on

int rtvDescriptorSize; // size of the rtv descriptor on the device (all front and back buffers will be the same size)
					   // function declarations

bool InitD3D(); // initializes direct3d 12

void Update(double delta); // update the game logic

void UpdatePipeline(); // update the direct3d pipeline (update command lists)

void Render(); // execute the command list

void Cleanup(); // release com ojects and clean up memory

void WaitForPreviousFrame(); // wait until gpu is finished with command list

ID3D12PipelineState* pipelineStateObject; // pso containing a pipeline state

ID3D12RootSignature* rootSignature; // root signature defines data shaders will access

D3D12_VIEWPORT viewport; // area that output from rasterizer will be stretched to.

D3D12_RECT scissorRect; // the area to draw in. pixels outside that area will not be drawn onto

ID3D12Resource* vertexBuffer; // a default buffer in GPU memory that we will load vertex data for our triangle into
ID3D12Resource* indexBuffer; // a default buffer in GPU memory that we will load index data for our triangle into

D3D12_VERTEX_BUFFER_VIEW vertexBufferView; // a structure containing a pointer to the vertex data in gpu memory
										   // the total size of the buffer, and the size of each element (vertex)

D3D12_INDEX_BUFFER_VIEW indexBufferView; // a structure holding information about the index buffer

ID3D12Resource* depthStencilBuffer; // This is the memory for our depth buffer. it will also be used for a stencil buffer in a later tutorial
ID3D12DescriptorHeap* dsDescriptorHeap; // This is a heap for our depth/stencil buffer descriptor

										// this is the structure of our constant buffer.
struct ConstantBufferPerObject {
	XMFLOAT4X4 wvpMat;
};

// Constant buffers must be 256-byte aligned which has to do with constant reads on the GPU.
// We are only able to read at 256 byte intervals from the start of a resource heap, so we will
// make sure that we add padding between the two constant buffers in the heap (one for cube1 and one for cube2)
// Another way to do this would be to add a float array in the constant buffer structure for padding. In this case
// we would need to add a float padding[50]; after the wvpMat variable. This would align our structure to 256 bytes (4 bytes per float)
// The reason i didn't go with this way, was because there would actually be wasted cpu cycles when memcpy our constant
// buffer data to the gpu virtual address. currently we memcpy the size of our structure, which is 16 bytes here, but if we
// were to add the padding array, we would memcpy 64 bytes if we memcpy the size of our structure, which is 50 wasted bytes
// being copied.
int ConstantBufferPerObjectAlignedSize = (sizeof(ConstantBufferPerObject) + 255) & ~255;

ConstantBufferPerObject cbPerObject; // this is the constant buffer data we will send to the gpu 
									 // (which will be placed in the resource we created above)

ID3D12Resource* constantBufferUploadHeaps[frameBufferCount]; // this is the memory on the gpu where constant buffers for each frame will be placed

UINT8* cbvGPUAddress[frameBufferCount]; // this is a pointer to each of the constant buffer resource heaps

XMFLOAT4X4 cameraProjMat; // this will store our projection matrix
XMFLOAT4X4 cameraViewMat; // this will store our view matrix

XMFLOAT4 cameraPosition; // this is our cameras position vector
XMFLOAT4 cameraTarget; // a vector describing the point in space our camera is looking at
XMFLOAT4 cameraUp; // the worlds up vector

XMFLOAT4X4 cube1WorldMat; // our first cubes world matrix (transformation matrix)
XMFLOAT4X4 cube1RotMat; // this will keep track of our rotation for the first cube
XMFLOAT4 cube1Position; // our first cubes position in space

XMFLOAT4X4 cube2WorldMat; // our first cubes world matrix (transformation matrix)
XMFLOAT4X4 cube2RotMat; // this will keep track of our rotation for the second cube
XMFLOAT4 cube2PositionOffset; // our second cube will rotate around the first cube, so this is the position offset from the first cube

int numCubeIndices; // the number of indices to draw the cube

ID3D12Resource* textureBuffer; // the resource heap containing our texture

int LoadImageDataFromFile(BYTE** imageData, D3D12_RESOURCE_DESC& resourceDescription, LPCWSTR filename, int &bytesPerRow);

DXGI_FORMAT GetDXGIFormatFromWICFormat(WICPixelFormatGUID& wicFormatGUID);
WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID);
int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat);

ID3D12DescriptorHeap* mainDescriptorHeap;
UINT srvHandleSize;

struct FontChar
{
	// the unicode id
	wchar_t id;

	// these need to be converted to texture coordinates 
	// (where 0.0 is 0 and 1.0 is textureWidth of the font)
	float u; // u texture coordinate
	float v; // v texture coordinate
	float twidth; // width of character on texture
	float theight; // height of character on texture

	float width; // width of character in screen coords
	float height; // height of character in screen coords

				  // these need to be normalized based on size of font
	float xoffset; // offset from current cursor pos to left side of character
	float yoffset; // offset from top of line to top of character
	float xadvance; // how far to move to right for next character
};

struct FontKerning
{
	int firstid; // the first character
	int secondid; // the second character
	float amount; // the amount to add/subtract to second characters x
};

struct Font
{
	std::wstring name; // name of the font
	std::wstring fontImage;
	int size; // size of font, lineheight and baseheight will be based on this as if this is a single unit (1.0)
	float lineHeight; // how far to move down to next line, will be normalized
	float baseHeight; // height of all characters, will be normalized
	int textureWidth; // width of the font texture
	int textureHeight; // height of the font texture
	int numCharacters; // number of characters in the font
	FontChar* CharList; // list of characters
	int numKernings; // the number of kernings
	FontKerning* KerningsList; // list to hold kerning values
	ID3D12Resource* textureBuffer; // the font texture resource
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle; // the font srv

										   // these are how much the character is padded in the texture. We
										   // add padding to give sampling a little space so it does not accidentally
										   // padd the surrounding characters. We will need to subtract these paddings
										   // from the actual spacing between characters to remove the gaps you would otherwise see
	float leftpadding;
	float toppadding;
	float rightpadding;
	float bottompadding;

	// this will return the amount of kerning we need to use for two characters
	float getKerning(wchar_t first, wchar_t second)
	{
		for (int i = 0; i < numKernings; ++i)
		{
			if ((wchar_t)KerningsList[i].firstid == first && (wchar_t)KerningsList[i].secondid == second)
				return KerningsList[i].amount;
		}
		return 0.0f;
	}

	// this will return a FontChar given a wide character
	FontChar* getChar(wchar_t c)
	{
		for (int i = 0; i < numCharacters; ++i)
		{
			if (c == (wchar_t)CharList[i].id)
				return &CharList[i];
		}
		return nullptr;
	}
};

struct Timer
{
	double timerFrequency = 0.0;
	long long lastFrameTime = 0;
	long long lastSecond = 0;
	double frameDelta = 0;
	int fps = 0;

	Timer()
	{
		LARGE_INTEGER li;
		QueryPerformanceFrequency(&li);

		// seconds
		//timerFrequency = double(li.QuadPart);

		// milliseconds
		timerFrequency = double(li.QuadPart) / 1000.0;

		// microseconds
		//timerFrequency = double(li.QuadPart) / 1000000.0;

		QueryPerformanceCounter(&li);
		lastFrameTime = li.QuadPart;
	}

	// Call this once per frame
	double GetFrameDelta()
	{
		LARGE_INTEGER li;
		QueryPerformanceCounter(&li);
		frameDelta = double(li.QuadPart - lastFrameTime) / timerFrequency;
		if (frameDelta > 0)
			fps = 1000 / frameDelta;
		lastFrameTime = li.QuadPart;
		return frameDelta;
	}
};

ID3D12PipelineState* textPSO; // pso containing a pipeline state

Font arialFont; // this will store our arial font information

int maxNumTextCharacters = 1024; // the maximum number of characters you can render during a frame. This is just used to make sure
								 // there is enough memory allocated for the text vertex buffer each frame

ID3D12Resource* textVertexBuffer[frameBufferCount];
D3D12_VERTEX_BUFFER_VIEW textVertexBufferView[frameBufferCount]; // a view for our text vertex buffer
UINT8* textVBGPUAddress[frameBufferCount]; // this is a pointer to each of the text constant buffers

										   // create an instance of timer
Timer timer;

Font LoadFont(LPCWSTR filename, int windowWidth, int windowHeight); // load a font

void RenderText(Font font, std::wstring text, XMFLOAT2 pos, XMFLOAT2 scale = XMFLOAT2(1.0f, 1.0f), XMFLOAT2 padding = XMFLOAT2(0.5f, 0.0f), XMFLOAT4 color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));