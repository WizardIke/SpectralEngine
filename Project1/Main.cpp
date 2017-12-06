#include <iostream>
#include <fstream>
#include <sstream>

int main()
{
	std::ofstream fout;
	unsigned int i = 0u;
	for (auto j = 0u; j < 10u; ++j)
	{
		for (auto k = 0u; k < 10u; ++k)
		{
			++i;
			std::stringstream path;
			path << "../DX12Engine/TestZoneModels/HighResPlaneModel" << i << ".h";
			fout.open(path.str());
			if (fout.fail())
			{
				std::cout << "couldn't open file " << path.str();
				return -1;
			}

			fout << ""
				"#include <DirectXMath.h>\n"
				"#include \"../Frustum.h\"\n"
				"#include \"../Settings.h\"\n"
				"#include <d3d12.h>\n"
				"\n"
				"class HighResPlaneModel" << i << "\n"
				"{\n"
				"public:\n"
				"	struct VSPerObjectConstantBuffer\n"
				"	{\n"
				"		DirectX::XMMATRIX worldMatrix;\n"
				"	};\n"
				"\n"
				"	struct PSPerObjectConstantBuffer\n"
				"	{\n"
				"		DirectX::XMFLOAT4 specularColor;\n"
				"		float specularPower;\n"
				"		UINT32 diffuseTexture;\n"
				"	};\n"
				"\n"
				"	D3D12_GPU_VIRTUAL_ADDRESS vsPerObjectCBVGpuAddress;\n"
				"	D3D12_GPU_VIRTUAL_ADDRESS psPerObjectCBVGpuAddress;\n"
				"\n"
				"\n"
				"	constexpr static unsigned int meshIndex = 0u;\n"
				"	constexpr static unsigned int diffuseTextureIndex = 4u;\n"
				"	constexpr static float positionX = " << k * 128 << ".0f, positionY = 0.0f, positionZ = " << j * 128 + 128 << ".0f;\n"
				"\n"
				"	HighResPlaneModel" << i << "() {}\n"
				"\n"
				"	HighResPlaneModel" << i << "(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress)\n"
				"	{\n"
				"		vsPerObjectCBVGpuAddress = constantBufferGpuAddress;\n"
				"		constexpr size_t vSPerObjectConstantBufferAlignedSize = (sizeof(VSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);\n"
				"		constantBufferGpuAddress += vSPerObjectConstantBufferAlignedSize;\n"
				"\n"
				"		psPerObjectCBVGpuAddress = constantBufferGpuAddress;\n"
				"		constexpr size_t pSPerObjectConstantBufferAlignedSize = (sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);\n"
				"		constantBufferGpuAddress += pSPerObjectConstantBufferAlignedSize;\n"
				"\n"
				"		VSPerObjectConstantBuffer* vsPerObjectCBVCpuAddress = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);\n"
				"		PSPerObjectConstantBuffer* psPerObjectCBVCpuAddress = reinterpret_cast<PSPerObjectConstantBuffer*>((reinterpret_cast<size_t>(constantBufferCpuAddress) + sizeof(VSPerObjectConstantBuffer) +\n"
				"		D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull));\n"
				"		constantBufferCpuAddress = reinterpret_cast<uint8_t*>((reinterpret_cast<size_t>(psPerObjectCBVCpuAddress) + sizeof(PSPerObjectConstantBuffer) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull)\n"
				"			& ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull));\n"
				"\n"
				"		vsPerObjectCBVCpuAddress->worldMatrix = DirectX::XMMatrixTranslation(positionX, positionY, positionZ);\n"
				"\n"
				"		psPerObjectCBVCpuAddress->specularColor = {0.5f, 0.5f, 0.5f, 1.0f};\n"
				"		psPerObjectCBVCpuAddress->specularPower = 4.0f;\n"
				"		psPerObjectCBVCpuAddress->diffuseTexture = diffuseTextureIndex;\n"
				"	}\n"
				"	~HighResPlaneModel" << i << "() {}\n"
				"\n"
				"	bool isInView(const Frustum& frustum)\n"
				"	{\n"
				"		return frustum.checkCuboid2(positionX + 128.0f, positionY, positionZ + 128.0f, positionX, positionY, positionZ);\n"
				"	}\n"
				"};";
			std::cout << "HighResPlaneModel" << i << ".h finished\n";
			fout.close();
		}
	}
	std::cout << "finished";
}