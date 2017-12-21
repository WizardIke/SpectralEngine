#include "UserInterface.h"
#include <DirectXMath.h>
#include <BaseExecutor.h>
#include "../Assets.h"

UserInterface::UserInterface(Executor* const executor) :
	CPUUsageSentence(executor->sharedResources->graphicsEngine.graphicsDevice, &((Assets*)executor->sharedResources)->arial, DirectX::XMFLOAT2(0.01f, 0.01f), DirectX::XMFLOAT2(1.0f, 1.0f),
		DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)),
	FPSSentence(executor->sharedResources->graphicsEngine.graphicsDevice, &((Assets*)executor->sharedResources)->arial, DirectX::XMFLOAT2(0.01f, 0.07f), DirectX::XMFLOAT2(1.0f, 1.0f)) {}

void UserInterface::update1(Executor* const executor)
{
	unsigned int frameIndex = executor->sharedResources->graphicsEngine.frameIndex;
	CPUUsageSentence.update(executor);
	FPSSentence.update(frameIndex, executor->sharedResources->timer.frameTime);
}

void UserInterface::update2(Executor* const executor)
{
	Assets* const assets = (Assets*)executor->sharedResources;
	auto frameIndex = assets->graphicsEngine.frameIndex;
	const auto opaqueDirectCommandList = executor->renderPass.colorSubPass().opaqueCommandList();
	

	opaqueDirectCommandList->SetPipelineState(assets->pipelineStateObjects.text);
	opaqueDirectCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, assets->arial.psPerObjectCBVGpuAddress);

	opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &CPUUsageSentence.textVertexBufferView[frameIndex]);
	opaqueDirectCommandList->DrawInstanced(4u, static_cast<UINT>(CPUUsageSentence.text.length()), 0u, 0u);

	opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &FPSSentence.textVertexBufferView[frameIndex]);
	opaqueDirectCommandList->DrawInstanced(4u, static_cast<UINT>(FPSSentence.text.length()), 0u, 0u);
}

void UserInterface::start(Executor* const executor)
{
	executor->updateJobQueue().push(Job(this, [](void*const requester, BaseExecutor*const executor1)
	{
		auto executor = reinterpret_cast<Executor*>(executor1);
		UserInterface* const ui = reinterpret_cast<UserInterface* const>(requester);
		ui->update1(executor);
		executor->renderJobQueue().push(Job(requester, [](void*const requester, BaseExecutor*const executor1)
		{
			auto executor = reinterpret_cast<Executor*>(executor1);
			UserInterface* const ui = reinterpret_cast<UserInterface* const>(requester);
			ui->update2(executor);
			ui->start(executor);
		}));
	}));
}