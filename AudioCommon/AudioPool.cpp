#include "AudioPool.h"

AudioPool::AudioPool(int systemSampleRate, int playerNumber, int stream_size, int bufferSampleLength, float volumeFactor)
: SYSTEM_SAMPLE_RATE(systemSampleRate), BUFFER_SAMPLE_LENGTH(bufferSampleLength)
{
	pthread_mutex_init(&soundArray_mutex, NULL);
    
	this->PLAYER_NUMBER = playerNumber;

	floatPcmData = (float **)malloc(sizeof(float*) * PLAYER_NUMBER);
	for(int i=0;i<PLAYER_NUMBER;i++)
		floatPcmData[i] = (float *)malloc((BUFFER_SAMPLE_LENGTH * 2 * 2) * sizeof(float));

	/* initialize variables for stream array */
	pthread_mutex_init(&streamLink_mutex, NULL);
	this->STREAM_SIZE = stream_size + 2; // headStream, tailStream added

	/* stream[0] (headStream) <-> stream[1] <-> stream[2] ... <-> stream[STREAM_SIZE-1] (tailStream) */
	/* head & tail Stream are not used to stream audio */

	Stream *prevStream = NULL;
	for(int i=0;i<STREAM_SIZE;i++)
	{
		streamMap[i] = new Stream(NULL, i, systemSampleRate, BUFFER_SAMPLE_LENGTH);
		streamMap[i]->prevStream = prevStream;
		if(prevStream!=NULL)
			prevStream->nextStream = streamMap[i];
		prevStream = streamMap[i];
	}
	headStream = streamMap[0];
	tailStream = streamMap[STREAM_SIZE - 1];

	streamMap[MRStreamID] = new Stream(NULL, MRStreamID, systemSampleRate, BUFFER_SAMPLE_LENGTH);
	streamMap[VoiceStreamID] = new Stream(NULL, VoiceStreamID, systemSampleRate, BUFFER_SAMPLE_LENGTH);
	otherInstEndID = otherInstStartID;

	this->volumeFactor = volumeFactor;
	this->compressor = (Compressor**) malloc(sizeof(Compressor*) * PLAYER_NUMBER);
	for(int i=0;i<PLAYER_NUMBER;i++)
		this->compressor[i] = new Compressor(systemSampleRate, BUFFER_SAMPLE_LENGTH);

	phaseVocoder = new PhaseVocoder(systemSampleRate, 10, BUFFER_SAMPLE_LENGTH, 8);
	reSampler = new ReSampler(systemSampleRate, BUFFER_SAMPLE_LENGTH);

}

AudioPool::~AudioPool()
{
	for(int i=0;i<PLAYER_NUMBER;i++)
		free(compressor[i]);

	free(compressor);

	/* realease streams */
	for(int i=0;i<STREAM_SIZE;i++)
	{
		Stream *stream = streamMap[i];
		delete stream;
	}

	pthread_mutex_destroy(&streamLink_mutex);

	/* release pcm buffer */
	for(int i=0;i<PLAYER_NUMBER;i++)
		free(floatPcmData[i]);
	free(floatPcmData);

	/* release sounds */
	std::map<unsigned int, Sound*>::iterator it;
	pthread_mutex_lock(&soundArray_mutex);
	for(it = soundMap.begin(); it!=soundMap.end();it++)
	{
		Sound *sound = it->second;
		delete sound;
	}


	pthread_mutex_unlock(&soundArray_mutex);
	pthread_mutex_destroy(&soundArray_mutex);

}

Sound *AudioPool::getSound(unsigned int soundID)
{
	pthread_mutex_lock(&soundArray_mutex); // sync soundArray

	std::map<unsigned int, Sound*>::iterator it = soundMap.find(soundID);
	if (it != soundMap.end()) //sound exist
	{
		pthread_mutex_unlock(&soundArray_mutex);
		return it->second;
	}
	else
	{
		pthread_mutex_unlock(&soundArray_mutex);
		return nullptr;
	}
}

int AudioPool::load(unsigned int soundID, Sound *sound)
{
	pthread_mutex_lock(&soundArray_mutex); // sync soundArray

	std::map<unsigned int, Sound*>::iterator it = soundMap.find(soundID);
	if (it != soundMap.end()) //sound exist
	{
		pthread_mutex_unlock(&soundArray_mutex);
		return false;
	}

	soundMap.insert(std::pair<unsigned int, Sound*>(soundID, sound));
	pthread_mutex_unlock(&soundArray_mutex);

	return 0;
}


int AudioPool::unload(unsigned int soundID)
{
	pthread_mutex_lock(&soundArray_mutex);

	std::map<unsigned int, Sound*>::iterator it = soundMap.find(soundID);
	if (it != soundMap.end()) //sound exist
	{
		Sound *sound = it->second;
		soundMap.erase(soundID);
		if(sound!=nullptr)
			delete sound;
		pthread_mutex_unlock(&soundArray_mutex);
		return 0;
	}
	else
	{
		pthread_mutex_unlock(&soundArray_mutex);
		return -1;
	}
}

int AudioPool::play
(int soundID, float pitch, float noteVelocity, float pan, int playerID) // return StreamID
{
	int i;
	Stream *stream;
	//pthread_mutex_lock(&streamLink_mutex);
		stream = tailStream->prevStream;

		// prevStream <-> stream <-> nextStream   =>   prevStream <-> nextStream
		stream->prevStream->nextStream = stream->nextStream;
		stream->nextStream->prevStream = stream->prevStream;

		// headStream <-> nextStream   =>   headStream <-> stream <-> nextStream
		stream->nextStream = headStream->nextStream;
		stream->prevStream = headStream;
		headStream->nextStream->prevStream = stream;
		headStream->nextStream = stream;
	//pthread_mutex_unlock(&streamLink_mutex);

	i = stream->getStreamID();
	if(stream->getState()!=STREAM_STATE_END)
		stopStream(i);

	stream->setSound(soundMap[soundID]);
	float volume = AudioTool::vel2FloatScale(noteVelocity) * AudioTool::userVolumeScale(MY_VOLUME);
	//cocos2d::log("play %f %f %f %f", userVolume, noteVelocity, volume, pan);
	stream->setPan(pan);
	stream->setVolume(volume);
	stream->setState(STREAM_STATE_PLAYING);
	stream->setPlayerID(playerID);

	return i;
}

int AudioPool::play
(int soundID, NoteObject *noteObj, int playerID)
{
	int i;
	Stream *stream;
	//pthread_mutex_lock(&streamLink_mutex);
		stream = tailStream->prevStream;

		// prevStream <-> stream <-> nextStream   =>   prevStream <-> nextStream
		stream->prevStream->nextStream = stream->nextStream;
		stream->nextStream->prevStream = stream->prevStream;

		// headStream <-> nextStream   =>   headStream <-> stream <-> nextStream
		stream->nextStream = headStream->nextStream;
		stream->prevStream = headStream;
		headStream->nextStream->prevStream = stream;
		headStream->nextStream = stream;
	//pthread_mutex_unlock(&streamLink_mutex);

	i = stream->getStreamID();
	if(stream->getState()!=STREAM_STATE_END)
		stopStream(i);

	stream->setSound(soundMap[soundID]);
	float volume;
	if(noteObj!=nullptr)
		volume = AudioTool::vel2FloatScale(noteObj->m_velocity) * AudioTool::userVolumeScale(MY_VOLUME);
	else
		volume = AudioTool::userVolumeScale(MY_VOLUME);

	if(noteObj!=nullptr)
		stream->setPan(noteObj->m_Pan);

	stream->setVolume(volume);

	if(noteObj!=nullptr)
    {
		if(noteObj->m_Pitch==1.0f)
			stream->setPitchRateEnable(false);
		else
		{
			stream->setPitchRateEnable(true);
			stream->setPitchRate(noteObj->m_Pitch);
		}
    }
    
	stream->setState(STREAM_STATE_PLAYING);
	stream->setPlayerID(playerID);

    return i;
}


void AudioPool::fadeStopStream(int streamID, float time/*in second*/)
{
	if(streamMap[streamID]->getState()==STREAM_STATE_PLAYING)
		streamMap[streamID]->setVolumeGradually(0.0f, time);

	streamMap[streamID]->setState(STREAM_STATE_FADEOUT);
}

void AudioPool::fadeStopStreamByPlayerID(int playerID, float time/*in second*/)
{
	for(int i=1;i<STREAM_SIZE-1;i++)
		if(streamMap[i]->getPlayerID() == playerID && streamMap[i]->getState()==STREAM_STATE_PLAYING)
		{
			streamMap[i]->setVolumeGradually(0.0f, time);
			streamMap[i]->setState(STREAM_STATE_FADEOUT);
		}
}

void AudioPool::stopStreamByPlayerID(int playerID)
{
	for(int i=1;i<STREAM_SIZE-1;i++)
		if(streamMap[i]->getPlayerID() == playerID && streamMap[i]->getState()!=STREAM_STATE_END)
			streamMap[i]->setState(STREAM_STATE_END);
}


int AudioPool::stopStream(int streamID)
{
	streamMap[streamID]->setState(STREAM_STATE_END);
	return 0;
}

int AudioPool::stopAllStream()
{
	int i;
	for(i=1;i<STREAM_SIZE-1;i++)
	{
		Stream *stream = streamMap[i];
		if(stream->getState()!=STREAM_STATE_END)
			stream->setState(STREAM_STATE_END);
	}
	return 0;
}

float AudioPool::getStreamVolume(int streamID)
{
	return streamMap[streamID]->getVolume();
}

int AudioPool::setStreamVolume(int streamID, float noteVelocity)
{
	float volume = AudioTool::vel2FloatScale(noteVelocity) * AudioTool::userVolumeScale(MY_VOLUME);
	streamMap[streamID]->setVolume(volume);

	return 0;
}

void AudioPool::setVolumes(float MRVolume, float otherVolume, float myVolume)
{
	this->MR_VOLUME = MRVolume;
	this->OTHER_VOLUME = otherVolume;
	this->MY_VOLUME = myVolume;
}

int AudioPool::process(short *shortBuffer, float *floatBuffer, long currentSamplePosition, int playerID, int bufferSampleLength)
{
	if(bufferSampleLength < 0)
		bufferSampleLength = BUFFER_SAMPLE_LENGTH;

	float *newFloatBuffer;
	if(floatBuffer == nullptr)
		newFloatBuffer = this->floatPcmData[playerID];
	else // case that have to write in float buffer
		newFloatBuffer = floatBuffer;

	memset(newFloatBuffer, 0, sizeof(float) * bufferSampleLength * 2);
	float countSamples = 0;

	/// write my instrument's sound ///
	for (int i=1;i<STREAM_SIZE-1;i++)
	{
		Stream *stream = streamMap[i];
		if(stream->getState()==STREAM_STATE_END)
			continue;

		if(stream->getPlayerID()!=playerID)
			continue;

		float samplesReaded = stream->process(newFloatBuffer, bufferSampleLength);
		countSamples += samplesReaded;
		if(samplesReaded == 0.0f)
			stream->setState(STREAM_STATE_END);
	}

	/// Write MR, voice and other instruments' sound ///
	if(playerID == MRPlayerID && currentSamplePosition >= 0)
	{
		for(int i=MRStreamID;i>otherInstEndID;i--)
		{
			Stream *stream = streamMap[i];
			long playSamplePosition = stream->getPlayPositionInSample();
			long playSampleDuration = stream->getPlayDurationInSample();
			long releaseSampleDuration = stream->getReleaseDurationInSample();

			if((currentSamplePosition + bufferSampleLength > playSamplePosition) &&
					(currentSamplePosition < playSamplePosition + playSampleDuration + releaseSampleDuration)
			  )
			{
				float sampleReaded = stream->process(newFloatBuffer, bufferSampleLength, currentSamplePosition);
				countSamples += sampleReaded;
			}
		}
	}

	if(volumeFactor!=1.0f)
	    {
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
	            for(int i=0;i<bufferSampleLength/2;i++)
	            {
	                float32x4_t floatPcmData_v = vld1q_f32(&newFloatBuffer[i*4]);
	                floatPcmData_v = vmulq_n_f32(floatPcmData_v, volumeFactor);
	                vst1q_f32(&newFloatBuffer[i*4], floatPcmData_v);
	            }
	        }
	        else
	        #endif
	        {
	            // use non-NEON fallback routines instead
	            for(int i=0;i<bufferSampleLength;i++)
	            {
	            	newFloatBuffer[i*2] *= volumeFactor;
	            	newFloatBuffer[i*2+1] *= volumeFactor;
	            }
	        }
	    }
	compressor[playerID]->process(newFloatBuffer, newFloatBuffer, bufferSampleLength);

	if(playerID==MRPlayerID)
	{
		//reSampler->process(newFloatBuffer, newFloatBuffer, bufferSampleLength);
		phaseVocoder->process(newFloatBuffer, newFloatBuffer, bufferSampleLength);
	}

	if(shortBuffer != nullptr)
		AudioTool::float2short(newFloatBuffer, shortBuffer, bufferSampleLength * 2);

	if(countSamples>0)
		return bufferSampleLength;
	else
		return 0;
}


int AudioPool::resetPlayer(int playerID)
{
	this->compressor[playerID]->reset();
}

int AudioPool::setStreamVolumeAbsolutely(int streamID, float volume)
{
	Stream *stream = streamMap[streamID];
	if(stream!=nullptr)
		stream->setVolume(volume);
    
    return 0;
}

int AudioPool::readyMR(int soundID, AudioType audioType)
{
	Sound *sound = soundMap[soundID];
	if(sound==nullptr)
		return -1;

	Stream *stream;
	switch(audioType)
	{
		case MR:
			stream = (Stream *) streamMap[MRStreamID];
			stream->setSound(sound);
			stream->setVolume(AudioTool::userVolumeScale(MR_VOLUME));
			stream->setPositionInTime(0);
			stream->setPlayPositionInTime(0);
			stream->setPlayDurationInSample(sound->getDurationSamples());
			stream->setReleaseDurationInTime(0);
			return MRStreamID;
			break;

		case VOICE:
			stream = (Stream *) streamMap[VoiceStreamID];
			stream->setSound(sound);
			stream->setVolume(AudioTool::userVolumeScale(MR_VOLUME));
			stream->setPositionInTime(0);
			stream->setPlayPositionInTime(0);
			stream->setPlayDurationInSample(sound->getDurationSamples());
			stream->setReleaseDurationInTime(0);
			return VoiceStreamID;
			break;

		default:
			return -1;
			break;
	}
}

int AudioPool::readyMR(int soundID, NoteObject *noteObj)
{
	Sound *sound = soundMap[soundID];
	if(sound==nullptr)
		return -1;

	int newID = otherInstEndID;
	otherInstEndID--;

	Stream *newStream = new Stream(sound, newID, SYSTEM_SAMPLE_RATE, BUFFER_SAMPLE_LENGTH);
	newStream->setPositionInTime(0);
	newStream->setVolume(AudioTool::vel2FloatScale(noteObj->m_velocity) * AudioTool::userVolumeScale(OTHER_VOLUME));
	newStream->setPlayPositionInTime(noteObj->m_time);
	newStream->setPlayDurationInTime(noteObj->m_duration);
	newStream->setReleaseDurationInTime(100);
	streamMap[newID] = newStream;

    //cocos2d::log("%s stream Count : %d", __FUNCTION__, newID);
	return newID;
}

int AudioPool::releaseMR()
{
//    Stream *MRStream = (Stream *) streamMap[MRStreamID];
//    Stream *VoiceStream = (Stream *) streamMap[VoiceStreamID];
//    MRStream->releaseSound();
//    VoiceStream->releaseSound();

	for(int i = otherInstEndID; i <= otherInstStartID; i++)
	{
//		Stream *stream = (Stream *) streamMap[i];
//		delete stream;
        
        auto it = streamMap.find(i);
        if (it != streamMap.end())
        {
            delete (Stream *)it->second;
            streamMap.erase(it);
        }
	}

	otherInstEndID = otherInstStartID;

	for(int i=0;i<PLAYER_NUMBER;i++)
		this->compressor[i]->reset();
    
    return 0;
}

long AudioPool::getStreamDuration(int streamID)
{
	Stream *stream = streamMap[streamID];
	if(stream!=nullptr)
		return stream->getDurationTime();

	return 0;
}

void AudioPool::setMRPlayerID(int MRPlayerID)
{
	this->MRPlayerID = MRPlayerID;
}

int AudioPool::getMRPlayerID()
{
	return MRPlayerID;
}
