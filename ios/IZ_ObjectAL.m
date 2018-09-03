//
//  IZ_ObjectAL.cpp
//  HappyPianistUS
//
//  Created by youngdoo kim on 2014. 4. 10..
//
//


#import "IZ_ObjectAL.h"

@implementation IZ_ObjectAL

static IZ_ObjectAL *sharedSoundManager = nil;

+ (IZ_ObjectAL *)getInstance {
	
	@synchronized(self) {
		
		if(sharedSoundManager == nil) {
			[[self alloc] init];
		}
	}
	
	return sharedSoundManager;
}

+ (id)allocWithZone:(NSZone *)zone {
    @synchronized(self) {
        if (sharedSoundManager == nil) {
            sharedSoundManager = [super allocWithZone:zone];
            return sharedSoundManager;
        }
    }
    return nil;
}

- (id)copyWithZone:(NSZone *)zone {
    return self;
}

- (id)init {
    
    if(nil != (self = [super init]))
    {
        objectAL = [OALSimpleAudio sharedInstance];
        
//        fadeFunction = [OALEaseAction easeFunctionForShape:kOALEaseShapeSine
//                                                     phase:kOALEaseInOut];

    }
    return self;
}

- (void) destroy {
	@synchronized(self) {
		if(sharedSoundManager != nil) {
            [objectAL release];
			[self dealloc];
		}
	}
}

- (BOOL) loadSound:(NSString*)_soundKey fileName:(NSString*)_fileName
{
    return [objectAL preloadEffect:_fileName];
}

- (void) stopAllEffect
{
    [objectAL stopAllEffects];
}

- (BOOL) unLoadSound:(NSString*)_soundKey
{
    return [objectAL unloadEffect:_soundKey];
}

- (BOOL) loadMusic:(NSString*)_musicKey fileName:(NSString*)_fileName
{
    return [objectAL preloadBg:_fileName];
}


- (int) playSound:(NSString*)_soundKey gain:(ALfloat)_gain pitch:(ALfloat)_pitch shouldLoop:(bool)_loop
{
    id<ALSoundSource> source = (id<ALSoundSource>)[objectAL playEffect:_soundKey loop:_loop];
    
    return [source getSourceId];
}

- (id<ALSoundSource>) playSound:(NSString*)_soundKey shouldLoop:(bool)_loop
{
    id<ALSoundSource> source = (id<ALSoundSource>)[objectAL playEffect:_soundKey loop:_loop];
    
    return source;
}

//- (void) stopSound:(NSString*)_soundKey
//{
//    [objectAL stopAllEffects];
//}

- (void) stopEffect:(int)_sourceId
{
    [objectAL stopEffect:_sourceId];
}



//- (void) playSoundFadeIn:(NSString*)_soundKey duration:(ALfloat)_duration
//{
//    [objectAL playEffectFadeIn:_soundKey duration:_duration];
//}
//
//- (void) playSoundFadeOut:(NSString*)_soundKey duration:(ALfloat) _duration
//{
//    [objectAL playEffectFadeOut:_soundKey duration:_duration];
//}

- (BOOL) playMusic:(NSString*)_musicKey loop:(bool)_loop
{
    return [objectAL playBg:_musicKey loop:_loop];
}

- (void) stopMusic
{
    [objectAL stopBg];
}

- (float) getMusicVolume
{
    return [objectAL bgVolume];
}

- (void) setMusicVolume:(ALfloat)_volume
{
    [objectAL setBgVolume:_volume];
}

- (void)dealloc
{
	[super dealloc];
}

- (void) pauseMusic
{
    [objectAL setPaused:YES];
}

- (void) resumeMusic
{
    [objectAL setPaused:NO];
}

- (BOOL) isMusicPlaying
{
    return [objectAL bgPlaying];
}

- (void) stopMusic:(BOOL)release
{
    [objectAL stopBg];
}

- (void) setEffectVolume:(ALfloat)_volume
{
    [objectAL setEffectsVolume:_volume];
}

- (ALfloat) getEffectVolume
{
    return [objectAL effectsVolume];
}

- (double) getMusicCurrentTime
{
    return [objectAL bgCurrentTime];
}

- (double) getMusicTotalTime
{
    return [objectAL bgTotalTime];
}

- (void) setMusicSeeking:(double) _seekTime
{
    [objectAL setBgCurrentTime:_seekTime];
}

//- (bool) preloadMusicStreaming:(NSURL*)_url
//{
//    return [objectAL preloadBgStreaming:_url];
//}

//- (bool) playMusicStreaming:(NSURL*)_url loop:(bool)_loop
//{
//    return [objectAL playBgStreaming:_url loop:_loop];
//}

//- (int) playEffectFadeOut:(NSString*)_fileName loopTime:(ALfloat)_loopTime
//{
//    return [objectAL playEffectFadeOut:_fileName duration:_loopTime];
//}

- (void) setVolume:(float)_volume sourceId:(NSInteger)_sourceId
{
    [objectAL setVolume:_volume sourceId:_sourceId];
}

- (float) getVolume:(NSInteger)_sourceId
{
    return [objectAL getVolume:_sourceId];
}

- (void) setPitch:(float)_pitch sourceId:(NSInteger)_sourceId
{
    [objectAL setPitch:_pitch sourceId:_sourceId];
}

- (float) getPitch:(NSInteger)_sourceId
{
    return [objectAL getPitch:_sourceId];
}

//- (void) setVelocity:(ALVector)_value sourceId:(NSInteger)_sourceId
//{
//    [objectAL setVelocity:_value sourceId:_sourceId];
//}
//
//- (ALVector) getVelocity:(NSInteger)_sourceId
//{
//    return [objectAL getVelocity:_sourceId];
//}


- (void) performSelector:(SEL)selector withObjects:(id)obj, ...
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
        [invo performSelector:@selector(invoke) withObject:nil];
    }
    
    va_end(args);
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

- (NSString*) getFullPath:(NSString*) _fileName
{
    NSString *firstString = [_fileName substringWithRange:NSMakeRange(0, 1)];
    
    NSString* file;
	NSString* fullPath;
    
    if ([firstString isEqualToString:@"/"])
    {
        file = [NSString stringWithFormat:@"%@", _fileName];
        fullPath = file;
    }
    else
    {
        file = [NSString stringWithFormat:@"/%@", _fileName];
        fullPath = [[NSBundle mainBundle] pathForResource:file ofType:nil];
    }
    
    return fullPath;
}

@end

