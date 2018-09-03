#import "OALSimpleAudioEx.h"
#import "ObjectALMacros.h"
#import "ARCSafe_MemMgmt.h"
#import "OALAudioSession.h"
#import "OpenALManager.h"

// By default, reserve all 32 sources.
#define kDefaultReservedSources 32

#pragma mark -
#pragma mark Private Methods

SYNTHESIZE_SINGLETON_FOR_CLASS_PROTOTYPE(OALSimpleAudioEx);

@interface OALSimpleAudioEx (Private)

- (ALBuffer*) internalPreloadEffect:(NSString*) filePath reduceToMono:(bool) reduceToMono;

@end


#pragma mark -
#pragma mark OALSimpleAudioEx

@implementation OALSimpleAudioEx

#pragma mark Object Management

SYNTHESIZE_SINGLETON_FOR_CLASS(OALSimpleAudioEx);

@synthesize device;
@synthesize context;

+ (OALSimpleAudioEx*) sharedInstanceWithSources:(int) sources
{
	return as_autorelease([[self alloc] initWithSources:sources]);
}

+ (OALSimpleAudioEx*) sharedInstanceWithReservedSources:(int) reservedSources
                                          monoSources:(int) monoSources
                                        stereoSources:(int) stereoSources
{
    return as_autorelease([[self alloc] initWithReservedSources:reservedSources
                                                    monoSources:monoSources
                                                  stereoSources:stereoSources]);
}

- (id) init
{
	return [self initWithSources:kDefaultReservedSources];
}

- (void) initCommon:(int) reservedSources
{
    [OpenALManager sharedInstance].currentContext = context;
    channel = [[ALChannelSource alloc] initWithSources:reservedSources];

    audioTracks = [[NSMutableArray arrayWithCapacity:4] retain];
    [audioTracks addObject:[OALAudioTrack track]];
    [audioTracks addObject:[OALAudioTrack track]];
    [audioTracks addObject:[OALAudioTrack track]];
    [audioTracks addObject:[OALAudioTrack track]];
    

#if NS_BLOCKS_AVAILABLE && OBJECTAL_CFG_USE_BLOCKS
    oal_dispatch_queue	= dispatch_queue_create("objectal.simpleaudio.queue", NULL);
#endif
    pendingLoadCount	= 0;

    self.preloadCacheEnabled = YES;
}

- (id) initWithReservedSources:(int) reservedSources
                   monoSources:(int) monoSources
                 stereoSources:(int) stereoSources
{
	if(nil != (self = [super init]))
	{
		OAL_LOG_DEBUG(@"%@: Init with %d reserved sources, %d mono, %d stereo",
                      self, reservedSources, monoSources, stereoSources);
		device = [[ALDevice alloc] initWithDeviceSpecifier:nil];
        if(device == nil)
		{
			OAL_LOG_ERROR(@"%@: Could not create OpenAL device", self);
            goto initFailed;
		}
        context = [[ALContext alloc] initOnDevice:device
                                  outputFrequency:44100
                                 refreshIntervals:10
                               synchronousContext:FALSE
                                      monoSources:monoSources
                                    stereoSources:stereoSources];
        if(context == nil)
		{
			OAL_LOG_ERROR(@"%@: Could not create OpenAL context", self);
            goto initFailed;
		}
        [self initCommon:reservedSources];
	}
	return self;

initFailed:
    [[self class] purgeSharedInstance];
    return nil;
}

- (id) initWithSources:(int) reservedSources
{
	if(nil != (self = [super init]))
	{
		OAL_LOG_DEBUG(@"%@: Init with %d reserved sources", self, reservedSources);
		device = [[ALDevice alloc] initWithDeviceSpecifier:nil];
        if(device == nil)
		{
			OAL_LOG_ERROR(@"%@: Could not create OpenAL device", self);
            goto initFailed;
		}
        context = [[ALContext alloc] initOnDevice:device attributes:nil];
        if(context == nil)
		{
			OAL_LOG_ERROR(@"%@: Could not create OpenAL contextself", self);
            goto initFailed;
		}
        [self initCommon:reservedSources];
	}
	return self;

initFailed:
    [[self class] purgeSharedInstance];
    return nil;
}

- (void) dealloc
{
	OAL_LOG_DEBUG(@"%@: Dealloc", self);
#if NS_BLOCKS_AVAILABLE && OBJECTAL_CFG_USE_BLOCKS && !__has_feature(objc_arc)
    if(oal_dispatch_queue != nil)
    {
        dispatch_release(oal_dispatch_queue);
    }
#endif

    [audioTracks release];
    
//	as_release(backgroundTrack);
    
	[channel stop];
	as_release(channel);
	as_release(context);
	as_release(device);
	as_release(preloadCache);
	as_superdealloc();
}

#pragma mark Properties

- (NSUInteger) preloadCacheCount
{
	OPTIONALLY_SYNCHRONIZED(self)
	{
		return [preloadCache count];
	}
}

- (bool) preloadCacheEnabled
{
    return nil != preloadCache;
}

- (void) setPreloadCacheEnabled:(bool) value
{
	OPTIONALLY_SYNCHRONIZED(self)
	{
		if(value != self.preloadCacheEnabled)
		{
			if(value)
			{
				preloadCache = [[NSMutableDictionary alloc] initWithCapacity:64];
			}
			else
			{
				if(pendingLoadCount > 0)
				{
					OAL_LOG_WARNING(@"Attempted to turn off preload cache while pending loads are queued.");
					return;
				}
				else
				{
					as_release(preloadCache);
					preloadCache = nil;
				}
			}
		}
	}
}

- (bool) allowIpod
{
	return [OALAudioSession sharedInstance].allowIpod;
}

- (void) setAllowIpod:(bool) value
{
	[OALAudioSession sharedInstance].allowIpod = value;
}

- (bool) useHardwareIfAvailable
{
	return [OALAudioSession sharedInstance].useHardwareIfAvailable;
}

- (void) setUseHardwareIfAvailable:(bool) value
{
	[OALAudioSession sharedInstance].useHardwareIfAvailable = value;
}


- (int) reservedSources
{
	return channel.reservedSources;
}

- (void) setReservedSources:(int) value
{
	channel.reservedSources = value;
}

@synthesize channel;

- (float) effectsVolume
{
	return [OpenALManager sharedInstance].currentContext.listener.gain;
}

- (void) setEffectsVolume:(float) value
{
	OPTIONALLY_SYNCHRONIZED(self)
	{
		[OpenALManager sharedInstance].currentContext.listener.gain = value;
	}
}

- (bool) honorSilentSwitch
{
	return [OALAudioSession sharedInstance].honorSilentSwitch;
}

- (void) setHonorSilentSwitch:(bool) value
{
	[OALAudioSession sharedInstance].honorSilentSwitch = value;
}


#pragma mark Background Music

- (int) playAudio:(NSString*) filePath
{
	return [self playAudio:filePath loop:NO];
}

- (int) playAudio:(NSString*) filePath loop:(bool) loop
{
	if(nil == filePath)
	{
		OAL_LOG_ERROR(@"filePath was NULL");
		return NO;
	}
    
    OALAudioTrack* track = nil;
    int audioID = -1;
    for (int i = 0; i < [audioTracks count]; i++)
    {
        track = [audioTracks objectAtIndex:i];
        if(!track.playing)
        {
            audioID = i;
            
            track.numberOfLoops = loop ? -1 : 0;
            
            [track preloadFile:filePath];
            [track play];
            
            break;
        }
    }
    
    return audioID;
}

- (int) playAudio:(NSString*) filePath
		 volume:(float) volume
			pan:(float) pan
		   loop:(bool) loop
{
    OALAudioTrack* track = nil;
    int audioID = -1;
    
    for (int i = 0; i < [audioTracks count]; i++)
    {
        track = [audioTracks objectAtIndex:i];
        if(!track.playing)
        {
            audioID = i;
            
            track.numberOfLoops = loop ? -1 : 0;
            track.gain = volume;
            track.pan = pan;
            
            [track preloadFile:filePath];
            [track play];
            
            break;
        }
    }
    
    return audioID;
}

- (void) stopAudio:(int) audioID
{
    if (audioID > -1)
    {
        OALAudioTrack* track = [audioTracks objectAtIndex:audioID];
        [track stop];
    }
}

- (void) stopAllAudios
{
    OALAudioTrack* track = nil;
    
    for (int i = 0; i < [audioTracks count]; i++)
    {
        track = [audioTracks objectAtIndex:i];
        if(track.playing)
        {
            [track stop];
        }
    }
}

- (void) pauseAudio:(int) audioID
{
    if (audioID > -1)
    {
        OALAudioTrack* track = [audioTracks objectAtIndex:audioID];
        track.paused = true;
    }
}

- (void) pauseAllAudios
{
    OALAudioTrack* track = nil;
    
    for (int i = 0; i < [audioTracks count]; i++)
    {
        track = [audioTracks objectAtIndex:i];
        if(track.playing)
        {
            track.paused = true;
        }
    }
}

- (void) resumeAudio:(int) audioID
{
    if (audioID > -1)
    {
        OALAudioTrack* track = [audioTracks objectAtIndex:audioID];
        track.paused = false;
    }
}

- (void) resumeAllAudios
{
    OALAudioTrack* track = nil;
    
    for (int i = 0; i < [audioTracks count]; i++)
    {
        track = [audioTracks objectAtIndex:i];
        if(track.paused)
        {
            track.paused = false;
        }
    }
}

- (float) getAudioVolume:(int) audioID
{
    if (audioID > -1)
    {
        OALAudioTrack* track = [audioTracks objectAtIndex:audioID];
        return track.gain;
    }
    
    return 0.0f;
}

- (void) setAudioVolume:(int) audioID volume:(float) volume
{
    if (audioID > -1)
    {
        OALAudioTrack* track = [audioTracks objectAtIndex:audioID];
        track.gain = volume;
    }
}

- (double) getAudioCurrentTime:(int) audioID
{
    if (audioID > -1)
    {
        OALAudioTrack* track = [audioTracks objectAtIndex:audioID];
        return track.currentTime;
    }
    
    return 0.0;
}

- (void) setAudioCurrentTime:(int) audioID time:(double) time
{
    if (audioID > -1)
    {
        OALAudioTrack* track = [audioTracks objectAtIndex:audioID];
        [track setCurrentTime:(NSTimeInterval)time];
    }
}

- (double) getAudioDuration:(int)audioID
{
    if (audioID > -1)
    {
        OALAudioTrack* track = [audioTracks objectAtIndex:audioID];
        return track.duration;
    }
    
    return 0.0;
}

#pragma mark Sound Effects

- (NSString*) cacheKeyForBuffer:(ALBuffer*) buffer
{
    return buffer.name;
}

- (NSString*) cacheKeyForEffectPath:(NSString*) filePath
{
    return [[OALTools urlForPath:filePath] description];
}

- (ALBuffer*) internalPreloadEffect:(NSString*) filePath reduceToMono:(bool) reduceToMono
{
	ALBuffer* buffer;
    NSString* cacheKey = [self cacheKeyForEffectPath:filePath];
	OPTIONALLY_SYNCHRONIZED(self)
	{
		buffer = [preloadCache objectForKey:cacheKey];
	}
	if(nil == buffer)
	{
		OAL_LOG_DEBUG(@"Effect not in cache. Loading %@", filePath);
		buffer = [[OpenALManager sharedInstance] bufferFromFile:filePath reduceToMono:reduceToMono];
		if(nil == buffer)
		{
			OAL_LOG_ERROR(@"Could not load effect %@", filePath);
			return nil;
		}

        buffer.name = cacheKey;
		OPTIONALLY_SYNCHRONIZED(self)
		{
			[preloadCache setObject:buffer forKey:cacheKey];
		}
	}

	return buffer;
}

- (ALBuffer*) preloadEffect:(NSString*) filePath
{
	return [self preloadEffect:filePath reduceToMono:NO];
}

- (ALBuffer*) preloadEffect:(NSString*) filePath reduceToMono:(bool) reduceToMono
{
	if(nil == filePath)
	{
		OAL_LOG_ERROR(@"filePath was NULL");
		return nil;
	}

    if(pendingLoadCount > 0)
    {
        OAL_LOG_WARNING(@"You are loading an effect synchronously, but have pending async loads that have not completed. Your load will happen after those finish. Your thread is now stuck waiting. Next time just load everything async please.");
    }

#if NS_BLOCKS_AVAILABLE && OBJECTAL_CFG_USE_BLOCKS
	//Using blocks with the same queue used to asynch load removes the need for locking
	//BUT be warned that if you had called preloadEffects and then called this method, your app will stall until all of the loading is done.
	//It is advised you just always use async loading
	__block ALBuffer* retBuffer = nil;
	pendingLoadCount++;
	dispatch_sync(oal_dispatch_queue,
                  ^{
                      retBuffer = [self internalPreloadEffect:filePath reduceToMono:reduceToMono];
                  });
	pendingLoadCount--;
	return retBuffer;
#else
	return [self internalPreloadEffect:filePath reduceToMono:reduceToMono];
#endif
}

#if NS_BLOCKS_AVAILABLE && OBJECTAL_CFG_USE_BLOCKS

- (BOOL) preloadEffect:(NSString*) filePath
          reduceToMono:(bool) reduceToMono
       completionBlock:(void(^)(ALBuffer *)) completionBlock
{
	if(nil == filePath)
	{
		OAL_LOG_ERROR(@"filePath was NULL");
		completionBlock(nil);
		return NO;
	}

	pendingLoadCount++;
	dispatch_async(oal_dispatch_queue,
                   ^{
                       OAL_LOG_INFO(@"Preloading effect: %@", filePath);

                       ALBuffer *retBuffer = [self internalPreloadEffect:filePath reduceToMono:reduceToMono];
                       if(!retBuffer)
                       {
                           OAL_LOG_WARNING(@"%@ failed to preload.", filePath);
                       }
                       dispatch_async(dispatch_get_main_queue(),
                                      ^{
                                          completionBlock(retBuffer);
                                          pendingLoadCount--;
                                      });
                   });
	return YES;
}

- (void) preloadEffects:(NSArray*) filePaths
           reduceToMono:(bool) reduceToMono
		  progressBlock:(void (^)(NSUInteger progress, NSUInteger successCount, NSUInteger total)) progressBlock
{
	NSUInteger total = [filePaths count];
	if(total < 1)
	{
		OAL_LOG_ERROR(@"Preload effects: No files to process");
		progressBlock(0,0,0);
		return;
	}

	__block NSUInteger successCount = 0;

	pendingLoadCount += total;
	dispatch_async(oal_dispatch_queue,
                   ^{
                       [filePaths enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop)
                        {
                            #pragma unused(stop)
                            OAL_LOG_INFO(@"Preloading effect: %@", obj);
                            ALBuffer *result = [self internalPreloadEffect:(NSString *)obj reduceToMono:reduceToMono];
                            if(!result)
                            {
                                OAL_LOG_WARNING(@"%@ failed to preload.", obj);
                            }
                            else
                            {
                                successCount++;
                            }
                            NSUInteger cnt = idx+1;
                            dispatch_async(dispatch_get_main_queue(),
                                           ^{
                                               if(cnt == total)
                                               {
                                                   pendingLoadCount		-= total;
                                               }
                                               progressBlock(cnt, successCount, total);
                                           });
                        }];
                   });
}
#endif

- (bool) unloadEffect:(NSString*) filePath
{
	if(nil == filePath)
	{
		OAL_LOG_ERROR(@"filePath was NULL");
		return NO;
	}
    NSString* cacheKey = [self cacheKeyForEffectPath:filePath];
	OAL_LOG_DEBUG(@"Remove effect from cache: %@", filePath);
    bool isSuccess = YES;
	OPTIONALLY_SYNCHRONIZED(self)
	{
        isSuccess = [channel removeBuffersNamed:cacheKey];
        if(isSuccess)
        {
            [preloadCache removeObjectForKey:cacheKey];
        }
	}
    if(!isSuccess)
    {
        OAL_LOG_DEBUG(@"Could not remove effect from cache because it is still playing: %@", filePath);
    }
    return isSuccess;
}

- (void) unloadAllEffects
{
    OAL_LOG_DEBUG(@"Remove all effects from cache");
	OPTIONALLY_SYNCHRONIZED(self)
	{
        for(ALBuffer* buffer in [channel clearUnusedBuffers])
        {
            [preloadCache removeObjectForKey:[self cacheKeyForBuffer:buffer]];
        }
	}
}

- (int) playEffect:(NSString*) filePath
{
	return [self playEffect:filePath volume:1.0f pitch:1.0f pan:0.0f loop:NO];
}

- (int) playEffect:(NSString*) filePath loop:(bool) loop
{
    return  [self playEffect:filePath volume:1.0f pitch:1.0f pan:0.0f loop:loop];
}

- (int) playEffect:(NSString*) filePath
						  volume:(float) volume
						   pitch:(float) pitch
							 pan:(float) pan
							loop:(bool) loop
{
    if(nil == filePath)
	{
		OAL_LOG_ERROR(@"filePath was NULL");
		return -1;
	}
	ALBuffer* buffer = [self internalPreloadEffect:filePath reduceToMono:NO];
	if(nil != buffer)
	{
        return [[channel play:buffer gain:volume pitch:pitch pan:pan loop:loop] sourceId];
	}
    
	return -1;
}

- (id<ALSoundSource>) playBuffer:(ALBuffer*) buffer
						  volume:(float) volume
						   pitch:(float) pitch
							 pan:(float) pan
							loop:(bool) loop
{
	if(nil == buffer)
	{
		OAL_LOG_ERROR(@"buffer was NULL");
		return nil;
	}
	return [channel play:buffer gain:volume pitch:pitch pan:pan loop:loop];
}

- (void) stopEffect:(int) sourceId
{
    ALSource* source = (ALSource*)[channel getSource:sourceId];
    
    if (source != nil)
    {
        [source stop];
    }
}

- (void) stopAllEffects
{
    OAL_LOG_DEBUG(@"Stop all effects");
    [channel stop];
    [channel clearUnusedBuffers];
}

#pragma mark Utility

- (void) stopEverything
{
	[self stopAllEffects];
	[self stopAllAudios];
}

- (void) resetToDefault
{
	OAL_LOG_DEBUG(@"Reset to default");
	[self stopEverything];
	[channel resetToDefault];
	self.reservedSources = kDefaultReservedSources;
}

- (bool) manuallySuspended
{
	return [OALAudioSession sharedInstance].manuallySuspended;
}

- (void) setManuallySuspended:(bool) value
{
	[OALAudioSession sharedInstance].manuallySuspended = value;
}

- (bool) interrupted
{
	return [OALAudioSession sharedInstance].interrupted;
}

- (bool) suspended
{
	return [OALAudioSession sharedInstance].suspended;
}

- (void) setEffectVolume:(NSInteger) sourceId volume:(ALfloat) volume
{
    ALSource* source = (ALSource*)[channel getSource:(ALint)sourceId];
    
    if (source == nil)
        return;

    [source setVolume:volume];
}

- (float) getEffectVolume:(NSInteger) sourceId
{
    ALSource* source = (ALSource*)[channel getSource:(ALint)sourceId];
    
    if (source == nil)
        return 0.0f;
        
    return source.volume;
}

- (void) setEffectPitch:(NSInteger) sourceId pitch:(ALfloat) pitch
{
    ALSource* source = (ALSource*)[channel getSource:(ALint)sourceId];
    
    if (source == nil)
        return;
    
    [source setPitch:pitch];
}

- (float) getEffectPitch:(NSInteger) sourceId
{
    ALSource* source = (ALSource*)[channel getSource:(ALint)sourceId];
    
    if (source == nil)
        return -1.0f;
    
    return source.pitch;
}

- (void) performSelector:(SEL)selector afterDelay:(NSTimeInterval)delay withObjects:(id)obj, ...
{
    va_list args;
    va_start(args, obj);
    
    NSMethodSignature *sig = [self methodSignatureForSelector:selector];
    if (sig) {
        NSInvocation* invo = [NSInvocation invocationWithMethodSignature:sig];
        [invo setTarget:self];
        [invo setSelector:selector];
        
        int i=2;
        [invo setArgument:&obj atIndex:i];
        
        while ((obj = va_arg(args, id)) != nil) {
            [invo setArgument:&obj atIndex:++i];
        }
        
        [invo retainArguments];
        [invo performSelector:@selector(invoke) withObject:nil afterDelay:delay];
    }
    
    va_end(args);
}

- (void) performSelectorOnMainThread:(SEL)_selector withObjects:(id)_obj, ... NS_REQUIRES_NIL_TERMINATION
{
    va_list args;
    va_start(args, _obj);
    NSMethodSignature *sig = [self methodSignatureForSelector:_selector];
    
    if(sig)
    {
        NSInvocation* invo = [NSInvocation invocationWithMethodSignature:sig];
        [invo setTarget:self];
        [invo setSelector:_selector];
        
        int i = 2;
        [invo setArgument:&_obj atIndex:i];
        
        while ((_obj = va_arg(args, id)) != nil)
        {
            [invo setArgument:&_obj atIndex:++i];
            
        }
        
        [invo getArgument:&_obj atIndex:1];
        [invo retainArguments];
        [invo performSelectorOnMainThread:@selector(invoke) withObject:nil waitUntilDone:NO];
    }
    
    va_end(args);
}

@end
