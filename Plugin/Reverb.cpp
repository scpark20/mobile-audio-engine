
#include "Reverb.h"

Reverb::Reverb(int sampleRate, int bufferSampleLength, float delay, float feedback, float dryWet)
:Plugin(sampleRate, bufferSampleLength)
{
	this->dryWet = dryWet;
	cocos2d::log("%s1",__FUNCTION__);

	int index = 0;
	for(int i=2;i<1000;i++)
	{
		bool r = false;
		for(int j=2;j<i;j++)
			if(i%j==0)
				r = true;

		if(r==false)
		{
			primeList[index] = i;
			index++;
		}
	}

	DELAY_INDEX_MAX = index;
	cocos2d::log("%s2",__FUNCTION__);

	/*********************DELAY VALUES ********************/
	int INPUT_APF_DELAY1 = 2;
	int INPUT_APF_DELAY2 = 3;
	int INPUT_APF_DELAY3 = 5;
	int INPUT_APF_DELAY4 = 7;

	int COMB_DELAY1 = primeList[(int)(delay * (float)DELAY_INDEX_MAX * 0.10f)];
	int COMB_DELAY2 = primeList[(int)(delay * (float)DELAY_INDEX_MAX * 0.12f)];
	int COMB_DELAY3 = primeList[(int)(delay * (float)DELAY_INDEX_MAX * 0.14f)];
	int COMB_DELAY4 = primeList[(int)(delay * (float)DELAY_INDEX_MAX * 0.16f)];
	int COMB_DELAY5 = primeList[(int)(delay * (float)DELAY_INDEX_MAX * 0.18f)];
	int COMB_DELAY6 = primeList[(int)(delay * (float)DELAY_INDEX_MAX * 0.20f)];

	int COMB_DELAY7 = primeList[(int)(delay * (float)DELAY_INDEX_MAX * 0.11f)];
	int COMB_DELAY8 = primeList[(int)(delay * (float)DELAY_INDEX_MAX * 0.13f)];
	int COMB_DELAY9 = primeList[(int)(delay * (float)DELAY_INDEX_MAX * 0.15f)];
	int COMB_DELAY10 = primeList[(int)(delay * (float)DELAY_INDEX_MAX * 0.17f)];
	int COMB_DELAY11 = primeList[(int)(delay * (float)DELAY_INDEX_MAX * 0.19f)];
	int COMB_DELAY12 = primeList[(int)(delay * (float)DELAY_INDEX_MAX * 0.21f)];

	int OUTPUT_APF_DELAY1 = 2;
	int OUTPUT_APF_DELAY2 = 5;
	int OUTPUT_APF_DELAY3 = 11;
	int OUTPUT_APF_DELAY4 = 17;

	int OUTPUT_APF_DELAY5 = 3;
	int OUTPUT_APF_DELAY6 = 7;
	int OUTPUT_APF_DELAY7 = 13;
	int OUTPUT_APF_DELAY8 = 19;


	/*********************GAIN VALUES ********************/

	float INPUT_ONEPOLE_FREQ = 3600;

	float INPUT_APF_GAIN1 = feedback * 0.5f;
	float INPUT_APF_GAIN2 = feedback * 0.6f;
	float INPUT_APF_GAIN3 = feedback * 0.7f;
	float INPUT_APF_GAIN4 = feedback * 0.8f;

	float COMB_GAIN1 = feedback * 0.5f;
	float COMB_GAIN2 = feedback * 0.55f;
	float COMB_GAIN3 = feedback * 0.6f;
	float COMB_GAIN4 = feedback * 0.65f;
	float COMB_GAIN5 = feedback * 0.7f;
	float COMB_GAIN6 = feedback * 0.75f;

	float COMB_GAIN7 = feedback * 0.525f;
	float COMB_GAIN8 = feedback * 0.575f;
	float COMB_GAIN9 = feedback * 0.625f;
	float COMB_GAIN10 = feedback * 0.675f;
	float COMB_GAIN11 = feedback * 0.725f;
	float COMB_GAIN12 = feedback * 0.775f;

	float OUTPUT_APF_GAIN1 = feedback * 0.5f;
	float OUTPUT_APF_GAIN2 = feedback * 0.6f;
	float OUTPUT_APF_GAIN3 = feedback * 0.7f;
	float OUTPUT_APF_GAIN4 = feedback * 0.8f;

	float OUTPUT_APF_GAIN5 = feedback * 0.55f;
	float OUTPUT_APF_GAIN6 = feedback * 0.65f;
	float OUTPUT_APF_GAIN7 = feedback * 0.75f;
	float OUTPUT_APF_GAIN8 = feedback * 0.85f;

	float OUTPUT_ONEPOLE_FREQ1 = 1800;
	float OUTPUT_ONEPOLE_FREQ2 = 2500;

	cocos2d::log("%s4",__FUNCTION__);

	this->combFilterArray = (CombFilter **) malloc(sizeof(CombFilter *) * 12);
	this->allPassFilterArray = (AllPassFilter **) malloc(sizeof(AllPassFilter *) * 12);
	this->onePoleLPFArray = (OnePoleLPF **) malloc(sizeof(OnePoleLPF *) * 3);

	combFilterArray[0] = new CombFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(COMB_DELAY1, sampleRate), COMB_GAIN1);
	combFilterArray[1] = new CombFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(COMB_DELAY2, sampleRate), COMB_GAIN2);
	combFilterArray[2] = new CombFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(COMB_DELAY3, sampleRate), COMB_GAIN3);
	combFilterArray[3] = new CombFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(COMB_DELAY4, sampleRate), COMB_GAIN4);
	combFilterArray[4] = new CombFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(COMB_DELAY5, sampleRate), COMB_GAIN5);
	combFilterArray[5] = new CombFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(COMB_DELAY6, sampleRate), COMB_GAIN6);

	combFilterArray[6] = new CombFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(COMB_DELAY7, sampleRate), COMB_GAIN7);
	combFilterArray[7] = new CombFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(COMB_DELAY8, sampleRate), COMB_GAIN8);
	combFilterArray[8] = new CombFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(COMB_DELAY9, sampleRate), COMB_GAIN9);
	combFilterArray[9] = new CombFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(COMB_DELAY10, sampleRate), COMB_GAIN10);
	combFilterArray[10] = new CombFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(COMB_DELAY11, sampleRate), COMB_GAIN11);
	combFilterArray[11] = new CombFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(COMB_DELAY12, sampleRate), COMB_GAIN12);

	allPassFilterArray[0] = new AllPassFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(INPUT_APF_DELAY1, sampleRate), INPUT_APF_GAIN1);
	allPassFilterArray[1] = new AllPassFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(INPUT_APF_DELAY2, sampleRate), INPUT_APF_GAIN2);
	allPassFilterArray[2] = new AllPassFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(INPUT_APF_DELAY3, sampleRate), INPUT_APF_GAIN3);
	allPassFilterArray[3] = new AllPassFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(INPUT_APF_DELAY4, sampleRate), INPUT_APF_GAIN4);

	allPassFilterArray[4] = new AllPassFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(OUTPUT_APF_DELAY1, sampleRate), OUTPUT_APF_GAIN1);
	allPassFilterArray[5] = new AllPassFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(OUTPUT_APF_DELAY2, sampleRate), OUTPUT_APF_GAIN2);
	allPassFilterArray[6] = new AllPassFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(OUTPUT_APF_DELAY3, sampleRate), OUTPUT_APF_GAIN3);
	allPassFilterArray[7] = new AllPassFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(OUTPUT_APF_DELAY4, sampleRate), OUTPUT_APF_GAIN4);

	allPassFilterArray[8] = new AllPassFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(OUTPUT_APF_DELAY5, sampleRate), OUTPUT_APF_GAIN5);
	allPassFilterArray[9] = new AllPassFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(OUTPUT_APF_DELAY6, sampleRate), OUTPUT_APF_GAIN6);
	allPassFilterArray[10] = new AllPassFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(OUTPUT_APF_DELAY7, sampleRate), OUTPUT_APF_GAIN7);
	allPassFilterArray[11] = new AllPassFilter(sampleRate, bufferSampleLength, AudioTool::ms2samples(OUTPUT_APF_DELAY8, sampleRate), OUTPUT_APF_GAIN8);

	onePoleLPFArray[0] = new OnePoleLPF(sampleRate, bufferSampleLength, INPUT_ONEPOLE_FREQ);
	onePoleLPFArray[1] = new OnePoleLPF(sampleRate, bufferSampleLength, OUTPUT_ONEPOLE_FREQ1);
	onePoleLPFArray[2] = new OnePoleLPF(sampleRate, bufferSampleLength, OUTPUT_ONEPOLE_FREQ2);

	buffer = (float **) malloc(sizeof(float*) * 12);
	for(int i=0;i<12;i++)
		buffer[i] = (float *) malloc(sizeof(float) * bufferSampleLength * 2);

}

Reverb::~Reverb()
{
	for(int i=0;i<4;i++)
		delete combFilterArray[i];

	for(int i=0;i<2;i++)
		delete allPassFilterArray[i];

	free(combFilterArray);
	free(allPassFilterArray);


}

int Reverb::process(float *inBuffer, float *outBuffer, int sampleLength, channel_t channel)
{
	memcpy(buffer[0], inBuffer, sizeof(float) * sampleLength * 2);
	onePoleLPFArray[0]->process(buffer[0], buffer[0], sampleLength);
	for(int i=0;i<4;i++)
		allPassFilterArray[i]->process(buffer[0], buffer[0], sampleLength);

	for(int i=1;i<12;i++)
		memcpy(buffer[i], buffer[0], sizeof(float) * sampleLength * 2);



	/*LEFT */
	for(int i=0;i<6;i++)
		combFilterArray[i]->process(buffer[i], buffer[i], sampleLength, LEFT);

	for(int j=1;j<6;j++)
		for(int i=0;i<sampleLength * 2;i++)
			buffer[0][i] += buffer[j][i] * 0.7f;

	for(int i=4;i<8;i++)
		allPassFilterArray[i]->process(buffer[0], buffer[0], sampleLength, LEFT);
	/*-----*/


	/* RIGHT */
	for(int i=6;i<12;i++)
		combFilterArray[i]->process(buffer[i], buffer[i], sampleLength, RIGHT);

	for(int j=7;j<12;j++)
		for(int i=0;i<sampleLength * 2;i++)
			buffer[6][i] += buffer[j][i] * 0.7f;

	for(int i=8;i<12;i++)
		allPassFilterArray[i]->process(buffer[6], buffer[6], sampleLength, RIGHT);
	/*-----*/

	float wet = dryWet;
	float dry =  1.0f - dryWet;

	for(int i=0;i<sampleLength;i++)
	{
		outBuffer[i*2] = buffer[0][i*2] * wet + inBuffer[i*2] * dry;
		outBuffer[i*2+1] = buffer[6][i*2+1] * wet + inBuffer[i*2+1] * dry;
	}

}

int Reverb::reset()
{

}
