/*
* OpenSLAudioModule_AllMixing.cpp
*
*  Created on: 2015. 5. 18.
*      Author: IZSoft-PSC
*/
#include "OpenSLAudioModule_AllMixing.h"

void CheckErr( SLresult res )
{
	if ( res != SL_RESULT_SUCCESS )
	{
	// Debug printing to be placed herefassetManager
//		OSLLOGE("%s CheckErr: %d", __FUNCTION__, res);
		return;
	}
}

OpenSLAudioModule_AllMixing::OpenSLAudioModule_AllMixing(int systemSampleRate, int streamSize, int bufferSampleLength, int systemBufferSampleLength)
:AudioModule(systemSampleRate, bufferSampleLength, systemBufferSampleLength, streamSize)
{
	audioPool = new AudioPool(systemSampleRate, 1, streamSize, bufferSampleLength, 1.00f);
	this->SYSTEM_BUFFER_SAMPLE_LENGTH = bufferSampleLength;

	this->assetManager = assetManager;
	SLresult res;
	SLEngineOption EngineOption[] = {
									(SLuint32) SL_ENGINEOPTION_THREADSAFE,
									(SLuint32) SL_BOOLEAN_TRUE};

	res = slCreateEngine( &slEngine, 1, EngineOption, 0, NULL, NULL);
	CheckErr(res);

	res = (*slEngine)->Realize(slEngine, SL_BOOLEAN_FALSE);
}

int OpenSLAudioModule_AllMixing::init(int playerNumberMax)
{
	int i;
	SLresult res;
	/* Callback context for the buffer queue callback function */
	context = (CallbackContext *) malloc(sizeof(CallbackContext));

	/* Get the SL Engine Interface which is implicit */
	res = (*slEngine)->GetInterface(slEngine, SL_IID_ENGINE, (void *)&EngineItf);
	CheckErr(res);
	const SLuint32 count = 1;
	const SLInterfaceID	ids[count] = {SL_IID_ENVIRONMENTALREVERB};
	const SLboolean req[count] = {SL_BOOLEAN_FALSE};

	// Create Output Mix object to be used by player
	res = (*EngineItf)->CreateOutputMix(EngineItf, &OutputMix, count, ids, req); CheckErr(res);

	// Realizing the Output Mix object in synchronous mode.
	res = (*OutputMix)->Realize(OutputMix, SL_BOOLEAN_FALSE); CheckErr(res);
	//res = (*OutputMix)->GetInterface(OutputMix, SL_IID_VOLUME, &volumeItf); CheckErr(res);
	/* Setup the data source structure for the buffer queue */
	bufferQueue.locatorType = SL_DATALOCATOR_BUFFERQUEUE;
	bufferQueue.numBuffers = 1;

	/* Setup the format of the content in the buffer queue */
	pcm.formatType = SL_DATAFORMAT_PCM;
	pcm.numChannels = 2;

	switch(SAMPLE_RATE)
	{
		case 44100:
			pcm.samplesPerSec = SL_SAMPLINGRATE_44_1;
			break;
		case 48000:
			pcm.samplesPerSec = SL_SAMPLINGRATE_48;
			break;
		case 64000:
			pcm.samplesPerSec = SL_SAMPLINGRATE_64;
			break;
		case 88200:
			pcm.samplesPerSec = SL_SAMPLINGRATE_88_2;
			break;
		case 96000:
			pcm.samplesPerSec = SL_SAMPLINGRATE_96;
			break;
		case 192000:
			pcm.samplesPerSec = SL_SAMPLINGRATE_192;
			break;
	}
	//pcm.samplePerSec = SL_SAMPLINGRATE_44_1;
	pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	pcm.containerSize = 16;
	pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
	pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
	audioSource.pFormat = (void *)&pcm;
	audioSource.pLocator = (void *)&bufferQueue;

	/* Setup the data sink structure */
	locator_outputmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
	locator_outputmix.outputMix = OutputMix;
	audioSink.pLocator = (void *)&locator_outputmix;
	audioSink.pFormat = NULL;

	/* Initialize the context for Buffer queue callbacks */
	shortPcmData = (short int *) malloc(sizeof(short int) * BUFFER_SAMPLE_LENGTH * 2);
	((CallbackContext *)context)->shortBuffer = (SLint16 *)shortPcmData;
	((CallbackContext *)context)->sampleLength = BUFFER_SAMPLE_LENGTH;
	((CallbackContext *)context)->audioPool = this->audioPool;
	((CallbackContext *)context)->audioModule = this;

	const SLuint32 arraySize = 2;
	const SLInterfaceID player_ids[arraySize] = { SL_IID_BUFFERQUEUE, SL_IID_VOLUME };
	const SLboolean player_req[arraySize] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

	/* Create the music player */
	res = (*EngineItf)->CreateAudioPlayer(EngineItf, &player, &audioSource, &audioSink, arraySize, player_ids, player_req); CheckErr(res);
	/* Realizing the player in synchronous mode */
	res = (*player)->Realize(player, SL_BOOLEAN_FALSE); CheckErr(res);
	/* Get seek and play interfaces */
	res = (*player)->GetInterface(player, SL_IID_PLAY, (void *)&playItf); CheckErr(res);
	res = (*player)->GetInterface(player, SL_IID_VOLUME, (void *)&volumeItf); CheckErr(res);
	res = (*player)->GetInterface(player, SL_IID_BUFFERQUEUE, (void*)&bufferQueueItf); CheckErr(res);

	/* Setup to receive buffer queue event callbacks */
	res = (*bufferQueueItf)->RegisterCallback(bufferQueueItf, OpenSLAudioModule_AllMixing::BufferQueueCallback, context); CheckErr(res);

	/* Before we start set volume to -3dB (-300mB) */
	res = (*volumeItf)->SetVolumeLevel(volumeItf, 0); CheckErr(res);


	/* Play the PCM samples using a buffer queue */
	res = (*playItf)->SetPlayState( playItf, SL_PLAYSTATE_PLAYING); CheckErr(res);
	/* Enqueue a few buffers to get the ball rolling */
	memset(context->shortBuffer, 0, BUFFER_SAMPLE_LENGTH * 2);
	res = (*bufferQueueItf)->Enqueue(bufferQueueItf, context->shortBuffer, BUFFER_SAMPLE_LENGTH * 2); /* Size given in bytes. */
	CheckErr(res);

	this->audioState = AUDIO_STATE_END;
	this->currentPosition = 0;
}

int OpenSLAudioModule_AllMixing::release()
{
	SLresult res;

	/// Make sure player is stopped ///
	res = (*playItf)->SetPlayState(playItf, SL_PLAYSTATE_STOPPED);

	/// Destroy the player ///
	(*player)->Destroy(player);

	/// Destroy Output ////
	(*OutputMix)->Destroy(OutputMix);

	free(context);
	free(shortPcmData);
}

void OpenSLAudioModule_AllMixing::BufferQueueCallback(SLBufferQueueItf queueItf, void *pContext)
{
	SLresult res;
	CallbackContext *context = (CallbackContext*)pContext;
	AudioPool *audioPool = context->audioPool;
	int sampleLength = context->sampleLength;
	OpenSLAudioModule_AllMixing *audioModule = (OpenSLAudioModule_AllMixing*) context->audioModule;

	//cocos2d::log("%s %d", __FUNCTION__, audioModule->currentPosition);
	if(audioModule->audioState == AUDIO_STATE_PLAYING)
	{
		audioPool->process((short*)context->shortBuffer, nullptr, audioModule->currentPosition);
		audioModule->currentPosition += sampleLength;
	}
	else
		memset(context->shortBuffer, 0, sizeof(short) * sampleLength * 2);

	res = (*queueItf)->Enqueue(queueItf, (void*)context->shortBuffer, sizeof(SLint16) * sampleLength * 2); /* Size given in bytes. */ CheckErr(res);
}



OpenSLAudioModule_AllMixing::~OpenSLAudioModule_AllMixing()
{

	SLresult res;
	res = (*playItf)->SetPlayState( playItf, SL_PLAYSTATE_STOPPED); CheckErr(res);

	(*player)->Destroy(player);
	(*OutputMix)->Destroy(OutputMix);
	(*slEngine)->Destroy(slEngine);

	free(shortPcmData);
	free(context);
}

// for MR AUDIO
int OpenSLAudioModule_AllMixing::readyMR(const char* path, AudioType audioType)
{
	unsigned int soundID = GetHashValue(path);
	return audioPool->readyMR(soundID, audioType);
}

int OpenSLAudioModule_AllMixing::readyMR(NoteObject* noteObj)
{
	unsigned int soundID = GetHashValue(noteObj->m_FullPath);
	return audioPool->readyMR(soundID, noteObj);
}

int OpenSLAudioModule_AllMixing::startMR()
{
	currentPosition = 0;
    long currentTime = AudioTool::getSystemCurrentTime();
	for(int j=0;j<POSITION_CHECK_COUNT;j++)
	{
		positionArray[j] = 0;
		timeArray[j] = currentTime;
	}

	audioState = AUDIO_STATE_PLAYING;
	return 0;
}

int OpenSLAudioModule_AllMixing::pauseMR()
{
	audioState = AUDIO_STATE_PAUSED;
	return 0;
}

int OpenSLAudioModule_AllMixing::resumeMR()
{
	long currentTime = AudioTool::getSystemCurrentTime();
	for(int j=0;j<POSITION_CHECK_COUNT;j++)
	{
		positionArray[j] = currentPosition;
		timeArray[j] = currentTime;
	}

	audioState = AUDIO_STATE_PLAYING;
	return 0;
}

int OpenSLAudioModule_AllMixing::stopMR()
{
	audioState = AUDIO_STATE_END;
	currentPosition = 0;
	return 0;
}

// for NOTE EFFECT
int OpenSLAudioModule_AllMixing::playEffect(const char *path, float pitch, float noteVelocity) // return Stream ID
{
	unsigned int soundID = GetHashValue(path);
	return audioPool->play(soundID, noteVelocity, 0);
}

int OpenSLAudioModule_AllMixing::playEffect(NoteObject *noteObj) // return Stream ID
{
	unsigned int soundID = GetHashValue(noteObj->m_FullPath);
	int streamID = audioPool->play(soundID, noteObj, 0);
	return streamID;
}

int OpenSLAudioModule_AllMixing::startEffect()
{
	return 0;
}
