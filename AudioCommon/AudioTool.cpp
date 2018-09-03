#include "AudioTool.h"

// y = 0.0000381788 x^2 + 0.00302531 x
const float AudioTool::vel2FloatScaleTable[128] = {0.0, 0.003063489, 0.0062033352, 0.00941954, 0.012712101, 0.01608102, 0.019526297, 0.023047931, 0.026645925, 0.030320274, 0.03407098, 0.03789805, 0.041801468, 0.045781247, 0.049837388, 0.053969882, 0.058178734, 0.062463943, 0.06682552, 0.07126344, 0.075777724, 0.08036836, 0.08503537, 0.08977872, 0.094598435, 0.0994945, 0.10446693, 0.10951572, 0.11464086, 0.119842365, 0.12512022, 0.13047445, 0.13590501, 0.14141195, 0.14699523, 0.15265489, 0.1583909, 0.16420326, 0.17009197, 0.17605706, 0.18209848, 0.18821627, 0.19441043, 0.20068094, 0.20702782, 0.21345103, 0.21995062, 0.22652656, 0.23317885, 0.2399075, 0.2467125, 0.25359386, 0.2605516, 0.2675857, 0.27469614, 0.28188294, 0.2891461, 0.2964856, 0.30390146, 0.31139368, 0.3189623, 0.32660723, 0.33432853, 0.3421262, 0.3500002, 0.3579506, 0.36597732, 0.37408042, 0.38225985, 0.39051569, 0.39884782, 0.40725636, 0.41574124, 0.42430246, 0.43294007, 0.44165403, 0.45044434, 0.459311, 0.46825403, 0.4772734, 0.48636913, 0.49554121, 0.5047897, 0.5141145, 0.5235157, 0.5329932, 0.5425471, 0.5521773, 0.5618839, 0.5716669, 0.5815262, 0.5914619, 0.6014739, 0.61156225, 0.62172705, 0.63196814, 0.6422856, 0.65267944, 0.6631496, 0.67369616, 0.684319, 0.6950183, 0.70579386, 0.71664584, 0.7275741, 0.73857886, 0.7496599, 0.7608173, 0.77205104, 0.78336114, 0.7947476, 0.8062104, 0.8177496, 0.82936513, 0.84105706, 0.8528253, 0.8646699, 0.8765909, 0.8885882, 0.90066195, 0.912812, 0.92503834, 0.9373411, 0.94972026, 0.9621757, 0.97470754, 0.9873158, 1.0000002};
//dB폭 30dB로 잡았을 때 //Ref. : http://www.dr-lex.be/info-stuff/volumecontrols.html#table1
const float AudioTool::userVolScaleTable[101] = {0.000, 0.032731198, 0.03388144, 0.03507211, 0.03630462, 0.037580445, 0.038901106, 0.040268175, 0.041683286, 0.04314813, 0.044664446, 0.046234053, 0.04785882, 0.049540684, 0.051281653, 0.053083804, 0.054949284, 0.05688032, 0.058879223, 0.060948364, 0.06309023, 0.06530735, 0.067602396, 0.069978096, 0.07243727, 0.07498288, 0.07761794, 0.08034561, 0.08316913, 0.08609187, 0.08911733, 0.09224911, 0.09549094, 0.09884671, 0.102320395, 0.10591616, 0.10963829, 0.113491215, 0.11747954, 0.12160803, 0.12588161, 0.13030536, 0.13488457, 0.13962471, 0.14453143, 0.14961058, 0.15486823, 0.16031064, 0.16594431, 0.17177595, 0.17781253, 0.18406126, 0.19052956, 0.19722518, 0.20415615, 0.21133062, 0.21875724, 0.22644484, 0.23440261, 0.24264002, 0.25116697, 0.25999352, 0.26913026, 0.27858806, 0.28837824, 0.2985125, 0.3090029, 0.31986195, 0.33110258, 0.34273824, 0.35478282, 0.36725065, 0.3801567, 0.3935162, 0.40734524, 0.4216602, 0.4364783, 0.45181707, 0.46769488, 0.4841308, 0.5011442, 0.5187555, 0.5369857, 0.5558565, 0.5753905, 0.5956111, 0.61654216, 0.63820875, 0.6606368, 0.683853, 0.7078851, 0.7327618, 0.7585127, 0.7851684, 0.81276095, 0.84132314, 0.870889, 0.90149415, 0.93317455, 0.9659684, 1.000};

 void AudioTool::float2short(float *floatbuffer, short *shortbuffer, unsigned int length)
{
	for(int i=0;i<length;i++)
	{
		int value = floatbuffer[i] * 32768.0f;
		if(value>32767)
			value = 32767;
		if(value<-32768)
			value = -32768;
		shortbuffer[i] = value;
	}
}

 void AudioTool::short2int(short *shortbuffer, int *intbuffer, unsigned int length, float maxAmplitude)
{
	for(int i=0;i<length;i++)
		intbuffer[i] = shortbuffer[i] * INT_MAXAMP / SHORT_MAXAMP;
}

 int AudioTool::getNeedLength2resample(int oriLength, int oriSampleRate, int dstSampleRate)
{
	return ceil((float)oriLength * (float)dstSampleRate / (float)oriSampleRate);
}

 float AudioTool::getResamplingIndex(int oriIndex, int oriSampleRate, int dstSampleRate)
{
	return (float)oriIndex * (float)dstSampleRate / (float)oriSampleRate;
}



 void AudioTool::resample(float *oriBuffer, int oriSampleLength, int oriSampleRate, float *dstBuffer, int dstSampleLength, int dstSampleRate)
{
	memset(dstBuffer, 0, sizeof(float) * dstSampleLength * 2);
	for(int i=0;i<dstSampleLength;i++)
	{
		float resamplingIndex = getResamplingIndex(i, dstSampleRate, oriSampleRate);
		dstBuffer[i*2] = getSampleByFloatIndex(oriBuffer, oriSampleLength, resamplingIndex, true);
		dstBuffer[i*2+1] =  getSampleByFloatIndex(oriBuffer, oriSampleLength, resamplingIndex, false);
	}
}


 float AudioTool::getSampleByFloatIndex(float *buffer, int bufferSampleLength, double sampleIndex, bool left)
{
	int prevSampleIndex = sampleIndex;
    int nextSampleIndex = prevSampleIndex + 1;

    if(prevSampleIndex >= bufferSampleLength - 1)
        return buffer[sampleIndex2bufferIndex(bufferSampleLength - 1, left)];

    float prevValue = buffer[sampleIndex2bufferIndex(prevSampleIndex, left)];
    float nextValue = buffer[sampleIndex2bufferIndex(nextSampleIndex, left)];

    float valueDifference = nextValue - prevValue;
	float sampleIndexDelta = sampleIndex - (float)prevSampleIndex;
	float valueDelta = valueDifference * sampleIndexDelta;
	float retValue = prevValue + valueDelta;

	return retValue;
}

int AudioTool::getSampleByFloatIndex2(float *buffer, int bufferSampleLength, double sampleIndex, float *leftValue, float *rightValue)
{
	int prevSampleIndex = sampleIndex;
    int nextSampleIndex = prevSampleIndex + 1;

    if(prevSampleIndex >= bufferSampleLength - 1)
    {
        *leftValue = buffer[bufferSampleLength*2];
        *rightValue = buffer[bufferSampleLength*2+1];
    }

    float leftPrevValue = buffer[prevSampleIndex*2];
    float rightPrevValue = buffer[prevSampleIndex*2+1];

    float leftNextValue = buffer[nextSampleIndex*2];
    float rightNextValue = buffer[nextSampleIndex*2+1];

    float leftValueDifference = leftNextValue - leftPrevValue;
    float rightValueDifference = rightNextValue - rightPrevValue;

	float sampleIndexDelta = sampleIndex - (float)prevSampleIndex;
	float leftValueDelta = leftValueDifference * sampleIndexDelta;
	float rightValueDelta = rightValueDifference * sampleIndexDelta;

	*leftValue = leftPrevValue + leftValueDelta;
	*rightValue = rightPrevValue + rightValueDelta;

	return 0;
}

 inline int AudioTool::sampleIndex2bufferIndex(int sampleIndex, bool left)
{
	return left ? sampleIndex * 2 : sampleIndex * 2 + 1;
}

 float AudioTool::vel2FloatScale(int velocity)
{
	if(velocity<0) velocity = 0;
	if(velocity>127) velocity = 127;

	/* 사용공식
	if(velocity == 0) return 0;
	float ret = (0.0379257F * powf(velocity, 1.5981F) + 12.7F) / 100.0F;
	if(ret>1.0) ret = 1.0F;
	*/

	return vel2FloatScaleTable[velocity];
}

 float AudioTool::userVolumeScale(float volume)
{

	int volIndex = volume;
	if(volIndex < 0) volIndex = 0;
	if(volIndex > 100) volIndex = 100;

	/* 사용공식
	float ret = 0.001F * powf(M_E, 6.908F * volume); //Dynamic Range 60dB
     */
	return userVolScaleTable[volIndex];
	

//	return powf(volume / 100.0f, 1.5f);
}

 int AudioTool::getRingBufferPosition(int position, int bufferLength)
{
	if(position >= 0)
		return position % bufferLength;
	else
		return bufferLength - (-position % bufferLength);
}

double AudioTool::getRingBufferFloatPosition(double position, int bufferLength)
{
	if(position >= 0)
		return fmod(position, bufferLength);
	else
		return bufferLength - fmod(-position, bufferLength);
}

 long AudioTool::getSystemCurrentTime()
{
	struct timeval tv;
	gettimeofday (&tv, NULL);
	return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

unsigned int AudioTool::getHashValue(const char *key)
{
    size_t len = strlen(key);
    const char *end = key + len;
    unsigned int hash;
    
    for (hash = 0; key < end; key++)
    {
        hash *= 16777619;
        hash ^= (unsigned int) (unsigned char) toupper(*key);
    }
    return (hash);
}



