#ifndef Header_AudioPool_Sound
#define Header_AudioPool_Sound

#include "../Superpowered/SuperpoweredDecoder.h"
#include "../Superpowered/SuperpoweredSimple.h"
#include "AudioTool.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "cocos2d.h"

#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
    #include <cpu-features.h>
    #include <arm_neon.h>
#endif

#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
#include <TargetConditionals.h>
#endif


#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS) && !(TARGET_IPHONE_SIMULATOR)
    #include <arm_neon.h>
#endif



class Sound
{
	public:
		Sound(const char *path, int offset, int length, int wantSampleRate); //if length == 0, read entire file.
		Sound(short int *buffer, long bufferLength, int oriSampleRate, int _wantSampleRate);
		~Sound();
		double read(float *buffer, double position, int length, float leftVolume, float rightVolume, float leftVolumeDelta, float rightVolumeDelta, float pitchRate); //write float buffer and return the samples be red
		long read(float *buffer, long position, int length, float leftVolume, float rightVolume, float leftVolumeDelta, float rightVolumeDelta); //write float buffer and return the samples be red

		float *getFloatBuffer();
		int getDurationSamples();

	private:
		SuperpoweredDecoder *decoder;
		float *floatBuffer;
		int durationSamples; // one sample = one short = two bytes
		void decode();
		int wantSampleRate;
};

#endif
