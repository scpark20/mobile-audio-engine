//
//  IZ_Recorder.m
//  HappyPianistUS
//
//  Created by youngdoo kim on 2014. 4. 30..
//
//

#import "IZ_Record.h"

static IZ_Record *sharedRecordManager = nil;

@implementation IZ_Record

+ (IZ_Record *)getInstance {
	
	@synchronized(self) {
		
		if(sharedRecordManager == nil) {
			[[self alloc] init];
		}
	}
	
	return sharedRecordManager;
}

+ (id)allocWithZone:(NSZone *)zone {
    @synchronized(self) {
        if (sharedRecordManager == nil) {
            sharedRecordManager = [super allocWithZone:zone];
            return sharedRecordManager;
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
        [self initAudioSession];
    }
    return self;
}

- (void) destroy {
	@synchronized(self) {
		if(sharedRecordManager != nil) {
            [recorder release];
            [player release];
			[self dealloc];
		}
	}
}

#pragma mark Playback

- (void) playerStart
{
    
    NSError *audioError;
    
    player = [[AVAudioPlayer alloc] initWithContentsOfURL:recordedAudioURL error:&audioError];
    
    if (player == nil)
    {
        NSLog(@"error: playerStart error");
    } else {
        [player play];
    }
}

- (void) playerStop
{
    if (player != NULL)
        [player stop];
}

- (void) playerPause
{
    if (player != NULL)
        [player pause];
}

- (void) playerResume
{
    if (player != NULL)
        [player play];
}


#pragma mark Recording


- (BOOL) initAudioSession
{
    AudioSessionInitialize(NULL, NULL, NULL, NULL);
    
    UInt32 sessionCategory = kAudioSessionCategory_PlayAndRecord;
    AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(sessionCategory), &sessionCategory);
    UInt32 audioRouteOverride = kAudioSessionOverrideAudioRoute_Speaker;
    AudioSessionSetProperty(kAudioSessionProperty_OverrideAudioRoute, sizeof(audioRouteOverride), &audioRouteOverride);
    
    AudioSessionSetActive(true);
    
    return true;
}

- (void) recordSetup:(NSString*)_filePath;
{

    recordedAudioFileName = [NSString stringWithFormat:@"%@", _filePath];
    
    // sets the path for audio file
//    NSArray *pathComponents = [NSArray arrayWithObjects:
//                               [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES) lastObject],
//                               [NSString stringWithFormat:@"%@", recordedAudioFileName],
//                               nil];
    
    NSArray *pathComponents = [NSArray arrayWithObjects:
                               [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject],
                               [NSString stringWithFormat:@"%@", recordedAudioFileName],
                               nil];
    
    recordedAudioURL = [NSURL fileURLWithPathComponents:pathComponents];
    
    float sampleRate[] = {
        8000.0,
        11025.0,
        16000.0,
        22050.0,
        44100.0
    };
    
    // create a new NSMutableDictionary for the record settings
    NSMutableDictionary *recordSettings = [[NSMutableDictionary alloc] init];
                                    [recordSettings setValue:[NSNumber numberWithInt:kAudioFormatMPEG4AAC] forKey:AVFormatIDKey];
                                    [recordSettings setValue:[NSNumber numberWithFloat:sampleRate[4]] forKey:AVSampleRateKey];
                                    [recordSettings setValue:[NSNumber numberWithInt:2] forKey:AVNumberOfChannelsKey];
                                    [recordSettings setValue:[NSNumber numberWithInt:16] forKey:AVLinearPCMBitDepthKey];
                                    [recordSettings setValue:[NSNumber numberWithBool:NO] forKey:AVLinearPCMIsBigEndianKey];
                                    [recordSettings setValue:[NSNumber numberWithBool:NO] forKey:AVLinearPCMIsFloatKey];
    
    // initiate recorder
    NSError *error;
    recorder = [[AVAudioRecorder alloc] initWithURL:recordedAudioURL settings:recordSettings error:&error];
    
    if (error)
    {
        NSLog(@"error: %@", [error localizedDescription]);
    } else {
        
        [recorder setDelegate:self];
        recorder.meteringEnabled = YES;
        
        [self checkMic];
        
        if([recorder prepareToRecord])
        {
            NSLog(@"prepareToRecord yes ");
        }
        else
        {
            NSLog(@"prepareToRecord no ");
        }
        
        if([recorder record])
        {
            NSLog(@"record yes ");
        }
        else
        {
            NSLog(@"record no ");
        }
        
        levelTimer = [NSTimer scheduledTimerWithTimeInterval:0 target:self selector:@selector(listenForBlow:) userInfo: nil repeats: YES];
        
    }
}

- (void)levelTimerCallback:(NSTimer *)timer
{
	[recorder updateMeters];
    
	const double ALPHA = 0.05;
	double peakPowerForChannel = pow(10, (0.05 * [recorder peakPowerForChannel:0]));
	lowPassResults = ALPHA * peakPowerForChannel + (1.0 - ALPHA) * lowPassResults;
    
	NSLog(@"Average input: %f Peak input: %f Low pass results: %f", [recorder averagePowerForChannel:0], [recorder peakPowerForChannel:0], lowPassResults);
}


- (void)listenForBlow:(NSTimer *)timer
{
	[recorder updateMeters];
    
	const double ALPHA = 0.05;
	double peakPowerForChannel = pow(10, (0.05 * [recorder peakPowerForChannel:0]));
	lowPassResults = ALPHA * peakPowerForChannel + (1.0 - ALPHA) * lowPassResults;
    
//	NSLog(@"Average input: %f Peak input: %f Low pass results: %f", [recorder averagePowerForChannel:0], [recorder peakPowerForChannel:0], lowPassResults);
//	
//    if (lowPassResults > 0.95)
//    {
//        [self recordPause];
//		NSLog(@"Mic blow detected");
//    }
//    else
//    {
//        [self recordResume];
//    }
    if (lowPassResults > 0.95)
        NSLog(@"Mic blow detected");
}




- (void) timerRelease
{
    [levelTimer invalidate];
    levelTimer = nil;
}


- (void) recordStart:(NSString*)_filePath;
{
    [self recordSetup:_filePath];
}

- (void) recordPause
{
//    if ([recorder isRecording])
//    {
        [recorder pause];
//    }
}

- (void) recordResume
{
    [recorder record];
}

- (void) recordStop
{
//    if ([recorder isRecording])
//    {
        [recorder stop];
        [self timerRelease];
//    }
}

- (void) checkMic
{
//	if ([audioSession inputIsAvailable]) {
//        
//		NSLog(@"Mic connected");
//	}
//	else {
//		NSLog(@"Mic not connected");
//	}
}


- (void) audioRecorderDidFinishRecording:(AVAudioRecorder *)recorder successfully:(BOOL)flag
{
	if (flag) {
		
		NSLog(@"audioRecorderDidFinishRecording OK");
	}
	else {
		
		NSLog(@"audioRecorderDidFinishRecording ERROR");
	}
}

- (void) audioRecorderEncodeErrorDidOccur:(AVAudioRecorder *)recorder error:(NSError *)error
{
    NSLog(@"error: audioRecorderEncodeErrorDidOccur");
    
}

- (void)audioRecorderBeginInterruption:(AVAudioRecorder *)recorder
{
    NSLog(@"error: audioRecorderBeginInterruption");
    
}

/* audioRecorderEndInterruption:withOptions: is called when the audio session interruption has ended and this recorder had been interrupted while recording. */
/* Currently the only flag is AVAudioSessionInterruptionFlags_ShouldResume. */
- (void)audioRecorderEndInterruption:(AVAudioRecorder *)recorder withOptions:(NSUInteger)flags NS_AVAILABLE_IOS(6_0)
{
    NSLog(@"error: audioRecorderEndInterruption");

}

- (void)audioRecorderEndInterruption:(AVAudioRecorder *)recorder withFlags:(NSUInteger)flags NS_DEPRECATED_IOS(4_0, 6_0)
{
    NSLog(@"error: audioRecorderEndInterruption");
    
}
/* audioRecorderEndInterruption: is called when the preferred method, audioRecorderEndInterruption:withFlags:, is not implemented. */
- (void)audioRecorderEndInterruption:(AVAudioRecorder *)recorder NS_DEPRECATED_IOS(2_2, 6_0)
{
    NSLog(@"error: audioRecorderEndInterruption");
    
}

//- (BOOL)prefersStatusBarHidden
//{
//    return ![[NSUserDefaults standardUserDefaults] boolForKey:@"showStatus"];
//}
//
//- (UIStatusBarStyle)preferredStatusBarStyle
//{
//    return UIStatusBarStyleLightContent;
//}

@end
