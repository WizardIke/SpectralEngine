#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <frameBufferCount.h>
#include <d3d12.h>

template<class X, class Y, class Z>
class FireModel
{
	struct VSPerObjectConstantBuffer
	{
		DirectX::XMMATRIX worldMatrix;
		float flameTime;
		DirectX::XMFLOAT3 scrollSpeeds;
		DirectX::XMFLOAT3 scales;
	};

	struct PSPerObjectConstantBuffer
	{
		DirectX::XMFLOAT2 distortion1;
		DirectX::XMFLOAT2 distortion2;
		DirectX::XMFLOAT2 distortion3;
		float distortionScale;
		float distortionBias;
		uint32_t noiseTexture;
		uint32_t fireTexture;
		uint32_t alphaTexture;
	};

	VSPerObjectConstantBuffer* vsPerObjectCBVCpuAddress;
	D3D12_GPU_VIRTUAL_ADDRESS perObjectCBVGpuAddress;

	constexpr static size_t VSPerObjectConstantBufferSize = (sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
	constexpr static size_t PSPerObjectConstantBufferSize = (sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
public:
	D3D12_GPU_VIRTUAL_ADDRESS vsConstantBufferGPU(uint32_t frameIndex)
	{
		return perObjectCBVGpuAddress + frameIndex * VSPerObjectConstantBufferSize + PSPerObjectConstantBufferSize;
	}

	D3D12_GPU_VIRTUAL_ADDRESS psConstantBufferGPU() { return perObjectCBVGpuAddress; };
	

	constexpr static unsigned int meshIndex = 5u;
	constexpr static float positionX = X::value();
	constexpr static float positionY = Y::value();
	constexpr static float positionZ = Z::value();

	float flameTime;

	FireModel() {}

	FireModel(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int normalTextureIndex, unsigned int diffuseTextureIndex, unsigned int alphaTextureIndex)
	{
		PSPerObjectConstantBuffer* psPerObjectCBVCpuAddress = reinterpret_cast<PSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		constantBufferCpuAddress = reinterpret_cast<uint8_t*>((reinterpret_cast<size_t>(constantBufferCpuAddress) + sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull)
			& ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull));

		psPerObjectCBVCpuAddress->distortion1 = DirectX::XMFLOAT2(0.1f, 0.2f);
		psPerObjectCBVCpuAddress->distortion2 = DirectX::XMFLOAT2(0.1f, 0.3f);
		psPerObjectCBVCpuAddress->distortion3 = DirectX::XMFLOAT2(0.1f, 0.1f);
		psPerObjectCBVCpuAddress->distortionScale = 0.8f;
		psPerObjectCBVCpuAddress->distortionBias = 0.5f;
		psPerObjectCBVCpuAddress->noiseTexture = normalTextureIndex;
		psPerObjectCBVCpuAddress->fireTexture = diffuseTextureIndex;
		psPerObjectCBVCpuAddress->alphaTexture = alphaTextureIndex;

		perObjectCBVGpuAddress = constantBufferGpuAddress;
		constantBufferGpuAddress += frameBufferCount * VSPerObjectConstantBufferSize + PSPerObjectConstantBufferSize;

		vsPerObjectCBVCpuAddress = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);
		constantBufferCpuAddress += frameBufferCount * VSPerObjectConstantBufferSize;

		auto start = vsPerObjectCBVCpuAddress + PSPerObjectConstantBufferSize;
		for (auto i = 0u; i < frameBufferCount; ++i)
		{

			start->scrollSpeeds = DirectX::XMFLOAT3(1.3f, 2.1f, 2.3f);
			start->scales = DirectX::XMFLOAT3(1.0f, 2.0f, 3.0f);

			start = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<uint8_t*>(start) + VSPerObjectConstantBufferSize);
		}
	}
	~FireModel() {}

	bool isInView(const Frustum& Frustum)
	{
		return Frustum.checkSphere(DirectX::XMVectorSet(positionX, positionY, positionZ, 1.0f), 1.5f);
	}
	void update(uint32_t frameIndex, const DirectX::XMFLOAT3& position, float frameTime)
	{
		flameTime += 0.6f * frameTime;
		if (flameTime > 1000.0f)
		{
			flameTime -= 1000.0f;
		}
		float rotation = atan2(positionX - position.x, positionZ - position.z);

		auto constantBuffer = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<uint8_t*>(vsPerObjectCBVCpuAddress) + frameIndex * VSPerObjectConstantBufferSize);
		constantBuffer->worldMatrix = DirectX::XMMatrixRotationY(rotation) * DirectX::XMMatrixTranslation(positionX, positionY, positionZ);
		constantBuffer->flameTime = flameTime;
	}
};