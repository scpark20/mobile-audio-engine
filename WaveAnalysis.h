/* written by scpark 2015.05.27 */

#ifndef WAVETOOL_WAVEANALYSIS_H_
#define WAVETOOL_WAVEANALYSIS_H_

#include <assert.h>
#include <stdlib.h>
#include <string>
#include <math.h>
#include <vector>
#include "LPF.h"
#include "cocos2d.h"
#if(CC_TARGET_PLATFORM != CC_PLATFORM_WIN32)
#include "SuperpoweredFFT.h"
#include "SuperpoweredDecoder.h"
#include "SuperpoweredFilter.h"
#include "SuperpoweredSimple.h"
#endif

#define OBSERVATION_LOWER_BOUND 100 //Hz
#define OBSERVATION_UPPER_BOUND 2000 //Hz

class WaveAnalysis {
public:
	WaveAnalysis(bool stereo, int sampleRate, int FFTSize);
	~WaveAnalysis();
	static int putMelodyLineArrayFromFile(const char *filepath, bool isStereo, int FFTSize, int wantLength, float *melodyLineArray); //FFTSize 8192 recommended
	static int putWaveformFromFile(const char *filepath, bool isStereo, int wantLength, float *waveformArray);

	int putShortBuffer( const short int *short_buffer, int samplesPerBuffer);

	int putMelodyLineArray(int wantLength, float *melodyLineArray);


private:
	int putHanningWindow(float *windowBuffer, int length);
	int sampleRate;
	int FFTSize;
	int logSize;
	int lowerBoundIndex;
	int upperBoundIndex;
	float LOG2UPPER;
	float LOG2LOWER;
	unsigned int durationOfSamples;
	bool stereo;
	float *window;
	std::vector<float *> *magBufferList;
#if(CC_TARGET_PLATFORM != CC_PLATFORM_WIN32)
	SuperpoweredFilter *lpf;
	SuperpoweredFilter *hpf;
#endif
};
#endif
