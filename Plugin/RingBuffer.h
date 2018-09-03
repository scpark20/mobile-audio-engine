/*
 * RingBuffer.h
 *
 *  Created on: Oct 23, 2015
 *      Author: scpark
 */

#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_

#include <AudioCommon/AudioTool.h>
#include <stdlib.h>

template <typename T>
class RingBuffer {
public:
	RingBuffer(int bufferSampleLength)
	{
		this->bufferSampleLength = bufferSampleLength;
		buffer = (T*) malloc(bufferSampleLength * sizeof(T) * 2);

		writePosition = 0;
		readPosition = 0;
	}

	virtual ~RingBuffer()
	{
		free(buffer);
	}

	int write(T *sourceBuffer, int sampleLength, int positionIncrement, bool MixWrite)
	{
		int modedWritePosition = writePosition % bufferSampleLength;
		int copySamples = modedWritePosition + sampleLength > bufferSampleLength ?
							bufferSampleLength - modedWritePosition :
							sampleLength;

		if(MixWrite)
		{
			int position = (writePosition + sampleLength - positionIncrement) % bufferSampleLength;
			int zeroBoundPosition = (writePosition + sampleLength) % bufferSampleLength;

			if(zeroBoundPosition < position)
			{
				memset(&buffer[position*2], 0, (bufferSampleLength - position) * sizeof(float) * 2);
				memset(&buffer[zeroBoundPosition*2], 0, zeroBoundPosition * sizeof(float) * 2);
			}
			else
				memset(&buffer[position*2], 0, (zeroBoundPosition - position) * sizeof(float) * 2);

			// Ossia
			/*
			while(position < zeroBoundPosition)
			{
				buffer[(position % bufferSampleLength) * 2] = 0.0f;
				buffer[(position % bufferSampleLength) * 2 + 1] = 0.0f;
				position++;
			}
			*/

			for(int i=0;i<copySamples;i++)
			{
				buffer[(modedWritePosition+i)*2] += sourceBuffer[i*2];
				buffer[(modedWritePosition+i)*2+1] += sourceBuffer[i*2+1];
			}

			for(int i=0;i<sampleLength-copySamples;i++)
			{
				buffer[i*2] += sourceBuffer[(copySamples+i)*2];
				buffer[i*2+1] += sourceBuffer[(copySamples+i)*2+1];
			}

			// Ossia
			/*
			for(int i=0;i<sampleLength;i++)
			{
				buffer[((modedWritePosition+i)%bufferSampleLength)*2] += sourceBuffer[i*2];
				buffer[((modedWritePosition+i)%bufferSampleLength)*2+1] += sourceBuffer[i*2+1];
			}
			*/
		}
		else
		{
			memcpy(&buffer[modedWritePosition*2], sourceBuffer, copySamples * sizeof(T) * 2);
			memcpy(buffer, &sourceBuffer[copySamples*2], (sampleLength-copySamples) * sizeof(T) * 2);

			// Ossia
			/*
			for(int i=0;i<sampleLength;i++)
			{
				buffer[((modedWritePosition+i)%bufferSampleLength)*2] = sourceBuffer[i*2];
				buffer[((modedWritePosition+i)%bufferSampleLength)*2+1] = sourceBuffer[i*2+1];
			}
			*/

		}

		return writePosition += positionIncrement;
	}

	int writeSeparately(T *leftBuffer, T *rightBuffer, int sampleLength, int positionIncrement, bool MixWrite)
	{
		int modedWritePosition = writePosition % bufferSampleLength;
		int copySamples = modedWritePosition + sampleLength > bufferSampleLength ?
							bufferSampleLength - modedWritePosition :
							sampleLength;

		if(MixWrite)
		{
			int position = (writePosition + sampleLength - positionIncrement) % bufferSampleLength;
			int zeroBoundPosition = (writePosition + sampleLength) % bufferSampleLength;

			if(zeroBoundPosition < position)
			{
				memset(&buffer[position*2], 0, (bufferSampleLength - position) * sizeof(float) * 2);
				memset(&buffer[zeroBoundPosition*2], 0, zeroBoundPosition * sizeof(float) * 2);
			}
			else
				memset(&buffer[position*2], 0, (zeroBoundPosition - position) * sizeof(float) * 2);

			for(int i=0;i<copySamples;i++)
			{
				buffer[(modedWritePosition+i)*2] += leftBuffer[i];
				buffer[(modedWritePosition+i)*2+1] += rightBuffer[i];
			}

			for(int i=0;i<sampleLength-copySamples;i++)
			{
				buffer[i*2] += leftBuffer[copySamples+i];
				buffer[i*2+1] += rightBuffer[copySamples+i];
			}
		}
		else
		{
			for(int i=0;i<copySamples;i++)
			{
				buffer[(modedWritePosition+i)*2] = leftBuffer[i];
				buffer[(modedWritePosition+i)*2+1] = rightBuffer[i];
			}

			for(int i=0;i<sampleLength-copySamples;i++)
			{
				buffer[i*2] = leftBuffer[copySamples+i];
				buffer[i*2+1] = rightBuffer[copySamples+i];
			}
		}

		return writePosition += positionIncrement;
	}

	int read(T *destBuffer, int readOffset, int sampleLength, float readInterval, double positionIncrement)
	{
		for(int i=0;i<sampleLength;i++)
		{
			double position = AudioTool::getRingBufferFloatPosition(readPosition + readOffset + i * readInterval, bufferSampleLength);
			AudioTool::getSampleByFloatIndex2(buffer, bufferSampleLength, position, &destBuffer[i*2], &destBuffer[i*2+1]);
		}

		return readPosition += positionIncrement;
	}

	int readSeparately(T *leftBuffer, T *rightBuffer, double readOffset, int sampleLength, double readInterval, double positionIncrement)
	{
		for(int i=0;i<sampleLength;i++)
		{
			double position = AudioTool::getRingBufferFloatPosition(readPosition + readOffset + i * readInterval, bufferSampleLength);
			AudioTool::getSampleByFloatIndex2(buffer, bufferSampleLength, position, &leftBuffer[i], &rightBuffer[i]);
		}

		return readPosition += positionIncrement;
	}

	double getRemainedSampleLength()
	{
		return writePosition - readPosition;
	}

	int reset()
	{
		this->writePosition = 0;
		this->readPosition = 0;
	}

private:
	T* buffer;
	int bufferSampleLength;

	int writePosition;
	double readPosition;
};

#endif /* RINGBUFFER_H_ */
