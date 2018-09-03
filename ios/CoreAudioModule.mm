//
//  CoreAudioModule.cpp
//  Musician
//
//  Created by musician on 2015. 4. 29..
//
//

#import "CoreAudioModule.h"
#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioToolbox.h>


#define NUM_BUFFERS 3
#define SAMPLE_TYPE short
#define NUM_CHANNELS 2
/*

#define BUFFER_SIZE 512
#define MAX_NUMBER 32767
#define SAMPLE_RATE 44100
*/

@interface CoreAudioModule : NSObject

-(id) init:(CoreAudioModuleProxy *) proxy;
-(void) cleanUp;

@end


@implementation CoreAudioModule
{
@public
    CoreAudioModuleProxy *proxy;
    SuperpoweredDecoder *decoder;
    AudioQueueRef outputQueue;
    AudioQueueBufferRef buffers[NUM_BUFFERS];

}

void OutputBufferCallback (void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer)
{
    [(CoreAudioModule *)inUserData processOutputBuffer:inBuffer queue:inAQ];
}

-(id) init:(CoreAudioModuleProxy *) proxyValue
{
    self = [super init];
    if (!self) return nil;
    
    proxy = proxyValue;
    decoder = new SuperpoweredDecoder();
    
    [self startOutputAudioQueue];
    
    return self;
}

- (void)startOutputAudioQueue
{
    OSStatus err;
    int i;
    
    // Set up stream format fields
    AudioStreamBasicDescription format;
    format.mSampleRate = proxy->SAMPLE_RATE;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    format.mBitsPerChannel = 16;
    format.mChannelsPerFrame = NUM_CHANNELS;
    format.mBytesPerFrame = sizeof(SAMPLE_TYPE) * NUM_CHANNELS;
    format.mFramesPerPacket = 1;
    format.mBytesPerPacket = format.mBytesPerFrame * format.mFramesPerPacket;
    format.mReserved = 0;
    
    // New output queue ---- PLAYBACK ----
    err = AudioQueueNewOutput (&format, OutputBufferCallback, self, nil, nil, 0, &outputQueue);
    if (err != noErr) NSLog(@"AudioQueueNewOutput() error: %d", err);
    
    // Enqueue buffers
    for (i = 0; i < NUM_BUFFERS; i++)
    {
        err = AudioQueueAllocateBuffer (outputQueue, proxy->BUFFER_SAMPLE_LENGTH * 4, &buffers[i]);
        if (err == noErr) {
            // Fill buffer.

            char *pcmData = (char *)malloc(sizeof(short) * proxy->BUFFER_SAMPLE_LENGTH * 2);
            memset(pcmData, 0, sizeof(short) * proxy->BUFFER_SAMPLE_LENGTH * 2);
            memcpy(buffers[i]->mAudioData, pcmData, proxy->BUFFER_SAMPLE_LENGTH * 4);
            buffers[i]->mAudioDataByteSize = proxy->BUFFER_SAMPLE_LENGTH * 4;
            err = AudioQueueEnqueueBuffer (outputQueue, buffers[i], 0, nil);
            free(pcmData);
            if (err != noErr) NSLog(@"AudioQueueEnqueueBuffer() error: %d", err);
        } else {
            NSLog(@"AudioQueueAllocateBuffer() error: %d", err);
            return;
        }
    }
    
    err = AudioQueueSetParameter (outputQueue, kAudioQueueParam_Volume, 1.0);
    if (err != noErr) NSLog(@"AudioQueueSetParameter() error: %d", err);
    
    // Start queue
    err = AudioQueueStart(outputQueue, nil);
    if (err != noErr) NSLog(@"AudioQueueStart() error: %d", err);
}

- (void) processOutputBuffer: (AudioQueueBufferRef) buffer queue:(AudioQueueRef) queue
{
    AudioPool *audioPool = proxy->audioPool;
    
    if(proxy->audioState == AUDIO_STATE_PLAYING)
    {
        audioPool->process((short *)buffer->mAudioData, nullptr, proxy->currentPosition);
        proxy->currentPosition += proxy->BUFFER_SAMPLE_LENGTH;
    }
    else
        memset(buffer->mAudioData, 0, sizeof(short) * proxy->BUFFER_SAMPLE_LENGTH * 2);

    buffer->mAudioDataByteSize = proxy->BUFFER_SAMPLE_LENGTH * 4;
    OSStatus err = AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
    if (err != noErr)
        NSLog(@"AudioQueueEnqueueBuffer() error %d", err);
}

- (void)cleanUp {
    OSStatus err;
    
    err = AudioQueueDispose (outputQueue, NO); // Also disposes of its buffers
    if (err != noErr) NSLog(@"AudioQueueDispose() error: %d", err);
    outputQueue = nil;
}

-(void) dealloc
{
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
    
    for (int i = 0; i < NUM_BUFFERS; i++)
    {
        free(buffers[i]);
    }
}

@end


//-----------------------------------------------------------------------------

CoreAudioModuleProxy::CoreAudioModuleProxy(int systemSampleRate, int bufferSampleLength, int systemBufferSampleLength, int streamSize)
:AudioModule(systemSampleRate, bufferSampleLength, systemBufferSampleLength, streamSize)
{
    audioModule = [[CoreAudioModule alloc] init:this];
    
    this->audioState = AUDIO_STATE_END;
    this->currentPosition = 0;
    this->audioPool = new AudioPool(SAMPLE_RATE, 1, STREAM_SIZE, BUFFER_SAMPLE_LENGTH, 1.00f);
    audioPool->setMRPlayerID(0);
}

CoreAudioModuleProxy::~CoreAudioModuleProxy()
{
    [(CoreAudioModule*)audioModule release];
}

int CoreAudioModuleProxy::init(int playerNumberMax)
{
    return playerNumberMax;
}

int CoreAudioModuleProxy::release()
{
    return 0;
}

// for MR AUDIO
int CoreAudioModuleProxy::readyMR(const char* path, AudioType audioType)
{
    unsigned int soundID = AudioTool::getHashValue(path);
    return audioPool->readyMR(soundID, audioType);
}

int CoreAudioModuleProxy::readyMR(NoteObject* noteObj)
{
    unsigned int soundID = AudioTool::getHashValue(noteObj->m_FullPath);
    return audioPool->readyMR(soundID, noteObj);
}

int CoreAudioModuleProxy::startMR()
{
    currentPosition = 0;
    long currentTime = AudioTool::getSystemCurrentTime();
    for(int j=0;j<POSITION_CHECK_COUNT;j++)
    {
        positionArray[j] = 0;
        timeArray[j] = currentTime;
    }
    
    audioState = AUDIO_STATE_PLAYING;
    return 0;
}

int CoreAudioModuleProxy::pauseMR()
{
    audioState = AUDIO_STATE_PAUSED;
    return 0;
}

int CoreAudioModuleProxy::resumeMR()
{
    long currentTime = AudioTool::getSystemCurrentTime();
    for(int j=0;j<POSITION_CHECK_COUNT;j++)
    {
        positionArray[j] = currentPosition;
        timeArray[j] = currentTime;
    }
    
    audioState = AUDIO_STATE_PLAYING;
    return 0;
}

int CoreAudioModuleProxy::stopMR()
{
    audioState = AUDIO_STATE_END;
    currentPosition = 0;
    return 0;
}

int CoreAudioModuleProxy::playEffect(const char *path, float pitch, float noteVelocity)
{
    unsigned int soundID = AudioTool::getHashValue(path);
    return audioPool->play(soundID, noteVelocity, 0);
}

int CoreAudioModuleProxy::playEffect(NoteObject *noteObj)
{
    unsigned int soundID = AudioTool::getHashValue(noteObj->m_FullPath);
    int streamID = audioPool->play(soundID, noteObj, 0);
    return streamID;
}

int CoreAudioModuleProxy::startEffect()
{
	return 0;
}



