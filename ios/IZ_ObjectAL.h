//
//  IZ_ObjectAL.h
//  HappyPianistUS
//
//  Created by youngdoo kim on 2014. 4. 10..
//
//

#ifndef __HappyPianistUS__IZ_ObjectAL__
#define __HappyPianistUS__IZ_ObjectAL__

#import <Foundation/Foundation.h>

#import "ObjectAL.h"
#import "OALSimpleAudio.h"
#import "IZ_Record.h"

@interface IZ_ObjectAL : NSObject {
    
    OALSimpleAudio*         objectAL;
    
}

+ (IZ_ObjectAL *)           getInstance;

- (id)                      init;
- (void)                    destroy;

/* fx */
- (int)                     playSound:(NSString*)_soundKey gain:(ALfloat)_gain pitch:(ALfloat)_pitch shouldLoop:(bool)_loop;
- (BOOL)                    loadSound:(NSString*)_soundKey fileName:(NSString*)_fileName;

- (BOOL)                    unLoadSound:(NSString*)_soundKey;
- (void)                    setEffectVolume:(ALfloat)_volume;
- (ALfloat)                 getEffectVolume;
- (void)                    stopAllEffect;
- (void)                    stopEffect:(int)_sourceId;

- (BOOL)                    playMusic:(NSString*)_musicKey loop:(bool)_loop;
- (BOOL)                    loadMusic:(NSString*)_musicKey fileName:(NSString*)_fileName;
- (void)                    setMusicVolume:(ALfloat)_volume;
- (ALfloat)                 getMusicVolume;
- (void)                    pauseMusic;
- (void)                    resumeMusic;
- (BOOL)                    isMusicPlaying;
- (void)                    stopMusic:(BOOL)_release;
- (double)                  getMusicTotalTime;
- (double)                  getMusicCurrentTime;
- (void)                    setMusicSeeking:(double) _seekTime;
- (void)                    performSelector:(SEL)selector withObjects:(id)obj, ...;
- (void)                    performSelector:(SEL)selector afterDelay:(NSTimeInterval)delay withObjects:(id)obj, ...;
- (void)                    performSelectorOnMainThread:(SEL)selector withObjects:(id)obj, ... NS_REQUIRES_NIL_TERMINATION;

- (void)                    setVolume:(float)_volume sourceId:(NSInteger)_sourceId;
- (float)                   getVolume:(NSInteger)_sourceId;

- (void)                    setPitch:(float)_pitch sourceId:(NSInteger)_sourceId;
- (float)                   getPitch:(NSInteger)_sourceId;
/* utility */
- (NSString*)               getFullPath:(NSString*) _fileName;


@end


#endif /* defined(__HappyPianistUS__IZ_ObjectAL__) */
