/*
* OpenSLAudioModule_ChordMixing.cpp
*
*  Created on: 2015. 5. 18.
*      Author: IZSoft-PSC
*/
#include "OpenSLAudioModule_ChordMixing.h"
#include <time.h>

static inline long myclock()
{
	struct timeval tv;
	gettimeofday (&tv, NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

void CheckErr(const char *str, SLresult res )
{
	if ( res != SL_RESULT_SUCCESS )
	{
	// Debug printing to be placed here
//		OSLLOGE("%s %s CheckErr: %d", str, __FUNCTION__, res);
		return;
	}
}

OpenSLAudioModule_ChordMixing::OpenSLAudioModule_ChordMixing(int systemSampleRate, int streamSize, int bufferSampleLength, int systemBufferSampleLength, bool fastTrack)
:AudioModule(systemSampleRate, bufferSampleLength, systemBufferSampleLength, streamSize)
{
	FAST_TRACK = fastTrack;
	if(!FAST_TRACK)
		this->SAMPLE_RATE = 44100;

	audioPool = new AudioPool(SAMPLE_RATE, SYSTEM_PLAYER_NUMBER_MAX, streamSize, bufferSampleLength, 1.00f);
	BUFFER_SAMPLE_LENGTH = bufferSampleLength;
	SYSTEM_BUFFER_SAMPLE_LENGTH = systemBufferSampleLength;

	this->assetManager = assetManager;
	SLresult res;
	SLEngineOption EngineOption[] = {
										(SLuint32) SL_ENGINEOPTION_THREADSAFE,
										(SLuint32) SL_BOOLEAN_TRUE};

	res = slCreateEngine( &slEngine, 1, EngineOption, 0, NULL, NULL);


	res = (*slEngine)->Realize(slEngine, SL_BOOLEAN_FALSE);

}


int OpenSLAudioModule_ChordMixing::init(int playerNumberMax)
{
	this->PLAYER_NUMBER_MAX = playerNumberMax;

	int i;
	SLresult res;

	/* Get the SL Engine Interface which is implicit */
	res = (*slEngine)->GetInterface(slEngine, SL_IID_ENGINE, (void *)&EngineItf);

	const SLuint32 count = 1;
	const SLInterfaceID	ids[count] = {SL_IID_ENVIRONMENTALREVERB};
	const SLboolean req[count] = {SL_BOOLEAN_FALSE};

	// Create Output Mix object to be used by player
	res = (*EngineItf)->CreateOutputMix(EngineItf, &OutputMix, count, ids, req);

	// Realizing the Output Mix object in synchronous mode.
	res = (*OutputMix)->Realize(OutputMix, SL_BOOLEAN_FALSE);

	/* Setup the data source structure for the buffer queue */
	bufferQueue.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
	bufferQueue.numBuffers = 4;

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
	//pcm.samplesPerSec = SL_SAMPLINGRATE_44_1;
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


	SLboolean FLAG = FAST_TRACK ? SL_BOOLEAN_FALSE : SL_BOOLEAN_TRUE;

	const SLuint32 arraySize = 4;
	const SLInterfaceID player_ids[arraySize] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_VOLUME, SL_IID_PLAYBACKRATE, SL_IID_EFFECTSEND };
	const SLboolean player_req[arraySize] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, FLAG, FLAG };
	PLAYER_NUMBER = 0;
	for(i=0;i<PLAYER_NUMBER_MAX;i++)
	{
		/* Create the music player */
		res = (*EngineItf)->CreateAudioPlayer(EngineItf, &player[i], &audioSource, &audioSink, arraySize, player_ids, player_req);
		if ( res != SL_RESULT_SUCCESS )
		    break;

		cocos2d::log("%s player alloc %d", __FUNCTION__, i);

		/* Realizing the player in synchronous mode */
		res = (*player[i])->Realize(player[i], SL_BOOLEAN_FALSE);

		if ( res != SL_RESULT_SUCCESS )
		    break;

		PLAYER_NUMBER++;

		cocos2d::log("%s player realized %d", __FUNCTION__, i);
		/* Get seek and play interfaces */
		res = (*player[i])->GetInterface(player[i], SL_IID_PLAY, (void *)&playItf[i]);
		res = (*player[i])->GetInterface(player[i], SL_IID_BUFFERQUEUE, (void*)&bufferQueueItf[i]);
		res = (*player[i])->GetInterface(player[i], SL_IID_VOLUME, (void *)&volumeItf[i]);
		(*volumeItf[i])->SetVolumeLevel(volumeItf[i], 0.0f);

		/* Initialize the context for Buffer queue callbacks */
		shortPcmData[i] = (short int *) malloc(sizeof(short int) * BUFFER_SAMPLE_LENGTH * 2 );
		context[i] = (CallbackContext *) malloc(sizeof(CallbackContext));
		((CallbackContext *)context[i])->shortBuffer = (SLint16 *)shortPcmData[i];
		((CallbackContext *)context[i])->playerID = i;
		((CallbackContext *)context[i])->audioPool = this->audioPool;
		((CallbackContext *)context[i])->playItf = playItf[i];

		((CallbackContext *)context[i])->bufferQueueItf = bufferQueueItf[i];
		((CallbackContext *)context[i])->audioModule = this;

		/* Setup to receive buffer queue event callbacks */
		res = (*bufferQueueItf[i])->RegisterCallback(bufferQueueItf[i], OpenSLAudioModule_ChordMixing::BufferQueueCallback, context[i]);
		this->playerOrderQueue[i] = i;

		(*playItf[i])->SetPlayState(playItf[i], SL_PLAYSTATE_STOPPED);
		playerStateArray[i] = STOPPED;

		playerStoppingEnqueueCount[i] = 0;
	}
	cocos2d::log("%s player allocated : %d\n", __FUNCTION__, PLAYER_NUMBER);

	MRPlayerID = PLAYER_NUMBER - 1;
	this->audioPool->setMRPlayerID(MRPlayerID);
	PLAYER_NUMBER--;
	this->currentPosition = 0;

    emptyShortPcmData = (short int *) malloc(sizeof(short int *) * BUFFER_SAMPLE_LENGTH * 2);
    memset(emptyShortPcmData, 0, sizeof(short int *) * BUFFER_SAMPLE_LENGTH * 2);

    cocos2d::log("%s %d\n", __FUNCTION__, PLAYER_NUMBER);

    return PLAYER_NUMBER;
}

int OpenSLAudioModule_ChordMixing::release()
{
	SLresult res;

	for(int i=0;i<=PLAYER_NUMBER;i++)
	{
		/// Make sure player is stopped ///
		res = (*playItf[i])->SetPlayState(playItf[i], SL_PLAYSTATE_STOPPED);

		/// Destroy the player ///
		(*player[i])->Destroy(player[i]);

		free(shortPcmData[i]);
		free(context[i]);
	}

	/// Destroy Output ////
	(*OutputMix)->Destroy(OutputMix);

	free(emptyShortPcmData);
}

long OpenSLAudioModule_ChordMixing::getBufferDelayTime()
{
	if(FAST_TRACK)
		return AudioTool::samples2ms(SYSTEM_BUFFER_SAMPLE_LENGTH, SAMPLE_RATE) - 80;
	else
		return AudioTool::samples2ms(SYSTEM_BUFFER_SAMPLE_LENGTH, SAMPLE_RATE);
}

void OpenSLAudioModule_ChordMixing::BufferQueueCallback(SLBufferQueueItf queueItf, void *pContext)
{
	CallbackContext *pcontext = (CallbackContext*)pContext;
	OpenSLAudioModule_ChordMixing *audioModule = (OpenSLAudioModule_ChordMixing*) ((CallbackContext*)pContext)->audioModule;

	AudioPool *audioPool = ((CallbackContext*)pContext)->audioPool;
	SLPlayItf playItf = ((CallbackContext*)pContext)->playItf;
	SLBufferQueueItf bufferQueueItf = ((CallbackContext*)pContext)->bufferQueueItf;
	int playerID = ((CallbackContext*)pContext)->playerID;

	if(audioModule->playerStateArray[playerID] == STOPPING)
	{
		int countMax = audioModule->SYSTEM_BUFFER_SAMPLE_LENGTH / audioModule->BUFFER_SAMPLE_LENGTH;
		if(audioModule->playerStoppingEnqueueCount[playerID]>countMax*15)
		{
			(*playItf)->SetPlayState(playItf, SL_PLAYSTATE_STOPPED);
			audioModule->playerStateArray[playerID] = STOPPED;
			return;
		}
		else
		{
			(*queueItf)->Enqueue(queueItf, (void*) audioModule->emptyShortPcmData, sizeof(short int) * audioModule->BUFFER_SAMPLE_LENGTH * 2);
			audioModule->playerStoppingEnqueueCount[playerID]++;
			return;
		}
	}

	short int * shortBuffer = (short int *)(pcontext->shortBuffer);
	memset(shortBuffer, 0, sizeof(short) * audioModule->BUFFER_SAMPLE_LENGTH * 2);

	int bufferSampleLength;
	if(playerID == audioModule->MRPlayerID)
	{
		bufferSampleLength = audioPool->process(shortBuffer, nullptr, audioModule->currentPosition, playerID, audioModule->BUFFER_SAMPLE_LENGTH);
		audioModule->currentPosition += audioModule->BUFFER_SAMPLE_LENGTH;
	}
	else
		bufferSampleLength = audioPool->process(shortBuffer, nullptr, -1, playerID);

	SLresult res;
	if(bufferSampleLength > 0)
	{
		res = (*queueItf)->Enqueue(queueItf, (void*) shortBuffer, sizeof(short int) * bufferSampleLength * 2); /* Size given in bytes. */
		audioModule->playerStoppingEnqueueCount[playerID]++;
		CheckErr(__FUNCTION__, res);
	}
	else
	{

		audioModule->playerStateArray[playerID] = STOPPING;
		audioPool->resetPlayer(playerID);
		(*queueItf)->Enqueue(queueItf, (void*) audioModule->emptyShortPcmData, sizeof(short int) * audioModule->BUFFER_SAMPLE_LENGTH * 2);

	}
}



OpenSLAudioModule_ChordMixing::~OpenSLAudioModule_ChordMixing()
{
	SLresult res;
	for(int i=0;i<PLAYER_NUMBER;i++)
	{
		res = (*playItf[i])->SetPlayState(playItf[i], SL_PLAYSTATE_STOPPED);
		(*player[i])->Destroy(player[i]);
		free(shortPcmData[i]);
		free(context[i]);
	}

	(*OutputMix)->Destroy(OutputMix);
	(*slEngine)->Destroy(slEngine);
}


// for MR AUDIO
int OpenSLAudioModule_ChordMixing::readyMR(const char* path, AudioType audioType)
{
	unsigned int soundID = GetHashValue(path);
	return audioPool->readyMR(soundID, audioType);
}

int OpenSLAudioModule_ChordMixing::readyMR(NoteObject* noteObj)
{
	unsigned int soundID = GetHashValue(noteObj->m_FullPath);
	return audioPool->readyMR(soundID, noteObj);
}

int OpenSLAudioModule_ChordMixing::startMR()
{
	currentPosition = 0;
    long currentTime = AudioTool::getSystemCurrentTime();
	for(int j=0;j<POSITION_CHECK_COUNT;j++)
	{
		positionArray[j] = 0;
		timeArray[j] = currentTime;
	}

	int i = MRPlayerID;

	int bufferSampleLength = audioPool->process(shortPcmData[i], nullptr, currentPosition, i);

	(*playItf[i])->SetPlayState(playItf[i], SL_PLAYSTATE_PLAYING);
	playerStoppingEnqueueCount[i] = 0;
	(*bufferQueueItf[i])->Enqueue(bufferQueueItf[i], (void*) shortPcmData[i], sizeof(short int) * BUFFER_SAMPLE_LENGTH * 2);
	return 0;
}

int OpenSLAudioModule_ChordMixing::pauseMR()
{
	(*playItf[MRPlayerID])->SetPlayState(playItf[MRPlayerID], SL_PLAYSTATE_STOPPED);
		return 0;
}

int OpenSLAudioModule_ChordMixing::resumeMR()
{
	long currentTime = AudioTool::getSystemCurrentTime();
	for(int j=0;j<POSITION_CHECK_COUNT;j++)
	{
		positionArray[j] = currentPosition;
		timeArray[j] = currentTime;
	}

	int i = MRPlayerID;
	int bufferSampleLength = audioPool->process(shortPcmData[i], nullptr, currentPosition, i);
	(*playItf[i])->SetPlayState(playItf[i], SL_PLAYSTATE_PLAYING);
	playerStoppingEnqueueCount[i] = 0;
	(*bufferQueueItf[i])->Enqueue(bufferQueueItf[i], (void*) shortPcmData[i], sizeof(short int) * BUFFER_SAMPLE_LENGTH * 2);
	return 0;
}

int OpenSLAudioModule_ChordMixing::stopMR()
{
	(*playItf[MRPlayerID])->SetPlayState(playItf[MRPlayerID], SL_PLAYSTATE_STOPPED);
	currentPosition = 0;
	return 0;
}

// for NOTE EFFECT
int OpenSLAudioModule_ChordMixing::playEffect(const char *path, float pitch, float noteVelocity) // return Stream ID
{
	return 0;
}

int OpenSLAudioModule_ChordMixing::playEffect(NoteObject *noteObj) // return Stream ID
{
	unsigned int soundID = GetHashValue(noteObj->m_FullPath);

	if(audioPool->getSound(soundID)==nullptr)
		return -1;

	if(newPlayerNumber < 0)
	{
		int i;

		// check if exist stopped player
		for(i=PLAYER_NUMBER-1;i>=0;i--)
			if(playerStateArray[playerOrderQueue[i]] == STOPPED)
				break; // if exist stopped player


		// check if exist stopping player
		if(i<0)
		{
			for(i=PLAYER_NUMBER-1;i>=0;i--)
				if(playerStateArray[playerOrderQueue[i]] == STOPPING)
					break; // if exist stopping player

			// no stopping|stopped player
			if(i<0)
				for(i=PLAYER_NUMBER-1;i>=0;i--)
					if(playerOrderQueue[i]!=-1)
						break; //last player

			i = playerOrderQueue[i];

			audioPool->stopStreamByPlayerID(i);
			(*playItf[i])->SetPlayState(playItf[i], SL_PLAYSTATE_STOPPED);
			playerStateArray[i] = STOPPED;
		}
		else
			i = playerOrderQueue[i];

		int lastIndex;
		for(lastIndex=0;lastIndex<PLAYER_NUMBER;lastIndex++)
				if(playerOrderQueue[lastIndex] == i)
					break;

		if(lastIndex<PLAYER_NUMBER)
				for(int j=lastIndex;j>0;j--)
					playerOrderQueue[j] = playerOrderQueue[j-1];


		playerOrderQueue[0] = i;
		newPlayerNumber = i;
	}


	return audioPool->play(soundID, noteObj, newPlayerNumber);
}

int OpenSLAudioModule_ChordMixing::startEffect()
{
	int retValue;

	if(newPlayerNumber < 0)
		retValue = -1;
	else
	{
		if(playerStateArray[newPlayerNumber]==STOPPED)
		{
			int i = newPlayerNumber;
			int bufferSampleLength = audioPool->process(shortPcmData[i], nullptr, currentPosition, i);

			SLresult res;
			playerStateArray[i] = PLAYING;

            (*bufferQueueItf[i])->Clear(bufferQueueItf[i]);
			(*playItf[i])->SetPlayState(playItf[i], SL_PLAYSTATE_PLAYING);
			playerStoppingEnqueueCount[i] = 0;
			res = (*bufferQueueItf[i])->Enqueue(bufferQueueItf[i], (void*) shortPcmData[i], sizeof(short int) * BUFFER_SAMPLE_LENGTH * 2);

			/// find stopCount Value by calculating play delta time
			long currentTime = this->getMRCurrentTime();
			int effectPlayTimeSize = effectTimeList.size();
			int startIndex = 0;

			for(int n=0;n<effectPlayTimeSize;n++)
				if(effectTimeList[n] > currentTime)
				{
					startIndex = n;
					break;
				}
			const int N = 20;

			int averageSize = N < effectPlayTimeSize - startIndex - 1 ? N : effectPlayTimeSize - startIndex - 1;
			int average = 0;

			for(int n=startIndex;n<startIndex+averageSize;n++)
				average += (effectTimeList[n+1] - effectTimeList[n]);

			if(averageSize == 0)
				average = effectTimeList[effectPlayTimeSize-1] - effectTimeList[effectPlayTimeSize-2];
			else
				average /= averageSize;

			int stopCountCondition = -0.053f * average + 16.0f;
			float fadeStopTime = 0.0005f * average;
			if(fadeStopTime < 0.1f)
				fadeStopTime = 0.1f;
			int stopCount = 0;
			for(int i=0;i<PLAYER_NUMBER;i++)
				if(playerStateArray[i] == STOPPED || playerStateArray[i] == STOPPING)
					stopCount++;

			if(stopCount < stopCountCondition)
				for(int j=PLAYER_NUMBER-1;j>=0;j--)
				{
					player_state state = playerStateArray[playerOrderQueue[j]];
					/*
					if(state == STOPPING)
					{
						(*playItf[playerOrderQueue[j]])->SetPlayState(playItf[playerOrderQueue[j]], SL_PLAYSTATE_STOPPED);
						playerStateArray[playerOrderQueue[j]] = STOPPED;
						break;
					}
					else*/ if(state == PLAYING)
					{
						audioPool->fadeStopStreamByPlayerID(playerOrderQueue[j], fadeStopTime);
						break;
					}

				}


		}
		newPlayerNumber = -1;
		retValue = 0;
	}

	return retValue;
}

