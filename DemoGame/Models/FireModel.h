#pragma once

#include <DirectXMath.h>
#include <Frustum.h>
#include <frameBufferCount.h>
#include <d3d12.h>
#include <Shaders/FireMaterialVS.h>
#include <Shaders/FireMaterialPS.h>

template<class X, class Y, class Z>
class FireModel
{
	unsigned char* constantBufferCpu;
	D3D12_GPU_VIRTUAL_ADDRESS constantBufferGpu;
	float flameTime = 0.0f;

	constexpr static size_t VSPerObjectConstantBufferSize = (sizeof(FireMaterialVS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
	constexpr static size_t PSPerObjectConstantBufferSize = (sizeof(FireMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);
public:
	D3D12_GPU_VIRTUAL_ADDRESS vsConstantBufferGPU(uint32_t frameIndex)
	{
		return constantBufferGpu + frameIndex * VSPerObjectConstantBufferSize;
	}

	D3D12_GPU_VIRTUAL_ADDRESS psConstantBufferGPU() { return constantBufferGpu + frameBufferCount * VSPerObjectConstantBufferSize; };
	
	Mesh* mesh;

	constexpr static float positionX = X::value();
	constexpr static float positionY = Y::value();
	constexpr static float positionZ = Z::value();

	FireModel() {}

	FireModel(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, unsigned char*& constantBufferCpuAddress)
	{
		constantBufferCpu = constantBufferCpuAddress;
		constantBufferGpu = constantBufferGpuAddress;
		constantBufferGpuAddress += frameBufferCount * VSPerObjectConstantBufferSize + PSPerObjectConstantBufferSize;
		for (auto i = 0u; i < frameBufferCount; ++i)
		{
			auto buffer = reinterpret_cast<FireMaterialVS*>(constantBufferCpuAddress);
			buffer->scrollSpeeds = DirectX::XMFLOAT3(1.3f, 2.1f, 2.3f);
			buffer->scales = DirectX::XMFLOAT3(1.0f, 2.0f, 3.0f);
			constantBufferCpuAddress += VSPerObjectConstantBufferSize;
		}
		auto psPerObjectCBVCpuAddress = reinterpret_cast<FireMaterialPS*>(constantBufferCpuAddress);
		constantBufferCpuAddress += PSPerObjectConstantBufferSize;

		psPerObjectCBVCpuAddress->distortion1 = DirectX::XMFLOAT2(0.1f, 0.2f);
		psPerObjectCBVCpuAddress->distortion2 = DirectX::XMFLOAT2(0.1f, 0.3f);
		psPerObjectCBVCpuAddress->distortion3 = DirectX::XMFLOAT2(0.1f, 0.1f);
		psPerObjectCBVCpuAddress->distortionScale = 0.8f;
		psPerObjectCBVCpuAddress->distortionBias = 0.4f;
	}
	~FireModel() {}

	bool isInView(const Frustum& Frustum)
	{
		return Frustum.checkSphere(DirectX::XMVectorSet(positionX, positionY, positionZ, 1.0f), 1.5f);
	}
	void update(float frameTime)
	{
		flameTime += 0.6f * frameTime;
		if (flameTime > 100.0f)
		{
			flameTime -= 100.0f;
		}
	}
	void beforeRender(uint32_t frameIndex, const DirectX::XMFLOAT3& position)
	{
		float rotation = atan2(positionX - position.x, positionZ - position.z);

		auto constantBuffer = reinterpret_cast<FireMaterialVS*>(constantBufferCpu + frameIndex * VSPerObjectConstantBufferSize);
		constantBuffer->worldMatrix = DirectX::XMMatrixRotationY(rotation) * DirectX::XMMatrixTranslation(positionX, positionY, positionZ);
		constantBuffer->flameTime = flameTime;
	}

	void setNormalTexture(uint32_t normalTexture, unsigned char* cpuStartAddress, D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress)
	{
		auto buffer = reinterpret_cast<FireMaterialPS*>(cpuStartAddress + (constantBufferGpu - gpuStartAddress + VSPerObjectConstantBufferSize * frameBufferCount));
		buffer->noiseTexture = normalTexture;
	}

	void setDiffuseTexture(uint32_t diffuseTexture, unsigned char* cpuStartAddress, D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress)
	{
		auto buffer = reinterpret_cast<FireMaterialPS*>(cpuStartAddress + (constantBufferGpu - gpuStartAddress + VSPerObjectConstantBufferSize * frameBufferCount));
		buffer->fireTexture = diffuseTexture;
	}

	void setAlphaTexture(uint32_t alphaTexture, unsigned char* cpuStartAddress, D3D12_GPU_VIRTUAL_ADDRESS gpuStartAddress)
	{
		auto buffer = reinterpret_cast<FireMaterialPS*>(cpuStartAddress + (constantBufferGpu - gpuStartAddress + VSPerObjectConstantBufferSize * frameBufferCount));
		buffer->alphaTexture = alphaTexture;
	}
};