#ifndef OpenSLAudioModule_ChordMixing_H_
#define OpenSLAudioModule_ChordMixing_H_

/*

 * OpenSLAudioModule_ChordMixing.h
 *
 *  Created on: 2015. 5. 18.
 *      Author: IZSoft-PSC
 */
#include <assert.h>
#include <jni.h>
#include <string.h>

// for native audio
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

// for native asset manager
#include <sys/types.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include <pthread.h>
#include <vector>
#include <map>
#include <stdio.h>
#include <unistd.h>

#include "OpenSLAudioModule.h"
#include "../AudioCommon/Sound.h"
#include "../AudioCommon/Stream.h"
#include "../AudioCommon/AudioPool.h"

#include <sched.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/time.h>

typedef enum {
	PLAYING = 1,
	STOPPING = 2,
	STOPPED = 3
} player_state;

#define SYSTEM_PLAYER_NUMBER_MAX 32
#define PLAY_TIME_NUMBER 10

class OpenSLAudioModule_ChordMixing : public AudioModule {
public:

	OpenSLAudioModule_ChordMixing(int _systemSampleRate, int streamSize, int bufferSampleLength, int systemBufferSampleLength, bool fastTrack);
	virtual ~OpenSLAudioModule_ChordMixing();

	virtual int init(int playNumberMax);
	virtual int release();
	long getBufferDelayTime();

	// for MR AUDIO
	virtual int readyMR(const char* path, AudioType audioType);
	virtual int readyMR(NoteObject* noteObj);

	virtual int startMR();
	virtual int pauseMR();
	virtual int resumeMR();
 	virtual int stopMR();

 	// for NOTE EFFECT
	virtual int playEffect(const char *path, float pitch, float noteVelocity); // return Stream ID
	virtual int playEffect(NoteObject *noteObj); // return Stream ID

	virtual int startEffect();

	/* Callback for Buffer Queue events */
	static void BufferQueueCallback(SLBufferQueueItf queueItf, void *pContext);
	int playerOrderQueue[32];
	player_state playerStateArray[SYSTEM_PLAYER_NUMBER_MAX];
	int playerStoppingEnqueueCount[SYSTEM_PLAYER_NUMBER_MAX];
	int newPlayerNumber = -1;

	short int *emptyShortPcmData;

	int MRPlayerID;
	int PLAYER_NUMBER_MAX;

private:
	int PLAYER_NUMBER;
	short int *shortPcmData[SYSTEM_PLAYER_NUMBER_MAX];

    SLObjectItf slEngine;
    SLObjectItf player[SYSTEM_PLAYER_NUMBER_MAX];
    SLObjectItf OutputMix;

    SLEngineItf EngineItf;
    SLVolumeItf volumeItf[SYSTEM_PLAYER_NUMBER_MAX];
	SLPlayItf playItf[SYSTEM_PLAYER_NUMBER_MAX];
	SLBufferQueueItf bufferQueueItf[SYSTEM_PLAYER_NUMBER_MAX];

	SLDataSource audioSource;
	SLDataLocator_BufferQueue bufferQueue;
	SLDataLocator_Address addressQueue;
	SLDataFormat_PCM pcm;
	SLDataSink audioSink;
	SLDataLocator_OutputMix locator_outputmix;

	SLBufferQueueState state;
	CallbackContext *context[SYSTEM_PLAYER_NUMBER_MAX];

	AAssetManager *assetManager;
	bool FAST_TRACK;

};

#endif /* OpenSLAudioModule_ChordMixing_H_ */
