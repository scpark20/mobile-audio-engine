#ifndef AudioModule_H_
#define AudioModule_H_

#include <NoteObject.h>
#include "AudioTool.h"
#include "AudioPool.h"
#include "../IZAudioEngine.h"

enum AudioState {
	AUDIO_STATE_PLAYING = 0,
	AUDIO_STATE_PAUSED = 1,
	AUDIO_STATE_END = 2
};

#define MRStreamID -1
#define VoiceStreamID -2
#define otherInstStartID -3

#define POSITION_CHECK_COUNT 10


class AudioModule {
public:

    AudioModule(int sampleRate, int bufferSampleLength, int systemBufferSampleLength, int streamSize);
    virtual ~AudioModule();

    virtual int init(int playerNumberMax) = 0;
    virtual int release() = 0;

    long getBufferDelayTime();
    void setVolumes(float MRVolume, float otherVolume, float myVolume);

	// for MR AUDIO
	virtual int readyMR(const char* path, AudioType audioType) = 0;
	virtual int readyMR(NoteObject* noteObj) = 0;

	virtual int startMR() = 0;
	virtual int pauseMR() = 0;
	virtual int resumeMR() = 0;
 	virtual int stopMR() = 0;
    int releaseMR();

	long getMRCurrentTime();
	int setMRCurrentTime(long time);

	long getMRDuration();

	// for NOTE EFFECT

	int markNotePlayTime(long time);
	int unmarkNotePlayTimeAll();

	virtual int playEffect(const char *path, float pitch, float noteVelocity) = 0; // return Stream ID
	virtual int playEffect(NoteObject *noteObj) = 0; // return Stream ID

	virtual int startEffect() = 0;

    void fadeStopEffect(int streamID, float time);
	int stopEffect(int streamID);
	int stopAllEffects();

	float getEffectVolume(int streamID);
	int setEffectVolume(int streamID, float noteVelocity);
	int setEffectVolumeAbsolutely(int streamID, float volume);

	// for COMMON
	int load(const char *, int offset, int length); // return sound ID
	int unload(const char *);

	unsigned int GetHashValue(const char *key);
    
    AudioPool *audioPool;
    
    int SAMPLE_RATE;
    int SYSTEM_BUFFER_SAMPLE_LENGTH;
    int BUFFER_SAMPLE_LENGTH;
    int STREAM_SIZE;
    
    // for MR Play
    AudioState audioState;
    long currentPosition;
    
    long positionArray[POSITION_CHECK_COUNT] = {0, };
    long timeArray[POSITION_CHECK_COUNT] = {0, };
    
    std::vector<long int> effectTimeList;

protected:


};

#endif
