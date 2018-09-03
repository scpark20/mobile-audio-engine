//
//  CoreAudioEngine.cpp
//  Musician
//
//  Created by musician on 2015. 4. 29..
//
//

#import "CoreAudioEngine.h"
#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioToolbox.h>
#import "../AudioCommon/AudioPool.h"
#import "../AudioCommon/Sound.h"

#define NUM_CHANNELS 2
#define NUM_BUFFERS 3
#define BUFFER_SIZE 512
#define SAMPLE_TYPE short
#define MAX_NUMBER 32767
#define SAMPLE_RATE 44100

unsigned int GetHashValue(const char *key)
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

@interface CoreAudioEngineIos : NSObject

-(id) init:(int) systemSampleRate :(int)streamSize :(int)minBufferSampleLength :(int)maxBufferSampleLength;
-(int) load:(const char*)path :(int)offset :(int)length;
-(int) unload:(const char*)path;
-(int) play:(const char*)path :(float)pitch :(float)userVolume :(float)noteVelocity;
-(int) play:(NoteObject*)noteObj :(float)userVolume;
-(void) fadeStopEffect:(int)streamID :(float)time;
-(int) stopStream:(int) streamID;
-(int) stopAllStream;
-(float) getStreamVolume:(int) streamID;
-(int) setStreamVolume:(int) streamID :(float) userVolume :(float) noteVelocity;
-(void) cleanUp;

@end

@implementation CoreAudioEngineIos
{
@public
    AudioPool *audioPool;
    SuperpoweredDecoder *decoder;
    AudioQueueRef outputQueue;
    AudioQueueBufferRef buffers[NUM_BUFFERS];
    
}

void OutputBufferCallback (void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer)
{
    [(CoreAudioEngineIos *)inUserData processOutputBuffer:inBuffer queue:inAQ];
}

-(id)init:(int) systemSampleRate :(int)streamSize :(int)minBufferSampleLength :(int)maxBufferSampleLength
{
    self = [super init];
    if (!self) return nil;
    
    audioPool = new AudioPool(SAMPLE_RATE, 1, streamSize, BUFFER_SIZE, BUFFER_SIZE);
    
    decoder = new SuperpoweredDecoder();
    
    [self startOutputAudioQueue];
    
    return self;
}

-(int) load:(const char*)path :(int)offset :(int)length
{
    unsigned int soundID = GetHashValue(path);
    
    if(audioPool->getSound(soundID)!=nullptr)
        return -1;
    
    Sound* sound = new Sound(path, 0, 0, SAMPLE_RATE);
    if(sound->getFloatBuffer()==NULL)
    	return -1;
    else	
    	return audioPool->load(soundID, sound);
}

-(int) unload:(const char *)path
{
    unsigned int soundID = GetHashValue(path);
    return audioPool->unload(soundID);
}

-(int) play:(const char*)path :(float)pitch :(float)userVolume :(float)noteVelocity
{
    unsigned int soundID = GetHashValue(path);
    return audioPool->play(soundID, pitch, userVolume, noteVelocity);
}

-(int) play:(NoteObject*)noteObj :(float)userVolume
{
    unsigned int soundID = GetHashValue(noteObj->m_FullPath);
    return audioPool->play(soundID, noteObj, userVolume);
}

-(void) fadeStopEffect:(int)streamID :(float)time
{
    audioPool->fadeStopStream(streamID, time);
}

-(int) stopStream:(int) streamID
{
    return audioPool->stopStream(streamID);
}

-(int) stopAllStream
{
    return audioPool->stopAllStream();
}

-(float) getStreamVolume:(int) streamID
{
    return audioPool->getStreamVolume(streamID);
}

-(int) setStreamVolume:(int) streamID :(float) userVolume :(float) noteVelocity
{
    return audioPool->setStreamVolume(streamID, noteVelocity);
}

- (void)startOutputAudioQueue
{
    OSStatus err;
    int i;
    
    // Set up stream format fields
    AudioStreamBasicDescription format;
    format.mSampleRate = SAMPLE_RATE;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    format.mBitsPerChannel = 8 * sizeof(SAMPLE_TYPE);
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
        err = AudioQueueAllocateBuffer (outputQueue, BUFFER_SIZE * 4, &buffers[i]);
        if (err == noErr) {
            // Fill buffer.
            char pcmData[BUFFER_SIZE * 4] = {0,};
            memcpy(buffers[i]->mAudioData, pcmData, BUFFER_SIZE * 4);
            buffers[i]->mAudioDataByteSize = BUFFER_SIZE * 4;
            err = AudioQueueEnqueueBuffer (outputQueue, buffers[i], 0, nil);
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
    audioPool->process((short*)buffer->mAudioData);
    
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
}

@end


//-----------------------------------------------------------------------------

CoreAudioEngine::CoreAudioEngine()
{
    
}

CoreAudioEngine::CoreAudioEngine(int systemSampleRate, int streamSize, int minBufferSampleLength, int maxBufferSampleLength)
{
    _engine = [[CoreAudioEngineIos alloc] init:systemSampleRate :streamSize :minBufferSampleLength :maxBufferSampleLength];
}

CoreAudioEngine::~CoreAudioEngine()
{
    
}

int CoreAudioEngine::load(const char *path, int offset, int length)
{
    return [(CoreAudioEngineIos*)_engine load:path :offset :length];
}

int CoreAudioEngine::unload(const char *path)
{
    return [(CoreAudioEngineIos*)_engine unload:path];
}

int CoreAudioEngine::play(const char *path, float pitch, float userVolume, float noteVelocity)
{
    return [(CoreAudioEngineIos*)_engine play:path :pitch :userVolume :noteVelocity];
}

int CoreAudioEngine::play(NoteObject *noteObj, float userVolume)
{
    return [(CoreAudioEngineIos*)_engine play:noteObj :userVolume];
}

void CoreAudioEngine::fadeStopEffect(int streamID, float time)
{
    [(CoreAudioEngineIos*)_engine fadeStopEffect:streamID :time];
}

int CoreAudioEngine::stopStream(int streamID)
{
    return [(CoreAudioEngineIos*)_engine stopStream:streamID];
}

int CoreAudioEngine::stopAllStream()
{
    return [(CoreAudioEngineIos*)_engine stopAllStream];
}

float CoreAudioEngine::getStreamVolume(int streamID)
{
    return [(CoreAudioEngineIos*)_engine getStreamVolume:streamID];
}

int CoreAudioEngine::setStreamVolume(int streamID, float userVolume, float noteVelocity)
{
    return [(CoreAudioEngineIos*)_engine setStreamVolume:streamID :userVolume :noteVelocity];
}

void CoreAudioEngine::release()
{
    
}



