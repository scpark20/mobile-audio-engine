//
//  IZ_Recorder.h
//  HappyPianistUS
//
//  Created by youngdoo kim on 2014. 4. 30..
//
//
#import <UIKit/UIKit.h>
#import <AVFoundation/AVFoundation.h>

@interface IZ_Record : NSObject <AVAudioRecorderDelegate>
{
    AVAudioPlayer           *player;
    AVAudioRecorder         *recorder;
    NSString                *recordedAudioFileName;
    NSURL                   *recordedAudioURL;
    NSTimer                 *levelTimer;
    double                  lowPassResults;
}

+ (IZ_Record *)             getInstance;

- (id)                      init;
- (void)                    destroy;

- (void)                    levelTimerCallback:(NSTimer *)timer;
- (void)                    listenForBlow:(NSTimer *)timer;
- (void)                    timerRelease;


- (BOOL)                    initAudioSession;

- (void)                    playerStart;
- (void)                    playerStop;
- (void)                    playerPause;
- (void)                    playerResume;

- (void)                    recordSetup:(NSString*)_filePath;
- (void)                    recordStart:(NSString*)_filePath;
- (void)                    recordStop;
- (void)                    recordPause;
- (void)                    recordResume;


@end
