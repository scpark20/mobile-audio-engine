#include "WaveAnalysis.h"

static int logi(int base, int x) { //integer log
	int ret = 1;
    for(int i=0;i<logf(x)/logf(base)+1;i++)
    {
    	if(ret==x)
    	    return i;
    	ret *= base;
    }
    return -1;
}

WaveAnalysis::WaveAnalysis(bool stereo, int sampleRate, int FFTSize) : durationOfSamples(0)
{
	this->stereo = stereo;
	this->sampleRate = sampleRate;
	this->FFTSize = FFTSize;

	/* allocation */
	magBufferList = new std::vector<float *>;

	/* windowing for FFT */
	window = (float *) malloc(sizeof(float) * FFTSize);
	putHanningWindow(window, FFTSize);

	/*set index bound */
	lowerBoundIndex = (float) FFTSize * ((float) OBSERVATION_LOWER_BOUND / (float) sampleRate); // lowerBound : in Hz -> in ArrayIndex
	upperBoundIndex = (float) FFTSize * ((float) OBSERVATION_UPPER_BOUND / (float) sampleRate); // upperBound : in Hz -> in ArrayIndex

	LOG2LOWER = logf(lowerBoundIndex) / logf(2);
	LOG2UPPER = logf(upperBoundIndex) / logf(2);

	//LOGV("%d %d %f %f", lowerBoundIndex, upperBoundIndex, LOG2LOWER, LOG2UPPER);
	/*init filter */
	lpf = new SuperpoweredFilter(SuperpoweredFilter_Resonant_Lowpass, 44100);
	lpf->setResonantParameters(100, 0.1);

	logSize = logi(2, FFTSize);
	//SuperpoweredFFTPrepare(logSize, true);
}

WaveAnalysis::~WaveAnalysis()
{
	/* dealloc */
	std::vector<float *>::iterator it;
	for(it = magBufferList->begin(); it!=magBufferList->end();it++)
		free(*it);
	delete(lpf);
	free(window);
	//SuperpoweredFFTCleanup(); //why this makes error???
}



int WaveAnalysis::putShortBuffer(const short int *short_buffer, int samplesPerBuffer)
{
	this->durationOfSamples += samplesPerBuffer;

	/* allocate buffer */
	float *mag_buf = (float *) malloc(sizeof(float) * FFTSize); // FFTSize should greater than or equal to samplesPerBuffer!!!!!!!!
	float *phase_buf = (float *) malloc(sizeof(float) * FFTSize);

	/* copy buffer */
	memset((void *)mag_buf, 0, sizeof(float) * FFTSize);
	memset((void *)phase_buf, 0, sizeof(float) * FFTSize);
	if(!stereo)
		for(int i=0;i<samplesPerBuffer;i++)
			mag_buf[i] = (float)short_buffer[i] / 32768.0f;
	else
		for(int i=0;i<samplesPerBuffer;i++)
			mag_buf[i] = ((float)short_buffer[i*2] + (float)short_buffer[i*2+1]) / 65536.0f;

	/*if amplitude too low, DO NOT Analysis */
	float ampSum = 0;

	for(int i=0;i<FFTSize;i++)
		ampSum += fabs(mag_buf[i]);


	if(ampSum < 0.02 * FFTSize)
    {
		if(magBufferList->size()==0)
        {
			memset(mag_buf, 0, sizeof(float) * FFTSize);
        }
		else
		{
			memcpy(mag_buf, magBufferList->back(), sizeof(float) * FFTSize);
			free(phase_buf);
			return 0;
		}
    }

	/* windowing */

	for(int i=0;i<FFTSize;i++)
		mag_buf[i] *= window[i];

	/* DO FFT */
	SuperpoweredFFTReal(mag_buf, phase_buf, logSize, true);
	//SuperpoweredPolarFFT(mag_buf, phase_buf, logSize, true);

	for (int i=0;i<FFTSize;i++)
		mag_buf[i] = sqrt(mag_buf[i] * mag_buf[i] + phase_buf[i] * phase_buf[i]);

	/* DO HPS http://cnx.org/contents/8b900091-908f-42ad-b93d-806415434b46@2/Pitch_Detection_Algorithms */
	for (int i=lowerBoundIndex;i<upperBoundIndex;i++)
	{
		for(int j=1;j<20;j++) // add from second harmonics to 20th harmonics
			if(i*j < FFTSize / 2)
				mag_buf[i] += mag_buf[i*j];
	}

	/* remain MAX magnitude frequency's value */
	int maxValueIndex = lowerBoundIndex;

	for(int i=lowerBoundIndex+1;i<upperBoundIndex;i++)
	{
		if(mag_buf[i] > mag_buf[maxValueIndex])
		{
			mag_buf[maxValueIndex] = 0;
			maxValueIndex = i;
		}
		else
			mag_buf[i] = 0;
	}

	magBufferList->push_back(mag_buf);

	/* dealloc */

	free(phase_buf);
    
    return 0;
}

int WaveAnalysis::putMelodyLineArray(int wantLength, float *melodyLineArray)
{
	LPF *lpf = new LPF(1, 8, 0.05);

    for(int i=0;i<wantLength;i++)
	{
		int listIndex = magBufferList->size() * i / wantLength;
		float *buf = magBufferList->at(listIndex);
		for(int j=lowerBoundIndex;j<upperBoundIndex;j++)
		{
			if(buf[j]!=0.0f)
			{
				melodyLineArray[i] = lpf->getOutput(((logf(j) / logf(2)) - LOG2LOWER) / (LOG2UPPER - LOG2LOWER));
				//melodyLineArray[i] = lpf->getOutput((float)j/(float)upperBoundIndex);
			}
		}
	}

    return 0;
	//lpf->process(melodyLineArray, melodyLineArray, wantLength);
}

int WaveAnalysis::putMelodyLineArrayFromFile(const char *filepath, bool isStereo, int FFTSize, int wantLength, float *melodyLineArray)
{
	/* instanciate decoder */
    SuperpoweredDecoder *decoder;
    decoder = new SuperpoweredDecoder();
    
    const char *openError = decoder->open(filepath, false, 0, 0);
	if(openError != NULL) { delete decoder; return -1;}

	int durationSamples = decoder->durationSamples;
	if(!(durationSamples>0)) { delete decoder; return -1;}

	/* set buffer */
	int samplesPerFrame = decoder->samplesPerFrame;
	int durationByteLength = durationSamples * sizeof(short int) * 2;
	short int *shortBuffer = (short int *) malloc(durationByteLength + 16384); // superpowered recommend that "+ 16384"
	unsigned int offset = 0;

	/* decode */
	while(true)
	{
		unsigned int decodedSamples = samplesPerFrame;
		int result = decoder->decode(shortBuffer + offset, &decodedSamples);
		offset += decodedSamples * (isStereo ? 2 : 1); // * 2 because has two channels
		if(result != SUPERPOWEREDDECODER_OK) break;
	}

	/* put short buffer to waveAnalysis */
	offset = 0;
	unsigned int remainingSamples;
	unsigned int samplesPerBuffer;
	WaveAnalysis *waveAnalysis = new WaveAnalysis(isStereo, decoder->samplerate, FFTSize);
	while(offset<durationSamples)
	{
		remainingSamples = durationSamples - offset;
		if(remainingSamples / FFTSize >= 1)
			samplesPerBuffer = FFTSize;
		else
			samplesPerBuffer = remainingSamples;
		waveAnalysis->putShortBuffer(shortBuffer + (isStereo ? offset * 2 : offset), samplesPerBuffer);
		offset += samplesPerBuffer;
	}
//	LOGV("magBufferListSize %d", waveAnalysis->magBufferList->size());
	waveAnalysis->putMelodyLineArray(wantLength, melodyLineArray);

	/* dealloc */
	free(shortBuffer);
	delete decoder;
	delete waveAnalysis;

    return 0;
}

int WaveAnalysis::putHanningWindow(float *windowBuffer, int length)
{
	for (int n = 0; n < length; n++)
		windowBuffer[n] = (float)(0.5 * (1 - cos(2 * M_PI*n / (length - 1))));
	return 0;
}

int WaveAnalysis::putWaveformFromFile(const char *filepath, bool isStereo, int wantLength, float *waveformArray)
{
	/* instanciate decoder */
    SuperpoweredDecoder *decoder;
    decoder = new SuperpoweredDecoder();
    
	const char *openError = decoder->open(filepath, false, 0, 0);
	if(openError != NULL) { delete decoder; return -1;}

	int durationSamples = decoder->durationSamples;
	if(!(durationSamples>0)) { delete decoder; return -1;}

	/* set buffer */
	int samplesPerFrame = decoder->samplesPerFrame;
	int durationByteLength = durationSamples * sizeof(short int) * 2;
	short int *shortBuffer = (short int *) malloc(durationByteLength + 16384); // superpowered recommend that "+ 16384"
	unsigned int offset = 0;

	/* decode */
	while(true)
	{
		unsigned int decodedSamples = samplesPerFrame;
		int result = decoder->decode(shortBuffer + offset, &decodedSamples);
		offset += decodedSamples * (isStereo ? 2 : 1); // * 2 because has two channels
		if(result != SUPERPOWEREDDECODER_OK) break;
	}

    return 0;
}
