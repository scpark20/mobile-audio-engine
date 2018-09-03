#ifndef OpenSLAudioModule_H_
#define OpenSLAudioModule_H_

/*

 * OpenSLAudioModule.h
 *
 *  Created on: 2015. 5. 18.
 *      Author: IZSoft-PSC
 */
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include "../AudioCommon/AudioModule.h"
#include "../AudioCommon/AudioPool.h"

struct CallbackContext {
	AudioPool *audioPool;
	AudioModule *audioModule;
	SLBufferQueueItf bufferQueueItf;
	int sampleLength;
	SLPlayItf playItf;
	int playerID;
	short int* shortBuffer; // Current address of local audio data storage
};

#endif /* OpenSLAudioModule_H_ */
