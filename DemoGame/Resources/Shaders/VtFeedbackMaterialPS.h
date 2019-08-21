#pragma once
#include <Shaders/VtFeedbackMaterialPS.h>
#include <cstddef>
#include <d3d12.h>
constexpr std::size_t vtFeedbackMaterialPsSize = (sizeof(VtFeedbackMaterialPS) + D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1ull);