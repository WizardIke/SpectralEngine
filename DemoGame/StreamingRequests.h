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
		void(*deallocateNode)(StreamingRequest* request, void* threadResources, void* globalResources),
		const wchar_t * filename, Zone<ThreadResources, GlobalResources>& zone) :
		VirtualTextureManager::TextureStreamingRequest(textureLoaded, deallocateNode, filename),
		zone(zone)
	{}
};

class TextureRequest : public TextureManager::TextureStreamingRequest
{
public:
	Zone<ThreadResources, GlobalResources>& zone;
	TextureRequest(void(*textureLoaded)(TextureManager::TextureStreamingRequest& request, void* tr, void* gr, unsigned int textureDescriptor),
		void(*deallocateNode)(StreamingRequest* request, void* threadResources, void* globalResources),
		const wchar_t * filename, Zone<ThreadResources, GlobalResources>& zone) :
		TextureManager::TextureStreamingRequest(textureLoaded, deallocateNode, filename),
		zone(zone)
	{}
};

class MeshRequest : public MeshManager::MeshStreamingRequest
{
public:
	Zone<ThreadResources, GlobalResources>& zone;
	MeshRequest(void(*textureLoaded)(MeshManager::MeshStreamingRequest& request, void* tr, void* gr, Mesh& texture),
		void(*deallocateNode)(StreamingRequest* request, void* threadResources, void* globalResources),
		const wchar_t * filename, Zone<ThreadResources, GlobalResources>& zone) :
		MeshManager::MeshStreamingRequest(textureLoaded, deallocateNode, filename),
		zone(zone)
	{}
};