#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
class Frustum;
struct Mesh;

class CaveModelPart1
{
	constexpr static float pi = 3.141592654f;
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
	};

	struct PSPerObjectConstantBuffer
	{
		float specularColor[4];
		float specularPower;
		uint32_t baseColorTexture;
	};

	constexpr static size_t vSPerObjectConstantBufferAlignedSize = (sizeof(VSPerObjectConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);
	constexpr static size_t pSPerObjectConstantBufferAlignedSize = (sizeof(PSPerObjectConstantBuffer) + (size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u) & ~((size_t)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - (size_t)1u);
public:


	D3D12_GPU_VIRTUAL_ADDRESS perObjectCBVGpuAddress;

	constexpr static unsigned int meshIndex = 0u;
	constexpr static unsigned int numSquares = 12u;
	constexpr static float positionX = 64.0f, positionY = -6.0f, positionZ = 91.0f;

	CaveModelPart1() {}

	CaveModelPart1(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int baseColorTextureIndex);
	~CaveModelPart1() {}

	bool CaveModelPart1::isInView(const Frustum& Frustum);

	void render(ID3D12GraphicsCommandList* const directCommandList, Mesh** meshes);
};