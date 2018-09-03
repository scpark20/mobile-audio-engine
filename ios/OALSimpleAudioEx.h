#import <Foundation/Foundation.h>
#import "SynthesizeSingleton.h"
#import "ALDevice.h"
#import "ALContext.h"
#import "ALSoundSource.h"
#import "ALChannelSource.h"
#import "OALAudioTrack.h"


#pragma mark OALSimpleAudioEx

@interface OALSimpleAudioEx : NSObject
{
	ALDevice* device;
	ALContext* context;

	ALChannelSource* channel;
	NSMutableDictionary* preloadCache;
    
#if NS_BLOCKS_AVAILABLE && OBJECTAL_CFG_USE_BLOCKS
	dispatch_queue_t oal_dispatch_queue;
#endif
	uint pendingLoadCount;
    NSMutableArray* audioTracks;
}


#pragma mark Properties

@property(nonatomic,readwrite,assign) bool allowIpod;

@property(nonatomic,readwrite,assign) bool useHardwareIfAvailable;

@property(nonatomic,readwrite,assign) bool honorSilentSwitch;

@property(nonatomic,readwrite,assign) int reservedSources;

@property(nonatomic,readonly,retain) ALDevice* device;

@property(nonatomic,readonly,retain) ALContext* context;

@property(nonatomic,readonly,retain) ALChannelSource* channel;

@property(nonatomic,readwrite,assign) bool preloadCacheEnabled;

@property(nonatomic,readonly,assign) NSUInteger preloadCacheCount;

@property(nonatomic,readwrite,assign) bool manuallySuspended;

@property(nonatomic,readonly,assign) bool interrupted;

@property(nonatomic,readonly,assign) bool suspended;



#pragma mark Object Management

SYNTHESIZE_SINGLETON_FOR_CLASS_HEADER(OALSimpleAudioEx);

+ (OALSimpleAudioEx*) sharedInstanceWithSources:(int) sources;

+ (OALSimpleAudioEx*) sharedInstanceWithReservedSources:(int) reservedSources
                                          monoSources:(int) monoSources
                                        stereoSources:(int) stereoSources;

- (id) initWithSources:(int) reservedSources;

- (id) initWithReservedSources:(int) reservedSources
                   monoSources:(int) monoSources
                 stereoSources:(int) stereoSources;


#pragma mark Long-play Audio

- (int) playAudio:(NSString*) path;

- (int) playAudio:(NSString*) path loop:(bool) loop;

- (int) playAudio:(NSString*) filePath
		 volume:(float) volume
			pan:(float) pan
		   loop:(bool) loop;

- (void) stopAudio:(int) audioID;

- (void) stopAllAudios;

- (void) pauseAudio:(int) audioID;

- (void) pauseAllAudios;

- (void) resumeAudio:(int) audioID;

- (void) resumeAllAudios;

- (float) getAudioVolume:(int) audioID;

- (void) setAudioVolume:(int) audioID volume:(float) volume;

- (double) getAudioCurrentTime:(int) audioID;

- (void) setAudioCurrentTime:(int) audioID time:(double) time;

- (double) getAudioDuration:(int) audioID;



#pragma mark Sound Effects

- (ALBuffer*) preloadEffect:(NSString*) filePath;

- (ALBuffer*) preloadEffect:(NSString*) filePath reduceToMono:(bool) reduceToMono;

#if NS_BLOCKS_AVAILABLE && OBJECTAL_CFG_USE_BLOCKS

- (BOOL) preloadEffect:(NSString*) filePath
				  reduceToMono:(bool) reduceToMono
	   completionBlock:(void(^)(ALBuffer *)) completionBlock;

- (void) preloadEffects:(NSArray*) filePaths
				   reduceToMono:(bool) reduceToMono
		  progressBlock:(void (^)(NSUInteger progress, NSUInteger successCount, NSUInteger total)) progressBlock;

#endif

- (bool) unloadEffect:(NSString*) filePath;

- (void) unloadAllEffects;

- (int) playEffect:(NSString*) filePath;

- (int) playEffect:(NSString*) filePath loop:(bool) loop;

- (int) playEffect:(NSString*) filePath
						volume:(float) volume
						 pitch:(float) pitch
						   pan:(float) pan
						  loop:(bool) loop;

- (id<ALSoundSource>) playBuffer:(ALBuffer*) buffer
						  volume:(float) volume
						   pitch:(float) pitch
							 pan:(float) pan
							loop:(bool) loop;

- (void) setEffectVolume:(NSInteger) sourceId volume:(ALfloat) volume;

- (float) getEffectVolume:(NSInteger) sourceId;

- (void) setEffectPitch:(NSInteger) sourceId pitch:(ALfloat) pitch;

- (float) getEffectPitch:(NSInteger) sourceId;

- (void) stopEffect:(int) sourceId;

- (void) stopAllEffects;


#pragma mark Utility

- (void) stopEverything;

- (void) resetToDefault;


- (void) performSelector:(SEL)selector afterDelay:(NSTimeInterval)delay withObjects:(id)obj, ...;

- (void) performSelectorOnMainThread:(SEL)_selector withObjects:(id)_obj, ... NS_REQUIRES_NIL_TERMINATION;


@end
