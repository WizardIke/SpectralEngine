#pragma once
#include <XAudio2SourceVoice.h>
#include <File.h>
#include <atomic>
#include <cstdint>
#include <AsynchronousFileManager.h>
class ThreadResources;
class GlobalResources;

class AmbientMusic : private IXAudio2VoiceCallback, private AsynchronousFileManager::IORequest
{
	struct BufferDescriptor
	{
		const unsigned char* data;
		std::atomic<const wchar_t*> filename = nullptr;
		std::size_t filePositionStart;
		std::size_t filePositionEnd;
	};
	//The size of one of the two sound buffers
	constexpr static size_t rawSoundDataBufferSize = 64 * 1024;
	XAudio2SourceVoice musicPlayer;
	std::atomic<unsigned int> bufferLoadingAndIsFirstBuffer = 0u;
	const wchar_t* const * files;
	std::size_t fileCount;
	std::size_t previousTrack;
	std::size_t filePosition;
	std::size_t bytesRemaining;
	std::size_t bytesNeeded;
	BufferDescriptor bufferDescriptors[3];
	BufferDescriptor* currentBuffer;
	GlobalResources& globalResources;
	void(*callback)(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources);

	virtual void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnBufferStart(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnLoopEnd(void* pBufferContext) override;
	virtual void STDMETHODCALLTYPE OnStreamEnd() override;
	virtual void STDMETHODCALLTYPE OnVoiceError(void* pBufferContext, HRESULT Error) override;
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override;
	virtual void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 BytesRequired) override;

	void update(ThreadResources& executor, GlobalResources& sharedResources);
	void findNextMusic(ThreadResources& threadResources, GlobalResources& globalResources, void(*callback)(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources));
	static void loadSoundData(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources);
	static void loadSoundDataPart2(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources);
	static void onSoundDataLoadingFinished(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources);
	static void submitBuffer(IXAudio2SourceVoice* musicPlayer, void* context, const unsigned char* data, std::size_t length);
	void startImpl(ThreadResources& executor, GlobalResources& sharedResources);
	static void onFirstBufferFinishedLoading(AmbientMusic& music, ThreadResources& threadResources, GlobalResources& globalResources);
public:
	AmbientMusic(ThreadResources& executor, GlobalResources& sharedResources, const wchar_t* const * files, std::size_t fileCount);
	~AmbientMusic();
	void start(ThreadResources& executor, GlobalResources& sharedResources);
};