#include "RenderPass1.h"
#include "GlobalResources.h"

RenderPass1::RenderPass1(TaskShedular<ThreadResources>& taskShedular, StreamingManager& streamingManager, GraphicsEngine& graphicsEngine, AsynchronousFileManager& asynchronousFileManager,
	uint32_t virtualTextureFeedbackWidth, uint32_t virtualTextureFeedbackHeight, Transform& mainCameraTransform, float fieldOfView) :
	data(taskShedular.threadCount(), 
		Tuple<void*, StreamingManager&, GraphicsEngine&, AsynchronousFileManager&, uint32_t, uint32_t, Transform&, float>{&taskShedular, streamingManager,
		graphicsEngine, asynchronousFileManager, virtualTextureFeedbackWidth, virtualTextureFeedbackHeight, mainCameraTransform, fieldOfView},
		Tuple<>{},
		Tuple<>{}
	)
{}

void RenderPass1::setConstantBuffers(D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpu, unsigned char*& constantBufferCpu)
{
	auto& subPass = virtualTextureFeedbackSubPass();
	subPass.setConstantBuffers(constantBufferGpu, constantBufferCpu);
}