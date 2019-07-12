#pragma once
#include <VirtualTextureManager.h>
#include <MeshManager.h>
#include <TextureManager.h>
#include <Mesh.h>
#include <Zone.h>
#include "ThreadResources.h"
#include "GlobalResources.h"
#include <atomic>
#include <array>
#include <makeArray.h>

class VirtualTextureRequest : public VirtualTextureManager::TextureStreamingRequest
{
public:
	Zone<ThreadResources>& zone;
	VirtualTextureRequest(void(*textureLoaded)(VirtualTextureManager::TextureStreamingRequest& request, void* tr, const VirtualTextureManager::Texture& texture),
		const wchar_t * filename, Zone<ThreadResources>& zone) :
		VirtualTextureManager::TextureStreamingRequest(textureLoaded, filename),
		zone(zone)
	{}
};

class TextureRequest : public TextureManager::TextureStreamingRequest
{
public:
	Zone<ThreadResources>& zone;
	TextureRequest(void(*textureLoaded)(TextureManager::TextureStreamingRequest& request, void* tr, unsigned int textureDescriptor),
		const wchar_t * filename, Zone<ThreadResources>& zone) :
		TextureManager::TextureStreamingRequest(textureLoaded, filename),
		zone(zone)
	{}
};

class MeshRequest : public MeshManager::MeshStreamingRequest
{
public:
	Zone<ThreadResources>& zone;
	MeshRequest(void(*textureLoaded)(MeshManager::MeshStreamingRequest& request, void* tr, Mesh& mesh),
		const wchar_t * filename, Zone<ThreadResources>& zone) :
		MeshManager::MeshStreamingRequest(textureLoaded, filename),
		zone(zone)
	{}
};

template<std::size_t numberOfComponents>
class VirtualTextureRequests
{
public:
	class RequestInfo
	{
	public:
		const wchar_t* filename;
		void(*loadingFinished)(VirtualTextureManager::TextureStreamingRequest& request, void* tr, const VirtualTextureManager::Texture& texture);
	};
private:
	class Request : public VirtualTextureRequest
	{
	public:
		void(*loadingFinished)(VirtualTextureManager::TextureStreamingRequest& request, void* tr, const VirtualTextureManager::Texture& texture);
		VirtualTextureRequests* requests;

		Request(void(*textureLoaded)(VirtualTextureManager::TextureStreamingRequest& request, void* tr, const VirtualTextureManager::Texture& texture),
			void(*deleteRequest)(VirtualTextureManager::TextureStreamingRequest& request),
			const wchar_t* filename,
			Zone<ThreadResources>& zone,
			void(*loadingFinished)(VirtualTextureManager::TextureStreamingRequest& request, void* tr, const VirtualTextureManager::Texture& texture),
			VirtualTextureRequests* requests) : 
			VirtualTextureRequest(textureLoaded, deleteRequest, filename, zone),
			loadingFinished(loadingFinished),
			requests(requests)
		{}
	};
	std::atomic<unsigned int> numberOfcomponentsLoaded = 0u;
	std::atomic<unsigned int> numberOfComponentsReadyToDelete = 0u;

	union
	{
		std::array<Request, numberOfComponents> requestLoaders;
	};

	void(*loadingFinished)(VirtualTextureRequests& request, ThreadResources& threadResources);
	void(*deleteRequest)(VirtualTextureRequests& request);

	static void componentLoaded(VirtualTextureManager::TextureStreamingRequest& request1, void* tr, const VirtualTextureManager::Texture& texture)
	{
		auto& request = static_cast<Request&>(request1);
		request.loadingFinished(request, tr, texture);
		auto requests = request.requests;
		if(requests->numberOfcomponentsLoaded.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponents - 1u))
		{
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			requests->loadingFinished(*requests, threadResources);
		}
	}

	static void freeComponent(VirtualTextureManager::TextureStreamingRequest& request)
	{
		auto requests = static_cast<Request&>(request).requests;
		if(requests->numberOfComponentsReadyToDelete.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponents - 1u))
		{
			requests->requestLoaders.~array();
			requests->deleteRequest(*requests);
		}
	}
public:
	VirtualTextureRequests(void(*loadingFinished)(VirtualTextureRequests& request, ThreadResources& threadResources), void(*deleteRequest)(VirtualTextureRequests& request)) :
		loadingFinished(loadingFinished),
		deleteRequest(deleteRequest)
	{}

	~VirtualTextureRequests() {}

	void load(const RequestInfo(&requestInfos)[numberOfComponents], Zone<ThreadResources>& zone, ThreadResources& threadResources, VirtualTextureManager& virtualTextureManager)
	{
		new(&requestLoaders) std::array<Request, numberOfComponents>{ makeArray<numberOfComponents>(
			[&](std::size_t i)
			{
				return Request(componentLoaded, freeComponent, requestInfos[i].filename, zone, requestInfos[i].loadingFinished, this);
			}) };

		for(auto& request : requestLoaders)
		{
			virtualTextureManager.load(&request, threadResources);
		}
	}
};

template<std::size_t numberOfComponents>
class TextureRequests
{
public:
	class RequestInfo
	{
	public:
		const wchar_t* filename;
		void(*loadingFinished)(TextureManager::TextureStreamingRequest& request, void* tr, unsigned int textureDescriptor);
	};
private:
	class Request : public TextureRequest
	{
	public:
		void(*loadingFinished)(TextureManager::TextureStreamingRequest& request, void* tr, unsigned int textureDescriptor);
		TextureRequests* requests;

		Request(void(*textureLoaded)(TextureManager::TextureStreamingRequest& request, void* tr, unsigned int textureDescriptor),
			const wchar_t* filename,
			Zone<ThreadResources>& zone,
			void(*loadingFinished)(TextureManager::TextureStreamingRequest& request, void* tr, unsigned int textureDescriptor),
			TextureRequests* requests) :
			TextureRequest(textureLoaded, filename, zone),
			loadingFinished(loadingFinished),
			requests(requests)
		{}
	};
	std::atomic<unsigned int> numberOfcomponentsLoaded = 0u;

	union
	{
		std::array<Request, numberOfComponents> requestLoaders;
	};

	void(*loadingFinished)(TextureRequests& request, ThreadResources& threadResources);

	static void componentLoaded(TextureManager::TextureStreamingRequest& request1, void* tr, unsigned int textureDescriptor)
	{
		auto& request = static_cast<Request&>(request1);
		request.loadingFinished(request, tr, textureDescriptor);
		auto requests = request.requests;
		if(requests->numberOfcomponentsLoaded.fetch_add(1u, std::memory_order::memory_order_acq_rel) == (numberOfComponents - 1u))
		{
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			requests->requestLoaders.~array();
			requests->loadingFinished(*requests, threadResources);
		}
	}
public:
	TextureRequests(void(*loadingFinished)(TextureRequests& request, ThreadResources& threadResources)) :
		loadingFinished(loadingFinished)
	{}

	~TextureRequests() {}

	void load(const RequestInfo(&requestInfos)[numberOfComponents], Zone<ThreadResources>& zone, ThreadResources& threadResources, TextureManager& textureManager)
	{
		new(&requestLoaders) std::array<Request, numberOfComponents>{makeArray<numberOfComponents>(
			[&](std::size_t i)
			{
				return Request(componentLoaded, requestInfos[i].filename, zone, requestInfos[i].loadingFinished, this);
			}) };

		for(auto& request : requestLoaders)
		{
			textureManager.load(request, threadResources);
		}
	}

	Zone<ThreadResources>& zone() { return requestLoaders[0].zone; }
};

template<std::size_t numberOfComponents>
class MeshRequests
{
public:
	class RequestInfo
	{
	public:
		const wchar_t* filename;
		void(*loadingFinished)(MeshManager::MeshStreamingRequest& request, void* tr, Mesh& mesh);
	};
private:
	class Request : public MeshRequest
	{
	public:
		void(*loadingFinished)(MeshManager::MeshStreamingRequest& request, void* tr, Mesh& mesh);
		MeshRequests* requests;

		Request(void(*meshLoaded)(MeshManager::MeshStreamingRequest& request, void* tr, Mesh& mesh),
			const wchar_t* filename,
			Zone<ThreadResources>& zone,
			void(*loadingFinished)(MeshManager::MeshStreamingRequest& request, void* tr, Mesh& mesh),
			MeshRequests* requests) :
			MeshRequest(meshLoaded, filename, zone),
			loadingFinished(loadingFinished),
			requests(requests)
		{}
	};
	std::atomic<unsigned int> numberOfcomponentsLoaded = 0u;

	union
	{
		std::array<Request, numberOfComponents> requestLoaders;
	};

	void(*loadingFinished)(MeshRequests& request, ThreadResources& threadResources);

	static void componentLoaded(MeshManager::MeshStreamingRequest& request1, void* tr, Mesh& mesh)
	{
		auto& request = static_cast<Request&>(request1);
		request.loadingFinished(request, tr, mesh);
		auto requests = request.requests;
		if(requests->numberOfcomponentsLoaded.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponents - 1u))
		{
			ThreadResources& threadResources = *static_cast<ThreadResources*>(tr);
			requests->requestLoaders.~array();
			requests->loadingFinished(*requests, threadResources);
		}
	}
public:
	MeshRequests(void(*loadingFinished)(MeshRequests& request, ThreadResources& threadResources)) :
		loadingFinished(loadingFinished)
	{}

	~MeshRequests() {}

	void load(const RequestInfo(&requestInfos)[numberOfComponents], Zone<ThreadResources>& zone, ThreadResources& threadResources, MeshManager& meshManager)
	{
		new(&requestLoaders) std::array<Request, numberOfComponents>{makeArray<numberOfComponents>(
			[&](std::size_t i)
			{
				return Request(componentLoaded, requestInfos[i].filename, zone, requestInfos[i].loadingFinished, this);
			}) };

		for(auto& request : requestLoaders)
		{
			meshManager.load(&request, threadResources);
		}
	}

	Zone<ThreadResources>& zone() { return requestLoaders[0].zone; }
};

template<std::size_t numberOfComponents>
class TextureUnloader
{
private:
	class Request : public TextureManager::UnloadRequest
	{
	public:
		TextureUnloader* unloader;

		Request(const wchar_t* filename, void(*deleteRequest)(AsynchronousFileManager::ReadRequest& request, void* tr), TextureUnloader* unloader) :
			TextureManager::UnloadRequest(filename, deleteRequest), unloader(unloader) {}
	};

	union
	{
		std::array<Request, numberOfComponents> requestUnloaders;
	};
	void(*deleteRequest)(TextureUnloader& unloader, void* tr);
	std::atomic<unsigned int> numberOfComponentsReadyToDelete = 0u;

	static void freeComponent(AsynchronousFileManager::ReadRequest& request, void* tr)
	{
		auto unloader = static_cast<Request&>(request).unloader;
		if(unloader->numberOfComponentsReadyToDelete.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponents - 1u))
		{
			unloader->requestUnloaders.~array();
			unloader->deleteRequest(*unloader, tr);
		}
	}
public:
	TextureUnloader(void(*deleteRequest)(TextureUnloader& unloader, void* tr)) : deleteRequest(deleteRequest) {}
	~TextureUnloader() {}

	void unload(const wchar_t* const(&filenames)[numberOfComponents], ThreadResources& threadResources, TextureManager& textureManager)
	{
		new(&requestUnloaders) std::array<Request, numberOfComponents>{makeArray<numberOfComponents>(
			[&](std::size_t i)
			{
				return Request(filenames[i], freeComponent, this);
			}) };

		for(auto& request : requestUnloaders)
		{
			textureManager.unload(request, threadResources);
		}
	}
};

template<std::size_t numberOfComponents>
class MeshUnloader
{
private:
	class Request : public MeshManager::UnloadRequest
	{
	public:
		MeshUnloader* unloader;

		Request(const wchar_t* filename, void(*deleteRequest)(AsynchronousFileManager::ReadRequest& request, void* tr), MeshUnloader* unloader) :
			MeshManager::UnloadRequest(filename, deleteRequest), unloader(unloader) {}
	};

	union
	{
		std::array<Request, numberOfComponents> requestUnloaders;
	};
	void(*deleteRequest)(MeshUnloader& unloader, void* tr);
	std::atomic<unsigned int> numberOfComponentsReadyToDelete = 0u;

	static void freeComponent(AsynchronousFileManager::ReadRequest& request, void* tr)
	{
		auto unloader = static_cast<Request&>(request).unloader;
		if(unloader->numberOfComponentsReadyToDelete.fetch_add(1u, std::memory_order_acq_rel) == (numberOfComponents - 1u))
		{
			unloader->requestUnloaders.~array();
			unloader->deleteRequest(*unloader, tr);
		}
	}
public:
	MeshUnloader(void(*deleteRequest)(MeshUnloader& unloader, void* tr)) : deleteRequest(deleteRequest) {}
	~MeshUnloader() {}

	void unload(const wchar_t* const(&filenames)[numberOfComponents], ThreadResources& threadResources, MeshManager& meshManager)
	{
		new(&requestUnloaders) std::array<Request, numberOfComponents>{makeArray<numberOfComponents>(
			[&](std::size_t i)
			{
				return Request(filenames[i], freeComponent, this);
			}) };

		for(auto& request : requestUnloaders)
		{
			meshManager.unload(&request, threadResources);
		}
	}
};

class AddReflectionCamera : public RenderPass1::RenderToTextureSubPass::AddCameraRequest
{
	using Base = RenderPass1::RenderToTextureSubPass::AddCameraRequest;
public:
	Zone<ThreadResources>& zone;

	AddReflectionCamera(unsigned long entity1, ReflectionCamera&& camera1, void(*callback1)(Base&, void* tr), Zone<ThreadResources>& zone1) : Base(entity1, std::move(camera1), callback1), zone(zone1) {}
};

class RemoveReflectionCameraRequest : public RenderPass1::RenderToTextureSubPass::RemoveCamerasRequest
{
	using Base = RenderPass1::RenderToTextureSubPass::RemoveCamerasRequest;
public:
	Zone<ThreadResources>& zone;

	RemoveReflectionCameraRequest(unsigned long entity1, void(*callback1)(Base&, void* tr), Zone<ThreadResources>& zone1) : Base(entity1, callback1), zone(zone1) {}
};