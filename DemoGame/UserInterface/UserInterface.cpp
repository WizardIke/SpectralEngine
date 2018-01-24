#include "UserInterface.h"
#include <DirectXMath.h>
#include <BaseExecutor.h>
#include "../Assets.h"

UserInterface::UserInterface(SharedResources& sharedResources) :
	CPUUsageSentence(sharedResources.graphicsEngine.graphicsDevice, &((Assets*)&sharedResources)->arial, DirectX::XMFLOAT2(0.01f, 0.01f), DirectX::XMFLOAT2(1.0f, 1.0f),
		DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)),
	FPSSentence(sharedResources.graphicsEngine.graphicsDevice, &((Assets*)&sharedResources)->arial, DirectX::XMFLOAT2(0.01f, 0.07f), DirectX::XMFLOAT2(1.0f, 1.0f)) {}

void UserInterface::update1(SharedResources& sharedResources)
{
	unsigned int frameIndex = sharedResources.graphicsEngine.frameIndex;
	CPUUsageSentence.update(sharedResources);
	FPSSentence.update(frameIndex, sharedResources.timer.frameTime);
}

void UserInterface::update2(Executor* const executor, SharedResources& sharedResources)
{
	Assets& assets = (Assets&)sharedResources;
	auto frameIndex = assets.graphicsEngine.frameIndex;
	const auto opaqueDirectCommandList = executor->renderPass.colorSubPass().opaqueCommandList();
	

	opaqueDirectCommandList->SetPipelineState(assets.pipelineStateObjects.text);
	opaqueDirectCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	opaqueDirectCommandList->SetGraphicsRootConstantBufferView(3u, assets.arial.psPerObjectCBVGpuAddress);

	opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &CPUUsageSentence.textVertexBufferView[frameIndex]);
	opaqueDirectCommandList->DrawInstanced(4u, static_cast<UINT>(CPUUsageSentence.text.length()), 0u, 0u);

	opaqueDirectCommandList->IASetVertexBuffers(0u, 1u, &FPSSentence.textVertexBufferView[frameIndex]);
	opaqueDirectCommandList->DrawInstanced(4u, static_cast<UINT>(FPSSentence.text.length()), 0u, 0u);
}

void UserInterface::restart(Executor* const executor)
{
	executor->updateJobQueue().push(Job(this, [](void*const requester, BaseExecutor*const executor1, SharedResources& sharedResources)
	{
		auto executor = reinterpret_cast<Executor*>(executor1);
		UserInterface* const ui = reinterpret_cast<UserInterface* const>(requester);
		ui->update1(sharedResources);
		executor->renderJobQueue().push(Job(requester, [](void*const requester, BaseExecutor*const executor1, SharedResources& sharedResources)
		{
			auto executor = reinterpret_cast<Executor*>(executor1);
			UserInterface* const ui = reinterpret_cast<UserInterface* const>(requester);
			ui->update2(executor, sharedResources);
			ui->restart(executor);
		}));
	}));
}

void UserInterface::start(Executor* const executor, SharedResources& sharedResources)
{
	sharedResources.syncMutex.lock();
	if (sharedResources.nextPhaseJob == Executor::update1NextPhaseJob)
	{
		sharedResources.syncMutex.unlock();
		restart(executor);
	}
	else
	{
		sharedResources.syncMutex.unlock();
		executor->renderJobQueue().push(Job(this, [](void*const requester, BaseExecutor*const executor1, SharedResources& sharedResources)
		{
			const auto executor = reinterpret_cast<Executor*>(executor1);
			const auto ui = reinterpret_cast<UserInterface*>(requester);
			ui->restart(executor);
		}));
	}
}