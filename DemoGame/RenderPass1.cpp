#include "RenderPass1.h"
#include "GlobalResources.h"

RenderPass1::RenderPass1(GlobalResources& globalResources, Transform& mainCameraTransform, uint32_t virtualTextureFeedbackWidth, uint32_t virtualTextureFeedbackHeight,
	D3D12_GPU_VIRTUAL_ADDRESS& constantBufferGpu, unsigned char*& constantBufferCpu, float feedbackFieldOfView) : data(globalResources.taskShedular.threadCount())
{
	auto& subPass = std::get<virtualTextureFeedbackSubPassIndex>(data.subPasses);
	subPass.~FeedbackAnalizerSubPass();
	new(&subPass) FeedbackAnalizerSubPass{ globalResources.graphicsEngine, mainCameraTransform, virtualTextureFeedbackWidth, virtualTextureFeedbackHeight,
		constantBufferGpu, constantBufferCpu, feedbackFieldOfView };
}