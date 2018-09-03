#ifndef AUDIOCOMMON_AUDIOPOOL_H_
#define AUDIOCOMMON_AUDIOPOOL_H_

#include <map>
#include <vector>
#include <atomic>
#include "NoteObject.h"
#include "cocos2d.h"
#include "Stream.h"

#include "../IZAudioEngine.h"
#include "../Plugin/Reverb.h"
#include "../Plugin/Limiter.h"
#include "../Plugin/Compressor.h"
#include "../Plugin/PhaseVocoder.h"
#include "../Plugin/ReSampler.h"

#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    #include <arm_neon.h>
    #include <cpu-features.h>
#endif

#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
#include <TargetConditionals.h>
#endif 

#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS) && !(TARGET_IPHONE_SIMULATOR)
#include <arm_neon.h>
#endif

#define MRStreamID -1
#define VoiceStreamID -2
#define otherInstStartID -3

class Stream;

class AudioPool {

public:
	AudioPool(int systemSampleRate, int playerNumber, int stream_size, int bufferSampleLength, float volumeFactor = 1.0f);
	~AudioPool();

	int load(unsigned int soundID, Sound *sound);
	int unload(unsigned int soundID);
	Sound *getSound(unsigned int soundID);

	// for MR
	int readyMR(int soundID, AudioType audioType);
	int readyMR(int soundID, NoteObject *noteObj);
	int pauseMR();
	int resumeMR();
	int releaseMR();
	long getStreamDuration(int streamID);

	void setMRPlayerID(int MRPlayerID);
	int getMRPlayerID();

	//for Effect
	int play(int soundID, float pitch, float noteVelocity, float pan = 0.5f, int playerID = 0); // return streamID
	int play(int soundID, NoteObject *noteObj, int playerID = 0);

	int stopStream(int streamID);
	void fadeStopStream(int streamID, float time/*in second*/);
	void fadeStopStreamByPlayerID(int playerID, float time/*in second*/);
	void stopStreamByPlayerID(int playerID);

	int stopAllStream();
	int resetPlayer(int playerID);

	int setStreamVolume(int streamID, float noteVelocity);
	float getStreamVolume(int streamID);

	void setVolumes(float MRVolume, float otherVolume, float myVolume);

	int process(short *shortBuffer, float *floatBuffer = nullptr, long currentSamplePosition = -1, int playerID = 0, int bufferSampleLength = -1);
	int setStreamVolumeAbsolutely(int streamID, float volume);

private:

	int PLAYER_NUMBER;
	float **floatPcmData;

	std::map<unsigned int, Sound*> soundMap;
	pthread_mutex_t soundArray_mutex;

	Reverb *reverb;
	Limiter *limiter;
	Compressor **compressor;
	PhaseVocoder *phaseVocoder;
	ReSampler *reSampler;

	std::map<int, Stream*> streamMap;
	Stream *headStream;
	Stream *tailStream;
	pthread_mutex_t streamLink_mutex;

	int otherInstEndID;
	int MRPlayerID = 0;

	int SYSTEM_SAMPLE_RATE;
	int STREAM_SIZE;
	int BUFFER_SAMPLE_LENGTH;

	float volumeFactor;
	float timeRate = 1.5f;

	float MR_VOLUME = 0.0f;
	float OTHER_VOLUME = 0.0f;
	float MY_VOLUME = 0.0f;
};

#endif
