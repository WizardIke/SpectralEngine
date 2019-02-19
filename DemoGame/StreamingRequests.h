#pragma once
#include <VirtualTextureManager.h>
#include <MeshManager.h>
#include <TextureManager.h>
#include <Mesh.h>
#include <Zone.h>
#include "ThreadResources.h"
#include "GlobalResources.h"

class VirtualTextureRequest : public VirtualTextureManager::TextureStreamingRequest
{
public:
	Zone<ThreadResources, GlobalResources>& zone;
	VirtualTextureRequest(void(*textureLoaded)(VirtualTextureManager::TextureStreamingRequest& request, void* tr, void* gr, const VirtualTextureManager::Texture& texture),
		void(*deleteRequest)(VirtualTextureManager::TextureStreamingRequest& request),
		const wchar_t * filename, Zone<ThreadResources, GlobalResources>& zone) :
		VirtualTextureManager::TextureStreamingRequest(textureLoaded, deleteRequest, filename),
		zone(zone)
	{}
};

class TextureRequest : public TextureManager::TextureStreamingRequest
{
public:
	Zone<ThreadResources, GlobalResources>& zone;
	TextureRequest(void(*textureLoaded)(TextureManager::TextureStreamingRequest& request, void* tr, void* gr, unsigned int textureDescriptor),
		void(*deleteRequest)(TextureManager::TextureStreamingRequest& request),
		const wchar_t * filename, Zone<ThreadResources, GlobalResources>& zone) :
		TextureManager::TextureStreamingRequest(textureLoaded, deleteRequest, filename),
		zone(zone)
	{}
};

class MeshRequest : public MeshManager::MeshStreamingRequest
{
public:
	Zone<ThreadResources, GlobalResources>& zone;
	MeshRequest(void(*textureLoaded)(MeshManager::MeshStreamingRequest& request, void* tr, void* gr, Mesh& texture),
		void(*deleteRequest)(MeshManager::MeshStreamingRequest& request),
		const wchar_t * filename, Zone<ThreadResources, GlobalResources>& zone) :
		MeshManager::MeshStreamingRequest(textureLoaded, deleteRequest, filename),
		zone(zone)
	{}
};