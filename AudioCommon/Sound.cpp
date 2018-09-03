#include "Sound.h"


using namespace std;

Sound::Sound(const char *path, int offset, int length, int _wantSampleRate) : wantSampleRate(_wantSampleRate)
{
	floatBuffer = nullptr;
    decoder = new SuperpoweredDecoder();
  
	const char *openError = decoder->open(path);

	if (openError != NULL)
	{
        delete decoder;
        cocos2d::log("%s CANNOT OPEN %s", __FUNCTION__, openError);
		return ;
	}

	decode();
	delete decoder;
}

Sound::Sound(short int *buffer, long bufferLength, int oriSampleRate, int _wantSampleRate) : wantSampleRate(_wantSampleRate)
{

	durationSamples = bufferLength / 2;

    float *float_temp_buffer = (float *) malloc(sizeof(float) * bufferLength);


	for(int i=0;i<durationSamples;i++)
		float_temp_buffer[i] = (float)buffer[i] / 32768.0f;


	int needSampleLength = AudioTool::getNeedLength2resample(durationSamples, oriSampleRate, wantSampleRate);
	floatBuffer = (float *) malloc(sizeof(float) * needSampleLength * 2);

	AudioTool::resample(float_temp_buffer, durationSamples, oriSampleRate, floatBuffer, needSampleLength, wantSampleRate);
	durationSamples = needSampleLength;

	free(float_temp_buffer);

}

Sound::~Sound()
{
	if(floatBuffer!=nullptr)
		free(floatBuffer);
}

double Sound::read(float *buffer, double position, int numberOfSamples, float leftVolume, float rightVolume, float leftVolumeDelta, float rightVolumeDelta, float pitchRate) //write float buffer and return the samples be red
{
	if(floatBuffer != nullptr)
	{
			double copySamples;
			if(position + numberOfSamples * pitchRate < durationSamples)
				copySamples = (double) numberOfSamples;
			else
				copySamples = ((double)durationSamples - position) / pitchRate;

			if(leftVolumeDelta!=0.0f || rightVolumeDelta!=0.0f)
			{
				int leftRemainded = leftVolume / leftVolumeDelta;
				int rightRemainded = rightVolume / rightVolumeDelta;

				int MAX; bool LEFT;
				if(leftRemainded > rightRemainded)
				{
					MAX = leftRemainded;
					LEFT = true;
				}
				else
				{
					MAX = rightRemainded;
					LEFT = false;
				}

				if(MAX < copySamples)
					copySamples = LEFT ? leftRemainded : rightRemainded;
			}

			for(int i=0;i<copySamples;i++)
			{
				double newPosition = position + (pitchRate * (float)i);
				float newLeftVolume = leftVolume - (leftVolumeDelta * (float)i);
				float newRightVolume = rightVolume - (rightVolumeDelta * (float)i);
				buffer[i*2] += AudioTool::getSampleByFloatIndex(floatBuffer, durationSamples, newPosition, true) * newLeftVolume;
				buffer[i*2+1] += AudioTool::getSampleByFloatIndex(floatBuffer, durationSamples, newPosition, false) * newRightVolume;
			}

			return copySamples;

	}
	else
		return 0;
}

long Sound::read(float *buffer, long position, int numberOfSamples, float leftVolume, float rightVolume, float leftVolumeDelta, float rightVolumeDelta)
{

	if(floatBuffer != nullptr)
	{
		int copySamples;
		if(position + numberOfSamples < durationSamples)
			copySamples = numberOfSamples;
		else
			copySamples = durationSamples - position;

		if(leftVolumeDelta!=0.0f || rightVolumeDelta!=0.0f)
		{
			int leftRemainded = leftVolume / leftVolumeDelta;
			int rightRemainded = rightVolume / rightVolumeDelta;

			int MAX; bool LEFT;
			if(leftRemainded > rightRemainded)
			{
				MAX = leftRemainded;
				LEFT = true;
			}
			else
			{
				MAX = rightRemainded;
				LEFT = false;
			}

			if(MAX < copySamples)
				copySamples = LEFT ? leftRemainded : rightRemainded;
		}

		int firstLoopCount = copySamples / 2;

		float newLeftVolume;
		float newRightVolume;
        
        bool NEON_AVAILABLE = true;
       
        #if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
			if (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM &&
				(android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0)
				NEON_AVAILABLE = true;
			else
				NEON_AVAILABLE = false;
        #endif

		#if ((CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID) || ((CC_TARGET_PLATFORM == CC_PLATFORM_IOS) && !(TARGET_IPHONE_SIMULATOR)))
        if(NEON_AVAILABLE)
        {
            // use NEON-optimized routines
            for(int i=0;i<firstLoopCount;i++)
            {

                //Volume
                float volume[4] = {leftVolume, rightVolume, leftVolume, rightVolume};
                float32x4_t volume_v = vld1q_f32(volume);

                float volumeDelta[4] = {leftVolumeDelta, rightVolumeDelta, leftVolumeDelta, rightVolumeDelta};
                float32x4_t volumeDelta_v = vld1q_f32(volumeDelta);

                float var1 = (float) i;
                float32x4_t var1_v = vld1q_dup_f32(&var1);

                var1_v = vmulq_n_f32(var1_v, 2.0f);
                const float var2[4] = {0.0f, 0.0f, 1.0f, 1.0f};
                float32x4_t var2_v = vld1q_f32(var2);

                var1_v = vaddq_f32(var1_v, var2_v);
                volumeDelta_v = vmulq_f32(volumeDelta_v, var1_v);

                volume_v = vsubq_f32(volume_v, volumeDelta_v);

                //buffer load
                float32x4_t buffer_v = vld1q_f32(&buffer[i*4]);

                float32x4_t floatBuffer_v = vld1q_f32(&floatBuffer[position*2 + i*4]);
                buffer_v = vmlaq_f32(buffer_v, floatBuffer_v, volume_v);


                //buffer store
                vst1q_f32(&buffer[i*4], buffer_v);

            }
        }
        else
        #endif
        {
            // use non-NEON fallback routines instead
            for(int i=0;i<firstLoopCount;i++)
            {
                newLeftVolume = leftVolume - (leftVolumeDelta * i*2);
                newRightVolume = rightVolume - (rightVolumeDelta * i*2);
                buffer[i*4] += floatBuffer[position*2 + i*4] * newLeftVolume;
                buffer[i*4+1] += floatBuffer[position*2 + i*4+1] * newRightVolume;

                newLeftVolume = leftVolume - (leftVolumeDelta * (i*2+1));
                newRightVolume = rightVolume - (rightVolumeDelta * (i*2+1));
                buffer[i*4+2] += floatBuffer[position*2 + i*4+2] * newLeftVolume;
                buffer[i*4+3] += floatBuffer[position*2 + i*4+3] * newRightVolume;
            }
        }


		for(int i=firstLoopCount * 2; i<copySamples;i++)
		{
			newLeftVolume = leftVolume - (leftVolumeDelta * i);
			newRightVolume = rightVolume - (rightVolumeDelta * i);
			buffer[i*2] += floatBuffer[(i+position)*2] * newLeftVolume;
			buffer[i*2+1] += floatBuffer[(i+position)*2+1] * newRightVolume;
		}
		return copySamples;
	}
	else
		return 0;

}

void Sound::decode()
{
	int oriSampleRate = decoder->samplerate;
	durationSamples = decoder->durationSamples;

	int samplesPerFrame = decoder->samplesPerFrame;

	int durationByteLength = durationSamples * sizeof(short int) * 2;
	short *shortBuffer = (short int *) malloc(durationByteLength + 16384);
	int offset = 0;

	while(true)
	{
		unsigned int decodedSamples = samplesPerFrame;
		int result = decoder->decode(shortBuffer + offset, &decodedSamples);
		//LOGV("samplesPerFrame: %d, decodedSamples : %d", samplesPerFrame, decodedSamples);
		offset += decodedSamples * 2; // two channel
		if(result != SUPERPOWEREDDECODER_OK) break;
	}

	int durationFloatLength = durationSamples * sizeof(float) * 2;
	float *float_temp_buffer = (float *) malloc(durationFloatLength);
	//SuperpoweredShortIntToFloat(short_buffer, float_temp_buffer, durationSamples);
    
    for(int i=0;i<durationSamples;i++)
    {
        float_temp_buffer[i*2] = (float)shortBuffer[i*2] / 32768.0f;
        float_temp_buffer[i*2+1] = (float)shortBuffer[i*2+1] / 32768.0f;
    }

	int needSampleLength = AudioTool::getNeedLength2resample(durationSamples, oriSampleRate, wantSampleRate);
	floatBuffer = (float *) malloc(sizeof(float) * needSampleLength * 2);
	AudioTool::resample(float_temp_buffer, durationSamples, oriSampleRate, floatBuffer, needSampleLength, wantSampleRate);

	durationSamples = needSampleLength;
	if(durationSamples < 0)
	{
		cocos2d::log("%s %d", __FUNCTION__, durationSamples);
		exit(0);
	}

	free(float_temp_buffer);
	free(shortBuffer);
}

int Sound::getDurationSamples()
{
	return this->durationSamples;
}

float *Sound::getFloatBuffer()
{
	return this->floatBuffer;
}
