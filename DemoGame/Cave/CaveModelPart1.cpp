#include "CaveModelPart1.h"
#include <Mesh.h>
#include <EulerRotation.h>
#include <Quaternion.h>
#include <Frustum.h>

CaveModelPart1::CaveModelPart1(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpuAddress, uint8_t*& constantBufferCpuAddress, unsigned int baseColorTextureIndex)
{

	perObjectCBVGpuAddress = constantBufferGpuAddress;
	constantBufferGpuAddress += vSPerObjectConstantBufferAlignedSize * numSquares + pSPerObjectConstantBufferAlignedSize;

	VSPerObjectConstantBuffer* vsPerObjectCBVCpuAddress = reinterpret_cast<VSPerObjectConstantBuffer*>(constantBufferCpuAddress);
	constantBufferCpuAddress += vSPerObjectConstantBufferAlignedSize * numSquares;
	PSPerObjectConstantBuffer* psPerObjectCBVCpuAddress = reinterpret_cast<PSPerObjectConstantBuffer*>(constantBufferCpuAddress);
	constantBufferCpuAddress += pSPerObjectConstantBufferAlignedSize;

	auto vsWorldMatrix = vsPerObjectCBVCpuAddress;
	Quaternion rotaion(EulerRotation(0.0f, 0.75f * pi, 0.0f));
	vsWorldMatrix->worldMatrix = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(rotaion.x, rotaion.y, rotaion.z, rotaion.w),
		DirectX::XMVectorSet(positionX, positionY, positionZ, 0.0f));

	rotaion = Quaternion(EulerRotation(0.0f, -0.75f * pi, pi));
	vsWorldMatrix = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<unsigned char*>(vsWorldMatrix) + vSPerObjectConstantBufferAlignedSize);
	vsWorldMatrix->worldMatrix = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(rotaion.x, rotaion.y, rotaion.z, rotaion.w),
		DirectX::XMVectorSet(positionX, positionY + 13.0f, positionZ + 10.0f, 0.0f));

	rotaion = Quaternion(EulerRotation(0.75f * pi, 0.0f, 0.5f * pi));
	vsWorldMatrix = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<unsigned char*>(vsWorldMatrix) + vSPerObjectConstantBufferAlignedSize);
	vsWorldMatrix->worldMatrix = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(rotaion.x, rotaion.y, rotaion.z, rotaion.w),
		DirectX::XMVectorSet(positionX + 10.0f, positionY + 5.0f, positionZ + 5.0f, 0.0f));

	rotaion = Quaternion(EulerRotation(0.75f * pi, 0.0f, -0.5f * pi));
	vsWorldMatrix = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<unsigned char*>(vsWorldMatrix) + vSPerObjectConstantBufferAlignedSize);
	vsWorldMatrix->worldMatrix = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(rotaion.x, rotaion.y, rotaion.z, rotaion.w),
		DirectX::XMVectorSet(positionX - 10.0f, positionY + 5.0f, positionZ + 5.0f, 0.0f));


	rotaion = Quaternion(EulerRotation(0.0f, 0.75f * pi, 0.0f));
	vsWorldMatrix = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<unsigned char*>(vsWorldMatrix) + vSPerObjectConstantBufferAlignedSize);
	vsWorldMatrix->worldMatrix = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(rotaion.x, rotaion.y, rotaion.z, rotaion.w),
		DirectX::XMVectorSet(positionX, positionY - 14.14213562f, positionZ + 14.14213562f, 0.0f));

	rotaion = Quaternion(EulerRotation(0.0f, -0.75f * pi, pi));
	vsWorldMatrix = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<unsigned char*>(vsWorldMatrix) + vSPerObjectConstantBufferAlignedSize);
	vsWorldMatrix->worldMatrix = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(rotaion.x, rotaion.y, rotaion.z, rotaion.w),
		DirectX::XMVectorSet(positionX, positionY - 1.14213562f, positionZ + 24.14213562f, 0.0f));

	rotaion = Quaternion(EulerRotation(0.75f * pi, 0.0f, 0.5f * pi));
	vsWorldMatrix = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<unsigned char*>(vsWorldMatrix) + vSPerObjectConstantBufferAlignedSize);
	vsWorldMatrix->worldMatrix = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(rotaion.x, rotaion.y, rotaion.z, rotaion.w),
		DirectX::XMVectorSet(positionX + 10.0f, positionY - 9.14213562f, positionZ + 19.14213562f, 0.0f));

	rotaion = Quaternion(EulerRotation(0.75f * pi, 0.0f, -0.5f * pi));
	vsWorldMatrix = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<unsigned char*>(vsWorldMatrix) + vSPerObjectConstantBufferAlignedSize);
	vsWorldMatrix->worldMatrix = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(rotaion.x, rotaion.y, rotaion.z, rotaion.w),
		DirectX::XMVectorSet(positionX - 10.0f, positionY - 9.14213562f, positionZ + 19.14213562f, 0.0f));


	rotaion = Quaternion(EulerRotation(0.0f, 0.5f * pi, 0.0f));
	vsWorldMatrix = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<unsigned char*>(vsWorldMatrix) + vSPerObjectConstantBufferAlignedSize);
	vsWorldMatrix->worldMatrix = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(rotaion.x, rotaion.y, rotaion.z, rotaion.w),
		DirectX::XMVectorSet(positionX, positionY - 21.21320344f, positionZ + 30.0f, 0.0f));


	rotaion = Quaternion(EulerRotation(0.0f, -0.5f * pi, pi));
	vsWorldMatrix = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<unsigned char*>(vsWorldMatrix) + vSPerObjectConstantBufferAlignedSize);
	vsWorldMatrix->worldMatrix = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(rotaion.x, rotaion.y, rotaion.z, rotaion.w),
		DirectX::XMVectorSet(positionX, positionY - 8.21320344f, positionZ + 41.2f, 0.0f));


	rotaion = Quaternion(EulerRotation(0.5f * pi, 0.0f, 0.5f * pi));
	vsWorldMatrix = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<unsigned char*>(vsWorldMatrix) + vSPerObjectConstantBufferAlignedSize);
	vsWorldMatrix->worldMatrix = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(rotaion.x, rotaion.y, rotaion.z, rotaion.w),
		DirectX::XMVectorSet(positionX + 10.0f, positionY - 16.21320344f, positionZ + 30.0f, 0.0f));

	rotaion = Quaternion(EulerRotation(0.5f * pi, 0.0f, -0.5f * pi));
	vsWorldMatrix = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<unsigned char*>(vsWorldMatrix) + vSPerObjectConstantBufferAlignedSize);
	vsWorldMatrix->worldMatrix = DirectX::XMMatrixAffineTransformation(DirectX::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f), DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f), DirectX::XMVectorSet(rotaion.x, rotaion.y, rotaion.z, rotaion.w),
		DirectX::XMVectorSet(positionX - 10.0f, positionY - 16.21320344f, positionZ + 30.0f, 0.0f));

	vsWorldMatrix = reinterpret_cast<VSPerObjectConstantBuffer*>(reinterpret_cast<unsigned char*>(vsWorldMatrix) + vSPerObjectConstantBufferAlignedSize);

	psPerObjectCBVCpuAddress->specularColor[0] = 0.5f;
	psPerObjectCBVCpuAddress->specularColor[1] = 0.5f;
	psPerObjectCBVCpuAddress->specularColor[2] = 0.5f;
	psPerObjectCBVCpuAddress->specularColor[3] = 1.0f;
	psPerObjectCBVCpuAddress->specularPower = 4.0f;
	psPerObjectCBVCpuAddress->baseColorTexture = baseColorTextureIndex;
}

void CaveModelPart1::render(ID3D12GraphicsCommandList* const directCommandList, Mesh** meshes)
{
	directCommandList->IASetVertexBuffers(0u, 1u, &meshes[meshIndex]->vertexBufferView);
	directCommandList->IASetIndexBuffer(&meshes[meshIndex]->indexBufferView);
	directCommandList->SetGraphicsRootConstantBufferView(3u, perObjectCBVGpuAddress + vSPerObjectConstantBufferAlignedSize * numSquares);

	//needs to draw instances
	directCommandList->SetGraphicsRootConstantBufferView(2u, perObjectCBVGpuAddress);
	directCommandList->DrawIndexedInstanced(meshes[meshIndex]->indexCount, 1u, 0u, 0, 0u);

	directCommandList->SetGraphicsRootConstantBufferView(2u, perObjectCBVGpuAddress + vSPerObjectConstantBufferAlignedSize);
	directCommandList->DrawIndexedInstanced(meshes[meshIndex]->indexCount, 1u, 0u, 0, 0u);

	directCommandList->SetGraphicsRootConstantBufferView(2u, perObjectCBVGpuAddress + vSPerObjectConstantBufferAlignedSize * 2u);
	directCommandList->DrawIndexedInstanced(meshes[meshIndex]->indexCount, 1u, 0u, 0, 0u);

	directCommandList->SetGraphicsRootConstantBufferView(2u, perObjectCBVGpuAddress + vSPerObjectConstantBufferAlignedSize * 3u);
	directCommandList->DrawIndexedInstanced(meshes[meshIndex]->indexCount, 1u, 0u, 0, 0u);


	directCommandList->SetGraphicsRootConstantBufferView(2u, perObjectCBVGpuAddress + vSPerObjectConstantBufferAlignedSize * 4u);
	directCommandList->DrawIndexedInstanced(meshes[meshIndex]->indexCount, 1u, 0u, 0, 0u);

	directCommandList->SetGraphicsRootConstantBufferView(2u, perObjectCBVGpuAddress + vSPerObjectConstantBufferAlignedSize * 5u);
	directCommandList->DrawIndexedInstanced(meshes[meshIndex]->indexCount, 1u, 0u, 0, 0u);

	directCommandList->SetGraphicsRootConstantBufferView(2u, perObjectCBVGpuAddress + vSPerObjectConstantBufferAlignedSize * 6u);
	directCommandList->DrawIndexedInstanced(meshes[meshIndex]->indexCount, 1u, 0u, 0, 0u);

	directCommandList->SetGraphicsRootConstantBufferView(2u, perObjectCBVGpuAddress + vSPerObjectConstantBufferAlignedSize * 7u);
	directCommandList->DrawIndexedInstanced(meshes[meshIndex]->indexCount, 1u, 0u, 0, 0u);


	directCommandList->SetGraphicsRootConstantBufferView(2u, perObjectCBVGpuAddress + vSPerObjectConstantBufferAlignedSize * 8u);
	directCommandList->DrawIndexedInstanced(meshes[meshIndex]->indexCount, 1u, 0u, 0, 0u);

	directCommandList->SetGraphicsRootConstantBufferView(2u, perObjectCBVGpuAddress + vSPerObjectConstantBufferAlignedSize * 9u);
	directCommandList->DrawIndexedInstanced(meshes[meshIndex]->indexCount, 1u, 0u, 0, 0u);

	directCommandList->SetGraphicsRootConstantBufferView(2u, perObjectCBVGpuAddress + vSPerObjectConstantBufferAlignedSize * 10u);
	directCommandList->DrawIndexedInstanced(meshes[meshIndex]->indexCount, 1u, 0u, 0, 0u);

	directCommandList->SetGraphicsRootConstantBufferView(2u, perObjectCBVGpuAddress + vSPerObjectConstantBufferAlignedSize * 11u);
	directCommandList->DrawIndexedInstanced(meshes[meshIndex]->indexCount, 1u, 0u, 0, 0u);
}

bool CaveModelPart1::isInView(const Frustum& Frustum)
{
	return Frustum.checkCuboid2(positionX + 5.0f, positionY + 20.0f, positionZ + 84.0f, positionX - 5.0f, positionY - 15.0f, positionZ - 44.0f);
}