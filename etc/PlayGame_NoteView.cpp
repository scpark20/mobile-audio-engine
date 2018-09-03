#include "Public.h"
#include "Config.h"
#include "PlayGame_NoteView.h"
#include "PlayGame_UiView.h"
#include "PlayGame.h"
#include "GameMain.h"
#include "NoteObject.h"
#include "EffectObjectPool.h"
#include "EffectObject.h"
#include "GameDB.h"

USING_NS_CC;

PlayGame_NoteView::PlayGame_NoteView() :
m_actionNoteEffectIdx(0) {
}

PlayGame_NoteView::~PlayGame_NoteView() {

}

bool PlayGame_NoteView::init() {
	Layer::init();

	//노트 레이어 3D로 기울이기
	{
		/* MAX 기준으로 잡은 Layer의 포지션 및 Rotation 값.				*/
		/* MAX 좌표계와 OpenGL 좌표계가 다름으로 변환을 필요로 한다.	*/
		/* Position : MAX(x, y, z) => COCOS(x, z, -y)					*/
		/* Rotation : MAX(x, y, z) => COCOS(-x, z, y)					*/
		/* Cococs에 현재 셋팅되어 있는 값은 변환되어 있는 좌표값 입니다.*/
		this->setRotation3D(cocos2d::Vec3(-49.0f, 0.0f, 0.0f));
	}

#ifdef READ_DOWNLOAD
#else
	//피아노
	PTR_GAMEMAIN->m_SelectChannel = 0;
	PTR_GAMEMAIN->m_SelectLevel = 2;
	PTR_GAMEMAIN->m_SelectMusicIdx = 10;
#endif
	initValue();
	loadNoteImage();
	loadNoteData();
	loadNoteLayer();

	m_touchListener = EventListenerTouchAllAtOnce::create();
	auto touchListener = m_touchListener;

	touchListener->onTouchesBegan = CC_CALLBACK_2(PlayGame_NoteView::touchesBegan, this);
	touchListener->onTouchesMoved = CC_CALLBACK_2(PlayGame_NoteView::touchesMove, this);
	touchListener->onTouchesEnded = CC_CALLBACK_2(PlayGame_NoteView::touchesEnd, this);

	this->getEventDispatcher()->addEventListenerWithSceneGraphPriority(touchListener, this);

	loadNoteEffect();
	return true;
}

bool PlayGame_NoteView::loadEffectSound() 
{
	PlayGame* pGame = (PlayGame*) this->getParent();
	//사운드 로딩 쓰레드로 따로 돌리기
	int max = 0;

	max = m_preLoadingIdx + 5;

	if (max > m_selMaxCnt[m_preLoadingChennel])
	{
		max = m_selMaxCnt[m_preLoadingChennel];
	}

	for (; m_preLoadingIdx < max; m_preLoadingIdx++)
	{
		NoteObject * _note = m_note[m_preLoadingChennel][m_preLoadingIdx];
		if (isMuteNote(_note) == false) 
		{
			preloadPlayEffect(_note, m_preLoadingChennel, m_preLoadingIdx, PTR_GAMEMAIN->m_MusicInfoInstType[m_preLoadingChennel]);
		}
	}

	char str[255] = { '\0' };
	int per = ((m_preLoadingIdx * 100) / m_selMaxCnt[m_preLoadingChennel]);
	if (PTR_GAMEMAIN->m_MusicInfoInstType[m_preLoadingChennel] == (int)InstType::PIANO)
	{
		sprintf(str, "setting the Piano   :   %d%%", per);
	}
	else if (PTR_GAMEMAIN->m_MusicInfoInstType[m_preLoadingChennel] == (int)InstType::GUITAR)
	{
		sprintf(str, "setting the Guitar   :   %d%%", per);
	}
	else if (PTR_GAMEMAIN->m_MusicInfoInstType[m_preLoadingChennel] == (int)InstType::DRUM)
	{
		sprintf(str, "setting the Drum   :   %d%%", per);
	}
	pGame->setLoadingText(str);

	if (m_preLoadingIdx == m_selMaxCnt[m_preLoadingChennel]) 
	{
		m_preLoadingChennel++;
		m_preLoadingIdx = 0;
		if (m_preLoadingChennel == PTR_GAMEMAIN->m_trackCount) 
		{
			return true;
		}
	}
	else 
	{
		if (m_preLoadingChennel == PTR_GAMEMAIN->m_trackCount - 1)
		{
			if (m_preLoadingIdx >= (m_selMaxCnt[m_preLoadingChennel] - 20))
			{
				log("++++ Start the Game ");
				pGame->setLoadingText("Start the Game");
			}
		}
	}

	return false;
}

void PlayGame_NoteView::onEnter() {
	Layer::onEnter();
	log("PlayGame_NoteView::onEnter()  ");
}

void PlayGame_NoteView::onExit() {
	Layer::onExit();

	//TweenerManager::getInstance()->removeAll();

	this->getEventDispatcher()->removeEventListenersForTarget(this);

	//	IZAudioEngine::getInstance()->stopBackgroundMusic(true);
	IZAudioEngine::getInstance()->stopAllEffects();

	char szFullPath[512];

	for (int i = 0; i < PTR_GAMEMAIN->m_trackCount; i++) {
		vector<NoteObject*> tmp = m_note[i];
		for (int k = 0; k < m_selMaxCnt[i]; k++) {
			if (k < tmp.size()) {
				if (effectPath(szFullPath, i, m_note[i][k]->m_effIndex,
					m_note[i][k]->m_NoteFx) == true) {
					IZAudioEngine::getInstance()->unloadEffect(szFullPath);
					//log("onExit() unloadEffect() szFullPath:%s  ", szFullPath);
				}
			}
		}

		vector<NoteObject*>::iterator _it = m_note[i].begin();

		while (_it != m_note[i].end()) {
			delete (*_it);
			_it = m_note[i].erase(_it);
		}
		m_note[i].clear();
	}
    
    IZAudioEngine::getInstance()->releaseMR();

#ifdef NEW_EFFECT_PROC
	CC_SAFE_DELETE(skeletonDataEffect);
#else
	destoryEffectObjArray();
#endif

	CC_SAFE_DELETE(midiInfo);

	//CC_SAFE_DELETE(skeletonDataFever);
}

void PlayGame_NoteView::noteClearForPreview(float _fTime) {

	float fDeleteTime(
		(float)(_fTime / midiInfo->GetBeatPerMS()) * pixelPerBeat);
	int startIdx = m_curNoteIdx[PTR_GAMEMAIN->m_SelectChannel];
	int maxCheckCnt = getMaxProcNoteCount(PTR_GAMEMAIN->m_SelectChannel);

	for (int n = 0;
		n
		< midiInfo->trackChunk->midiEvent[PTR_GAMEMAIN->m_SelectChannel].midiData.size();
	n++) {
		NoteObject* note = m_note[PTR_GAMEMAIN->m_SelectChannel][n];

		CCLOG(
			"[PlayGame_NoteView::noteClearForPreview] note->m_time = %.2f / %.2f", note->m_time, fDeleteTime);
		if (note && note->m_time < _fTime) {

			if (note->m_sprNote) {
				note->m_sprNote->setVisible(false);
			}
			if (note->m_sprLine) {
				note->m_sprLine->setVisible(false);
			}

			note->m_isNotTouch = true;
			note->m_isNoteVisible = false;
			note->m_effPlay = false;
			note->m_isLineVisible = false;
			//note->m_isDelete = true;
			CCLOG(
				"[PlayGame_NoteView::noteClearForPreview] note->m_time = %d / %d", n, midiInfo->trackChunk->midiEvent[PTR_GAMEMAIN->m_SelectChannel].midiData.size());

		}
	}

}

void PlayGame_NoteView::loadNoteEffect() {
	int InstIdx = 0;

	if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::PIANO) {
		InstIdx = PTR_GAMEMAIN->m_SelectPiano;
	}
	else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::GUITAR) {
		InstIdx = PTR_GAMEMAIN->m_SelectGuitar;
	}
	else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::DRUM) {
		InstIdx = PTR_GAMEMAIN->m_SelectDrum;
	}

	if (PTR_GAMEMAIN->m_isSampler == true) {
		InstIdx = 400;
	}

#ifdef NEW_EFFECT_PROC
	{
		char jsonFile[256];
		char atlasFile[256];

		//악기 타입별루 다르게 처리
		if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::PIANO)//PIANO
		{
			sprintf(jsonFile, "res/ingame/effect/piano_eff.json");
			sprintf(atlasFile, "res/ingame/effect/piano_eff.atlas");
		}
		else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::GUITAR) //GUITAR
		{
			if (InstIdx == 5 || InstIdx == 6)
			{
				sprintf(jsonFile, "res/ingame/effect/electronic_guitar_eff.json");
				sprintf(atlasFile, "res/ingame/effect/electronic_guitar_eff.atlas");
			}
			else
			{
				sprintf(jsonFile, "res/ingame/effect/guitar_eff.json");
				sprintf(atlasFile, "res/ingame/effect/guitar_eff.atlas");
			}
		}
		else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::DRUM) //DRUM
		{
			sprintf(jsonFile, "res/ingame/effect/drums_eff.json");
			sprintf(atlasFile, "res/ingame/effect/drums_eff.atlas");
		}

		m_nowEffectIdx = 0;

		spAtlas* atlas = spAtlas_createFromFile(atlasFile, 0);
		CCASSERT(atlas, "Error reading atlas file.");

		spSkeletonJson* json = spSkeletonJson_create(atlas);

		skeletonDataEffect = spSkeletonJson_readSkeletonDataFile(json, jsonFile);

		CCASSERT(skeletonDataEffect, json->error ? json->error : "Error reading skeleton data file.");

		spSkeletonJson_dispose(json);

		for (int i = 0; i < MAX_EFFECT_CNT; i++)
		{
			//auto spineEffect = spine::SkeletonAnimation::createWithFile(jsonFile, atlasFile, 1.0f);
			auto spineEffect = spine::SkeletonAnimation::createWithData(skeletonDataEffect);
			this->addChild(spineEffect, TAG_NOTE_EFFECT_LAYER + i, TAG_NOTE_EFFECT_LAYER + i);
			spineEffect->setVisible(false);
		}

		CC_SAFE_DELETE(atlas);
	}
#else
	m_effectObjPool = new (std::nothrow) EffectObjectPool();

	if (m_effectObjPool == NULL) {
		//CCASSERT(false, "m_effectObjPool == NULL!!");
	}
	else if (m_effectObjPool->init(MAX_EFFECT_CNT,
		PTR_GAMEMAIN->m_GameInstrumType[0], InstIdx) == false) {
		//CCASSERT(false, "EffectObjectPool init failed!!");
	}

	m_effectObjArray.reserve(MAX_EFFECT_CNT);
	//m_noteObjRemoveEvent = std::bind(&PlayGame_NoteView::setEffectObjArray, this, std::placeholders::_1);

#endif
}

void PlayGame_NoteView::initValue() {
	m_pTimeUpdate = 0;
	midiInfo = NULL;

	m_iSelTotalCnt = 0;
	memset(m_curNoteIdx, 0, sizeof(m_curNoteIdx));

	m_perfectPosition = PERFECT_POSITION_WIDE;

	if (PTR_GAMEMAIN->m_IsWide == false) {
		m_perfectPosition = PERFECT_POSITION;
	}

	//memset(m_notePlayTime, 0, sizeof(m_notePlayTime));
	//memset(m_notePlayID, 0, sizeof(m_notePlayID));
	for (int ch_i = 0; ch_i < MAX_TRACK_CNT; ch_i++) 
	{
		for (int i = 0; i < MAX_STOP_CNT; i++)
		{
			m_notePlayTime[ch_i][i] = 0;
			m_notePlayID[ch_i][i] = 0;
		}
	}

	m_pTimeUpdateBefore = m_pTimeUpdate = 0.0f;

	initTouchValue();

	/*
	 for (int i = 0; i < 3; i++)
	 {
	 for (int j = 0; j < 1000; j++)
	 {
	 m_isLoading[i][j] = false;
	 }
	 }
	 */

	m_preLoadingChennel = 0;
	m_preLoadingIdx = 0;

	m_isSkanUp = true;
	m_isAniBlink = false;
}

void PlayGame_NoteView::initTouchValue() {
	for (int i = 0; i < MAX_TOUCHES; i++) {
		touchMgr.touches[i].id = 0;
		touchMgr.touches[i].px = 0;
		touchMgr.touches[i].py = 0;
		touchMgr.touches[i].realX = 0;
		touchMgr.touches[i].realY = 0;
		touchMgr.touches[i].active = false;
		touchMgr.touches[i].type = TOUCH_TYPE_NONE;
		touchMgr.touches[i].effIdx = -1;

		touchMgr.touches[i].groupID[0] = -1;
		touchMgr.touches[i].groupID[1] = -1;
	}

	for (int i = 0; i < MAX_COMPARE_VALUE; i++) {
		PTR_GAMEMAIN->m_compare[i].key = 0;
		PTR_GAMEMAIN->m_compare[i].value = 0;
	}
}

void PlayGame_NoteView::destoryEffectObjArray() {
#ifndef	NEW_EFFECT_PROC
	EffectObjArrayIt it = m_effectObjArray.begin();
	for (it; it != m_effectObjArray.end(); ++it) {
		EffectObject* effectObject = (*it);
		this->removeChild(effectObject->getSkeletonAnimation());
		//m_effectObjPool->pushEffectObject(effectObject);
	}
	m_effectObjArray.clear();
	CC_SAFE_DELETE(m_effectObjPool);
#endif
}

void PlayGame_NoteView::loadNoteImage() {
	SpriteFrameCache::getInstance()->addSpriteFramesWithFile(
		"res/ingame/note_pack.plist");
	this->addChild(SpriteBatchNode::create("note_pack.png"), TAG_NOTE_IMG_PACK,
		TAG_NOTE_IMG_PACK);

	SpriteFrameCache::getInstance()->addSpriteFramesWithFile(
		"res/ingame/sampler_note_pack.plist");
	this->addChild(SpriteBatchNode::create("sampler_note_pack.png"),
		TAG_NOTE_IMG_PACK_SAMPLER, TAG_NOTE_IMG_PACK_SAMPLER);


	this->addChild(SpriteBatchNode::create("res/ingame/touch/piano_box.png"), TAG_NOTE_IMG_PIANO_BOX, TAG_NOTE_IMG_PIANO_BOX);
	this->addChild(SpriteBatchNode::create("res/ingame/touch/piano_touch.png"), TAG_NOTE_IMG_PIANO_TOUCH, TAG_NOTE_IMG_PIANO_TOUCH);

	this->addChild(SpriteBatchNode::create("res/ingame/touch/guitar_box.png"), TAG_NOTE_IMG_GUITAR_BOX, TAG_NOTE_IMG_GUITAR_BOX);
	this->addChild(SpriteBatchNode::create("res/ingame/touch/guitar_touch.png"), TAG_NOTE_IMG_GUITAR_TOUCH, TAG_NOTE_IMG_GUITAR_TOUCH);

	this->addChild(SpriteBatchNode::create("res/ingame/touch/snare_box.png"), TAG_NOTE_IMG_DRUM_BOX, TAG_NOTE_IMG_DRUM_BOX);
	this->addChild(SpriteBatchNode::create("res/ingame/touch/snare_touch.png"), TAG_NOTE_IMG_DRUM_TOUCH, TAG_NOTE_IMG_DRUM_TOUCH);

	this->addChild(SpriteBatchNode::create("res/ingame/touch/sampler_box.png"), TAG_NOTE_IMG_SAMPLER_BOX, TAG_NOTE_IMG_SAMPLER_BOX);
	this->addChild(SpriteBatchNode::create("res/ingame/touch/sampler_touch.png"), TAG_NOTE_IMG_SAMPLER_TOUCH, TAG_NOTE_IMG_SAMPLER_TOUCH);


	this->addChild(SpriteBatchNode::create("res/ingame/drum_note_line.png"), TAG_NOTE_IMG_DRUM_SAME_LINE, TAG_NOTE_IMG_DRUM_SAME_LINE);
	this->addChild(SpriteBatchNode::create("res/ingame/section_line.png"), TAG_NOTE_IMG_SECTION_LINE, TAG_NOTE_IMG_SECTION_LINE);


	//Sprite::create("res/ingame/cloud.png");
	this->addChild(SpriteBatchNode::create("res/ingame/cloud.png"), TAG_NOTE_IMG_CLOUD, TAG_NOTE_IMG_CLOUD);
	this->addChild(SpriteBatchNode::create("res/ingame/distEffect/ingame_disturbance_vfx_head.png"), TAG_NOTE_IMG_CLOUD_2, TAG_NOTE_IMG_CLOUD_2);



	this->addChild(SpriteBatchNode::create("res/ingame/highlight_head.png"), TAG_NOTE_IMG_HIGHLIGHT_HEAD, TAG_NOTE_IMG_HIGHLIGHT_HEAD);
	this->addChild(SpriteBatchNode::create("res/ingame/highlight_mid.png"), TAG_NOTE_IMG_HIGHLIGHT_MID, TAG_NOTE_IMG_HIGHLIGHT_MID);
}

void PlayGame_NoteView::loadNoteLayer() {
	float Psize = NOTESCENE_WIDTH;
	float len = 3200.0f; //Director::getInstance()->getVisibleSize().height*3.0f;

	//퍼펙트라인
	char _name[256] = { 0, };
	sprintf(_name, "res/ingame/touch_line.png");
	auto sprTest = Sprite::create(_name);
	sprTest->setAnchorPoint(Point(0.5f, 0.5f));
	sprTest->setPosition(
		Point(Director::getInstance()->getVisibleSize().width / 2,
		m_perfectPosition));
	sprTest->setScaleX(
		(float)(PTR_GAMEMAIN->m_WinSize.width
		/ sprTest->getContentSize().width) * 2.0f);
	this->addChild(sprTest, TAG_NOTE_BG_PERFECT, TAG_NOTE_BG_PERFECT);
	sprTest->setCameraMask((unsigned short)CameraFlag::USER1);

	//각 악기별 라인그리기
	if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::GUITAR) {
		if (PTR_GAMEMAIN->m_isSampler == true) {
			Sprite* sprLine = Sprite::create("res/ingame/drum_line.png");
			sprLine->setAnchorPoint(Point(0.5, 0));
			sprLine->setScaleY(len);
			sprLine->setPosition(
				Point(PTR_GAMEMAIN->m_WinSize.width / 2, 0.0f));
			this->addChild(sprLine, TAG_NOTE_BG_LINE, TAG_NOTE_BG_LINE);
			sprLine->setCameraMask((unsigned short)CameraFlag::USER1);
		}
		else {
			float draw_yy = 400.0f;

			if (PTR_GAMEMAIN->m_IsWide == false) 
			{
				if (PTR_GAMEMAIN->m_SelectGuitar == 5 || PTR_GAMEMAIN->m_SelectGuitar == 6) //ElectricGuitar
				{
					draw_yy = 270.0f;
				}
				else 
				{
					//draw_yy = 370.0f;
					draw_yy = 270.0f;
				}
			}
			else {
				if (PTR_GAMEMAIN->m_SelectGuitar == 5 || PTR_GAMEMAIN->m_SelectGuitar == 6) //ElectricGuitar
				{
					draw_yy = 400.0f;
				}
				else
				{
					//draw_yy = 570.0f;
					draw_yy = 400.0f;
				}
			}
			Sprite* sprBG1 = NULL;
			if (PTR_GAMEMAIN->m_SelectGuitar == 5 || PTR_GAMEMAIN->m_SelectGuitar == 6) //ElectricGuitar
				sprBG1 = Sprite::create("res/ingame/bg/neck_base.png");
			else
				sprBG1 = Sprite::create("res/ingame/bg/neck_base_02.png");
			sprBG1->setAnchorPoint(Point(0.5f, 0.0f));
			sprBG1->setPosition(Point(PTR_GAMEMAIN->m_WinSize.width / 2, draw_yy));
			this->addChild(sprBG1, TAG_NOTE_BG_CAHR);
			sprBG1->setCameraMask((unsigned short)CameraFlag::USER1);

			Sprite* sprBG2 = NULL;
			if (PTR_GAMEMAIN->m_SelectGuitar == 5 || PTR_GAMEMAIN->m_SelectGuitar == 6) //ElectricGuitar
				sprBG2 = Sprite::create("res/ingame/bg/neck_piece.png");
			else
				sprBG2 = Sprite::create("res/ingame/bg/neck_piece_02.png");
			sprBG2->setAnchorPoint(Point(0.5f, 0.0f));
			sprBG2->setPosition(Point(PTR_GAMEMAIN->m_WinSize.width / 2, draw_yy + 203.0f));
			sprBG2->setScaleY(310.0f);
			this->addChild(sprBG2, TAG_NOTE_BG_CAHR);
			sprBG2->setCameraMask((unsigned short)CameraFlag::USER1);

			for (int i = 0; i < 15; i++)
			{
				Sprite* sprBG3 = NULL;
				if (PTR_GAMEMAIN->m_SelectGuitar == 5 || PTR_GAMEMAIN->m_SelectGuitar == 6) //ElectricGuitar
					sprBG3 = Sprite::create("res/ingame/bg/flat.png");
				else
					sprBG3 = Sprite::create("res/ingame/bg/flat_02.png");
				sprBG3->setAnchorPoint(Point(0.5f, 0.0f));
				sprBG3->setPosition(Point(PTR_GAMEMAIN->m_WinSize.width / 2, draw_yy + 100.0f + (i * 200.0f)));
				this->addChild(sprBG3, TAG_NOTE_BG_CAHR);
				sprBG3->setCameraMask((unsigned short)CameraFlag::USER1);

				if (i % 2 == 0)
				{
					Sprite* sprBG4 = NULL;
					if (PTR_GAMEMAIN->m_SelectGuitar == 5 || PTR_GAMEMAIN->m_SelectGuitar == 6) //ElectricGuitar
						sprBG4 = Sprite::create("res/ingame/bg/inlay.png");
					else
						sprBG4 = Sprite::create("res/ingame/bg/inlay_02.png");
					sprBG4->setAnchorPoint(Point(0.5f, 0.0f));
					sprBG4->setPosition( Point(PTR_GAMEMAIN->m_WinSize.width / 2, draw_yy + 200.0f + (i * 200.0f)));
					this->addChild(sprBG4, TAG_NOTE_BG_CAHR);
					sprBG4->setCameraMask((unsigned short)CameraFlag::USER1);
				}
			}

			float lineWidth = Psize / (float)getInstrumentLineCnt(PTR_GAMEMAIN->m_SelectChannel);
			float startLineP = Director::getInstance()->getVisibleSize().width / 2 - (Psize / 2) + (lineWidth / 2);

			float pickup_x = 0.0f;
			if (PTR_GAMEMAIN->m_IsWide == false)
			{
				pickup_x = 140.0f;
			}
			else 
			{
				pickup_x = 207.0f;
			}

			for (int i = 0;i < getInstrumentLineCnt(PTR_GAMEMAIN->m_SelectChannel); i++)
			{
				Sprite* sprLine = Sprite::create("res/ingame/custom_guitar.png");
				sprLine->setAnchorPoint(Point(0.5, 0));
				sprLine->setScaleY(len);
				sprLine->setPosition(Point((startLineP + (i * lineWidth)), 0.0f));
				this->addChild(sprLine, TAG_NOTE_BG_LINE);
				sprLine->setCameraMask((unsigned short)CameraFlag::USER1);
			}
		}
	}
	else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::PIANO) {
		Sprite* sprLine = Sprite::create("res/ingame/piano_line.png");
		sprLine->setAnchorPoint(Point(0.5, 0));
		sprLine->setScaleY(len);
		sprLine->setPosition(Point(PTR_GAMEMAIN->m_WinSize.width / 2, 0.0f));
		this->addChild(sprLine, TAG_NOTE_BG_LINE, TAG_NOTE_BG_LINE);
		sprLine->setCameraMask((unsigned short)CameraFlag::USER1);
	}
	else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::DRUM) {
		Sprite* sprLine = Sprite::create("res/ingame/drum_line.png");
		sprLine->setAnchorPoint(Point(0.5, 0));
		sprLine->setScaleY(len);
		sprLine->setPosition(Point(PTR_GAMEMAIN->m_WinSize.width / 2, 0.0f));
		this->addChild(sprLine, TAG_NOTE_BG_LINE, TAG_NOTE_BG_LINE);
		sprLine->setCameraMask((unsigned short)CameraFlag::USER1);
	}


#ifdef PLAY_NOTE_GUIDE
	{
		int linCnt = getInstrumentLineCnt(PTR_GAMEMAIN->m_SelectChannel);
		float lineWidth = Psize / (float)linCnt;
		float startLineP = Director::getInstance()->getVisibleSize().width / 2 - (Psize / 2) + (lineWidth / 2);
		

		SpriteBatchNode* sprBatchBox = NULL;
		SpriteBatchNode* sprBatchTouch = NULL;

		for (int i = 0; i < linCnt; i++)
		{
			float noteX = (startLineP + (i * lineWidth));

			if (PTR_GAMEMAIN->m_isSampler == true)
			{
				sprBatchBox = (SpriteBatchNode*)this->getChildByTag(TAG_NOTE_IMG_SAMPLER_BOX);
			}
			else
			{
				if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::PIANO)
					sprBatchBox = (SpriteBatchNode*)this->getChildByTag(TAG_NOTE_IMG_PIANO_BOX);
				else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::GUITAR)
					sprBatchBox = (SpriteBatchNode*)this->getChildByTag(TAG_NOTE_IMG_GUITAR_BOX);
				else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::DRUM)
					sprBatchBox = (SpriteBatchNode*)this->getChildByTag(TAG_NOTE_IMG_DRUM_BOX);
			}

			

			//auto sprBack = Sprite::create("res/ingame/blank.png", Rect(0, 0, 100, 100));
			auto sprBack = Sprite::createWithTexture(sprBatchBox->getTexture());
			sprBack->setPosition(noteX, m_perfectPosition);
			sprBack->setAnchorPoint(Point(0.5f, 0.5f));
			//sprBack->setColor(Color3B::RED);
			//sprBack->setOpacity(100);
			sprBack->setVisible(false);
			addChild(sprBack, TAG_NOTE_GUIDE_BACK_1 + i, TAG_NOTE_GUIDE_BACK_1 + i);
			sprBack->setCameraMask((unsigned short)CameraFlag::USER1);


			if (PTR_GAMEMAIN->m_isSampler == true)
			{
				sprBatchTouch = (SpriteBatchNode*)this->getChildByTag(TAG_NOTE_IMG_SAMPLER_TOUCH);
			}
			else
			{
				if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::PIANO)
					sprBatchTouch = (SpriteBatchNode*)this->getChildByTag(TAG_NOTE_IMG_PIANO_TOUCH);
				else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::GUITAR)
					sprBatchTouch = (SpriteBatchNode*)this->getChildByTag(TAG_NOTE_IMG_GUITAR_TOUCH);
				else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::DRUM)
					sprBatchTouch = (SpriteBatchNode*)this->getChildByTag(TAG_NOTE_IMG_DRUM_TOUCH);
			}

			//auto spr = Sprite::create("res/ingame/blank.png", Rect(0, 0, 100, 100));
			auto spr = Sprite::createWithTexture(sprBatchTouch->getTexture());
			spr->setPosition(noteX, m_perfectPosition);
			spr->setAnchorPoint(Point(0.5f, 0.5f));
			//spr->setColor(Color3B::RED);
			//spr->setOpacity(180);
			spr->setVisible(false);
			addChild(spr, TAG_NOTE_GUIDE_1 + i, TAG_NOTE_GUIDE_1 + i);
			spr->setCameraMask((unsigned short)CameraFlag::USER1);
		}
	}
#endif


	//피버 이펙트
	{
		for (int i = 0; i < 2; i++) 
		{
			auto spineFever = spine::SkeletonAnimation::createWithFile( "res/ingame/effect/ingame_fever_vfx.json", "res/ingame/effect/ingame_fever_vfx.atlas", 1.0f);

			if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::GUITAR) 
			{
				spineFever->setPosition(Point(90.0f + (i * 1100.0f), 0.0f));
			}
			else
			{
				spineFever->setPosition(Point(0.0f + (i * 1280.0f), 0.0f));
			}

			if (i == 1)
			{
				spineFever->getSkeleton()->flipX = 1;
			}
			spineFever->setAnchorPoint(Point(0.5f, 0.0f));

			spineFever->setAnimation(0, "idle", true);
			this->addChild(spineFever, TAG_NOTE_BG_FEVER_EFFECT_LEFT + i, TAG_NOTE_BG_FEVER_EFFECT_LEFT + i);
			spineFever->setCameraMask((unsigned short)CameraFlag::USER1);
			spineFever->setVisible(false);
		}
	}

	

	//SUDDEN
	{
		int y = DEFAULT_FAR_PLANE;
		int size = getHindranceData(HINDRANCE_SUDDEN, 0);
		if (size == 0)
			size = 100;

		setFogImage(y, size, TAG_FOG_SUDDEN_TOP, 1.0f);	
	}

	//MIST
	{
		int y = DEFAULT_FAR_PLANE / 2-200;
		int size = getHindranceData(HINDRANCE_MIST, 0);
		if (size == 0)
			size = 100;

		setFogImage(y, size, TAG_FOG_MIST_TOP, 0.5f);
	}

	//HIDDEN
	{
		int y = m_perfectPosition;
		int size = getHindranceData(HINDRANCE_HIDDEN, 0);
		if (size == 0)
			size = 100;

		setFogImage(y, size, TAG_FOG_HIDDEN_TOP, 0.5f);
	}

	float width = 1280.0f;
	if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::GUITAR)
	{
		width = 1100;
	}
	//셔터
	auto spineSurtter = spine::SkeletonAnimation::createWithFile("res/ingame/distEffect/ingame_disturbance_shutter_vfx.json", "res/ingame/distEffect/ingame_disturbance_shutter_vfx.atlas", 1.0f);
	spineSurtter->setVisible(false);
	spineSurtter->setPosition(PTR_GAMEMAIN->m_WinSize.width/2, DEFAULT_FAR_PLANE / 2-200);
	this->addChild(spineSurtter, TAG_FOG_SURTTER, TAG_FOG_SURTTER);
	spineSurtter->setCameraMask((unsigned short)CameraFlag::USER1);
	spineSurtter->setScaleX((float)(width / 1280.0f));
	spineSurtter->setAnimation(0, "active", true);

	//와이퍼
	auto spineWiper = spine::SkeletonAnimation::createWithFile("res/ingame/distEffect/ingame_disturbance_wiper_vfx.json", "res/ingame/distEffect/ingame_disturbance_wiper_vfx.atlas", 1.0f);
	spineWiper->setVisible(false);
	spineWiper->setPosition(0.0f, 0.0f);
	this->addChild(spineWiper, TAG_WIPER, TAG_WIPER);
	spineWiper->setCameraMask((unsigned short)CameraFlag::USER1);
	spineWiper->setAnimation(0, "active", true);
}


void PlayGame_NoteView::loadNoteData() {
	char _buffer[200000] = { 0, };
	char _pszSongId[64] = { 0, };
	int _MIdx = PTR_GAMEMAIN->m_SelectMusicIdx; //노래 인덱스

#ifdef READ_DOWNLOAD
	sprintf(_pszSongId, "%d", _MIdx);
#else
	sprintf(_pszSongId, "res/%d", _MIdx);
#endif

	//MP3 로딩
	{

		//log(  "PTR_GAMEMAIN->m_filePath   size=%d", strlen(PTR_GAMEMAIN->m_filePath_Voice) );

		std::string m_cFullPathFileName;
#ifdef MUSICIAN_US
		sprintf(PTR_GAMEMAIN->m_filePath, "musics/%d/%s.ntt", PTR_GAMEMAIN->m_SelectMusicIdx, _pszSongId);
#else
		sprintf(PTR_GAMEMAIN->m_filePath, "musics/%d/%s.mp3",
			PTR_GAMEMAIN->m_SelectMusicIdx, _pszSongId);
#endif
		m_cFullPathFileName = PTR_GAMEMAIN->m_filePath;

#ifdef READ_DOWNLOAD
		//다운받은 곳에서 읽기
		sprintf(PTR_GAMEMAIN->m_filePath, "%s%s",
			FileUtils::sharedFileUtils()->getWritablePath().c_str(),
			m_cFullPathFileName.c_str());
#else
		//어플내에서 읽기
		std::string _pathKey = m_cFullPathFileName.c_str();
		_pathKey = FileUtils::sharedFileUtils()->fullPathForFilename(_pathKey.c_str());
		sprintf(PTR_GAMEMAIN->m_filePath, "%s", _pathKey.c_str());
#endif

		{
			sprintf(PTR_GAMEMAIN->m_filePath_Voice, "musics/%d/%s.m4a", PTR_GAMEMAIN->m_SelectMusicIdx, _pszSongId);
			m_cFullPathFileName = PTR_GAMEMAIN->m_filePath_Voice;
			sprintf(PTR_GAMEMAIN->m_filePath_Voice, "%s%s",FileUtils::sharedFileUtils()->getWritablePath().c_str(), m_cFullPathFileName.c_str());

			if (FileUtils::getInstance()->isFileExist(
				PTR_GAMEMAIN->m_filePath_Voice)) {
				PTR_GAMEMAIN->m_isVoice = true;
			}
			else {
				log("!!!!!!!!!!!!!!!!!!!!!  Nothing Voice !!!!!!!!!!!!!!!!!");
			}
		}
	}
    
    IZAudioEngine::getInstance()->preloadEffect(PTR_GAMEMAIN->m_filePath);
    IZAudioEngine::getInstance()->readyMR(PTR_GAMEMAIN->m_filePath, MR);
    if(PTR_GAMEMAIN->m_isVoice)
    {
        IZAudioEngine::getInstance()->preloadEffect(PTR_GAMEMAIN->m_filePath_Voice);
        IZAudioEngine::getInstance()->readyMR(PTR_GAMEMAIN->m_filePath_Voice, VOICE);
    }
    
	NTMLoader ntmLoader;
	ntmLoader.makeMidiInfoCount(PTR_GAMEMAIN->m_trackCount);

	for (int mmk = 0; mmk < PTR_GAMEMAIN->m_trackCount; mmk++) {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[mmk] > 0)
			//노트데이타 로딩
		{
			char diff[5][100] =
			{ "NONE", "EASY", "NORMAL", "HARD", "SUPER_HARD" };
			char fileName[256];
			int diffIdx = PTR_GAMEMAIN->m_SelectLevel[0];

			if (PTR_GAMEMAIN->m_GameInstrumType[0]
				== PTR_GAMEMAIN->m_MusicInfoInstType[mmk]) {
				diffIdx = PTR_GAMEMAIN->m_SelectLevel[0];
			}
			else {
#ifdef MUSICIAN_US
				const MusicInstDifficultInfo& mu = GameDB::getInstance()->getMinMusicDifficultInfos(PTR_GAMEMAIN->m_SelectMusicIdx, (InstType)(PTR_GAMEMAIN->m_MusicInfoInstType[mmk]));
				diffIdx = mu.inst_difficult();
#else
				const MusicDifficultInfo& mu =
					GameDB::getInstance()->getMinMusicDifficultInfo(
					PTR_GAMEMAIN->m_SelectMusicIdx,
					(InstType)(PTR_GAMEMAIN->m_MusicInfoInstType[mmk]));
				diffIdx = mu.difficult();
#endif
			}
			sprintf(fileName, "musics/%d/%s_%d_%s.ntm",
				PTR_GAMEMAIN->m_SelectMusicIdx, _pszSongId,
				(PTR_GAMEMAIN->m_MusicInfoInstType[mmk]), diff[diffIdx]);

			if (diffIdx <= 0) {
				log(
					"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
				log("diffIdx = %d   ERROR!!!!!!!!!!!!!!!!!!!!!!!!! ", diffIdx);
				log(
					"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
			}

			//sprintf(fileName, "musics/%d/%s_%s.ntm", PTR_GAMEMAIN->m_SelectMusicIdx, _pszSongId, diff[diffIdx]);

#ifdef READ_DOWNLOAD
			PTR_GAMEMAIN->ReadFileData(fileName, _buffer, true);
#else
			PTR_GAMEMAIN->ReadFileData(fileName, _buffer, false);
#endif

			log("@@@ fileName:%s  ", fileName);

			//midiInfo = ntmLoader.NTMLoadToFileBuffer(_buffer);
			ntmLoader.NTMLoadToFileBuffer(_buffer);
		}
	}

	{
		midiInfo = ntmLoader.getMidiInfo();
		if (midiInfo == NULL) {
			log(
				"@@@@@@@@@@@@@@@@@@@@@@@@@@@   load ERROR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			PlayGame* pGame = (PlayGame*) this->getParent();
			pGame->GameError(0);
			return;
		}
		else {
			PTR_GAMEMAIN->m_bpm = midiInfo->GetBPM();
			log("PTR_GAMEMAIN->m_bpm : %d     PixelPerBeat:%d",
				PTR_GAMEMAIN->m_bpm, midiInfo->GetPixelPerBeat());

			int _instType = 0;

			for (int t_i = 0; t_i < PTR_GAMEMAIN->m_trackCount; t_i++) {
				int noteIdx = 0;

				pixelPerBeat = getSpeedVal(t_i);

				m_trackLine[t_i] = midiInfo->trackChunk->metaEvent[t_i].GetNoteLine();

				std::string str = midiInfo->trackChunk->metaEvent[t_i].GetTrackName();

				_instType = midiInfo->trackChunk->metaEvent[t_i].instrumentType;
				_instType += 1;

				m_selMaxCnt[t_i] = midiInfo->trackChunk->midiEvent[t_i].midiData.size();
				m_iSelTotalCnt += m_selMaxCnt[t_i];

				_instType = PTR_GAMEMAIN->m_MusicInfoInstType[t_i];

				if (PTR_GAMEMAIN->m_GameInstrumType[0] == _instType)
				{
					PTR_GAMEMAIN->m_SelectChannel = t_i; //선택된 악기의 트랙 인덱스 저장
					PTR_GAMEMAIN->m_noteCount = m_selMaxCnt[t_i];
				}

				int Count = 0;
				int EffectInfo[127];
				memset(EffectInfo, 0, sizeof(EffectInfo));

				//채널안 노트 데이타
				MidiData_Vecter_It it = midiInfo->trackChunk->midiEvent[t_i].midiData.begin();
				for (it; it != midiInfo->trackChunk->midiEvent[t_i].midiData.end(); ++it)
				{
					//노트 생성
					if (createNote(it, NULL, _instType, t_i, noteIdx, 0.0f) == true) 
					{
						return;
					}
					noteIdx++; //노트인덱스 증가
				}

				if (PTR_GAMEMAIN->m_SelectChannel == t_i) 
				{
#ifdef SOUNDTEAM_TEST_BUILD
					//마디 만들기
					{
						auto it = GameDB::getInstance()->getMusicInfo(PTR_GAMEMAIN->m_SelectMusicIdx);
						const MusicInfo& musicInfo = it->second;
						const MusicBaseInfo& musicBaseInfo = musicInfo.music_base_info();
						PTR_GAMEMAIN->m_measureCount = (musicBaseInfo.play_time()/midiInfo->GetPixelPerBeat())/midiInfo->GetBeat();
						PTR_GAMEMAIN->m_measureCount = PTR_GAMEMAIN->m_measureCount / 10;
						log("**************************** PTR_GAMEMAIN->m_measureCount = %d", PTR_GAMEMAIN->m_measureCount);

						for (int m_i = 1; m_i < PTR_GAMEMAIN->m_measureCount; m_i++)
						{
							//auto spr = Sprite::create("res/ingame/blank.png", Rect(0, 0, PTR_GAMEMAIN->m_WinSize.width, 10));
							auto sprBatchTouch = (SpriteBatchNode*)this->getChildByTag(TAG_NOTE_IMG_SECTION_LINE);
							auto spr = Sprite::createWithTexture(sprBatchTouch->getTexture());
							spr->setPosition(PTR_GAMEMAIN->m_WinSize.width / 2, m_perfectPosition + (m_i*midiInfo->GetBeat()*getSpeedVal(t_i)));
							spr->setAnchorPoint(Point(0.5f, 0.5f));
							spr->setColor(Color3B::RED);
							spr->setVisible(false);
							addChild(spr, TAG_MEASURE_LINE + m_i, TAG_MEASURE_LINE + m_i);
							spr->setCameraMask((unsigned short)CameraFlag::USER1);

							char _buffer[64] = {0,};
							sprintf(_buffer, "%d", m_i+1);
							auto txt = LabelAtlas::create(_buffer, "res/ingame/num.png", 33, 41, '0');
							txt->setPosition(-50, 0);
							txt->setAnchorPoint(Point(0.5f, 0.0f));
							txt->setColor(Color3B::WHITE);
							txt->setScale(2.0f);
							//this->setRotation3D(cocos2d::Vec3(_rotateVal, 0.0f, 0.0f));
							txt->setRotation3D(cocos2d::Vec3(49.0f, 0.0f, 0.0f));
							txt->setCameraMask((unsigned short)CameraFlag::USER1);
							spr->addChild(txt);

						}
					}
#endif
					if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_ILLUSION] == true || PTR_GAMEMAIN->m_isHindrance[HINDRANCE_SUPER_ILLUSION] == true)
					{
						setIllusion();
					}
					if (PTR_GAMEMAIN->m_GameMode != PlayMode::STAGE)
					{
						//드럼노트중 같은 라인에 있을경우 연결
						if (PTR_GAMEMAIN->m_MusicInfoInstType[t_i] == (int)InstType::DRUM)
						{
							int checkCount = 0;

							for (int kk = 0; kk < m_selMaxCnt[t_i]; kk++)
							{
								NoteObject * _note = m_note[t_i][kk];
								int count = 0;
								if (_note->m_isJudgeNote == true)
								{
									for (int zz = 0; zz < m_selMaxCnt[t_i]; zz++)
									{
										NoteObject * _tmpNote = m_note[t_i][zz];

										if (kk != zz && _note->m_point == _tmpNote->m_point && _tmpNote->m_isJudgeNote == true)
										{
											if (_note->m_sameLine == -1)
												_note->m_sameLine = checkCount;

											if (_tmpNote->m_sameLine == -1)
											{
												_tmpNote->m_sameLine = checkCount;
												count++;
											}
										}
									}
								}

								if (count > 0)
								{
									checkCount++;
								}
							}

							checkCount = 0;
							for (int kk = 0; kk < m_selMaxCnt[t_i]; kk++)
							{
								NoteObject * _note = m_note[t_i][kk];

								if (_note->m_sameLine == checkCount && _note->m_sprNote != NULL)
								{
									//_note->m_sprDrumLine = Sprite::create( "res/ingame/blank.png", Rect(0, 0,PTR_GAMEMAIN->m_WinSize.width,10));
									auto sprBatchTouch = (SpriteBatchNode*)this->getChildByTag(TAG_NOTE_IMG_DRUM_SAME_LINE);
									_note->m_sprDrumLine = Sprite::createWithTexture(sprBatchTouch->getTexture());

									_note->m_sprDrumLine->setPosition(PTR_GAMEMAIN->m_WinSize.width / 2, _note->m_sprNote->getPositionY());
									_note->m_sprDrumLine->setAnchorPoint(Point(0.5f, 0.5f));
									//_note->m_sprDrumLine->setColor(Color3B::WHITE);
									//_note->m_sprDrumLine->setOpacity(100);
									_note->m_sprDrumLine->setVisible(false);
									addChild(_note->m_sprDrumLine, TAG_NOTE_LONG_LINE);
									_note->m_sprDrumLine->setCameraMask((unsigned short)CameraFlag::USER1);
									checkCount++;
								}
							}
						}
					}

					//AUTO노트 이어주기
					for (int kk = 0; kk < m_selMaxCnt[t_i]; kk++)
					{
						NoteObject * _note = m_note[t_i][kk];

						if (_note->m_isGroup[BIG_GROUP] == true)
						{
							if (_note->m_childSize[BIG_GROUP] > 0 && isHaveNoteLine(_note)) {
								for (int child_i = 0; child_i < _note->m_childSize[BIG_GROUP]- 1; child_i++)
								{
									NoteObject * _note1 = m_note[t_i][_note->m_childIdx[BIG_GROUP][child_i]];

									if (_note1->m_sprNote->isVisible() == false && _note1->m_isNoteVisible == false)
										continue;

									//중간에 보이지 않는 노트는 선에서 빼준당
									for (int find_i = 1; find_i < 8; find_i++) 
									{
										if (child_i + find_i < _note->m_childSize[BIG_GROUP]) 
										{
											int tmpIdx = _note->m_childIdx[BIG_GROUP][child_i + find_i];
											if (m_selMaxCnt[t_i] < tmpIdx)
												continue;
											NoteObject * _note2 = m_note[t_i][tmpIdx];
											if ((_note2->m_sprNote->isVisible()== true || (_note2->m_sprNote->isVisible() == false && _note2->m_isNoteVisible == true))
												&& _note1->m_GroupID[BIG_GROUP] == _note2->m_GroupID[BIG_GROUP])
											{
												DrawAutoLine(_note, _note1,
													_note2, _instType);
												break;
											}
										}
									}
								}
							}
						}
						else {
							if (_note->m_isGroup[SMALL_GROUP] == true
								&& _note->m_GroupID[BIG_GROUP] <= 0
								&& _note->m_childSize[SMALL_GROUP] > 0
								&& isHaveNoteLine(_note)) {
								for (int child_i = 0;
									child_i
									< _note->m_childSize[SMALL_GROUP]
									- 1; child_i++) {
									NoteObject * _note1 =
										m_note[t_i][_note->m_childIdx[SMALL_GROUP][child_i]];

									if (_note1->m_sprNote->isVisible() == false
										&& _note1->m_isNoteVisible == false)
										continue;

									//중간에 보이지 않는 노트는 선에서 빼준당
									for (int find_i = 1; find_i < 8; find_i++) {
										if (child_i + find_i
											< _note->m_childSize[SMALL_GROUP]) {
											int tmpIdx =
												_note->m_childIdx[SMALL_GROUP][child_i
												+ find_i];
											if (m_selMaxCnt[t_i] < tmpIdx)
												continue;
											NoteObject * _note2 =
												m_note[t_i][tmpIdx];
											if ((_note2->m_sprNote->isVisible()
												== true
												|| (_note2->m_sprNote->isVisible()
												== false
												&& _note2->m_isNoteVisible
												== true))
												&& _note1->m_GroupID[SMALL_GROUP]
												== _note2->m_GroupID[SMALL_GROUP]) {
												DrawAutoLine(_note, _note1,
													_note2, _instType);
												break;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	log("@@@@@@@@@@@@@@@@@@@@@@@@@@@   LoadNoteData DONE!!! !!!!!!!!!!!!!!!!!!!!!! ");
}


void PlayGame_NoteView::setIllusion()
{
	bool tmpCan[50];
	srand(time(NULL));
	if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_ILLUSION] == true)
	{
		for (int ki = 0; ki < 50; ki++)
		{
			int Number = rand() % 100 + 1;

			if (Number < 60)
				tmpCan[ki] = true;
			else
				tmpCan[ki] = false;
		}
	}

	int num[2] = {0, 0};
	if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_ILLUSION] == true)
	{
		num[0] = getHindranceData(HINDRANCE_ILLUSION, 0);
		num[1] = getHindranceData(HINDRANCE_ILLUSION, 1);
	}
	else if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_SUPER_ILLUSION] == true)
	{
		num[0] = getHindranceData(HINDRANCE_SUPER_ILLUSION, 0);
		num[1] = getHindranceData(HINDRANCE_SUPER_ILLUSION, 1);
	}
	float SDATA[11] = { 0.5f, 1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 3.5f, 4.0f, 4.5f, 5.0f, 5.5f };
	int SpeedNum = rand() % (num[1] - num[0]);

	SpeedNum = (SpeedNum + num[0]);

	if (SpeedNum > 10)
		SpeedNum = 10;
	
	PTR_GAMEMAIN->m_illusion_speed = SDATA[SpeedNum];


	for (int kk = 0; kk < m_selMaxCnt[PTR_GAMEMAIN->m_SelectChannel]; kk++)
	{
		NoteObject * _note = m_note[PTR_GAMEMAIN->m_SelectChannel][kk];

		bool isIllusion = false;
		int m_type = isIllusionNote(_note);

		if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_SUPER_ILLUSION] == true)
		{
			int Number = rand() % 100 + 1;

			if (Number < 50)
			{
				isIllusion = true;
			}
		}
		else
		{
			if (tmpCan[_note->m_lineX] == true)
			{
				isIllusion = true;
			}
		}

		if (m_type > 0)
		{
			if (isIllusion == true)
			{
				_note->m_isIllusion = true;
				if (m_type == 2) //SMALL_GROP
				{
					for (int g_i = 0; g_i < _note->m_childSize[SMALL_GROUP]; g_i++)
					{
						NoteObject * _tmpNnote = m_note[PTR_GAMEMAIN->m_SelectChannel][_note->m_childIdx[SMALL_GROUP][g_i]];
						_tmpNnote->m_isIllusion = true;

						float yy = m_perfectPosition + ((_tmpNnote->m_point) * (getSpeedVal(PTR_GAMEMAIN->m_SelectChannel)*PTR_GAMEMAIN->m_illusion_speed));
						if (_tmpNnote->m_sprNote != NULL)
							_tmpNnote->m_sprNote->setPositionY(yy);

					}
				}
				else if (m_type == 3)//BIG_GROUP
				{
					for (int g_i = 0; g_i < _note->m_childSize[BIG_GROUP]; g_i++)
					{
						NoteObject * _tmpNnote = m_note[PTR_GAMEMAIN->m_SelectChannel][_note->m_childIdx[BIG_GROUP][g_i]];
						_tmpNnote->m_isIllusion = true;

						float yy = m_perfectPosition + ((_tmpNnote->m_point) * (getSpeedVal(PTR_GAMEMAIN->m_SelectChannel)*PTR_GAMEMAIN->m_illusion_speed));
						if (_tmpNnote->m_sprNote != NULL)
							_tmpNnote->m_sprNote->setPositionY(yy);

					}
				}
			}
		}
	}
}

void PlayGame_NoteView::DrawAutoLine(NoteObject *note, NoteObject *note1,
	NoteObject *note2, int insType) {
	float tmp = TouchMgr::GetDistanceSq(note1->m_sprNote->getPosition(),
		note2->m_sprNote->getPosition());

	Point po = cocos2d::ccpSub(note1->m_sprNote->getPosition(),
		note2->m_sprNote->getPosition());
	float tmp2 = cocos2d::ccpToAngle(po);
	float degreeAngle = CC_RADIANS_TO_DEGREES(-1 * tmp2);

	degreeAngle -= 90.0f;

	int ddd = 10;

	//note1->m_sprLine = Sprite::create("res/ingame/blank.png", Rect(0, 0, ddd, tmp));
	/*
	 auto batchNodeNote = (SpriteBatchNode*)this->getChildByTag(TAG_IMG_DRAG_BODY);
	 note1->m_sprLine = Sprite::createWithTexture(batchNodeNote->getTexture());
	 */
	note1->m_sprLine = Sprite::createWithSpriteFrameName("drag_tail.png");

	note1->m_sprLine->setScaleY(tmp);

	note1->m_sprLine->setPosition(note1->m_sprNote->getPosition());
	note1->m_sprLine->setAnchorPoint(Point(0.5f, 0.0f));
	note1->m_sprLine->setRotation(degreeAngle);

	if (note->m_noteType == NOTE_TYPE_AUTO
		|| note->m_noteType == NOTE_TYPE_NORMAL_AUTO
		|| note->m_noteType == NOTE_TYPE_AUTO_CHORD) {
		note1->m_sprLine->setColor(Color3B(203, 203, 255));
		note1->m_sprLine->setOpacity(AUTO_NOTE_OPACITY);
	}
	else if (note->m_noteType == NOTE_TYPE_BANDING
		|| note->m_noteType == NOTE_TYPE_BANDING_MUTE) {
		note1->m_sprLine->setColor(Color3B(0, 255, 0));
		note1->m_sprLine->setOpacity(150.0f);
	}
	else if (note->m_noteType == NOTE_TYPE_VIBRATO
		|| note->m_noteType == NOTE_TYPE_VIBRATO_MUTE) {
		note1->m_sprLine->setColor(Color3B(108, 203, 82));
		note1->m_sprLine->setOpacity(150.0f);
	}
	else {
		//note1->m_sprLine->setColor(Color3B(255, 0, 0));
		//note1->m_sprLine->setOpacity(150.0f);
	}
	this->addChild(note1->m_sprLine, TAG_NOTE_LONG_LINE);
	note1->m_sprLine->setCameraMask((unsigned short)CameraFlag::USER1);
	note1->m_sprLine->setVisible(false);
	note1->m_isLineVisible = true;
}

void PlayGame_NoteView::preloadPlayEffect(NoteObject* _note, int _chIdx, int noteIdx, int _instType) 
{
	bool isLoad = true;
	char szFullPath[512];
	int istType = 0;

	if (_instType == (int)InstType::GUITAR)
		istType = 1;
	else if (_instType == (int)InstType::DRUM)
		istType = 2;

	//if (m_isLoading[istType][_note->m_effIndex] == false)
	{
		if (effectPath(szFullPath, _chIdx, _note->m_effIndex, _note->m_NoteFx)
			== true) {
			//log("~~~~~~~~~~~~~preloadPlayEffect() [%d][%d] effect:%d   fx:%d  %s", _chIdx, noteIdx, _note->m_effIndex, _note->m_NoteFx, szFullPath);
			memcpy(_note->m_FullPath, szFullPath, 512);
			IZAudioEngine::getInstance()->preloadEffect(szFullPath);
            
            if(_chIdx != PTR_GAMEMAIN->m_SelectChannel)
                IZAudioEngine::getInstance()->readyMR(_note);
			//m_isLoading[istType][_note->m_effIndex] = true;
		}
		else {
			memcpy(_note->m_FullPath, 0, 512);
		}
	}
}

bool PlayGame_NoteView::effectPath(char* path, int channel, int effIdx,
	int fxIdx) {
	if (effIdx < 0) {
		//log("++++++++++++++++++++++++  effectPath()  channel:%d   effidx:%d   fxIdx:%d", channel, effIdx, fxIdx);
		return false;
	}

	if (fxIdx > 1000)
		fxIdx = 0;

	char inst[256];

	if (PTR_GAMEMAIN->m_MusicInfoInstType[channel] == (int)InstType::PIANO) {
		sprintf(inst, "%d", PTR_GAMEMAIN->m_SelectPiano);
	}
	else if (PTR_GAMEMAIN->m_MusicInfoInstType[channel]
		== (int)InstType::GUITAR) {
		sprintf(inst, "%d", PTR_GAMEMAIN->m_SelectGuitar);
	}
	else if (PTR_GAMEMAIN->m_MusicInfoInstType[channel]
		== (int)InstType::DRUM) {
		sprintf(inst, "%d", PTR_GAMEMAIN->m_SelectDrum);
	}

	int idx = 0;

	if (fxIdx > 0) {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[channel]
			== (int)InstType::PIANO) {
			sprintf(path, "musics/%d/%d_FX_Piano_%03d.ntt",
				PTR_GAMEMAIN->m_SelectMusicIdx,
				PTR_GAMEMAIN->m_SelectMusicIdx, fxIdx);
		}
		else if (PTR_GAMEMAIN->m_MusicInfoInstType[channel]
			== (int)InstType::GUITAR) {
			sprintf(path, "musics/%d/%d_FX_Guitar_%03d.ntt",
				PTR_GAMEMAIN->m_SelectMusicIdx,
				PTR_GAMEMAIN->m_SelectMusicIdx, fxIdx);
		}
		else if (PTR_GAMEMAIN->m_MusicInfoInstType[channel]
			== (int)InstType::DRUM) {
			sprintf(path, "musics/%d/%d_FX_Drum_%03d.ntt",
				PTR_GAMEMAIN->m_SelectMusicIdx,
				PTR_GAMEMAIN->m_SelectMusicIdx, fxIdx);
		}
	}
	else {
		sprintf(path, "instruments/%s/%s_%03d.ntt", inst, inst, effIdx);
	}

	std::string fileName = path;
	int pointIndex = fileName.find('.', 1);

#ifdef MUSICIAN_US
#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
	path[pointIndex + 1] = 'n';
	path[pointIndex + 2] = 't';
	path[pointIndex + 3] = 'a';
#else
	path[pointIndex + 1] = 'n';
	path[pointIndex + 2] = 't';
	path[pointIndex + 3] = 'g';
#endif
#else
#if(CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	path[pointIndex + 1] = 'o';
	path[pointIndex + 2] = 'g';
	path[pointIndex + 3] = 'g';
#elif(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#ifdef MIXING
	path[pointIndex + 1] = 'm';
	path[pointIndex + 2] = '4';
	path[pointIndex + 3] = 'a';
#else
	path[pointIndex + 1] = 'o';
	path[pointIndex + 2] = 'g';
	path[pointIndex + 3] = 'g';
#endif
#else
	path[pointIndex + 1] = 'm';
	path[pointIndex + 2] = '4';
	path[pointIndex + 3] = 'a';
#endif
#endif

	char szFullPath[512];

	sprintf(szFullPath, "%s%s",
		FileUtils::sharedFileUtils()->getWritablePath().c_str(), path);
	sprintf(path, "%s", szFullPath);
	//log("++++++++++++++++++ path:%s", path);
	return true;
}

bool PlayGame_NoteView::createNote(MidiData_Vecter_It it, NoteObject *nObj,
	int instType, int _channelIdx, int _idx, float note_y) {
	/*
	 if (_channelIdx == PTR_GAMEMAIN->m_SelectChannel)
	 {

	 int pTime = (float)(it->y*midiInfo->GetBeatPerMS());
	 //float pTime = (float)((float)(it->y/midiInfo->GetPixelPerBeat())*midiInfo->GetBeatPerMS());

	 log("NOTE[%d][%d] type:%d effectIdx:%d  pTime:%d X:%d, Y:%d VEL:%d, DU:%d   length:%d  it->groupID[0]:%d  it->groupID[1]:%d  it->childSize[0]:%d  it->childSize[1]:%d", _channelIdx, _idx, it->type, it->noteEffectID, pTime, it->x, it->y, it->velocity, it->duration, it->length, it->groupID[0], it->groupID[1], it->childSize[0], it->childSize[1]);
	 }
	 */

	int linCnt = getInstrumentLineCnt(_channelIdx);

	NoteObject* noteObj = new NoteObject();

	if (note_y > 0.0f) {
		noteObj = nObj;
	}

	noteObj->m_time = ((float)(it->y) * midiInfo->GetBeatPerMS());
	//log("++++++++++++++++++++   noteObj->m_time:%f   it->y:%d   ms:%f", noteObj->m_time, it->y, midiInfo->GetBeatPerMS());
	if (note_y == 0.0f) {
		noteObj->m_sprNote = NULL;
		noteObj->m_sprLine = NULL;
		noteObj->m_sprDrumLine = NULL;
	}
	noteObj->m_idx = _idx;
	noteObj->m_velocity = it->velocity;
	noteObj->m_noteType = it->type;
	noteObj->m_effIndex = it->noteEffectID;
	noteObj->m_lineX = it->x;
	noteObj->m_point = it->y;
	noteObj->m_length = it->length;
	noteObj->m_isAutoProcessNote = false;
	noteObj->m_isNoteVisible = false;
	noteObj->m_isLineVisible = false;
	noteObj->m_isNotTouch = false;

	noteObj->m_Pan = (it->Pan / 127.0f);
	noteObj->m_Pitch = 1.0f;

	noteObj->m_isJudgeNote = false;

	noteObj->m_duration = it->duration;

	noteObj->m_NoteImageProp = it->fxGroupID; //노트이미지 속성

	noteObj->m_lineLength = noteObj->m_lineLength_effect = it->lineLength;
	noteObj->m_lineLength -= 1;
	noteObj->m_longNoteTouch = 0;
	noteObj->m_lineLength_effectIdx = -1;

	noteObj->m_NoteFx = it->fxSoundID;

	noteObj->m_isDelete = false;
	noteObj->m_effPlay = true;
	noteObj->m_effPlaySound = true;

	noteObj->m_isIllusion = false;
	noteObj->m_isChangeSizeBig = false;
	noteObj->m_isChangeSizeSmall = false;

	noteObj->m_touchID = -1;

	noteObj->m_GroupID[SMALL_GROUP] = it->groupID[SMALL_GROUP];
	noteObj->m_GroupID[BIG_GROUP] = it->groupID[BIG_GROUP];

	noteObj->m_isGroup[SMALL_GROUP] = it->controllerID[SMALL_GROUP];
	noteObj->m_isGroup[BIG_GROUP] = it->controllerID[BIG_GROUP];

	noteObj->m_childSize[SMALL_GROUP] = it->childSize[SMALL_GROUP];
	noteObj->m_childSize[BIG_GROUP] = it->childSize[BIG_GROUP];

	noteObj->m_sameLine = -1;
	noteObj->m_opacity = 255;

	//에러 처리
	if (it->childUniqueID.size() > 300
		|| it->secondChildUniqueID.size() > 300) {
		log("child size   small:%d    big:%d", it->childUniqueID.size(),
			it->secondChildUniqueID.size());
		PlayGame* pGame = (PlayGame*) this->getParent();
		pGame->GameError(1);
		return true;
	}


	noteObj->m_longChileSize = it->longChildSize;
	if (noteObj->m_longChileSize > 0) {
		for (int jjj = 0; jjj < 2; jjj++) {
			noteObj->m_longChileIdx[jjj] = it->longChildUniqueID[jjj];
		}
	}


	//첫노트 시간
	if (_channelIdx == PTR_GAMEMAIN->m_SelectChannel && noteObj->m_idx == 0) {
		PTR_GAMEMAIN->m_PreViewStartTime = noteObj->m_time;
	}

	//그룹노트 처리하기
	{
		for (int group_i = 0; group_i < 2; group_i++) {
			if (noteObj->m_isGroup[group_i] == true) //그룹노트임
			{
				if ((group_i == SMALL_GROUP && it->childUniqueID.size() > 0)
					|| (group_i == BIG_GROUP
					&& it->secondChildUniqueID.size() > 0)) {
					for (int i = 0; i < noteObj->m_childSize[group_i]; i++) {
						if (group_i == 0) {
							//log("NOTE[%d][%d] noteObj->m_childSize:%d UqIdx:%d", _channelIdx, _idx, noteObj->m_childSize[group_i], it->childUniqueID[i]);
							noteObj->m_childIdx[group_i][i] =
								it->childUniqueID[i];
						}
						else {
							//log("NOTE[%d][%d] noteObj->m_childSize:%d secondChildUniqueID:%d", _channelIdx, _idx, noteObj->m_childSize[group_i], it->secondChildUniqueID[i]);
							noteObj->m_childIdx[group_i][i] =
								it->secondChildUniqueID[i];
						}
					}
				}
			}

		}

	}

	if (PTR_GAMEMAIN->m_SelectChannel == _channelIdx)
		noteObj->m_isSprLoad = true;
	else
		noteObj->m_isSprLoad = false;

	float value = getSpeedVal(_channelIdx);

	float yy = m_perfectPosition + (noteObj->m_point) * value;

	if (note_y > 0.0f) {
		yy = note_y;
	}

	float Psize = NOTESCENE_WIDTH;
	//if (PTR_GAMEMAIN->m_IsWide == false)
	//	Psize = NOTESCENE_WIDTH-200.0f;

	if (noteObj->m_isSprLoad) {
		float lineWidth = Psize / (float)linCnt;
		float startLineP = Director::getInstance()->getVisibleSize().width / 2
			- (Psize / 2) + (lineWidth / 2);
		//SpriteBatchNode* batchNodeNote;

		int tmp = 0;
		int _maxNoteLen = 0;

		if (noteObj->m_noteType != NOTE_TYPE_VIBRATO_MUTE && isMuteNote(noteObj)
			&& noteObj->m_length > 1) {
			float longNoteVal = value * (1.334f);

			float noteX = (startLineP + (noteObj->m_lineX * lineWidth));
			noteObj->m_sprNote = Sprite::create("res/ingame/blank.png",
				Rect(0, 0, 20, longNoteVal));
			noteObj->m_sprNote->setZOrder(
				TAG_NOTE_LAYER + (m_selMaxCnt[_channelIdx] - _idx));
			noteObj->m_sprNote->setAnchorPoint(Point(0.5f, 0.5f));

			if (noteObj->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG) {
				noteObj->m_sprNote->setColor(Color3B(255, 156, 156));
			}
			else if (noteObj->m_noteType == NOTE_TYPE_BANDING_MUTE) {
				noteObj->m_sprNote->setColor(Color3B(0, 255, 0));
				noteX += 100.0f;
			}
			else {
				noteObj->m_sprNote->setColor(Color3B(255, 255, 0));
			}
			noteObj->m_sprNote->setOpacity(200);
			noteObj->m_opacity = 200;
			noteObj->m_sprNote->setPosition(Point(noteX, yy));
		}
		else {
			//노트 이미지종류 얻어오기
			/*
			 int noteImageTag = getNoteImageIdx(noteObj, instType);
			 batchNodeNote = (SpriteBatchNode*)this->getChildByTag(noteImageTag);
			 noteObj->m_sprNote = Sprite::createWithTexture(batchNodeNote->getTexture());
			 */
			char imageName[255] = { '\0' };
			getNoteImageIdx(imageName, noteObj, instType);
			noteObj->m_sprNote = Sprite::createWithSpriteFrameName(imageName);

			noteObj->m_sprNote->setZOrder(
				TAG_NOTE_LAYER + (m_selMaxCnt[_channelIdx] - _idx));
			noteObj->m_sprNote->setAnchorPoint(Point(0.5f, 0.5f));

			float noteX = (startLineP + (noteObj->m_lineX * lineWidth));
			if (noteObj->m_noteType == NOTE_TYPE_BANDING_MUTE) {
				noteX += 100.0f;
			}
			else if (noteObj->m_noteType == NOTE_TYPE_VIBRATO_MUTE
				&& noteObj->m_length > 1) {
				if (noteObj->m_length % 2 == 0) {
					noteX += 50.0f;
				}
				else {
					noteX -= 50.0f;
				}
			}
			noteObj->m_sprNote->setPosition(Point(noteX, yy));


			//롱노트 꼬리 적용
			if (noteObj->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG || noteObj->m_noteType == NOTE_TYPE_MUTE_NORMAL_LONG)
			{
				float tmp = value * noteObj->m_lineLength;

				if (noteObj->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG)
				{
					noteObj->m_sprLine = Sprite::createWithSpriteFrameName("drag_long_tail.png");
				}
				else 
				{
					noteObj->m_sprLine = Sprite::createWithSpriteFrameName("long_tail.png");
				}

				noteObj->m_sprLine->setPosition(Point(noteObj->m_sprNote->getPositionX(), noteObj->m_sprNote->getPositionY() + tmp));
				noteObj->m_sprLine->setScaleY(tmp);
				noteObj->m_sprLine->setAnchorPoint(Point(0.5f, 1.0f));

				//ofEnableSmoothing
				//noteObj->m_sprLine->getTexture()->setAliasTexParameters();

				this->addChild(noteObj->m_sprLine, TAG_NOTE_LONG_LINE);
				noteObj->m_sprLine->setCameraMask((unsigned short)CameraFlag::USER1);
				noteObj->m_sprLine->setVisible(false);
				noteObj->m_isLineVisible = true;
			}
		}

		if (noteObj->m_noteType == NOTE_TYPE_BANDING) {
			noteObj->m_sprNote->setColor(Color3B(0, 255, 0));
		}

		//악기별 크기조정
		/*
		if (isMuteNote(noteObj) == false) {
			if (instType == (int)InstType::DRUM) {
			}
			else if (instType == (int)InstType::PIANO) {
				noteObj->m_sprNote->setScale(0.9f);
			}
			else {
				noteObj->m_sprNote->setScale(1.2f);
			}
		}
		*/

		//노트 타입별 특별 처리
		if (noteObj->m_noteType == NOTE_TYPE_LEFT_SLID
			|| noteObj->m_noteType == NOTE_TYPE_RIGHT_SLID) {
			if (noteObj->m_childSize[SMALL_GROUP] > 0) {
			}
			else {
				noteObj->m_sprNote->setOpacity(80);
				noteObj->m_opacity = 80;
			}
		}
		else if (noteObj->m_noteType == NOTE_TYPE_AUTO
			|| noteObj->m_noteType == NOTE_TYPE_NORMAL_AUTO
			|| noteObj->m_noteType == NOTE_TYPE_AUTO_CHORD) {
			if (isAutoHead(noteObj, false) == false) {
				noteObj->m_sprNote->setOpacity(AUTO_NOTE_OPACITY);
				noteObj->m_opacity = AUTO_NOTE_OPACITY;
				noteObj->m_isNotTouch = true;
				noteObj->m_isAutoProcessNote = true;
			}
		}

		if (isALLChordNote(noteObj) == true
			&& noteObj->m_childSize[SMALL_GROUP] == 0) {
			noteObj->m_isNotTouch = true; //터치도 안되게
			noteObj->m_sprNote->setVisible(false);
		}
		else {
			noteObj->m_isNoteVisible = true;
			noteObj->m_sprNote->setVisible(false);

			if (noteObj->m_noteType == NOTE_TYPE_BANDING_MUTE) {
				if (noteObj->m_duration - 5 < noteObj->m_length) {
					noteObj->m_isNoteVisible = false;
				}
			}
		}

		if (isHiddenNote(noteObj) == true) {
			noteObj->m_isNoteVisible = false;
			noteObj->m_sprNote->setVisible(false);
		}

		this->addChild(noteObj->m_sprNote, TAG_NOTE_IMG); //TAG_NOTE_LAYER
		noteObj->m_sprNote->setCameraMask((unsigned short)CameraFlag::USER1);
	}

#ifdef DEBUG_TEST_VELOCITY_VIEW
	if (PTR_GAMEMAIN->m_SelectChannel == _channelIdx)
	{
		char _buffer[64] = {0,};
		sprintf(_buffer, "%d", noteObj->m_velocity);
		noteObj->m_sprVelocity = LabelAtlas::create(_buffer, "res/ingame/num.png", 33, 41, '0');
		noteObj->m_sprVelocity->setPosition(noteObj->m_sprNote->getPosition());
		noteObj->m_sprVelocity->setAnchorPoint(Point(0.5f, 0.5f));
		noteObj->m_sprVelocity->setColor(Color3B::RED);
		this->addChild(noteObj->m_sprVelocity, TAG_NOTE_VELOCITY);
		noteObj->m_sprVelocity->setCameraMask((unsigned short)CameraFlag::USER1);

	}
#endif

#ifdef DEBUG_TEST_IDX_VIEW
	if (PTR_GAMEMAIN->m_SelectChannel == _channelIdx)
	{
		bool isDraw = true;
		if (noteObj->m_noteType == NOTE_TYPE_DRAG_CHORD && noteObj->m_childSize[SMALL_GROUP] <= 0)
			isDraw = false;
		else if (noteObj->m_noteType == NOTE_TYPE_CHORD && noteObj->m_childSize[SMALL_GROUP] <= 0)
			isDraw = false;

		if (isDraw)
		{
			char _buffer[64] = {0,};
			//sprintf(_buffer, "%d", noteObj->m_idx);
			sprintf(_buffer, "%d", noteObj->m_point);
			noteObj->m_sprVelocity = LabelAtlas::create(_buffer, "res/ingame/num.png", 33, 41, '0');
			noteObj->m_sprVelocity->setPosition(noteObj->m_sprNote->getPosition());
			noteObj->m_sprVelocity->setAnchorPoint(Point(0.5f, 0.5f));
			noteObj->m_sprVelocity->setColor(Color3B::WHITE);
			noteObj->m_sprVelocity->setScale(1.5f);
			this->addChild(noteObj->m_sprVelocity, TAG_NOTE_VELOCITY);
			noteObj->m_sprVelocity->setScale(3.0f);
			noteObj->m_sprVelocity->setRotation3D(cocos2d::vertex3(49, 0, 0));
			noteObj->m_sprVelocity->setCameraMask((unsigned short)CameraFlag::USER1);
		}
		else
			noteObj->m_sprVelocity = NULL;

	}
#endif

	if (note_y == 0.0f) {
		m_note[_channelIdx].push_back(noteObj);
	}

	//테스트 용도
	if (noteObj->m_isNoteVisible == true) {
		if (PTR_GAMEMAIN->m_SelectChannel == _channelIdx) {
			if (isMuteNote(noteObj) == true) {
				//if (noteObj->m_length % PROC_LONG_NOTE == 0 )
				if (noteObj->m_length == 1) {
					PTR_GAMEMAIN->m_JudgeNoteCount[0]++;
					noteObj->m_isJudgeNote = true;
				}
			}
			else {
				if (noteObj->m_noteType == NOTE_TYPE_LEFT_SLID
					|| noteObj->m_noteType == NOTE_TYPE_RIGHT_SLID) {
					if (noteObj->m_childSize[SMALL_GROUP] > 0) {
						PTR_GAMEMAIN->m_JudgeNoteCount[0]++;
						noteObj->m_isJudgeNote = true;
					}
				}
				else if (noteObj->m_noteType == NOTE_TYPE_AUTO
					|| noteObj->m_noteType == NOTE_TYPE_NORMAL_AUTO
					|| noteObj->m_noteType == NOTE_TYPE_AUTO_CHORD) {
					if (isAutoHead(noteObj, false) == true) {
						PTR_GAMEMAIN->m_JudgeNoteCount[0]++;
						noteObj->m_isJudgeNote = true;
					}
				}
				else {
					PTR_GAMEMAIN->m_JudgeNoteCount[0]++;
					noteObj->m_isJudgeNote = true;
				}
			}
		}
	}

#ifdef	PLAY_NOTE_GUIDE
	if (noteObj->m_isJudgeNote == true)
	{
		for (int i = 0; i < 1000; i++)
		{
			if (PTR_GAMEMAIN->m_noteLineData[noteObj->m_lineX][i] == -1)
			{
				PTR_GAMEMAIN->m_noteLineData[noteObj->m_lineX][i] = noteObj->m_idx;
				break;
			}
		}
	}
#endif

	return false;
}

char * PlayGame_NoteView::getInstName(int instType) {
	if (PTR_GAMEMAIN->m_isSampler == true) {
		return "sampler";
	}
	else {
		if (instType == (int)InstType::PIANO) {
			return "piano";
		}
		else if (instType == (int)InstType::GUITAR) {
			return "guitar";
		}
		else if (instType == (int)InstType::DRUM) {
			return "drum";
		}
	}

	return "piano";
}

void PlayGame_NoteView::getNoteImageIdx(char * name, NoteObject* _note,
	int instType) {
	if (instType == (int)InstType::DRUM
		&& PTR_GAMEMAIN->m_isSampler == false) {
		sprintf(name, "snare_large.png");
		{
			if (_note->m_noteType == NOTE_TYPE_NORMAL_DRAG
				|| _note->m_noteType == NOTE_TYPE_DRAG_CHORD
				|| _note->m_noteType == NOTE_TYPE_DRAG_LONG) {
				getDrumImageIdx(name, _note->m_effIndex, true);
			}
			else if (_note->m_noteType == NOTE_TYPE_AUTO
				|| _note->m_noteType == NOTE_TYPE_NORMAL_AUTO
				|| _note->m_noteType == NOTE_TYPE_AUTO_CHORD) {
				if (_note->m_GroupID[BIG_GROUP] > 0) {
					if (_note->m_childSize[BIG_GROUP] > 0) {
						if (_note->m_childSize[SMALL_GROUP] > 0) {
							getDrumImageIdx(name, _note->m_effIndex, false);
						}
						else {
							getDrumImageIdx(name, _note->m_effIndex, false);
						}
					}
					else {
						sprintf(name, "drum_note_auto2.png");
					}
				}
				else //스몰그룹
				{
					if (_note->m_childSize[SMALL_GROUP] > 0) {
						getDrumImageIdx(name, _note->m_effIndex, false);
					}
					else {
						sprintf(name, "drum_note_auto2.png");
					}
				}
			}
			else if (_note->m_noteType == NOTE_TYPE_CHORD) {
				getDrumImageIdx(name, _note->m_effIndex, false);
			}
			else if (_note->m_noteType == NOTE_TYPE_NORMAL_LONG) {
				getDrumImageIdx(name, _note->m_effIndex, false);
			}
			else if (isMuteNote(_note)) {
				if (_note->m_length == 1) {
					if (_note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG)
						sprintf(name, "drag_long_tail_end.png");
					else
						sprintf(name, "long_tail_end.png");
				}
				else {
					if (_note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG)
						sprintf(name, "drag_long_tail_end.png");
					else
						;
					sprintf(name, "long_tail_end.png");
				}
			}
			else if (isHiddenNote(_note) == true) {
				sprintf(name, "drum_note_hidden.png");
			}
			else {
				getDrumImageIdx(name, _note->m_effIndex, false);
			}
		}
	}
	else {
		sprintf(name, "%s_note_normal.png", getInstName(instType));
		{
			if (_note->m_noteType == NOTE_TYPE_NORMAL_DRAG
				|| _note->m_noteType == NOTE_TYPE_DRAG_LONG) {
				sprintf(name, "%s_note_dreg1.png", getInstName(instType));
			}
			else if (_note->m_noteType == NOTE_TYPE_AUTO
				|| _note->m_noteType == NOTE_TYPE_NORMAL_AUTO
				|| _note->m_noteType == NOTE_TYPE_AUTO_CHORD) {
				if (_note->m_GroupID[BIG_GROUP] > 0) {
					if (_note->m_childSize[BIG_GROUP] > 0) {
						if (_note->m_childSize[SMALL_GROUP] > 0) {
							sprintf(name, "%s_note_autocode1.png",
								getInstName(instType));
							if (_note->m_childSize[SMALL_GROUP] == 3) {
								sprintf(name, "%s_note_autocode2.png",
									getInstName(instType));
							}
							else if (_note->m_childSize[SMALL_GROUP] == 4) {
								sprintf(name, "%s_note_autocode3.png",
									getInstName(instType));
							}
						}
						else {
							sprintf(name, "%s_note_auto1.png",
								getInstName(instType));
						}
					}
					else {
						sprintf(name, "%s_note_auto2.png",
							getInstName(instType));
					}
				}
				else //스몰그룹
				{
					if (_note->m_childSize[SMALL_GROUP] > 0) {
						sprintf(name, "%s_note_auto1.png",
							getInstName(instType));
					}
					else {
						sprintf(name, "%s_note_auto2.png",
							getInstName(instType));
					}
				}
			}
			else if (_note->m_noteType == NOTE_TYPE_CHORD) {
				sprintf(name, "%s_note_code1.png", getInstName(instType));
				if (_note->m_childSize[SMALL_GROUP] == 3) {
					sprintf(name, "%s_note_code2.png", getInstName(instType));
				}
				else if (_note->m_childSize[SMALL_GROUP] == 4) {
					sprintf(name, "%s_note_code3.png", getInstName(instType));
				}
			}
			else if (_note->m_noteType == NOTE_TYPE_DRAG_CHORD) {
				sprintf(name, "%s_note_dregcode1.png", getInstName(instType));
				if (_note->m_childSize[SMALL_GROUP] == 3) {
					sprintf(name, "%s_note_dregcode2.png",
						getInstName(instType));
				}
				else if (_note->m_childSize[SMALL_GROUP] == 4) {
					sprintf(name, "%s_note_dregcode3.png",
						getInstName(instType));
				}
			}
			else if (_note->m_noteType == NOTE_TYPE_LEFT_SLID) {
				sprintf(name, "%s_note_slide_l.png", getInstName(instType));
			}
			else if (_note->m_noteType == NOTE_TYPE_RIGHT_SLID) {
				sprintf(name, "%s_note_slide_r.png", getInstName(instType));
			}
			else if (_note->m_noteType == NOTE_TYPE_NORMAL_LONG) {
				sprintf(name, "%s_note_long1.png", getInstName(instType));
			}
			else if (_note->m_noteType == NOTE_TYPE_DOUBBLE_SLIDE) {
				sprintf(name, "%s_note_double.png", getInstName(instType));
			}
			else if (_note->m_noteType == NOTE_TYPE_TREMOLO) {
				if (_note->m_isGroup[SMALL_GROUP] == true) {
					sprintf(name, "%s_note_tremolo1.png",
						getInstName(instType));
				}
				else {
					sprintf(name, "%s_note_tremolo2.png",
						getInstName(instType));
				}
			}
			else if (_note->m_noteType == NOTE_TYPE_VIBRATO) {
				sprintf(name, "%s_note_tremolo1.png", getInstName(instType));
			}
			else if (_note->m_noteType == NOTE_TYPE_VIBRATO_MUTE) {
				sprintf(name, "%s_note_tremolo2.png", getInstName(instType));
				if (_note->m_length == 1) {
					sprintf(name, "%s_note_tremolo1.png",
						getInstName(instType));
				}
			}
			else if (isMuteNote(_note)) {
				if (_note->m_length == 1) {
					if (_note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG)
						sprintf(name, "drag_long_tail_end.png");
					else
						sprintf(name, "long_tail_end.png");
				}
				else {
					if (_note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG)
						sprintf(name, "%s_note_dreg2.png",
						getInstName(instType));
					else
						sprintf(name, "%s_note_long2.png",
						getInstName(instType));
				}
			}
			else if (isHiddenNote(_note) == true) {
				sprintf(name, "%s_note_hidden.png", getInstName(instType));
			}
			else {
				sprintf(name, "%s_note_normal.png", getInstName(instType));
			}
		}
	}

	/*
	 sprintf(name, "piano_note_normal.png");
	 if (instType == (int)InstType::PIANO)
	 {
	 if (_note->m_noteType == NOTE_TYPE_NORMAL_DRAG || _note->m_noteType == NOTE_TYPE_DRAG_LONG)
	 {
	 sprintf(name, "piano_note_dreg1.png");
	 }
	 else if (_note->m_noteType == NOTE_TYPE_AUTO || _note->m_noteType == NOTE_TYPE_NORMAL_AUTO || _note->m_noteType == NOTE_TYPE_AUTO_CHORD )
	 {
	 if (_note->m_GroupID[BIG_GROUP] > 0)
	 {
	 if (_note->m_childSize[BIG_GROUP] > 0)
	 {
	 if (_note->m_childSize[SMALL_GROUP] > 0)
	 {
	 sprintf(name, "piano_note_autocode1.png");
	 if (_note->m_childSize[SMALL_GROUP] == 3)
	 {
	 sprintf(name, "piano_note_autocode2.png");
	 }
	 else if (_note->m_childSize[SMALL_GROUP] == 4)
	 {
	 sprintf(name, "piano_note_autocode3.png");
	 }
	 }
	 else
	 {
	 sprintf(name, "piano_note_auto1.png");
	 }
	 }
	 else
	 {
	 sprintf(name, "piano_note_auto2.png");
	 }
	 }
	 else //스몰그룹
	 {
	 if (_note->m_childSize[SMALL_GROUP] > 0)
	 {
	 sprintf(name, "piano_note_auto1.png");
	 }
	 else
	 {
	 sprintf(name, "piano_note_auto2.png");
	 }
	 }
	 }
	 else if (_note->m_noteType == NOTE_TYPE_CHORD)
	 {
	 sprintf(name, "piano_note_code1.png");

	 if (_note->m_childSize[SMALL_GROUP] == 3)
	 {
	 sprintf(name, "piano_note_code2.png");
	 }
	 else if (_note->m_childSize[SMALL_GROUP] == 4)
	 {
	 sprintf(name, "piano_note_code3.png");
	 }
	 }
	 else if (_note->m_noteType == NOTE_TYPE_DRAG_CHORD)
	 {
	 sprintf(name, "piano_note_dregcode1.png");

	 if (_note->m_childSize[SMALL_GROUP] == 3)
	 {
	 sprintf(name, "piano_note_dregcode2.png");
	 }
	 else if (_note->m_childSize[SMALL_GROUP] == 4)
	 {
	 sprintf(name, "piano_note_dregcode3.png");
	 }
	 }
	 else if (_note->m_noteType == NOTE_TYPE_NORMAL_LONG)
	 {
	 sprintf(name, "piano_note_long1.png");
	 }
	 else if (isMuteNote(_note))
	 {
	 if (_note->m_length == 1)
	 {
	 if (_note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG)
	 sprintf(name, "drag_long_tail_end.png");
	 else
	 sprintf(name, "long_tail_end.png");
	 }
	 else
	 {
	 if (_note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG)
	 sprintf(name, "piano_note_dreg2.png");
	 else
	 sprintf(name, "piano_note_long2.png");
	 }
	 }
	 else if (isHiddenNote(_note) == true)
	 {
	 sprintf(name, "piano_note_hidden.png");
	 }
	 else
	 {
	 sprintf(name, "piano_note_normal.png");
	 }
	 }
	 else if (instType == (int)InstType::GUITAR)
	 {
	 sprintf(name, "guitar_note_normal.png");
	 {
	 if (_note->m_noteType == NOTE_TYPE_NORMAL_DRAG || _note->m_noteType == NOTE_TYPE_DRAG_LONG)
	 {
	 sprintf(name, "guitar_note_dreg1.png");
	 }
	 else if (_note->m_noteType == NOTE_TYPE_AUTO || _note->m_noteType == NOTE_TYPE_NORMAL_AUTO || _note->m_noteType == NOTE_TYPE_AUTO_CHORD)
	 {
	 if (_note->m_GroupID[BIG_GROUP] > 0)
	 {
	 if (_note->m_childSize[BIG_GROUP] > 0)
	 {
	 if (_note->m_childSize[SMALL_GROUP] > 0)
	 {
	 sprintf(name, "guitar_note_autocode1.png");
	 if (_note->m_childSize[SMALL_GROUP] == 3)
	 {
	 sprintf(name, "guitar_note_autocode2.png");
	 }
	 else if (_note->m_childSize[SMALL_GROUP] == 4)
	 {
	 sprintf(name, "guitar_note_autocode3.png");
	 }
	 }
	 else
	 {
	 sprintf(name, "guitar_note_auto1.png");
	 }
	 }
	 else
	 {
	 sprintf(name, "guitar_note_auto2.png");
	 }
	 }
	 else //스몰그룹
	 {
	 if (_note->m_childSize[SMALL_GROUP] > 0)
	 {
	 sprintf(name, "guitar_note_auto1.png");
	 }
	 else
	 {
	 sprintf(name, "guitar_note_auto2.png");
	 }
	 }
	 }
	 else if (_note->m_noteType == NOTE_TYPE_CHORD)
	 {
	 sprintf(name, "guitar_note_code1.png");
	 if (_note->m_childSize[SMALL_GROUP] == 3)
	 {
	 sprintf(name, "guitar_note_code2.png");
	 }
	 else if (_note->m_childSize[SMALL_GROUP] == 4)
	 {
	 sprintf(name, "guitar_note_code3.png");
	 }
	 }
	 else if (_note->m_noteType == NOTE_TYPE_DRAG_CHORD)
	 {
	 sprintf(name, "guitar_note_dregcode1.png");
	 if (_note->m_childSize[SMALL_GROUP] == 3)
	 {
	 sprintf(name, "guitar_note_dregcode2.png");
	 }
	 else if (_note->m_childSize[SMALL_GROUP] == 4)
	 {
	 sprintf(name, "guitar_note_dregcode3.png");
	 }
	 }
	 else if (_note->m_noteType == NOTE_TYPE_LEFT_SLID)
	 {
	 sprintf(name, "guitar_note_slide_l.png");
	 }
	 else if (_note->m_noteType == NOTE_TYPE_RIGHT_SLID)
	 {
	 sprintf(name, "guitar_note_slide_r.png");
	 }
	 else if (_note->m_noteType == NOTE_TYPE_NORMAL_LONG)
	 {
	 sprintf(name, "guitar_note_long1.png");
	 }
	 else if (_note->m_noteType == NOTE_TYPE_DOUBBLE_SLIDE)
	 {
	 sprintf(name, "guitar_note_double.png");
	 }
	 else if (_note->m_noteType == NOTE_TYPE_TREMOLO)
	 {
	 if (_note->m_isGroup[SMALL_GROUP] == true)
	 {
	 sprintf(name, "guitar_note_tremolo1.png");
	 }
	 else
	 {
	 sprintf(name, "guitar_note_tremolo2.png");
	 }
	 }
	 else if (_note->m_noteType == NOTE_TYPE_VIBRATO)
	 {
	 sprintf(name, "guitar_note_tremolo1.png");
	 }
	 else if (_note->m_noteType == NOTE_TYPE_VIBRATO_MUTE)
	 {
	 sprintf(name, "guitar_note_tremolo2.png");
	 if (_note->m_length == 1)
	 {
	 sprintf(name, "guitar_note_tremolo1.png");
	 }
	 }
	 else if (isMuteNote(_note))
	 {
	 if (_note->m_length == 1)
	 {
	 if (_note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG)
	 sprintf(name, "drag_long_tail_end.png");
	 else
	 sprintf(name, "long_tail_end.png");
	 }
	 else
	 {
	 if (_note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG)
	 sprintf(name, "guitar_note_dreg2.png");
	 else
	 sprintf(name, "guitar_note_long2.png");
	 }
	 }
	 else if (isHiddenNote(_note) == true)
	 {
	 sprintf(name, "guitar_note_hidden.png");
	 }
	 else
	 {
	 sprintf(name, "guitar_note_normal.png");
	 }
	 }
	 }
	 else if (instType == (int)InstType::DRUM)
	 {
	 sprintf(name, "snare_large.png");
	 {
	 if (_note->m_noteType == NOTE_TYPE_NORMAL_DRAG || _note->m_noteType == NOTE_TYPE_DRAG_CHORD || _note->m_noteType == NOTE_TYPE_DRAG_LONG)
	 {
	 getDrumImageIdx(name, _note->m_effIndex, true);
	 }
	 else if (_note->m_noteType == NOTE_TYPE_AUTO || _note->m_noteType == NOTE_TYPE_NORMAL_AUTO || _note->m_noteType == NOTE_TYPE_AUTO_CHORD)
	 {
	 if (_note->m_GroupID[BIG_GROUP] > 0)
	 {
	 if (_note->m_childSize[BIG_GROUP] > 0)
	 {
	 if (_note->m_childSize[SMALL_GROUP] > 0)
	 {
	 getDrumImageIdx(name, _note->m_effIndex, false);
	 }
	 else
	 {
	 getDrumImageIdx(name, _note->m_effIndex, false);
	 }
	 }
	 else
	 {
	 sprintf(name, "drum_note_auto2.png");
	 }
	 }
	 else //스몰그룹
	 {
	 if (_note->m_childSize[SMALL_GROUP] > 0)
	 {
	 getDrumImageIdx(name, _note->m_effIndex, false);
	 }
	 else
	 {
	 sprintf(name, "drum_note_auto2.png");
	 }
	 }
	 }
	 else if (_note->m_noteType == NOTE_TYPE_CHORD)
	 {
	 getDrumImageIdx(name, _note->m_effIndex, false);
	 }
	 else if (_note->m_noteType == NOTE_TYPE_NORMAL_LONG)
	 {
	 getDrumImageIdx(name, _note->m_effIndex, false);
	 }
	 else if (isMuteNote(_note))
	 {
	 if (_note->m_length == 1)
	 {
	 if (_note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG)
	 sprintf(name, "drag_long_tail_end.png");
	 else
	 sprintf(name, "long_tail_end.png");
	 }
	 else
	 {
	 if (_note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG)
	 sprintf(name, "drag_long_tail_end.png");
	 else;
	 sprintf(name, "long_tail_end.png");
	 }
	 }
	 else if (isHiddenNote(_note) == true)
	 {
	 sprintf(name, "drum_note_hidden.png");
	 }
	 else
	 {
	 getDrumImageIdx(name, _note->m_effIndex, false);
	 }
	 }
	 }
	 */
}

int PlayGame_NoteView::getDrumImageIdx(char* name, int idx, bool isDrag) {
	int num = TAG_DRUM_IMG_SNARE_LARGE;

	switch (idx) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 32:
	case 33:
	case 34:
	case 35: {
		if (isDrag == true) {
			num = TAG_DRUM_IMG_SNARE_DRAG_LARGE;
			sprintf(name, "drum_snare_drag_large.png");
		}
		else {
			num = TAG_DRUM_IMG_SNARE_LARGE;
			sprintf(name, "drum_snare_large.png");
		}
	}
			 break;

	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 22:
	case 28:
	case 29:
	case 30:
	case 31: {
		if (isDrag == true) {
			num = TAG_DRUM_IMG_CYMBAL_DRAG_LARGE;
			sprintf(name, "drum_cymbal_drag_large.png");
		}
		else {
			num = TAG_DRUM_IMG_CYMBAL_LARGE;
			sprintf(name, "drum_cymbal_large.png");
		}
	}
			 break;

	case 36:
	case 37:
	case 38:
	case 39:
	case 40:
	case 41:
	case 42:
	case 59:
	case 60:
	case 61:
	case 62:
	case 63:
	case 68:
	case 69:
	case 70:
	case 71: {
		if (isDrag == true) {
			num = TAG_DRUM_IMG_SNARE_DRAG_MEDIUM;
			sprintf(name, "drum_snare_drag_medium.png");
		}
		else {
			num = TAG_DRUM_IMG_SNARE_MEDIUM;
			sprintf(name, "drum_snare_medium.png");
		}
	}
			 break;

	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
	case 48:
	case 49:
	case 50:
	case 51:
	case 52:
	case 53:
	case 54:
	case 55:
	case 56:
	case 57:
	case 58:
	case 64:
	case 65:
	case 66:
	case 67: {
		if (isDrag == true) {
			num = TAG_DRUM_IMG_CYMBAL_DRAG_MEDIUM;
			sprintf(name, "drum_cymbal_drag_medium.png");
		}
		else {
			num = TAG_DRUM_IMG_CYMBAL_MEDIUM;
			sprintf(name, "drum_cymbal_medium.png");
		}
	}
			 break;

	case 72:
	case 73:
	case 75:
	case 76:
	case 77:
	case 78:
	case 95:
	case 96:
	case 97:
	case 98:
	case 99:
	case 104:
	case 105:
	case 106:
	case 107: {
		if (isDrag == true) {
			num = TAG_DRUM_IMG_SNARE_DRAG_SMALL;
			sprintf(name, "drum_snare_drag_small.png");
		}
		else {
			num = TAG_DRUM_IMG_SNARE_SMALL;
			sprintf(name, "drum_snare_small.png");
		}
	}
			  break;

	case 79:
	case 80:
	case 81:
	case 82:
	case 83:
	case 84:
	case 85:
	case 86:
	case 87:
	case 88:
	case 89:
	case 90:
	case 91:
	case 92:
	case 93:
	case 94:
	case 100:
	case 101:
	case 102:
	case 103: {
		if (isDrag == true) {
			num = TAG_DRUM_IMG_CYMBAL_DRAG_SMALL;
			sprintf(name, "drum_cymbal_drag_small.png");
		}
		else {
			num = TAG_DRUM_IMG_CYMBAL_SMALL;
			sprintf(name, "drum_cymbal_small.png");
		}
	}
			  break;

	default:
		if (isDrag == true) {
			num = TAG_DRUM_IMG_SNARE_DRAG_LARGE;
			sprintf(name, "drum_snare_drag_large.png");
		}
		else {
			num = TAG_DRUM_IMG_SNARE_LARGE;
			sprintf(name, "drum_snare_large.png");
		}
		break;
	}

	return num;
}

void PlayGame_NoteView::ProC(float dt) {
	
	SkanMoveProC();

	noteMoveProc(dt);

#ifdef CHORD_MIXING
#if(CC_TARGET_PLATFORM != CC_PLATFORM_WIN32 )
	IZAudioEngine::getInstance()->startEffect();
#endif
#endif
    PlayGame* pGame = (PlayGame*) this->getParent();
  
	//판정 가이드
#ifdef PLAY_NOTE_GUIDE
	{
		int nIdx = 0;
		for(int i=0; i<48; i++)
		{
			nIdx = PTR_GAMEMAIN->m_noteLineData[i][PTR_GAMEMAIN->m_noteLineDataIdx[i]];

			if (nIdx == -1)
				continue;

		
			NoteObject* note = m_note[PTR_GAMEMAIN->m_SelectChannel][nIdx];
			auto sprBack = getChildByTag(TAG_NOTE_GUIDE_BACK_1 + note->m_lineX);
			auto spr = getChildByTag(TAG_NOTE_GUIDE_1 + note->m_lineX);

			if (note->m_isDelete == false)
			{
				if ((m_pTimeUpdate >= (note->m_time - 1000)) && (m_pTimeUpdate <= (note->m_time + 200)))
				{
					if (note->m_effPlay == true)
					{
						//보이게 설정
						if (spr->isVisible() == false)
						{
							spr->setVisible(true);
							sprBack->setVisible(true);
							spr->setScale(0.1f);
						}

						float tmp = note->m_time - m_pTimeUpdate;
						if (tmp < 0)
							tmp = 0.0f;
						tmp = tmp / 1000.0f;

						spr->setScale((1.0f - tmp));
						spr->setPositionY(m_perfectPosition);
						sprBack->setPositionY(m_perfectPosition);
					}
					else //터치했을경우
					{
						spr->setVisible(false);
						sprBack->setVisible(false);
					}
				}
				else if (m_pTimeUpdate > (note->m_time + 200))
				{
					spr->setVisible(false);
					sprBack->setVisible(false);
					PTR_GAMEMAIN->m_noteLineDataIdx[i]++; //다음으로 넘김
			
				}
			}
			else
			{
				spr->setVisible(false);
				sprBack->setVisible(false);	
			}
		}
	}
#endif

#ifdef NEW_EFFECT_PROC
	{
		for (int i = 0; i < MAX_EFFECT_CNT; i++)
		{
			auto spineEffect = (spine::SkeletonAnimation*)getChildByTag(TAG_NOTE_EFFECT_LAYER + i);
			if (spineEffect->isVisible() == true)
			{
				spTrackEntry* trackEntry = spineEffect->getCurrent();

				if (trackEntry == NULL)
				{

					spineEffect->setVisible(false);
				}
			}
		}
	}
#else
	updateEffectObject(dt);
#endif

	for (int i = 0; i < MAX_TOUCHES; i++) {
		TouchMgr::CTouch* touch = touchMgr.GetTouch(i);

		if (touch->moveCnt > 0) {
			touch->moveCnt--;
		}
	}

	//노트음 체크 
	//long long curTime = (PTR_GAMEMAIN->getUMicroSecond() / 1000);
	for (int chi = 0; chi < 3; chi++)
	{
		for (int i = 0; i < MAX_STOP_CNT; i++) 
		{
			//if (m_notePlayTime[chi][i] > 0 && m_notePlayTime[chi][i] <= curTime) 
			if (m_notePlayTime[chi][i] > 0 && m_notePlayTime[chi][i] <= m_pTimeUpdate )
			{
				auto playID = m_notePlayID[chi][i];

				m_notePlayID[chi][i] = 0;
				m_notePlayTime[chi][i] = 0;

				IZAudioEngine::getInstance()->fadeStopEffect(playID, 0.1f);
				//IZAudioEngine::getInstance()->stopEffect(playID);
			}
		}
	}
}

/*
 void PlayGame_NoteView::update(float dt)
 {
 Layer::update(dt);
 }
 */

//한번에 체크할 노트갯수
int PlayGame_NoteView::getMaxProcNoteCount(int trackIdx) {
	int maxCheckCnt = m_curNoteIdx[trackIdx] + NOTE_CHECK_CNT;
	if (maxCheckCnt > m_selMaxCnt[trackIdx])
		maxCheckCnt = m_selMaxCnt[trackIdx];

	if (m_selMaxCnt[trackIdx] < 5)
		maxCheckCnt = 0;

	return maxCheckCnt;
}

//노트 복원해주기
void PlayGame_NoteView::setPreViewNote() {
	long long playTime = 0.0f;
	//playTime = PTR_GAMEMAIN->getMicroSecond() - m_MusicPlayTime;
	//playTime = 1;// (PTR_GAMEMAIN->getUMicroSecond() - m_MusicPlayTimeUM) / 1000.0f;

	pixelPerBeat = getSpeedVal(PTR_GAMEMAIN->m_SelectChannel);

	float scrollY = playTime / midiInfo->GetBeatPerMS() * pixelPerBeat;

	for (int ch_i = 0; ch_i < PTR_GAMEMAIN->m_trackCount; ch_i++) {
		int idx = 0;
		bool isfirst = true;

		m_curNoteIdx[ch_i] = 0;
		MidiData_Vecter_It it =
			midiInfo->trackChunk->midiEvent[ch_i].midiData.begin();
		for (it; it != midiInfo->trackChunk->midiEvent[ch_i].midiData.end();
			++it) {
			NoteObject* note = m_note[ch_i][idx];

			//float posY = (note->m_point * pixelPerBeat) - scrollY;
			float posY = m_perfectPosition + (note->m_point) * pixelPerBeat;

			if (ch_i == PTR_GAMEMAIN->m_SelectChannel) {
				if (note->m_isDelete == true) {
					if (posY > m_perfectPosition) {
						//복구해줄 인덱스
						//log("@@@@@@@@@@@@@@@@@@@@   setPreViewNote()     idx=%d", note->m_idx);
						createNote(it, note,
							PTR_GAMEMAIN->m_MusicInfoInstType[ch_i], ch_i,
							idx, posY);
					}

					if (note->m_sprLine != NULL) {
						note->m_isLineVisible = true;
						note->m_sprLine->setPositionY(posY);
					}

				}
				else {
					if (note->m_sprLine != NULL) {
						note->m_isLineVisible = true;
						note->m_sprLine->setPositionY(posY);
					}

					if (note->m_sprNote != NULL) {
						note->m_sprNote->setPositionY(posY);
						if (note->m_isJudgeNote == true) {
							note->m_isNoteVisible = true;
							note->m_effPlay = true;
						}
					}
				}
			}
			else {
				createNote(it, note, PTR_GAMEMAIN->m_MusicInfoInstType[ch_i],
					ch_i, idx, posY);
			}
			idx++;
		}
	}
}

//노트 복원해주기
void PlayGame_NoteView::setReStartNote() {
}

//RESUME시 노트 이동시키기
void PlayGame_NoteView::setResumeNoteClear()
{
	PlayGame* pGame = (PlayGame*) this->getParent();
	long long playTime = 0.0f;

	/*
	playTime = ((PTR_GAMEMAIN->getUMicroSecond()
		- PTR_GAMEMAIN->m_MusicPlayTimeUM) / 1000.0f)
		- PTR_GAMEMAIN->m_NowMusicPlayTime - (3000);
		*/

	playTime = ((PTR_GAMEMAIN->getUMicroSecond() - PTR_GAMEMAIN->m_MusicPlayTimeUM) / 1000.0f) - PTR_GAMEMAIN->m_NowMusicPlayTime;

	pixelPerBeat = getSpeedVal(PTR_GAMEMAIN->m_SelectChannel);

	float scrollY = playTime / midiInfo->GetBeatPerMS() * pixelPerBeat;

	for (int ch_i = 0; ch_i < PTR_GAMEMAIN->m_trackCount; ch_i++) 
	{
		int maxCheckCnt = getMaxProcNoteCount(ch_i);

		if (maxCheckCnt > 0)
		{
			for (int n = m_curNoteIdx[ch_i]; n < maxCheckCnt; n++)
			{
				NoteObject* note = m_note[ch_i][n];

				if (note->m_isDelete == true)
					continue;

				//매번 노트의 좌표를 다시 계산해줌
				float posY = (note->m_point * pixelPerBeat) - scrollY;

				if (note->m_isSprLoad)
				{
					note->m_sprNote->setPositionY(posY);
					if (note->m_sprLine != NULL)
					{
						note->m_sprLine->setPositionY(posY);
					}
#ifdef DEBUG_TEST_VELOCITY_VIEW
					if (note->m_sprVelocity != NULL)
					{
						note->m_sprVelocity->setPositionY(posY + 40);
					}
#endif
#ifdef DEBUG_TEST_IDX_VIEW
					if (note->m_sprVelocity != NULL)
					{
						note->m_sprVelocity->setPositionY(posY + 80);
					}
#endif
				}
			}
		}
	}



	for (int i = 0; i < 48; i++)
	{
		int nIdx = PTR_GAMEMAIN->m_noteLineData[i][PTR_GAMEMAIN->m_noteLineDataIdx[i]];

		if (nIdx >= 0)
		{

			auto sprBack = getChildByTag(TAG_NOTE_GUIDE_BACK_1 + i);
			auto spr = getChildByTag(TAG_NOTE_GUIDE_1 + i);

			sprBack->setVisible(false);
			spr->setVisible(false);
		}
	}
}


int PlayGame_NoteView::isIllusionNote(NoteObject * note)
{
	if (note->m_noteType == NOTE_TYPE_NORMAL)
	{
		return 1;
	}
	else if (note->m_noteType == NOTE_TYPE_CHORD && note->m_childSize[SMALL_GROUP] > 0)
	{
		return 1;
	}
	else if (note->m_noteType == NOTE_TYPE_LEFT_SLID && note->m_childSize[SMALL_GROUP] > 0)
	{
		return 2;
	}
	else if (note->m_noteType == NOTE_TYPE_RIGHT_SLID && note->m_childSize[SMALL_GROUP] > 0)
	{
		return 2;
	}
	else if (note->m_noteType == NOTE_TYPE_AUTO || note->m_noteType == NOTE_TYPE_NORMAL_AUTO || note->m_noteType == NOTE_TYPE_AUTO_CHORD)
	{
		if (note->m_childSize[BIG_GROUP] > 0)
		{
			return 3;
		}
		else
		{
			if (note->m_GroupID[BIG_GROUP] <= 0 && note->m_childSize[SMALL_GROUP] > 0)
			{
				return 2;
			}
		}
	}
	/*
	else if (note->m_noteType == NOTE_TYPE_DRAG_CHORD || note->m_noteType == NOTE_TYPE_NORMAL_DRAG )
	{
		if (note->m_childSize[BIG_GROUP] > 0)
		{
			return 3;
		}
		else
		{
			if (note->m_GroupID[BIG_GROUP] <= 0 && note->m_childSize[SMALL_GROUP] > 0)
			{
				return 2;
			}
		}
	}
	*/

	

	return 0;
}

void PlayGame_NoteView::noteMoveProc(float _dt)
{
	//UserPlayData& userPlayData = GameDB::getInstance()->getUserPlayData();
	//std::vector<SaveNoteData>& saveNoteDatas = userPlayData.getSaveNoteDatas();

	PlayGame* pGame = (PlayGame*) this->getParent();
	float scrollY = 0.0f;
	float scrollY2 = 0.0f;
	float revisionMS_Time = 0.0f;

    /*
	m_pTimeUpdate = (float)(PTR_GAMEMAIN->getUMicroSecond() - PTR_GAMEMAIN->m_MusicPlayTimeUM) / 1000.0f;
	
	if (m_pTimeUpdate > 100)
		m_pTimeUpdateBefore = m_pTimeUpdate;
	revisionMS_Time = (m_pTimeUpdate - m_pTimeUpdateBefore) / 2.0f;
	m_pTimeUpdateBefore = m_pTimeUpdate;
	*/
    m_pTimeUpdate = (float) IZAudioEngine::getInstance()->getMRCurrentTime();

	//하일라이트
	if (PTR_GAMEMAIN->m_GameMode == PlayMode::STAGE)
	{
		HightlightProC();
	}

	for (int ch_i = 0; ch_i < PTR_GAMEMAIN->m_trackCount; ch_i++) 
	{
		pixelPerBeat = getSpeedVal(ch_i);
		scrollY = ((float)(m_pTimeUpdate / midiInfo->GetBeatPerMS()) * pixelPerBeat);
		scrollY2 = ((float)(m_pTimeUpdate / midiInfo->GetBeatPerMS()) * (pixelPerBeat * PTR_GAMEMAIN->m_illusion_speed));


#ifdef SOUNDTEAM_TEST_BUILD
		//마디
		if (ch_i == PTR_GAMEMAIN->m_SelectChannel)
		{
			for (int m_i = 1; m_i < PTR_GAMEMAIN->m_measureCount; m_i++)
			{
				auto spr = getChildByTag(TAG_MEASURE_LINE + m_i);

				float posY = m_perfectPosition + (m_i*midiInfo->GetBeat()*getSpeedVal(ch_i) - scrollY);
				spr->setPositionY(posY);

				if (posY < DEFAULT_FAR_PLANE)
				{
					if (posY < m_perfectPosition)
						spr->setVisible(false);
					else
						spr->setVisible(true);
				}
			}
		}
#endif

		/*
		if (ch_i != PTR_GAMEMAIN->m_SelectChannel)
			continue;
		*/

		int startIdx = m_curNoteIdx[ch_i];
		int maxCheckCnt = getMaxProcNoteCount(ch_i);

		if (maxCheckCnt > 0)
		{
			for (int n = startIdx; n < maxCheckCnt; n++) 
			{
				NoteObject* note = m_note[ch_i][n];

				if (note->m_isDelete == true)
					continue;

				//매번 노트의 좌표를 다시 계산해줌
				float posY = m_perfectPosition + (((float)(note->m_point) * pixelPerBeat) - scrollY);

				if (note->m_isIllusion == true)
				{
					posY = m_perfectPosition + (((float)(note->m_point) * (pixelPerBeat * PTR_GAMEMAIN->m_illusion_speed)) - scrollY2);
				}

				if (note->m_isSprLoad)
				{
					if (note->m_isNoteVisible == true)
					{
						if (isMuteNote(note) == true)
						{
							note->m_sprNote->setVisible(true);
							note->m_isNoteVisible = false;
						}
						else
						{
							if (posY < DEFAULT_FAR_PLANE)
							{
								note->m_sprNote->setVisible(true);
								note->m_isNoteVisible = false;

								if (note->m_sprDrumLine != NULL)
								{
									note->m_sprDrumLine->setVisible(true);
								}
							}
						}
					}

					//if (note->m_isJudgeNote == true)
					{
						HindranceProC(note);
					}
	

					note->m_sprNote->setPositionY(posY);
					if (note->m_sprDrumLine != NULL)
					{
						note->m_sprDrumLine->setPositionY(posY);
					}

					if (note->m_sprLine != NULL)
					{
						if (note->m_isLineVisible == true)
						{
							if (isMuteNote(note) == true)
							{
								note->m_sprLine->setVisible(true);
								note->m_isLineVisible = false;
							}
							else
							{
								if (posY < DEFAULT_FAR_PLANE)
								{
									note->m_sprLine->setVisible(true);
									note->m_isLineVisible = false;
								}
							}
						}
						note->m_sprLine->setPositionY(posY);
					}
#ifdef DEBUG_TEST_VELOCITY_VIEW
					if (note->m_sprVelocity != NULL)
					{
						note->m_sprVelocity->setPositionY(posY + 40);
					}
#endif
#ifdef DEBUG_TEST_IDX_VIEW
					if (note->m_sprVelocity != NULL)
					{
						note->m_sprVelocity->setPositionY(posY + 80);
					}
#endif

					//롱노트 길이조정
					if (note->m_noteType == NOTE_TYPE_MUTE_NORMAL_LONG || note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG) 
					{
						if (note->m_sprLine != NULL)
						{
							{
								float beforeLength = note->m_lineLength * getSpeedVal(ch_i);
								float tmpLength = (note->m_sprLine->getPositionY()) - m_perfectPosition;

								if (tmpLength < beforeLength)
								{
									if (note->m_lineLength_effect > 0)
									{
										if (tmpLength <= 0)
											note->m_lineLength_effect = 0;
										else 
										{
											note->m_lineLength_effect = (tmpLength / getSpeedVal(ch_i));
										}
									}
								}
							}

							if (note->m_longNoteTouch == 2) 
							{
								float tmp = getSpeedVal(ch_i) * note->m_lineLength;
								if (note->m_sprLine->getPositionY() - tmp < m_perfectPosition)
								{
									float scaleVal = (note->m_sprLine->getPositionY()) - m_perfectPosition;
									if (scaleVal >= 0) 
									{
										note->m_sprLine->setScaleY(scaleVal);
									}
								}
							}
						}
					}
				}

				//if (posY < (NOTE_DELETE_END_LINE*PTR_GAMEMAIN->m_GameSpeed))
				if ((m_pTimeUpdate + revisionMS_Time) >= (note->m_time + 500))
				{
					if (PTR_GAMEMAIN->m_SelectChannel == ch_i) 
					{
						if (note->m_effPlay == true) 
						{
							//if (isNotFailed(note) == false && note->m_sprNote->isVisible() == true && note->m_isNotTouch == false)
							if (note->m_isJudgeNote == true)
							{
								pGame->setScore(note, NOTE_MISS);

								if (note->m_noteType == NOTE_TYPE_DRAG_LONG || note->m_noteType == NOTE_TYPE_NORMAL_LONG) 
								{
									if (note->m_longChileSize > 0) 
									{
										NoteObject * _tmpNote = m_note[ch_i][note->m_longChileIdx[1]];
										_tmpNote->m_sprLine->setColor(Color3B(100, 100, 100));
										_tmpNote->m_sprNote->setColor(Color3B(100, 100, 100));
										_tmpNote->m_longNoteTouch = 1;
									}
								}
							}
						}
						deleteNote(note, ch_i);
					}
					else
					{
						note->m_effPlay = false;
					}

					note->m_isDelete = true;
					m_curNoteIdx[ch_i] = n;
				}
				else 
				{
					long long effID = 0;
					if (PTR_GAMEMAIN->m_SelectChannel == ch_i)
					{
						if (pGame->m_TestAutoPaly == true) 
						{
							autoProcess(note, revisionMS_Time, ch_i, n);
						}
						else 
						{
							if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_AUTOPLAY].isHave == true)
							{
								autoPlaySound(note, revisionMS_Time, ch_i, n);

								if (note->m_isAutoProcessNote == true) 
								{
									autoProcess(note, revisionMS_Time, ch_i, n);
								}
							}
							else 
							{
								if (note->m_isAutoProcessNote == true) 
								{
									autoProcess(note, revisionMS_Time, ch_i, n);
								}
							}
						}
					}
					else //다른악기 처리
					{
						//임시로 다른 악기는 무조건 플레이 시켜준다
						//autoProcess(note, revisionMS_Time, ch_i, n);
					}
				}

				if (pGame->m_TestAutoPaly == false) {
					//롱노트처리
					if (PTR_GAMEMAIN->m_SelectChannel == ch_i) {
						if (isMuteNote(note) == true
							|| note->m_noteType == NOTE_TYPE_TREMOLO) {
							ProC_LongNote(note, ch_i, n);
						}
					}
				}
			}
		}
	}
}

void PlayGame_NoteView::HindranceProC(NoteObject * _note)
{
	if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_BLINK] == true)
	{
		if (m_isAniBlink == true)
		{
			int op = _note->m_sprNote->getOpacity();
			op -= 5;

			if (op <= 0)
			{
				op = 0;
				m_isAniBlink = false;
			}
			_note->m_sprNote->setOpacity(op);
		}
		else
		{
			int op = _note->m_sprNote->getOpacity();
			op += 5;

			if (op >= 255)
			{
				op = 255;
				m_isAniBlink = true;
			}
			_note->m_sprNote->setOpacity(op);
		}
	}
	else if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_BLINK] == false && _note->m_sprNote->getOpacity() != _note->m_opacity)
	{
		_note->m_sprNote->setOpacity(_note->m_opacity);
	}

	/*
	if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_BIG_NOTE] == true)
	{
		if (_note->m_isChangeSizeBig == false)
		{
			float _scale = (getHindranceData(HINDRANCE_BIG_NOTE, 0) / 100.0f);

			_note->m_sprNote->setScale(_scale);

			_note->m_isChangeSizeBig = true;
		}
	}
	else if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_BIG_NOTE] == false && _note->m_isChangeSizeBig == true)
	{
		_note->m_sprNote->setScale(1.0f);

		_note->m_isChangeSizeBig = false;
	}

	if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_SMALL_NOTE] == true && _note->m_isChangeSizeSmall == false)
	{
		float _scale = (getHindranceData(HINDRANCE_SMALL_NOTE, 0) / 100.0f);

		_note->m_sprNote->setScale(_scale);
		_note->m_isChangeSizeSmall = true;
	}
	else if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_SMALL_NOTE] == false && _note->m_isChangeSizeSmall == true)
	{
		_note->m_sprNote->setScale(1.0f);

		_note->m_isChangeSizeSmall = false;
	}
	*/
}


void PlayGame_NoteView::settingNoteSize(bool isHighlight, int highLightIdx, float size)
{

	for (int n = 0; n < m_selMaxCnt[PTR_GAMEMAIN->m_SelectChannel]; n++)
	{
		NoteObject* note = m_note[PTR_GAMEMAIN->m_SelectChannel][n];

		if (note->m_isDelete == true)
			continue;


		if (isHighlight == true)
		{

			if ((PTR_GAMEMAIN->m_Highlight[highLightIdx].startPosition <= note->m_point) && (PTR_GAMEMAIN->m_Highlight[highLightIdx].endPosition >= note->m_point))
			{
				if (note->m_sprNote != nullptr)
				{
					note->m_sprNote->setScale(size);
				}
			}
		}
		else //처음부터 셋팅
		{
			if (note->m_sprNote != nullptr)
			{
				note->m_sprNote->setScale(size);
			}
		}
	}
}

void PlayGame_NoteView::HightlightProC() 
{
	bool startAni = false;
	float scrollY = ((float)(m_pTimeUpdate / midiInfo->GetBeatPerMS()) * getSpeedVal(PTR_GAMEMAIN->m_SelectChannel));
	for (int i = 0; i < 3; i++)
	{
		if (PTR_GAMEMAIN->m_Highlight[i].startPosition > 0)
		{
			auto spr = getChildByTag(TAG_NOTE_BG_HIGHTLIGHT_0 + (i*3));
			auto spr2 = getChildByTag(TAG_NOTE_BG_HIGHTLIGHT_0 + 1 + (i * 3));
			auto spr3 = getChildByTag(TAG_NOTE_BG_HIGHTLIGHT_0 + 2 + (i * 3));

			if (spr != NULL && spr->isVisible() == true) 
			{
				float posY = m_perfectPosition + (((float)(PTR_GAMEMAIN->m_Highlight[i].startPosition) * getSpeedVal(PTR_GAMEMAIN->m_SelectChannel)) - scrollY);
				spr->setPositionY(posY);
				spr2->setPositionY(posY);
				spr3->setPositionY(posY);
				if (posY <= m_perfectPosition && PTR_GAMEMAIN->m_Highlight[i].isStart == false)
				{
					PTR_GAMEMAIN->m_Highlight[i].isStart = true;
					startAni = true;

					if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_HINDRANCE].isHave == false)
					{
						int count = 0;
						for (int j = 0; j < 5; j++)
						{
							if (PTR_GAMEMAIN->m_Hindrance[j].condition == 2 && PTR_GAMEMAIN->m_Hindrance[j].type >= 0)
							{
								PTR_GAMEMAIN->m_isHindrance[PTR_GAMEMAIN->m_Hindrance[j].type] = true;

								startHidrance(j, i);
								count++;
							}
						}

						if (count > 0)
						{
							//IZAudioEngine::getInstance()->playAudio("res/audio/Disturb.m4a");
						}
					}
				}

				float posY2 = m_perfectPosition + (((float)(PTR_GAMEMAIN->m_Highlight[i].endPosition) * getSpeedVal(PTR_GAMEMAIN->m_SelectChannel)) - scrollY);

				if (posY2 <= 0.0f) 
				{
					spr->setVisible(false);
					spr2->setVisible(false);
					spr3->setVisible(false);

					if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_HINDRANCE].isHave == false)
					{
						for (int j = 0; j < 5; j++)
						{
							if (PTR_GAMEMAIN->m_Hindrance[j].condition == 2 && PTR_GAMEMAIN->m_Hindrance[j].type >= 0)
							{
								PTR_GAMEMAIN->m_isHindrance[PTR_GAMEMAIN->m_Hindrance[j].type] = false;

								switch (PTR_GAMEMAIN->m_Hindrance[j].type) {
								case HINDRANCE_HIDDEN:
								case HINDRANCE_SUDDEN:
								case HINDRANCE_MIST:
								case HINDRANCE_SHUTTER:
									setFog(PTR_GAMEMAIN->m_Hindrance[j].type, false, 0.0f);
									break;

								case HINDRANCE_WIPER:
									setWiper(false);
									break;
								}
							}
						}
					}
				}
			}
		}
	}


	if (startAni == true)
	{
		PlayGame* pGame = (PlayGame*) this->getParent();

		
		if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_HINDRANCE].isHave == true)
		{
			pGame->m_UILayer->ShieldOn();
		}
		
		pGame->m_UILayer->startDisturbanceIcon(2);
		
	}
}


void PlayGame_NoteView::startHidrance(int idx, int highLightIdx)
{
	switch (PTR_GAMEMAIN->m_Hindrance[idx].type)
	{
	case HINDRANCE_HIDDEN:
		setFog(PTR_GAMEMAIN->m_Hindrance[idx].type, true, m_perfectPosition);
		break;

	case HINDRANCE_SUDDEN:
		setFog(PTR_GAMEMAIN->m_Hindrance[idx].type, true, DEFAULT_FAR_PLANE - 200);
		break;

	case HINDRANCE_MIST:
		setFog(PTR_GAMEMAIN->m_Hindrance[idx].type, true, DEFAULT_FAR_PLANE / 2 - 200);
		break;
	case HINDRANCE_SHUTTER:
		setFog(PTR_GAMEMAIN->m_Hindrance[idx].type, true, DEFAULT_FAR_PLANE / 2 - 200);
		m_isAniUp = false;
		break;

	case HINDRANCE_WIPER:
		setWiper(true);
		break;

	case HINDRANCE_BLINK:
		m_isAniBlink = true;
		break;
	
	case HINDRANCE_BIG_NOTE:
	{
		float _scale = (getHindranceData(HINDRANCE_BIG_NOTE, 0) / 100.0f);
		if (highLightIdx == -1) //처음에 걸리는 경우만 적용
		{
			settingNoteSize(false, highLightIdx, _scale);
		}
	}
		break;

	case HINDRANCE_SMALL_NOTE:
	{
		float _scale = (getHindranceData(HINDRANCE_SMALL_NOTE, 0) / 100.0f);
		if (highLightIdx == -1) //처음에 걸리는 경우만 적용
		{
			settingNoteSize(false, highLightIdx, _scale);
		}
	}
		break;

	}
}


void PlayGame_NoteView::autoPlaySound(NoteObject * _note,
	float _revisionMS_Time, int _ch_i, int _n) {
	int effID = 0;
	if ((m_pTimeUpdate + _revisionMS_Time) >= _note->m_time
		&& _note->m_effPlaySound == true) {
		if (isMuteNote(_note) == false) {
			effID = _playEffect(_note, _ch_i);

			if (effID >= 0) {
				setPlayNoteTime(_ch_i, effID, _note->m_duration, _note->m_time);
			}
			_note->m_effPlaySound = false;
		}
	}
}

void PlayGame_NoteView::autoProcess(NoteObject * _note, float _revisionMS_Time, int _ch_i, int _n) 
{
	int effID = 0;
	PlayGame* pGame = (PlayGame*) this->getParent();


	if (_note->m_noteType == NOTE_TYPE_MUTE_NORMAL_LONG || _note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG)
	{
		if (_note->m_sprLine != NULL)
		{
			float beforeLength = _note->m_lineLength * getSpeedVal(_ch_i);
			float tmpLength = (_note->m_sprLine->getPositionY())
				- m_perfectPosition;

			if (tmpLength < beforeLength && _note->m_longNoteTouch == 2) 
			{
				if (_note->m_lineLength_effect % 7 == 0 && _note->m_lineLength_effect > 0)
				{
					actionNoteEffect(_ch_i, _n, _note->m_sprNote->getPositionX(), m_perfectPosition);
					pGame->addLongNoteFever();
				}
			}
		}
	}

	if ((m_pTimeUpdate + _revisionMS_Time) >= _note->m_time && _note->m_effPlay == true)
	{
		if (_note->m_noteType == NOTE_TYPE_NORMAL_LONG
			|| _note->m_noteType == NOTE_TYPE_DRAG_LONG) {
			NoteObject * _tmpNote = m_note[_ch_i][_note->m_longChileIdx[1]];
			_tmpNote->m_longNoteTouch = 2;
		}

		if (isMuteNote(_note) == true) //무음노트
		{
			if (PTR_GAMEMAIN->m_SelectChannel == _ch_i) 
			{
				if (pGame->m_TestAutoPaly == true)
				{
					if (_note->m_length == 1) 
					{
						setTouchSuccess(_note);
					}
				}
				else 
				{
					if (_note->m_noteType == NOTE_TYPE_VIBRATO_MUTE) 
					{
						if (_note->m_length == 1) 
						{
							actionNoteEffect(_ch_i, _n,
								_note->m_sprNote->getPositionX(),
								m_perfectPosition);
							setTouchSuccess(_note);
						}
						else if (_note->m_length % PROC_LONG_NOTE == 0)
						{
							actionNoteEffect(_ch_i, _n,
								_note->m_sprNote->getPositionX(),
								m_perfectPosition);
						}
					}
				}
			}
		}
		else
		{
			if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_AUTOPLAY].isHave == false || (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_AUTOPLAY].isHave == true && _ch_i != PTR_GAMEMAIN->m_SelectChannel))
			{
				effID = _playEffect(_note, _ch_i);

				if (effID >= 0) 
				{
					setPlayNoteTime(_ch_i, effID, _note->m_duration, _note->m_time);
				}
			}

			if (PTR_GAMEMAIN->m_SelectChannel == _ch_i) {
				//드래그 롱노트 처리 때문에..
				if (_note->m_touchID >= 0) {
					TouchMgr::CTouch *touch = touchMgr.GetTouch(
						_note->m_touchID);
					if (touch != NULL) {
						touch->soundID = effID;
					}
				}
			}

			if (PTR_GAMEMAIN->m_SelectChannel == _ch_i) {
				if (_note->m_sprNote->isVisible() == true) {
					if (isALLChordNote(_note) == true
						&& _note->m_noteType != NOTE_TYPE_AUTO_CHORD) {
						ProC_ChordNoteEffect(_note, _ch_i);
					}
					else {
						if (_note->m_noteType >= NOTE_TYPE_AUTO
							&& _note->m_noteType <= NOTE_TYPE_NORMAL_AUTO) {
							if (isAutoHead(_note, true) == true) {
								if (_note->m_noteType == NOTE_TYPE_AUTO_CHORD) {
									ProC_ChordNoteEffect(_note, _ch_i);
								}
								else {
									actionNoteEffect(_ch_i, _n,
										_note->m_sprNote->getPositionX(),
										m_perfectPosition);
									setTouchSuccess(_note);
								}
							}
						}
						else if (_note->m_noteType == NOTE_TYPE_LEFT_SLID
							|| _note->m_noteType == NOTE_TYPE_RIGHT_SLID) {
							actionNoteEffect(_ch_i, _n,
								_note->m_sprNote->getPositionX(),
								m_perfectPosition);
							if (_note->m_childSize[SMALL_GROUP] > 0) {
								setTouchSuccess(_note);
							}
						}
						else {
							actionNoteEffect(_ch_i, _n,
								_note->m_sprNote->getPositionX(),
								m_perfectPosition);
							setTouchSuccess(_note);
						}
					}
					actionNoteEffectLine(_ch_i, _n,
						_note->m_sprNote->getPositionX(),
						m_perfectPosition);
				}
			}
		}

		if (PTR_GAMEMAIN->m_SelectChannel == _ch_i) {
			deleteNote(_note, _ch_i);
		}
		else {
			_note->m_effPlay = false;
		}
}
			}

void PlayGame_NoteView::ProC_ChordNoteEffect(NoteObject* _note, int ch_i) 
{
	if (PTR_GAMEMAIN->m_MusicInfoInstType[ch_i] == (int)InstType::DRUM) 
	{
		if (_note->m_childSize[SMALL_GROUP] > 0) 
		{
			actionNoteEffect(ch_i, _note->m_idx, _note->m_sprNote->getPositionX(), m_perfectPosition);
			setTouchSuccess(_note);
		}
	}
	else
	{
		int count = 2;
		if (_note->m_noteType == NOTE_TYPE_DRAG_CHORD)
			count = 1;
		for (int chrod_i = 0; chrod_i < _note->m_childSize[SMALL_GROUP]; chrod_i++)
		{
#ifdef ROW_LEVEL_PHONE
			if (chrod_i < count)
			{
				actionNoteEffect(ch_i, _note->m_childIdx[SMALL_GROUP][chrod_i], _note->m_sprNote->getPositionX() + (75 * chrod_i), m_perfectPosition);
			}
#else
			actionNoteEffect(ch_i, _note->m_childIdx[SMALL_GROUP][chrod_i], _note->m_sprNote->getPositionX() + (75 * chrod_i), m_perfectPosition);
#endif
		}
		setTouchSuccess(_note);
	}	
}

bool PlayGame_NoteView::isRealDragNote(NoteObject * note) {
	bool check = false;

	switch (note->m_noteType) {
	case NOTE_TYPE_NORMAL_DRAG:
		//case NOTE_TYPE_DRAG_LONG:
	case NOTE_TYPE_DRAG_CHORD:
		check = true;
		break;
	}

	return check;
}

bool PlayGame_NoteView::isSlide(NoteObject * note) {
	bool check = false;

	switch (note->m_noteType) {
	case NOTE_TYPE_LEFT_SLID:
	case NOTE_TYPE_RIGHT_SLID:
		check = true;
		break;
	}

	return check;
}

bool PlayGame_NoteView::isDragNote(NoteObject * note) {
	bool check = false;

	switch (note->m_noteType) {
	case NOTE_TYPE_NORMAL_DRAG:
	case NOTE_TYPE_LEFT_SLID:
	case NOTE_TYPE_RIGHT_SLID:
	case NOTE_TYPE_DRAG_LONG:
	case NOTE_TYPE_DRAG_CHORD:
		check = true;
		break;
	}

	return check;
}

bool PlayGame_NoteView::isMovePoCNote(NoteObject * note) {
	bool check = false;
	switch (note->m_noteType) {
	case NOTE_TYPE_NORMAL_DRAG:
	case NOTE_TYPE_LEFT_SLID:
	case NOTE_TYPE_RIGHT_SLID:
	case NOTE_TYPE_DRAG_LONG:
	case NOTE_TYPE_DRAG_CHORD:
	case NOTE_TYPE_VIBRATO_MUTE:
	case NOTE_TYPE_VIBRATO:
	case NOTE_TYPE_BANDING:
	case NOTE_TYPE_BANDING_MUTE:
	case NOTE_TYPE_DOUBBLE_SLIDE:
		check = true;
		break;
	}

	return check;
}

bool PlayGame_NoteView::isHaveNoteLine(NoteObject * note) {
	bool check = false;

	switch (note->m_noteType) {
	case NOTE_TYPE_NORMAL_DRAG:
	case NOTE_TYPE_DRAG_CHORD:
	case NOTE_TYPE_AUTO:
	case NOTE_TYPE_NORMAL_AUTO:
	case NOTE_TYPE_VIBRATO:
	case NOTE_TYPE_VIBRATO_MUTE:
	case NOTE_TYPE_BANDING:
	case NOTE_TYPE_BANDING_MUTE:
		check = true;
		break;

	}

	return check;
}

bool PlayGame_NoteView::isMuteNote(NoteObject * note) {
	bool check = false;

	switch (note->m_noteType) {
	case NOTE_TYPE_MUTE_HIDDEN_LONG:
	case NOTE_TYPE_MUTE_NORMAL_LONG:
	case NOTE_TYPE_MUTE_DRAG_LONG:
	case NOTE_TYPE_VIBRATO_MUTE:
	case NOTE_TYPE_BANDING_MUTE:
		check = true;
		break;
	}

	return check;
}

bool PlayGame_NoteView::isHiddenNote(NoteObject * note) {
	bool check = false;
	switch (note->m_noteType) {
	case NOTE_TYPE_HIDDEN:
	case NOTE_TYPE_HIDDEN_LONG:
	case NOTE_TYPE_HIDDEN_CHORD:
		check = true;
		break;
	}

	return check;
}

bool PlayGame_NoteView::isChordNote(NoteObject * note) {
	bool check = false;
	switch (note->m_noteType) {
	case NOTE_TYPE_CHORD:
	case NOTE_TYPE_HIDDEN_CHORD:
	case NOTE_TYPE_AUTO_CHORD:
		check = true;
		break;
	}

	return check;
}

bool PlayGame_NoteView::isALLChordNote(NoteObject * note) {
	bool check = false;
	switch (note->m_noteType) {
	case NOTE_TYPE_CHORD:
	case NOTE_TYPE_HIDDEN_CHORD:
	case NOTE_TYPE_AUTO_CHORD:
	case NOTE_TYPE_DRAG_CHORD:
		check = true;
		break;
	}

	return check;
}

bool PlayGame_NoteView::isAutoNote(NoteObject * note) {
	bool check = false;
	switch (note->m_noteType) {
	case NOTE_TYPE_AUTO:
	case NOTE_TYPE_NORMAL_AUTO:
	case NOTE_TYPE_AUTO_CHORD:
		check = true;
		break;
	}

	return check;
}

/************************************************************************/
/* 노트 화면에서 지우기                                                 */
/************************************************************************/
void PlayGame_NoteView::deleteNote(NoteObject * nNote, int trackIdx) {
	if (nNote->m_isSprLoad) {
		nNote->m_sprNote->setVisible(false);
	}
	if (nNote->m_sprLine != NULL) {
		nNote->m_sprLine->setVisible(false);
	}

	if (nNote->m_sprDrumLine != NULL) {
		nNote->m_sprDrumLine->setVisible(false);
	}

#ifdef DEBUG_TEST_VELOCITY_VIEW
	if (nNote->m_sprVelocity != NULL)
	{
		nNote->m_sprVelocity->setVisible(false);
	}
#endif
#ifdef DEBUG_TEST_IDX_VIEW
	if (nNote->m_sprVelocity != NULL)
	{
		nNote->m_sprVelocity->setVisible(false);
	}
#endif

	nNote->m_effPlay = false;
	//log("--------------------  deleteNote() noteType:%d  note idx=%d", nNote->m_noteType, nNote->m_idx);
}

void PlayGame_NoteView::ProC_LongNote(NoteObject * note, int trackIdx,
	int idx) {
	PlayGame* pGame = (PlayGame*) this->getParent();
	float noteTouchW = NOTE_TOUCH_W * 2;
	float revisionMS_Time = (m_pTimeUpdate - m_pTimeUpdateBefore) / 2.0f;

	for (int i = 0; i < MAX_TOUCHES; i++) {
		TouchMgr::CTouch* touch = touchMgr.GetTouch(i);

		if (touch->type != TOUCH_TYPE_LONG
			&& touch->type != TOUCH_TYPE_VIBRATO) {
			continue;
		}

		if (touch->type == TOUCH_TYPE_VIBRATO
			&& note->m_noteType == NOTE_TYPE_VIBRATO_MUTE
			&& touch->groupID[SMALL_GROUP]
			== note->m_GroupID[SMALL_GROUP]) {
			if (touch->moveCnt <= 0
				&& (note->m_length > 1 && note->m_isAutoProcessNote == false)) {
				log(
					"+++++++++++++++++++++++++ [] moveCnt:%d  %d group failed!!!!!!!!!!",
					touch->moveCnt, note->m_GroupID[SMALL_GROUP]);
				if (touch->soundID >= 0) {
					auto playID = touch->soundID;
					IZAudioEngine::getInstance()->fadeStopEffect(playID, 0.1f);
				}
				pGame->setScore(note, NOTE_MISS);
				touchInit(i);
				return;
			}
		}
		else {
			if (touch->py > -1000.0f) {
				if (note->m_isDelete == false) {

					if (note->m_noteType == NOTE_TYPE_MUTE_NORMAL_LONG
						|| note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG) {
						if (note->m_sprLine != NULL) {
							float beforeLength = note->m_lineLength
								* getSpeedVal(trackIdx);
							float tmpLength = (note->m_sprLine->getPositionY())
								- m_perfectPosition;

							if (tmpLength < beforeLength
								&& note->m_longNoteTouch == 2) {
								if (note->m_lineLength_effect % 7 == 0
									&& note->m_lineLength_effect > 0
									&& note->m_lineLength_effectIdx
									!= note->m_lineLength_effect) {
									actionNoteEffect(trackIdx, idx,
										note->m_sprNote->getPositionX(),
										m_perfectPosition);
									note->m_lineLength_effectIdx =
										note->m_lineLength_effect;
									pGame->addLongNoteFever();
								}
							}
						}
					}

					//if (note->m_sprNote->getPositionY() > m_perfectPosition)
					if ((m_pTimeUpdate + revisionMS_Time) < note->m_time) 
					{
						continue;
					}

					if (note->m_effPlay == true)
					{
						if (note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG) 
						{
							if (note->m_length == 1)
							{
								setTouchSuccess(note);
							}
							deleteNote(note, trackIdx);
						}
						else
						{
							if (touch->lineNum == note->m_lineX)
							{
								bool isCheck = true;
								if (note->m_noteType
									== NOTE_TYPE_BANDING_MUTE) {
									if (touch->isMoving == false) {
										isCheck = false;
										if (touch->soundID >= 0) {
											auto playID = touch->soundID;
											IZAudioEngine::getInstance()->fadeStopEffect(
												playID, 0.1f);
										}
										pGame->setScore(note, NOTE_MISS);
										touchInit(i);
									}
								}
								if (isCheck)
								{
									//터치 이펙트
									if (isMuteNote(note) == true) 
									{
										if (note->m_length == 1) 
										{
											setTouchSuccess(note);
										}
										deleteNote(note, trackIdx);
									}
									else if (note->m_noteType == NOTE_TYPE_TREMOLO)
									{
										procTouchNote(trackIdx, idx, note,touch);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void PlayGame_NoteView::touchesBegan(
	const std::vector<cocos2d::Touch*>& pTouches, cocos2d::Event* pEvnet) {
	PlayGame* pGame = (PlayGame*) this->getParent();
	/*if (pGame->m_MenuLayer->isVisible() == true)
	 return;*/

	if (pGame->m_isPause == true)
		return;

	for (auto &touch : pTouches) {
		auto location = touch->getLocation();
		//auto locationb = touch->getLocationInView();

		float val = (location.y / PTR_GAMEMAIN->m_WinSize.height);
		Point tmpTP;
		if (location.x < (PTR_GAMEMAIN->m_WinSize.width / 2)) {
			tmpTP.x =
				location.x
				- (((PTR_GAMEMAIN->m_WinSize.width / 2) - location.x)
				* val); //0.025f
		}
		else {
			tmpTP.x =
				location.x
				+ ((location.x - (PTR_GAMEMAIN->m_WinSize.width / 2))
				* val);
		}
		tmpTP.y = location.y;

		//log("#### val:%f _location x = %f, y = %f    x = %f, y = %f", val, location.x, location.y, tmpTP.x, tmpTP.y);
		//log("#### <%s:%d> _location x = %f, y = %f    x = %f, y = %f", __FUNCTION__, __LINE__, location.x, location.y, tmpTP.x, tmpTP.y);
		if (pGame->m_TestFreePaly == false) {
			touchBeganHandler(tmpTP, location, touch->getID());
		}
		else {
			//touchBeganHandler_FreePlay(tmpTP, touch->getID());
		}
	}
}

void PlayGame_NoteView::touchesMove(
	const std::vector<cocos2d::Touch*>& pTouches, cocos2d::Event* pEvnet) {
	PlayGame* pGame = (PlayGame*) this->getParent();
	/*
	 if (pGame->m_MenuLayer->isVisible() == true)
	 return;*/

	if (pGame->m_isPause == true)
		return;

	for (auto &touch : pTouches) {
		auto location = touch->getLocation();

		float val = (location.y / PTR_GAMEMAIN->m_WinSize.height);
		Point tmpTP;
		if (location.x < (PTR_GAMEMAIN->m_WinSize.width / 2)) {
			tmpTP.x =
				location.x
				- (((PTR_GAMEMAIN->m_WinSize.width / 2) - location.x)
				* val); //0.025f
		}
		else {
			tmpTP.x =
				location.x
				+ ((location.x - (PTR_GAMEMAIN->m_WinSize.width / 2))
				* val);
		}
		tmpTP.y = location.y;

		if (pGame->m_TestFreePaly == false) {
			touchMoveHandler(tmpTP, location, touch->getID());
		}
	}

}

void PlayGame_NoteView::touchesEnd(const std::vector<cocos2d::Touch*>& pTouches,
	cocos2d::Event* pEvnet) {
	PlayGame* pGame = (PlayGame*) this->getParent();
	/*
	 if (pGame->m_MenuLayer->isVisible() == true)
	 return;*/

	if (pGame->m_isPause == true)
		return;

	for (auto &touch : pTouches) {
		auto location = touch->getLocation();

		float val = (location.y / PTR_GAMEMAIN->m_WinSize.height);
		Point tmpTP;
		if (location.x < (PTR_GAMEMAIN->m_WinSize.width / 2)) {
			tmpTP.x =
				location.x
				- (((PTR_GAMEMAIN->m_WinSize.width / 2) - location.x)
				* val); //0.025f
		}
		else {
			tmpTP.x =
				location.x
				+ ((location.x - (PTR_GAMEMAIN->m_WinSize.width / 2))
				* val);
		}
		tmpTP.y = location.y;

		if (pGame->m_TestFreePaly == false) {
			touchEndHandler(tmpTP, touch->getID());
		}
	}
}

void PlayGame_NoteView::procTouchNote(int _trackIdx, int _noteIdx,
	NoteObject* _note, TouchMgr::CTouch* _touch) {
	if (_note->m_sprNote != NULL) {
		_note->m_sprNote->setOpacity(0);
	}

	long long effID = 0;
	//char path[512];

	//if (effectPath(path, _trackIdx, _note->m_effIndex, _note->m_NoteFx) == true)
	if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_AUTOPLAY].isHave == false || (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_AUTOPLAY].isHave == true && _trackIdx != PTR_GAMEMAIN->m_SelectChannel)) {
		effID = _playEffect(_note, _trackIdx);

		//log("++++procTouchNote()    idx[%d]        _note->m_lineNum : %d    effID:%lld ", _note->m_idx, _note->m_lineNum, effID);

		if (effID >= 0) {
			setPlayNoteTime(_trackIdx, effID, _note->m_duration, _note->m_time);
		}
	}

	if (_touch != NULL) {
		_touch->soundID = effID;
		//_touch->effIdx = _note->m_effIndex;
		if (_note->m_noteType == NOTE_TYPE_VIBRATO
			|| _note->m_noteType == NOTE_TYPE_VIBRATO_MUTE) {
			_touch->type = TOUCH_TYPE_VIBRATO;
			log("procTouchNote()     TOUCH_TYPE_VIBRATO !!!!!!!!!!!!!!!    ");
		}
		else if ((_note->m_noteType >= NOTE_TYPE_HIDDEN_LONG
			&& _note->m_noteType <= NOTE_TYPE_DRAG_LONG)
			|| (_note->m_noteType == NOTE_TYPE_BANDING
			|| _note->m_noteType == NOTE_TYPE_BANDING_MUTE)) //롱노트일때
		{
			_touch->type = TOUCH_TYPE_LONG;
		}
		else if (_note->m_noteType == NOTE_TYPE_TREMOLO) {
			_touch->type = TOUCH_TYPE_TREMOLO;
		}
		else {
			_touch->type = TOUCH_TYPE_NONE;
		}
		//log("@@@@@@@@@@@@@@@@@@@@@@@@@@@ = %d, %d", _touch->type, _touch->id);
	}

	if (PTR_GAMEMAIN->m_SelectChannel == _trackIdx) {
		if (isMuteNote(_note) == false) {
			//if (_note->m_sprNote->getPositionY() < PTR_GAMEMAIN->m_WinSize.height)
			{
				_note->m_effPlay = false;
				if (isALLChordNote(_note) == true
					&& _note->m_noteType != NOTE_TYPE_AUTO_CHORD) {
					ProC_ChordNoteEffect(_note, _trackIdx);
				}
				else {
					if (_note->m_noteType >= NOTE_TYPE_AUTO
						&& _note->m_noteType <= NOTE_TYPE_NORMAL_AUTO) {
						if (isAutoHead(_note, true) == true) {
							if (_note->m_noteType == NOTE_TYPE_AUTO_CHORD) {
								ProC_ChordNoteEffect(_note, _trackIdx);
							}
							else {
								actionNoteEffect(_trackIdx, _noteIdx,
									_note->m_sprNote->getPositionX(),
									m_perfectPosition);
								setTouchSuccess(_note);
							}
						}
					}
					else if (_note->m_noteType == NOTE_TYPE_LEFT_SLID
						|| _note->m_noteType == NOTE_TYPE_RIGHT_SLID) {
						actionNoteEffect(_trackIdx, _noteIdx,
							_note->m_sprNote->getPositionX(),
							m_perfectPosition);
						if (_note->m_childSize[SMALL_GROUP] > 0) {
							setTouchSuccess(_note);
						}
					}
					else {
						actionNoteEffect(_trackIdx, _noteIdx,
							_note->m_sprNote->getPositionX(),
							m_perfectPosition);
						setTouchSuccess(_note);
					}
				}
				actionNoteEffectLine(_trackIdx, _noteIdx,
					_note->m_sprNote->getPositionX(), m_perfectPosition);
			}
		}
	}
	deleteNote(_note, _trackIdx);
}

void PlayGame_NoteView::setTouchSuccess(NoteObject * note)
{
	PlayGame* playGame = (PlayGame*) this->getParent();

	playGame->setScore(note, NOTE_PERFECT);
	//log("++++++++++setTouchFailed()  SuccessCNT:%d   m_ComboCnt[0]:%d   m_ComboCnt[1]:%d ", m_SuccessCnt[0], m_ComboCnt[0], m_ComboCnt[1]);
}

void PlayGame_NoteView::ProC_ChordNote(NoteObject * note, int trackIdx,
	int GroupType, TouchMgr::CTouch* touch, bool isDragChord, bool isAuto) {
	int startIdx = note->m_idx - 6;

	if (startIdx < 0)
		startIdx = 0;
	for (int k = startIdx; k < startIdx + 20; k++) {
		if (k < m_selMaxCnt[trackIdx]) {
			NoteObject* findNote = m_note[trackIdx][k];

			if (findNote->m_isDelete == false) {
				if (findNote->m_isSprLoad == true) {
					if (isDragChord == true) {
						if (findNote->m_noteType == NOTE_TYPE_DRAG_CHORD
							&& findNote->m_GroupID[GroupType] > 0
							&& findNote->m_GroupID[GroupType]
							== note->m_GroupID[GroupType]) //그룹아이디가 같으면 처리
						{
							if (isAuto) {
								findNote->m_isAutoProcessNote = true;
							}
							else {
								procTouchNote(trackIdx, k, findNote, touch);
							}
						}
					}
					else if (isDragChord == false) {
						if (isChordNote(findNote)
							&& findNote->m_GroupID[GroupType] > 0
							&& findNote->m_GroupID[GroupType]
							== note->m_GroupID[GroupType]) //그룹아이디가 같으면 처리
						{
							if (note->m_idx != k) {
								if (isAuto) {
									findNote->m_isAutoProcessNote = true;
								}
								else {
									procTouchNote(trackIdx, k, findNote, touch);
								}
							}
						}
					}
				}
			}
		}
	}
}

/*
 void PlayGame_NoteView::touchBeganHandler_FreePlay(cocos2d::Point _tPos, int _touchId)
 {
 PlayGame* pGame = (PlayGame*)this->getParent();
 if (pGame->m_TestAutoPaly == true)
 return;

 Point notePt;
 int t = PTR_GAMEMAIN->m_SelectChannel;
 int noteCnt = 0;

 TouchMgr::CTouch* touch = touchMgr.GetTouch(_touchId);

 //터치관련 초기화및 값 셋팅
 touch->px = _tPos.x;
 touch->py = _tPos.y;
 touch->effIdx = -1;
 touch->soundID = -1;


 int maxCheckCnt = m_curNoteIdx[t] + NOTE_TOUCH_CHECK_CNT;

 if (maxCheckCnt > m_selMaxCnt[t])
 maxCheckCnt = m_selMaxCnt[t];

 int loopCount = 0;
 for (int n = m_curNoteIdx[t]; n < maxCheckCnt; n++)
 {
 loopCount++;
 NoteObject* note = m_note[t][n];
 if (note->m_isDelete == false)
 {
 if (note->m_sprNote->isVisible() == false)
 continue;
 notePt.x = note->m_sprNote->getPositionX();
 notePt.y = note->m_sprNote->getPositionY();


 if (notePt.y > DEFAULT_FAR_PLANE)
 continue;

 if (note->m_effPlay == false)
 continue;


 if (isMuteNote(note) == false)
 {
 Point CheckPo;
 CheckPo.x = _tPos.x;
 CheckPo.y = m_perfectPosition;

 PTR_GAMEMAIN->m_compare[noteCnt].value = notePt.y;
 PTR_GAMEMAIN->m_compare[noteCnt].key = n;

 if (noteCnt < MAX_COMPARE_VALUE - 1)
 {
 noteCnt++;
 }
 }
 }

 if (loopCount > NOTE_TOUCH_CHECK_CNT)
 break;
 }


 if (noteCnt > 0)
 {
 if (noteCnt > 1)
 {
 PTR_GAMEMAIN->_bubbleSort(PTR_GAMEMAIN->m_compare, noteCnt, pGame->m_TestFreePaly);
 }

 int noteIdx = PTR_GAMEMAIN->m_compare[0].key;



 NoteObject* note = m_note[t][noteIdx];


 {

 long long effID = 0;
 char path[512];
 int startIdx = note->m_idx - 6;

 if (startIdx < 0)
 startIdx = 0;
 for (int k = startIdx; k < startIdx + 20; k++)
 {
 if (k < m_selMaxCnt[t])
 {
 NoteObject* findNote = m_note[t][k];

 if (findNote->m_isDelete == false)
 {
 if (note->m_point == findNote->m_point)
 {
 if (effectPath(path, t, findNote->m_effIndex, findNote->m_NoteFx) == true)
 {
 effID = _playEffect(findNote, t, path);

 if (effID >= 0)
 {
 setPlayNoteTime(t, effID, findNote->m_duration);
 }
 }

 if (findNote->m_sprNote != NULL)
 {
 //actionNoteEffect(t, findNote->m_idx, findNote->m_sprNote->getPositionX(), m_perfectPosition);
 actionNoteEffectLine(t, findNote->m_idx, findNote->m_sprNote->getPositionX(), m_perfectPosition);
 }
 deleteNote(findNote, t);
 }
 }
 }
 }
 }
 }
 }
 */

void PlayGame_NoteView::touchBeganHandler(cocos2d::Point _tPos,
	cocos2d::Point _real_tPos, int _touchId) {
	PlayGame* pGame = (PlayGame*) this->getParent();
	if (pGame->m_TestAutoPaly == true)
		return;

	float noteTouchW = NOTE_TOUCH_W;
	Point notePt;
	int t = PTR_GAMEMAIN->m_SelectChannel;
	float touchPosY = getJudgmentValue(0);
	int noteCnt = 0;

	TouchMgr::CTouch* touch = touchMgr.GetTouch(_touchId);

	//터치관련 초기화및 값 셋팅
	touch->px = _tPos.x;
	touch->py = _tPos.y;
	touch->realX = _real_tPos.x;
	touch->realY = _real_tPos.y;

	touch->effIdx = -1;
	touch->soundID = -1;

	Rect NoteRect;

	int maxCheckCnt = m_curNoteIdx[t] + NOTE_TOUCH_CHECK_CNT;

	if (maxCheckCnt > m_selMaxCnt[t])
		maxCheckCnt = m_selMaxCnt[t];

	int loopCount = 0;
	for (int n = m_curNoteIdx[t]; n < maxCheckCnt; n++) {
		loopCount++;
		NoteObject* note = m_note[t][n];
		if (note->m_isDelete == false) {
			if (note->m_sprNote->isVisible() == false)
				continue;
			notePt.x = note->m_sprNote->getPositionX();
			notePt.y = note->m_sprNote->getPositionY();

			NoteRect.setRect(notePt.x - (NOTE_TOUCH_W),
				notePt.y - (NOTE_TOUCH_H), NOTE_TOUCH_W * 2,
				NOTE_TOUCH_H * 2);

			//if (notePt.y > GET_VISIBLE_SIZE().height)
			if ((m_pTimeUpdate) <= (note->m_time - 500))
				continue;

			if (note->m_effPlay == false)
				continue;
			Point ttp;
			ttp.setPoint(_tPos.x, m_perfectPosition);
			//if (NoteRect.containsPoint(_tPos))
			if (NoteRect.containsPoint(ttp)) {
				//log("++++   x:%f, y:%f     touch x:%f y:%f", notePt.x, notePt.y, _tPos.x, _tPos.y);
				if (isMuteNote(note) == false && note->m_isNotTouch == false) // && isRealDragNote(note) == false)
				{
					if (isRealDragNote(note) == false
						|| (isRealDragNote(note) == true
						&& note->m_childSize[BIG_GROUP] > 0)
						|| (isRealDragNote(note) == true
						&& note->m_childSize[SMALL_GROUP] > 0)) {
						Point CheckPo;
						CheckPo.x = _tPos.x;
						CheckPo.y = m_perfectPosition;

						float dis = TouchMgr::GetDistanceSq(CheckPo, notePt);
						PTR_GAMEMAIN->m_compare[noteCnt].value = notePt.y;
						PTR_GAMEMAIN->m_compare[noteCnt].value2 = dis;
						PTR_GAMEMAIN->m_compare[noteCnt].key = n;

						if (noteCnt < MAX_COMPARE_VALUE - 1) {
							noteCnt++;
						}
					}
				}
			}
		}

		if (loopCount > NOTE_TOUCH_CHECK_CNT)
			break;
	}

	if (noteCnt > 0) {
		if (noteCnt > 1) {
			PTR_GAMEMAIN->_bubbleSort(PTR_GAMEMAIN->m_compare, noteCnt,
				pGame->m_TestFreePaly);
		}

		int noteIdx = PTR_GAMEMAIN->m_compare[0].key;

		//log("############3333######## =%d, %f", m_compare[0].key, m_compare[0].value2);

		NoteObject* note = m_note[t][noteIdx];

		float n_yy = note->m_sprNote->getPositionY();

		if ((note->m_noteType == NOTE_TYPE_AUTO
			|| note->m_noteType == NOTE_TYPE_NORMAL_AUTO
			|| note->m_noteType == NOTE_TYPE_AUTO_CHORD)
			&& note->m_childSize[BIG_GROUP] > 0) {
			//AUTO NOTE가 대그룹일때 먼저 처리
			if (note->m_isAutoProcessNote == false
				&& note->m_GroupID[BIG_GROUP] > 0) {
				int tmpIdx = note->m_idx;
				for (int auto_i = 0; auto_i < note->m_childSize[BIG_GROUP];
					auto_i++) {
					NoteObject * tmpNote =
						m_note[t][note->m_childIdx[BIG_GROUP][auto_i]];
					if (tmpNote->m_isAutoProcessNote == false) {
						tmpNote->m_isAutoProcessNote = true;
					}
				}
			}
		}
		else {
			if (isChordNote(note) == true) {
				// 같은 높이의 화음노트를 다 터트려줌
				ProC_ChordNote(note, t, SMALL_GROUP, touch, false, false);
			}

			if (isSlide(note) == true) {
				touch->active = true;
			}

			touch->isMoving = false;
			touch->moveCnt = 20;
			touch->isNotCheckMove = false;
			touch->lineNum = note->m_lineX;
			touch->groupID[SMALL_GROUP] = note->m_GroupID[SMALL_GROUP];
			touch->groupID[BIG_GROUP] = note->m_GroupID[BIG_GROUP];
			touch->tailIdx = 0;

			if (note->m_longChileSize > 0) {
				touch->tailIdx = note->m_longChileIdx[1];
				NoteObject * _tmpNote = m_note[t][touch->tailIdx];
				_tmpNote->m_longNoteTouch = 2;
			}

			if (note->m_noteType == NOTE_TYPE_DOUBBLE_SLIDE) {
				return;
			}

			if (isDragNote(note) == false) {
				procTouchNote(t, noteIdx, note, touch);

				if (note->m_noteType == NOTE_TYPE_NORMAL
					|| note->m_noteType == NOTE_TYPE_CHORD) {
					touch->isNotCheckMove = true;
				}
			}
			else if ((isRealDragNote(note) == true
				&& note->m_childSize[BIG_GROUP] > 0)
				|| (isRealDragNote(note) == true
				&& note->m_childSize[SMALL_GROUP] > 0)) {
				note->m_isAutoProcessNote = true;
			}

			if ((note->m_noteType == NOTE_TYPE_AUTO
				|| note->m_noteType == NOTE_TYPE_NORMAL_AUTO)
				&& note->m_childSize[SMALL_GROUP] > 0) {
				//AUTO NOTE가 소그룹일때 먼저 처리
				for (int auto_i = 0; auto_i < note->m_childSize[SMALL_GROUP];
					auto_i++) {
					NoteObject * tmpNote =
						m_note[t][note->m_childIdx[SMALL_GROUP][auto_i]];
					if (tmpNote->m_isAutoProcessNote == false) {
						tmpNote->m_isAutoProcessNote = true;
					}
				}
			}
		}
	}
}

void PlayGame_NoteView::touchMoveHandler(cocos2d::Point _tPos,
	cocos2d::Point _real_tPos, int _touchId) {
	PlayGame* pGame = (PlayGame*) this->getParent();
	if (pGame->m_TestAutoPaly == true)
		return;

	Rect NoteRect;
	TouchMgr::CTouch *touch = touchMgr.GetTouch(_touchId);

	float noteTouchW = NOTE_TOUCH_W;
	float noteX, noteY;
	int t = PTR_GAMEMAIN->m_SelectChannel;
	float touchPosY = getJudgmentValue(0);

	if (_tPos.y > touchPosY)
		return;

	if (touch->py == -1000.0f)
		return;

	int tmpDir = 0;

	if (touch->isNotCheckMove == true) //일반노트가 앞에서 처리가 되면 무시하자
		return;

	if (touch->groupID[SMALL_GROUP] > 0 && touch->type == TOUCH_TYPE_LONG) //banding 처리 
	{
		if (touch->isMoving == false) {
			log("touchMoveHandler()  groupID:%d  touch->px:%f  _tPos.x:%f",
				touch->groupID[SMALL_GROUP], touch->realX, _real_tPos.x);
			//if (touch->realX + 5.0f < _real_tPos.x)
			if (abs(_real_tPos.x - touch->realX) >= 2.0f) {
				log("touchMoveHandler()  SUCCESS!!!  val:%f  _tPos.x:%f",
					(_real_tPos.x - touch->realX));
				touch->isMoving = true;
			}
		}
	}

	int maxCheckCnt = m_curNoteIdx[t] + 15;

	if (maxCheckCnt > m_selMaxCnt[t])
		maxCheckCnt = m_selMaxCnt[t];

	int loopCount = 0;

	for (int n = m_curNoteIdx[t]; n < maxCheckCnt; n++) 
	{
		loopCount++;
		NoteObject* note = m_note[t][n];
		if (note->m_isDelete == false) 
		{
			//if (note->m_sprNote->isVisible() == false)
			//	continue;

			if (isMovePoCNote(note) == false)
				continue;

			if (note->m_effPlay == false)
				continue;

			noteX = note->m_sprNote->getPositionX();
			noteY = note->m_sprNote->getPositionY();
			NoteRect.setRect(noteX - (NOTE_TOUCH_W), noteY - (NOTE_TOUCH_H), NOTE_TOUCH_W * 2, NOTE_TOUCH_H * 2);

			if (NoteRect.containsPoint(_tPos))
			{
				switch (note->m_noteType) {
				case NOTE_TYPE_NORMAL_DRAG:
				case NOTE_TYPE_DRAG_LONG:
				case NOTE_TYPE_DRAG_CHORD:
				{
					if (note->m_isAutoProcessNote == true)
						continue;
					//if (noteY <= 100.0f)

					if ((note->m_time - 100) <= m_pTimeUpdate)
					{
						if (note->m_noteType == NOTE_TYPE_DRAG_LONG) 
						{
							touch->lineNum = note->m_lineX;
							touch->type = TOUCH_TYPE_LONG;
							note->m_isAutoProcessNote = true;
							note->m_touchID = _touchId;
							if (note->m_longChileSize > 0)
							{
								touch->tailIdx = note->m_longChileIdx[1];
								NoteObject * _tmpNote = m_note[t][touch->tailIdx];
								_tmpNote->m_longNoteTouch = 2;
							}
						}
						else //DRAG 노트
						{
							if (NoteRect.containsPoint(_tPos)) 
							{
								if (isRealDragNote(note) == true && touch->active == false)
								{
									touch->active = true;
									touch->lineNum = note->m_lineX;
									touch->groupID[SMALL_GROUP] = note->m_GroupID[SMALL_GROUP];
									touch->groupID[BIG_GROUP] = note->m_GroupID[BIG_GROUP];
								}
							}

							if (touch->active == true && note->m_GroupID[BIG_GROUP] > 0 && touch->groupID[BIG_GROUP] == note->m_GroupID[BIG_GROUP]) 
							{
								if (note->m_noteType == NOTE_TYPE_DRAG_CHORD && note->m_GroupID[SMALL_GROUP] > 0) 
								{
									// 같은 높이의 화음노트를 다 터트려줌
									ProC_ChordNote(note, t, SMALL_GROUP, touch, true, true);
								}
								else 
								{
									note->m_isAutoProcessNote = true;
								}
							} //스몰 그룹은 드래그 노트만 처리
							else if (touch->active == true && note->m_GroupID[SMALL_GROUP] > 0 && touch->groupID[SMALL_GROUP] == note->m_GroupID[SMALL_GROUP])
							{
								if (note->m_noteType == NOTE_TYPE_NORMAL_DRAG)
								{
									note->m_isAutoProcessNote = true;
								}
								else if (note->m_noteType == NOTE_TYPE_DRAG_CHORD && note->m_GroupID[SMALL_GROUP] > 0)
								{
									// 같은 높이의 화음노트를 다 터트려줌
									ProC_ChordNote(note, t, SMALL_GROUP, touch, true, true);
								}
							}
						}
					}
					//else if (noteY <= 0.0f)
					else if (note->m_time <= m_pTimeUpdate)
					{
						if (note->m_noteType == NOTE_TYPE_DRAG_LONG) 
						{
							touch->lineNum = note->m_lineX;
							procTouchNote(t, n, note, touch);
						}
						else //DRAG 노트
						{
							if (NoteRect.containsPoint(_tPos)) 
							{
								if (isRealDragNote(note) == true && touch->active == false)
								{
									touch->active = true;
									touch->lineNum = note->m_lineX;
									touch->groupID[SMALL_GROUP] = note->m_GroupID[SMALL_GROUP];
									touch->groupID[BIG_GROUP] = note->m_GroupID[BIG_GROUP];
								}
							}

							if (touch->active == true && note->m_GroupID[BIG_GROUP] > 0 && touch->groupID[BIG_GROUP] == note->m_GroupID[BIG_GROUP])
							{
								if (note->m_noteType == NOTE_TYPE_DRAG_CHORD && note->m_GroupID[SMALL_GROUP] > 0) 
								{
									// 같은 높이의 화음노트를 다 터트려줌
									ProC_ChordNote(note, t, SMALL_GROUP, touch, true, false);
								}
								else 
								{
									procTouchNote(t, n, note, NULL);
								}
							} //스몰 그룹은 드래그 노트만 처리
							else if (touch->active == true && note->m_GroupID[SMALL_GROUP] > 0 && touch->groupID[SMALL_GROUP] == note->m_GroupID[SMALL_GROUP]) 
							{
								if (note->m_noteType == NOTE_TYPE_NORMAL_DRAG)
								{
									procTouchNote(t, n, note, NULL);
								}
								else if (note->m_noteType == NOTE_TYPE_DRAG_CHORD && note->m_GroupID[SMALL_GROUP] > 0)
								{
									// 같은 높이의 화음노트를 다 터트려줌
									ProC_ChordNote(note, t, SMALL_GROUP, touch, true, false);
								}
							}
						}
					}
				}
										   break;

				case NOTE_TYPE_LEFT_SLID:
				case NOTE_TYPE_RIGHT_SLID: {
					if (noteY > getJudgmentValue(0))
						continue;

					if (touch->realX + 30 < _real_tPos.x) {
						tmpDir = 1; //RIGHT
					}
					else if (touch->realX - 30 > _real_tPos.x) {
						tmpDir = 2; //LEFT
					}

					if (note->m_noteType == NOTE_TYPE_LEFT_SLID) {
						if (touch->active == true && tmpDir == 2
							&& note->m_isGroup[SMALL_GROUP] == true
							&& touch->groupID[SMALL_GROUP]
							== note->m_GroupID[SMALL_GROUP]
							&& note->m_isAutoProcessNote == false) {
							if (note->m_childSize[SMALL_GROUP] > 0) {
								for (int child_i = 0;
									child_i < note->m_childSize[SMALL_GROUP];
									child_i++) {
									int tmpIdx =
										note->m_childIdx[SMALL_GROUP][child_i];
									NoteObject* tmpNote = m_note[t][tmpIdx];
									tmpNote->m_isAutoProcessNote = true;
									//log("!!!!!!!!!!!!!!!!!!! <NOTE_TYPE_LEFT_SLID>  tmpIdx:%d", tmpIdx);
								}
								tmpDir = 0;
								return;
							}
						}
					}
					else if (note->m_noteType == NOTE_TYPE_RIGHT_SLID) {
						if (touch->active == true && tmpDir == 1
							&& note->m_isGroup[SMALL_GROUP] == true
							&& touch->groupID[SMALL_GROUP]
							== note->m_GroupID[SMALL_GROUP]
							&& note->m_isAutoProcessNote == false) {
							if (note->m_childSize[SMALL_GROUP] > 0) {
								for (int child_i = 0;
									child_i < note->m_childSize[SMALL_GROUP];
									child_i++) {
									int tmpIdx =
										note->m_childIdx[SMALL_GROUP][child_i];
									NoteObject* tmpNote = m_note[t][tmpIdx];
									tmpNote->m_isAutoProcessNote = true;
									//log("!!!!!!!!!!!!!!!!!!! <NOTE_TYPE_RIGHT_SLID> tmpIdx:%d", tmpIdx);
								}
								tmpDir = 0;
								return;
							}
						}
					}
				}
										   break;

				case NOTE_TYPE_VIBRATO:
				case NOTE_TYPE_VIBRATO_MUTE: {
					if (note->m_isAutoProcessNote == true)
						continue;

					if ((note->m_time - 100) <= m_pTimeUpdate) {
						if (note->m_GroupID[SMALL_GROUP] > 0
							&& touch->groupID[SMALL_GROUP]
							== note->m_GroupID[SMALL_GROUP]) {
							if (touch->moveCnt > 0) {
								if (abs(touch->realX - _real_tPos.x) > 0.5f) {
									touch->moveCnt = 20;
									note->m_isAutoProcessNote = true;
								}
								/*
								 else
								 {
								 touch->moveCnt = 0;
								 }
								 */
								touch->realX = _real_tPos.x;
								touch->realY = _real_tPos.y;
							}
						}
					}
				}
											 break;
											 /*
											  case NOTE_TYPE_BANDING:
											  case NOTE_TYPE_BANDING_MUTE:
											  {
											  if (noteY > getJudgmentValue(0))
											  continue;

											  //if (touch->lineNum == note->m_lineX)
											  if (note->m_GroupID[SMALL_GROUP] > 0 && touch->groupID[SMALL_GROUP] == note->m_GroupID[SMALL_GROUP])
											  {
											  if (touch->px + 10 < _tPos.x)
											  {
											  touch->isMoving = true;
											  }
											  }

											  }
											  break;
											  */

				case NOTE_TYPE_DOUBBLE_SLIDE: {
					if (noteY > getJudgmentValue(0))
						continue;
					if (touch->realX + 10 < _real_tPos.x) {
						if (touch->lineNum == note->m_lineX) {
							note->m_isAutoProcessNote = true;
							touchInit(_touchId);
							//return;
						}
					}
				}
											  break;

				}
			}
		}
		if (loopCount > 15)
			break;
	}

	if (touch->type == TOUCH_TYPE_VIBRATO) {
		touch->realX = _real_tPos.x;
		touch->realY = _real_tPos.y;
	}
}

void PlayGame_NoteView::touchEndHandler(cocos2d::Point _tPos, int _touchId) {
	PlayGame* pGame = (PlayGame*) this->getParent();

	if (pGame->m_TestAutoPaly == true)
		return;

	TouchMgr::CTouch *touch = touchMgr.GetTouch(_touchId);

	for (int i = 0; i < MAX_COMPARE_VALUE; i++) {
		PTR_GAMEMAIN->m_compare[i].key = 0;
		PTR_GAMEMAIN->m_compare[i].value = 0;
	}

	if (touch->type == TOUCH_TYPE_NONE) {
	}
	else if (touch->type == TOUCH_TYPE_LONG
		|| touch->type == TOUCH_TYPE_VIBRATO) {
		if (touch->soundID >= 0) {
			auto playID = touch->soundID;
			IZAudioEngine::getInstance()->fadeStopEffect(playID, 0.1f);
		}

		noteTouchEnd(_touchId);
	}

	touchInit(_touchId);
}

void PlayGame_NoteView::touchInit(int _touchID) {
	TouchMgr::CTouch* touch = touchMgr.GetTouch(_touchID);

	touch->type = TOUCH_TYPE_NONE;
	touch->px = 0.0f;
	touch->py = -1000.0f;
	touch->effIdx = -1;
	touch->active = false;
	touch->lineNum = -1;
	touch->groupID[SMALL_GROUP] = -1;
	touch->groupID[BIG_GROUP] = -1;
	touch->isNotCheckMove = false;
	touch->isMoving = false;

	touch->tailIdx = 0;
}

void PlayGame_NoteView::noteTouchEnd(int _touchId) {
	int count = 0;
	int t = PTR_GAMEMAIN->m_SelectChannel;
	float noteTouchW = NOTE_TOUCH_W;
	Point notePt;
	TouchMgr::CTouch* touch = touchMgr.GetTouch(_touchId);


	if (touch->tailIdx > 0) {
		PlayGame* pGame = (PlayGame*) this->getParent();
		//NoteObject* note = m_note[t][noteNum];
		//pGame->setScore(note, NOTE_MISS);

		NoteObject * _tmpNote = m_note[t][touch->tailIdx];

		if (_tmpNote->m_sprLine != NULL)
		{
			if (_tmpNote->m_sprLine->getPositionY() > m_perfectPosition) 
			{
				log("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& LONG NOTE  not yet!!!!!!!!!!!!!!!!!!");
				_tmpNote->m_sprLine->setColor(Color3B(100, 100, 100));
				_tmpNote->m_sprNote->setColor(Color3B(100, 100, 100));
				_tmpNote->m_longNoteTouch = 1;

				pGame->setScore(_tmpNote, NOTE_MISS);
			}
		}
	}
}

float PlayGame_NoteView::getJudgmentValue(int type) {
	float value = m_perfectPosition + 240.0f;

	return value;
}

long long PlayGame_NoteView::_playEffect(NoteObject* _note, int track) {
	PlayGame* pGame = (PlayGame*) this->getParent();

#ifdef MUSICIAN_US
	float vol = 127.0f;
	if (_note != NULL)
	{
		vol = (float)_note->m_velocity;
	}
	long long id = -1;

	vol = vol / 127.0f;

	if (isHiddenNote(_note) == true)
	{
		return -1;
	}

	if (PTR_GAMEMAIN->m_SelectChannel == track)
	{
		vol = (vol * (pGame->m_Volume_main / 100.0f));

		if (pGame->m_Volume_main < 1.0f)
			return -1;
	}
	else
	{
		vol = (vol * (pGame->m_Volume_other / 100.0f));
		if (pGame->m_Volume_other < 1.0f)
			return -1;
	}

	if (vol< 0.0f || vol > 1.0f)
	{
		vol = 1.0f;
	}

	id = IZAudioEngine::getInstance()->playEffect(_note->m_FullPath, false, 1.0f, 0.0f, vol);
#else
	float noteVeloicty = 127.0f;
	if (_note != NULL) {
		noteVeloicty = (float)_note->m_velocity;
	}
	long long id = -1;

	float userVolume = 0.0f;
	if (PTR_GAMEMAIN->m_SelectChannel == track) {

		userVolume = pGame->m_Volume_main;
		if (pGame->m_Volume_main < 1.0f)
			return -1;
	}
	else {

		userVolume = pGame->m_Volume_other;
		if (pGame->m_Volume_other < 1.0f)
			return -1;
	}

	//log("+++++++++++++ _playEffect()  [%d] %f ", _note->m_idx, _note->m_Pan);

#if(CC_TARGET_PLATFORM == CC_PLATFORM_WIN32 )
	id = IZAudioEngine::getInstance()->playEffect(_note->m_FullPath, false, _note->m_Pitch, _note->m_Pan, noteVeloicty, userVolume);
#else
#ifdef CHORD_MIXING
	id = IZAudioEngine::getInstance()->playEffect(_note);
#else
	id = IZAudioEngine::getInstance()->playEffect(_note);
#endif
#endif

	if (PTR_GAMEMAIN->m_GameMode == PlayMode::PREMIUM) {
		int playTime = 0;
		if (PTR_GAMEMAIN->m_SelectChannel == track) {
			playTime = (PTR_GAMEMAIN->getUMicroSecond()
				- PTR_GAMEMAIN->m_MusicPlayTimeUM) / 1000.0f;

			if (pGame->m_FeverState > 0) {
				pGame->getOption().getNoteRecord().recordNote(_note->m_idx,
					playTime, _note->m_effIndex, _note->m_velocity, true);
			}
			else {
				pGame->getOption().getNoteRecord().recordNote(_note->m_idx,
					playTime, _note->m_effIndex, _note->m_velocity, false);
			}
		}
	}
#endif

	return id;
}

void PlayGame_NoteView::setRotate(float dt) {

	_rotateVal += dt;

	this->setRotation3D(cocos2d::Vec3(_rotateVal, 0.0f, 0.0f));
}

void PlayGame_NoteView::setFog(int type, bool isVisible, float posY) {

	if (type == HINDRANCE_SHUTTER)
	{
		auto spine = (spine::SkeletonAnimation*)this->getChildByTag(TAG_FOG_SURTTER);
		spine->setVisible(isVisible);

		if (isVisible == true)
		{
			float time = (getHindranceData(HINDRANCE_SHUTTER, 0) / 100.0f);
			if (time <= 0.0f)
				time = 1.0f;
			spine->setToSetupPose();
			spine->setTimeScale(time);
			spine->clearTrack();
			spine->setAnimation(0, "active", true);
		}
		else
		{
			spine->setToSetupPose();
			spine->clearTrack();
		}
	}
	else if (type == HINDRANCE_HIDDEN)
	{
		setFogVisible(TAG_FOG_HIDDEN_TOP, isVisible, posY);
	}
	else if (type == HINDRANCE_SUDDEN)
	{
		setFogVisible(TAG_FOG_SUDDEN_TOP, isVisible, 0.0f);
	}
	else if (type == HINDRANCE_MIST)
	{
		setFogVisible(TAG_FOG_MIST_TOP, isVisible, 0.0f);
	}
}



void PlayGame_NoteView::pauseHidrance(bool isPause)
{
	if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_WIPER] == true)
	{
		auto spineWiper = (spine::SkeletonAnimation*)this->getChildByTag(TAG_WIPER);
		if (isPause == true)
			spineWiper->pause();
		else
			spineWiper->resume();
	}

	if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_SHUTTER] == true)
	{
		auto spineSurtter = (spine::SkeletonAnimation*)this->getChildByTag(TAG_FOG_SURTTER);
		if (isPause == true)
			spineSurtter->pause();
		else
			spineSurtter->resume();
	}

}


void PlayGame_NoteView::SkanMoveProC()
{
	bool isMove = false;
	int moveVal = getHindranceData(HINDRANCE_SKAN, 0);
	int maxVal = getHindranceData(HINDRANCE_SKAN, 1);


	if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_SKAN] == true)
	{
		isMove = true;
	}
	else if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_SKAN] == false)
	{
		if (PTR_GAMEMAIN->m_IsWide == true)
		{
			if (m_perfectPosition != PERFECT_POSITION_WIDE)
			{
				isMove = true;
			}
		}
		else
		{
			if (m_perfectPosition < PERFECT_POSITION)
			{
				isMove = true;
			}
		}
	}


	if (isMove == true)
	{
		if (m_isSkanUp == true) 
		{
			m_perfectPosition += moveVal;
			if (m_perfectPosition > maxVal)
			{
				m_isSkanUp = false;
			}
		}
		else 
		{
			m_perfectPosition -= moveVal;

			if (PTR_GAMEMAIN->m_IsWide == true)
			{
				if (m_perfectPosition < PERFECT_POSITION_WIDE)
				{
					m_perfectPosition = PERFECT_POSITION_WIDE;
					m_isSkanUp = true;
				}
			}
			else
			{
				if (m_perfectPosition < PERFECT_POSITION)
				{
					m_perfectPosition = PERFECT_POSITION;
					m_isSkanUp = true;
				}
			}
		}
		auto sprPerfectLine = getChildByTag(TAG_NOTE_BG_PERFECT);
		sprPerfectLine->setPositionY(m_perfectPosition);

		if (PTR_GAMEMAIN->m_isHindrance[HINDRANCE_HIDDEN] == true)
		{
			setFog(HINDRANCE_HIDDEN, true, m_perfectPosition);
		}
	}
}

void PlayGame_NoteView::setWiper(bool isVisible)
{
	auto spine = (spine::SkeletonAnimation*)this->getChildByTag(TAG_WIPER);
	spine->setVisible(isVisible);

	if (isVisible == true)
	{
		float time = (getHindranceData(HINDRANCE_WIPER, 0) / 100.0f);
		if (time <= 0.0f)
			time = 1.0f;

		spine->setToSetupPose();
		spine->clearTrack();
		spine->setTimeScale(time);
		if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::GUITAR)
		{
			spine->setAnimation(0, "guitar_active", true);
		}
		else
		{
			spine->setAnimation(0, "active", true);
		}
	}
	else
	{
		spine->setToSetupPose();
		spine->clearTrack();
	}

	m_isAniRight = true;
}


void PlayGame_NoteView::setHightlight(int num) {

	float height = m_perfectPosition + ((PTR_GAMEMAIN->m_Highlight[num].endPosition - PTR_GAMEMAIN->m_Highlight[num].startPosition) * getSpeedVal(PTR_GAMEMAIN->m_SelectChannel));
	float yy = m_perfectPosition + ((PTR_GAMEMAIN->m_Highlight[num].startPosition) * getSpeedVal(PTR_GAMEMAIN->m_SelectChannel));


	int size = 0;
	if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::GUITAR)
	{
		size = 1100;
	}
	else
	{
		size = 1280;
	}

	auto highLightHeadBatchTouch = (SpriteBatchNode*)this->getChildByTag(TAG_NOTE_IMG_HIGHLIGHT_HEAD);
	auto highLightMidBatchTouch = (SpriteBatchNode*)this->getChildByTag(TAG_NOTE_IMG_HIGHLIGHT_MID);

	Sprite* sprLeft = Sprite::createWithTexture(highLightHeadBatchTouch->getTexture());
	sprLeft->setPosition(PTR_GAMEMAIN->m_WinSize.width / 2 - ((size - 116)/2), yy);
	sprLeft->setAnchorPoint(Point(1.0f, 0.0f));
	sprLeft->setScaleY((float)(height / sprLeft->getContentSize().height));
	sprLeft->setCameraMask((unsigned short)CameraFlag::USER1);
	addChild(sprLeft, TAG_NOTE_BG_HIGHTLIGHT_0 + (num * 3), TAG_NOTE_BG_HIGHTLIGHT_0 + (num * 3));


	Sprite* sprMid = Sprite::createWithTexture(highLightMidBatchTouch->getTexture());
	sprMid->setPosition(PTR_GAMEMAIN->m_WinSize.width / 2, yy);
	sprMid->setAnchorPoint(Point(0.5f, 0.0f));
	sprMid->setScaleY((float)(height / sprMid->getContentSize().height));
	sprMid->setScaleX((size-116));
	sprMid->setCameraMask((unsigned short)CameraFlag::USER1);
	addChild(sprMid, TAG_NOTE_BG_HIGHTLIGHT_0+ 1 + (num * 3), TAG_NOTE_BG_HIGHTLIGHT_0 + 1 + (num * 3));


	Sprite* sprRight = Sprite::createWithTexture(highLightHeadBatchTouch->getTexture());
	sprRight->setPosition(PTR_GAMEMAIN->m_WinSize.width / 2 + ((size - 116) / 2), yy);
	sprRight->setAnchorPoint(Point(0.0f, 0.0f));
	sprRight->setScaleY((float)(height / sprRight->getContentSize().height));
	sprRight->setFlipX(true);
	sprRight->setCameraMask((unsigned short)CameraFlag::USER1);
	addChild(sprRight, TAG_NOTE_BG_HIGHTLIGHT_0 + 2 + (num * 3), TAG_NOTE_BG_HIGHTLIGHT_0 + 2+ (num * 3));

}

int PlayGame_NoteView::getInstrumentLineCnt(int trackIdx) {
	return m_trackLine[trackIdx];
}

void PlayGame_NoteView::setPlayNoteTime(int ch, long long id, int time, int noteTime)
{
	for (int i = 0; i < MAX_STOP_CNT; i++) 
	{
		if (m_notePlayTime[ch][i] == 0) 
		{
			/*
			 #if(CC_TARGET_PLATFORM == CC_PLATFORM_WIN32 )
			 m_notePlayTime[ch][i] = PTR_GAMEMAIN->getMicroSecond() + (time) + 1500;
			 #else
			 m_notePlayTime[ch][i] = PTR_GAMEMAIN->getMicroSecond() + (time);
			 #endif
			 */
			//m_notePlayTime[ch][i] = (PTR_GAMEMAIN->getUMicroSecond() / 1000) + (time);
			
			m_notePlayTime[ch][i] = (time + noteTime);
			m_notePlayID[ch][i] = id;
			//log("++++++++++++setPlayNoteTime()   time:%d    m_notePlayID[%d][%d]:%lld ", time, ch, i, m_notePlayID[ch][i]);
			break;
		}
	}
}

int PlayGame_NoteView::findEffectIdx() {
	int idx = 0;
#ifdef NEW_EFFECT_PROC
	for (int i = 0; i < MAX_EFFECT_CNT; i++)
	{
		auto spineEffect = (spine::SkeletonAnimation*)getChildByTag(TAG_NOTE_EFFECT_LAYER + i);
		if (spineEffect->isVisible() == false)
		{
			return i;
		}
	}
#endif
	return idx;
}

void PlayGame_NoteView::setEffectObjArray(Vec2& _pos, const char* _motionStr, const char* _skinStr, bool isZ, bool isLine)
{
#ifdef NEW_EFFECT_PROC
	/*
	 m_nowEffectIdx++;
	 if( m_nowEffectIdx >=  MAX_EFFECT_CNT)
	 {
	 m_nowEffectIdx = 0;
	 }
	 */

	m_nowEffectIdx = findEffectIdx();

	auto spineEffect = (spine::SkeletonAnimation*)getChildByTag(TAG_NOTE_EFFECT_LAYER + m_nowEffectIdx);
	spineEffect->setCameraMask((unsigned short)CameraFlag::USER1);
	spineEffect->setPosition(_pos);
	//spineEffect->clearTrack();
	spineEffect->setAnimation(0, _motionStr, false);

	if (_skinStr != NULL)
	{
		spineEffect->setSkin(_skinStr);
	}
	//spineEffect->setRotation3D(Vec3(0, 0, 0));
	//spineEffect->setScale(1.0f);

	//애니 초기화
	spineEffect->setToSetupPose();

	spineEffect->setVisible(true);

#else
	EffectObject* effectObj = m_effectObjPool->getEffectObject();
	if (effectObj == NULL) {
		//CCASSERT(false, "EffectObject is NULL!!");
		return;
	}

	if (isLine == true)
		this->addChild(effectObj->getSkeletonAnimation(), TAG_NOTE_EFFECT_LAYER);
	else
		this->addChild(effectObj->getSkeletonAnimation(), TAG_NOTE_EFFECT_TOP);


	effectObj->getSkeletonAnimation()->setCameraMask(
		(unsigned short)CameraFlag::USER1);
	effectObj->getSkeletonAnimation()->setPosition(_pos);
	//effectObj->getSkeletonAnimation()->clearTrack();
	//effectObj->getSkeletonAnimation()->setAnimation(0, "animation", false);
	effectObj->getSkeletonAnimation()->setAnimation(0, _motionStr, false);

	if (_skinStr != NULL) {
		effectObj->getSkeletonAnimation()->setSkin(_skinStr);
	}

	/*
	 if( isZ == false )
	 {
	 effectObj->getSkeletonAnimation()->setRotation3D(Vec3(-(_rotateVal), 0, 0));
	 }
	 else
	 {
	 effectObj->getSkeletonAnimation()->setRotation3D(Vec3(0, 0, 0));
	 }
	 */
	effectObj->getSkeletonAnimation()->setRotation3D(Vec3(0, 0, 0));

	//effectObj->getSkeletonAnimation()->setTimeScale(0.5f);

	//애니 초기화
	effectObj->getSkeletonAnimation()->setToSetupPose();

	m_effectObjArray.push_back(effectObj);
#endif
}

void PlayGame_NoteView::updateEffectObject(float _dt) {
#ifndef	NEW_EFFECT_PROC
	EffectObjArrayIt effectObjIt = m_effectObjArray.begin();
	for (effectObjIt; effectObjIt != m_effectObjArray.end();) {
		EffectObject* effectObject = (*effectObjIt);
		if (effectObject->isComplete()) {
			effectObject->resetComplete();
			effectObjIt = m_effectObjArray.erase(effectObjIt);
		}
		else {
			effectObject->update(_dt);
			++effectObjIt;
		}
	}
#endif
}

void PlayGame_NoteView::deleteEffectObject() {
#ifndef	NEW_EFFECT_PROC
	EffectObjArrayIt effectObjIt = m_effectObjArray.begin();
	for (effectObjIt; effectObjIt != m_effectObjArray.end();) {
		EffectObject* effectObject = (*effectObjIt);

		effectObject->resetComplete();
		effectObjIt = m_effectObjArray.erase(effectObjIt);
	}

	memset(m_notePlayID, 0, sizeof(m_notePlayID));
#endif
}

void PlayGame_NoteView::deleteActionNoteEffect(Node *_node) {
	if (_node == NULL)
		return;
	Sprite* spr = (Sprite *)_node;
	this->removeChild(spr, true);
}

void PlayGame_NoteView::actionNoteEffect(int _chIdx, int _idx, float _x,
	float _y) {
	if (PTR_GAMEMAIN->m_SelectChannel != _chIdx)
		return;
	NoteObject* note = m_note[_chIdx][_idx];
	cocos2d::Vec2 pos = (note->m_sprNote->getPosition());
	pos.x = _x;
	pos.y = _y;

	char motionStr[50] = { 0, };
	char motionSkin[50] = { 0, };

	//int type = note->m_idx % 3;
	int type = m_actionNoteEffectIdx % 3;
	if (type == 0) {
		sprintf(motionStr, "%s", "ani_01");
	}
	else if (type == 1) {
		sprintf(motionStr, "%s", "ani_02");
	}
	else if (type == 2) {
		sprintf(motionStr, "%s", "ani_03");
	}

	bool isDrumChange = false;
	int drumIdx = 0;
	if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
		char tmpText[255] = { '\0' };
		drumIdx = getDrumImageIdx(tmpText, note->m_effIndex, false);
	}

	if (note->m_noteType == NOTE_TYPE_NORMAL_LONG
		|| note->m_noteType == NOTE_TYPE_MUTE_NORMAL_LONG) {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
			isDrumChange = true;
		}
		else
			sprintf(motionSkin, "%s", "long");
	}
	else if (note->m_noteType == NOTE_TYPE_AUTO
		|| note->m_noteType == NOTE_TYPE_NORMAL_AUTO) {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
			isDrumChange = true;
		}
		else
			sprintf(motionSkin, "%s", "auto");
	}
	else if (note->m_noteType == NOTE_TYPE_NORMAL_DRAG
		|| note->m_noteType == NOTE_TYPE_DRAG_LONG
		|| note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG) {
		sprintf(motionSkin, "%s", "drag");
	}
	else if (note->m_noteType == NOTE_TYPE_LEFT_SLID
		|| note->m_noteType == NOTE_TYPE_RIGHT_SLID) {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
			isDrumChange = true;
		}
		else
			sprintf(motionSkin, "%s", "slide");
	}
	else if (note->m_noteType == NOTE_TYPE_AUTO_CHORD) {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
			isDrumChange = true;
		}
		else
			sprintf(motionSkin, "%s", "auto");
	}
	else if (note->m_noteType == NOTE_TYPE_CHORD) {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
			isDrumChange = true;
		}
		else
			sprintf(motionSkin, "%s", "code");
	}
	else if (note->m_noteType == NOTE_TYPE_DRAG_CHORD) {
		sprintf(motionSkin, "%s", "drag");
	}
	else {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
			isDrumChange = true;
		}
		else
			sprintf(motionSkin, "%s", "normal");
	}

	if (isDrumChange == true) {
		if (drumIdx >= TAG_DRUM_IMG_SNARE_LARGE
			&& drumIdx <= TAG_DRUM_IMG_SNARE_SMALL)
			sprintf(motionSkin, "%s", "snare");
		else
			sprintf(motionSkin, "%s", "cymbal");
	}

	m_actionNoteEffectIdx++;

	setEffectObjArray(pos, motionStr, motionSkin, false, false);
}

void PlayGame_NoteView::actionNoteEffectLine(int _chIdx, int _idx, float _x,
	float _y) {
	if (PTR_GAMEMAIN->m_SelectChannel != _chIdx)
		return;

	NoteObject* note = m_note[_chIdx][_idx];

	if (note->m_sprNote->isVisible() == false)
		return;

	cocos2d::Vec2 pos = (note->m_sprNote->getPosition());
	pos.x = _x;
	pos.y = _y;

	char motionStr[50] = { 0, };
	char motionSkin[50] = { 0, };

	sprintf(motionStr, "%s", "ani_line");

	bool isDrumChange = false;
	int drumIdx = 0;
	if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
		char tmpText[255] = { '\0' };
		drumIdx = getDrumImageIdx(tmpText, note->m_effIndex, false);
	}

	if (note->m_noteType == NOTE_TYPE_NORMAL_LONG
		|| note->m_noteType == NOTE_TYPE_MUTE_NORMAL_LONG) {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
			isDrumChange = true;
		}
		else
			sprintf(motionSkin, "%s", "long");
	}
	else if (note->m_noteType == NOTE_TYPE_AUTO
		|| note->m_noteType == NOTE_TYPE_NORMAL_AUTO) {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
			isDrumChange = true;
		}
		else
			sprintf(motionSkin, "%s", "auto");
	}
	else if (note->m_noteType == NOTE_TYPE_NORMAL_DRAG
		|| note->m_noteType == NOTE_TYPE_DRAG_LONG
		|| note->m_noteType == NOTE_TYPE_MUTE_DRAG_LONG) {
		sprintf(motionSkin, "%s", "drag");
	}
	else if (note->m_noteType == NOTE_TYPE_LEFT_SLID
		|| note->m_noteType == NOTE_TYPE_RIGHT_SLID) {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
			isDrumChange = true;
		}
		else
			sprintf(motionSkin, "%s", "slide");
	}
	else if (note->m_noteType == NOTE_TYPE_AUTO_CHORD) {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
			isDrumChange = true;
		}
		else
			sprintf(motionSkin, "%s", "auto");
	}
	else if (note->m_noteType == NOTE_TYPE_CHORD) {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
			isDrumChange = true;
		}
		else
			sprintf(motionSkin, "%s", "code");
	}
	else if (note->m_noteType == NOTE_TYPE_DRAG_CHORD) {
		sprintf(motionSkin, "%s", "drag");
	}
	else {
		if (PTR_GAMEMAIN->m_MusicInfoInstType[_chIdx] == (int)InstType::DRUM) {
			isDrumChange = true;
		}
		else
			sprintf(motionSkin, "%s", "normal");
	}

	if (isDrumChange == true) {
		if (drumIdx >= TAG_DRUM_IMG_SNARE_LARGE
			&& drumIdx <= TAG_DRUM_IMG_SNARE_SMALL)
			sprintf(motionSkin, "%s", "snare");
		else
			sprintf(motionSkin, "%s", "cymbal");
	}

	setEffectObjArray(pos, motionStr, motionSkin, true, true);
}

float PlayGame_NoteView::getSpeedVal(int ch) {
	float tmp = 0;
	float val = 0;

	int bit = midiInfo->GetBeat();
	tmp = (float)midiInfo->GetPixelPerBeat();
	val = (11 - tmp);

	if (val < 1.0f)
		val = 1.0f;

	val *= 1.4f;

	//이동값 가변형
	pixelPerBeat = midiInfo->GetPixelPerBeat() * val * getDubbleSpeedValue();

	if (PTR_GAMEMAIN->m_SelectChannel == ch) {
		PTR_GAMEMAIN->m_BaseMoveSpeed = pixelPerBeat / getDubbleSpeedValue();
	}
	//이동값 노래별로 고정값지정(서버에서 받아온 값으로 변경예정)
	if (PTR_GAMEMAIN->m_MoveSpeed[ch] > 0) {
		pixelPerBeat = (PTR_GAMEMAIN->m_MoveSpeed[ch] * getDubbleSpeedValue());
	}

	return pixelPerBeat;
}

float PlayGame_NoteView::getDubbleSpeedValue() {
	float val = PTR_GAMEMAIN->m_GameSpeed;

	int speed = (int)val;

	switch (speed) {
	case 1:
		val = 1.5f;
		break;
	case 2:
		val = 2.0f;
		break;
	case 3:
		val = 3.0f;
		break;
	case 4:
		val = 3.5f;
		break;
	}
	return val;
}

bool PlayGame_NoteView::isAutoHead(NoteObject* note, bool isEffect) {
	if (note->m_childSize[BIG_GROUP] > 0) {
	}
	else {
		if (note->m_GroupID[BIG_GROUP] <= 0
			&& note->m_childSize[SMALL_GROUP] > 0) {
			/*
			 if (isEffect == true)
			 return false;
			 */
		}
		else {
			return false;
		}
	}

	return true;
}

int PlayGame_NoteView::getBPM() {
	return midiInfo->GetBPM();
}

void PlayGame_NoteView::startFeverEffect(int state) {
	for (int i = 0; i < 2; i++) 
	{
		auto spineFever = (spine::SkeletonAnimation*) getChildByTag( TAG_NOTE_BG_FEVER_EFFECT_LEFT + i);

		if (state == 1)
			spineFever->setAnimation(0, "active", false);
		else
			spineFever->setAnimation(0, "idle", true);
		float scaleVal = midiInfo->GetBPM() / 120.0f;
		spineFever->setTimeScale(scaleVal);

		m_FeverTick = (float)(PTR_GAMEMAIN->getUMicroSecond() / 1000.0f);

		//애니 초기화
		spineFever->setToSetupPose();
		spineFever->setVisible(true);

	}
}

void PlayGame_NoteView::endFeverEffect() {
	for (int i = 0; i < 2; i++)
	{
		auto spineFever = (spine::SkeletonAnimation*) getChildByTag(TAG_NOTE_BG_FEVER_EFFECT_LEFT + i);
		spineFever->setVisible(false);
	}
}

void PlayGame_NoteView::pauseFeverEffect(bool isPause) {
	for (int i = 0; i < 2; i++) 
	{
		auto spineFever = (spine::SkeletonAnimation*) getChildByTag(TAG_NOTE_BG_FEVER_EFFECT_LEFT + i);
		if (isPause == true) 
		{
			spineFever->pause();
		}
		else
		{
			spineFever->resume();
		}
	}
}

bool PlayGame_NoteView::feverEffectChange() {
	auto spineFever = (spine::SkeletonAnimation*) getChildByTag(TAG_NOTE_BG_FEVER_EFFECT_LEFT);
	spTrackEntry* trackEntry = spineFever->getCurrent();

	if (trackEntry == NULL) {
		return true;
	}
	return false;
}

void PlayGame_NoteView::FeverEffectProC() {
	return;
	auto spineFever = (spine::SkeletonAnimation*) getChildByTag(
		TAG_NOTE_BG_FEVER_EFFECT_LEFT);
	int val = spineFever->getOpacity();
	if (val < 255) {
		float time = (float)(PTR_GAMEMAIN->getUMicroSecond() / 1000.0f)
			- m_FeverTick;

		time = time / 1000.0f;

		int data = (time / (float)(60.0f / getBPM())) * 8;

		val = 55 + data;

		if (val > 255) {
			val = 255;
		}
	}

	for (int i = 0; i < 2; i++) {
		auto spineFever = (spine::SkeletonAnimation*) getChildByTag(
			TAG_NOTE_BG_FEVER_EFFECT_LEFT + i);
		spineFever->setOpacity(val);
		//log("++++++++++++++++++++FeverEffectProC()   %d ", val);
	}
}


void PlayGame_NoteView::setFogImage(int y, int size, int tag, float ancho)
{
	auto cloudBatchTouch = (SpriteBatchNode*)this->getChildByTag(TAG_NOTE_IMG_CLOUD_2);


	float width = 1280.0f;
	float _y = 0.0f;

	if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::GUITAR)
	{
		width = 1100;
	}


	Sprite* sprFogSuddenTop = Sprite::createWithTexture(cloudBatchTouch->getTexture());
	if (ancho == 1.0f)
		_y = y;
	else
		_y = y + (size / 2);
	sprFogSuddenTop->setPosition(Point(PTR_GAMEMAIN->m_WinSize.width / 2, _y));
	sprFogSuddenTop->setAnchorPoint(Point(0.5f, 0.0f));
	sprFogSuddenTop->setVisible(false);
	sprFogSuddenTop->setScaleY(2.0f);
	sprFogSuddenTop->setScaleX((float)(width / sprFogSuddenTop->getContentSize().width));
	sprFogSuddenTop->setCameraMask((unsigned short)CameraFlag::USER1);
	this->addChild(sprFogSuddenTop, tag, tag);

	auto sprFogSudden = Sprite::create("res/ingame/blank.png", Rect(0, 0, width, size));
	sprFogSudden->setPosition(PTR_GAMEMAIN->m_WinSize.width / 2, y);
	sprFogSudden->setColor(Color3B::BLACK);
	sprFogSudden->setAnchorPoint(Point(0.5f, ancho));
	sprFogSudden->setVisible(false);
	sprFogSudden->setCameraMask((unsigned short)CameraFlag::USER1);
	this->addChild(sprFogSudden, tag + 1, tag+1);


	Sprite* sprFogSuddenBottom = Sprite::createWithTexture(cloudBatchTouch->getTexture());
	sprFogSuddenBottom->setFlipY(true);
	if (ancho == 1.0f)
		_y = y - (size);
	else
		_y = y - (size / 2);

	sprFogSuddenBottom->setPosition(Point(PTR_GAMEMAIN->m_WinSize.width / 2, _y));
	sprFogSuddenBottom->setAnchorPoint(Point(0.5f, 1.0f));
	sprFogSuddenBottom->setVisible(false);
	sprFogSuddenBottom->setScaleY(2.0f);
	sprFogSuddenBottom->setScaleX((float)(width / sprFogSuddenBottom->getContentSize().width));
	sprFogSuddenBottom->setCameraMask((unsigned short)CameraFlag::USER1);
	this->addChild(sprFogSuddenBottom, tag + 2, tag+2);
}


void PlayGame_NoteView::setFogVisible(int tag, bool isVisible, float positionY)
{
	Sprite* sprFogSuddenTop = (Sprite*)this->getChildByTag(tag);
	sprFogSuddenTop->setVisible(isVisible);

	Sprite* sprFogSudden = (Sprite*)this->getChildByTag(tag+1);
	sprFogSudden->setVisible(isVisible);

	Sprite* sprFogSuddenBottom = (Sprite*)this->getChildByTag(tag+2);
	sprFogSuddenBottom->setVisible(isVisible);


	if (positionY > 0.0f)
	{
		sprFogSuddenTop->setPositionY(positionY + (sprFogSudden->getContentSize().height/2));
		sprFogSudden->setPositionY(positionY);
		sprFogSuddenBottom->setPositionY(positionY - (sprFogSudden->getContentSize().height / 2));
	}
}



int PlayGame_NoteView::getHindranceData(int type, int idx)
{
	int val = 0;

	for (int i = 0; i < 5; i++)
	{
		if (PTR_GAMEMAIN->m_Hindrance[i].type == type)
		{
			val = PTR_GAMEMAIN->m_Hindrance[i].value[idx];
			break;
		}
	}

	return val;
}
