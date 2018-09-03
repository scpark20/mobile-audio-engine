/*
 * Stream.h
 *
 *  Created on: 2015. 5. 15.
 *      Author: IZSoft-PSC
 */
#ifndef Header_AudioPool_Stream
#define Header_AudioPool_Stream

#include "Sound.h"
#include "AudioPool.h"
#include "AudioTool.h"
#include "assert.h"

#include <atomic>

typedef enum {
	STREAM_STATE_PLAYING = 0,
	STREAM_STATE_PAUSED = 1,
	STREAM_STATE_END = 2,
	STREAM_STATE_FADEOUT = 3
} StreamState;

class Stream {
public:
	Stream(Sound *sound, int streamID, int sampleRate, int bufferSampleLength);

	~Stream();
	unsigned int process(float *float_buffer, int numberOfSamples);
	unsigned int process(float *destFloatBuffer, int numberOfSamples, long currentSamplePosition);
	int setSound(Sound *sound);
    int releaseSound();

	int setState(StreamState state);
	int getState();

	int setType(int type) {this->type = type;}
	int getType() {return this->type;}

	int setStreamID(int streamID) {this->streamID = streamID;}
	int getStreamID();
	int setPitchRate(float pitchRate) {this->pitchRate = pitchRate;}
	int setVolume(float vol);
	int setVolumeGradually(float volume, float deltaTime);
	float getVolume();
	void setPan(float pan);
	Sound *getSound();
	long getStartTime();
	Stream *prevStream;
	Stream *nextStream;
	int getPlayerID() {return playerID;}
	int setPlayerID(int playerID) {this->playerID = playerID;}
	int setPitchRateEnable(bool pitchRateEnable) {this->pitchRateEnable = pitchRateEnable;}

	long getCurrentTime() {return position / (double) sampleRate * 1000.0f;}
	int setCurrentTime(double time) {this->position = time * (double)sampleRate / 1000.0f;}
	long getDurationTime() {return sound->getDurationSamples() / (double) sampleRate * 1000.0f;}

	int setPositionInTime(long time);
	long getPositionInTime();

	int setPlayPositionInTime(long time);
	long getPlayPositionInTime();

	int setPlayPositionInSample(long sample);
	long getPlayPositionInSample();

	int setPlayDurationInTime(long time);
	long getPlayDurationInTime();

	int setPlayDurationInSample(long sample);
	long getPlayDurationInSample();

	int setReleaseDurationInTime(long time);
	long getReleaseDurationInTime();

	int setReleaseDurationInSample(long sample);
	long getReleaseDurationInSample();
	
private:
	bool END_READY;
	int bufferSampleLength;
	float volume;
	float leftVolumeDelta;
	float rightVolumeDelta;
	std::atomic<StreamState> state;
	int type;

	double position;
	double prevPosition;

	long playSamplePosition;
	long playSampleDuration;
	long releaseSampleDuration;

	long prevTime;

	int streamID;
	int sampleRate;
	float pitchRate = 1.0f;
	Sound *sound;

	int playerID = -1;
	float pan = 0.5f;
	float leftPan = 1.0f;
	float rightPan = 1.0f;
	float leftVolume;
	float rightVolume;

	bool pitchRateEnable = false;
	float *floatBuffer;
};

#endif /* STREAM_H_ */
