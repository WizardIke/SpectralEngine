#include <filesystem>

extern "C"
{
	typedef bool(*ImportResourceType)(void* context, const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath);

#ifdef _WIN32
		__declspec(dllexport)
#endif
		bool importResource(const std::filesystem::path& baseInputPath, const std::filesystem::path& baseOutputPath, const std::filesystem::path& relativeInputPath, void* importResourceContext,
			ImportResourceType importResource, bool forceReImport) noexcept
	{
		//these are already handled
		return true;
	}
}