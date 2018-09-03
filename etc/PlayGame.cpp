
#include "Public.h"
#include "PlayGame.h"
#include "GameMain.h"
#include "ProtobufTaskManager.h"
#include "PlayResultScreen.h"
#include "NoteObject.h"
#include "PausePopup.h"
#include "MusicListScreen.h"
#include "LogoScreen.h"
#include "UIAlertPopup.h"
#include "EventDefine.h"
#include "PlayGame_NoteView.h"
#include "PlayGame_BgView.h"
#include "PlayGame_UiView.h"
#include "GameDB.h"
#include "UIFullScreenAnimationLayer.h"
#include "NoteObject.h"
#include "EventDefine.h"
#include "Config.h"
#include "TypeDefine.h"
#include "HomeScreen.h"
#include "SettingData.h"
#include "StageDB.h"
#include "StageListScreen.h"
#include "UIConfirmPopup.h"
#include "UserDB.h"

USING_NS_CC;

PlayGame::PlayGame()
: _downloadManager(nullptr)
, _bgmID(-1)
, _voiceID(-1)
{
    
}

PlayGame::~PlayGame()
{
    
}

PlayGame* PlayGame::create(void* data)
{
	PlayGame *ret = new(std::nothrow) PlayGame();
	if (ret && ret->init(data))
	{
		ret->autorelease();
		return ret;
	}
	else
	{
		delete ret;
		ret = nullptr;
		return nullptr;
	}

	return ret;
}


bool PlayGame::init(void* data)
{
	if (!Screen::init())
	{
		return false;
	}

	_option = *(static_cast<VOOption*>(data));

	initVal();

	{
		char optionData[255];
		if (PTR_GAMEMAIN->ReadFileData("option.dat", optionData, true) == true)
		{
			PTR_GAMEMAIN->m_MySyncValue = PTR_GAMEMAIN->UM_ByteToInt32(optionData, 0);
			log(" PTR_GAMEMAIN->m_MySyncValue = %0.2f ", (float)(PTR_GAMEMAIN->m_MySyncValue / 1000.0f));
		}
	}
    
	PTR_GAMEMAIN->m_GameSpeed = 2;

	getOptionData();

	PTR_GAMEMAIN->m_GameMode = _option.getPlayMode();
	if (PTR_GAMEMAIN->m_GameMode == PlayMode::PREMIUM || PTR_GAMEMAIN->m_GameMode == PlayMode::PVP)
	{
		//아이템
		std::vector<int> item = _option.get1Player().getItemIDs();
		log("item cnt = %d", item.size());

		for (int i = 0; i < item.size(); i++)
		{
			PTR_GAMEMAIN->m_HaveItem[i] = item[i];
			log("[%d] = %d", i, PTR_GAMEMAIN->m_HaveItem[i]);

			int num = PTR_GAMEMAIN->m_HaveItem[i] - 800;
			PTR_GAMEMAIN->m_ItemInfo[num].isHave = true;
		}
	}
	else if (PTR_GAMEMAIN->m_GameMode == PlayMode::PREVIEW_PLAY)
	{
		PTR_GAMEMAIN->m_isPreViewPlay = true;
	}
	else if (PTR_GAMEMAIN->m_GameMode == PlayMode::STAGE)
	{
		SetStageData();
	}

	PTR_GAMEMAIN->m_SelectMusicIdx = _option.getMusicID();
	PTR_GAMEMAIN->m_SelectLevel[0] = _option.get1Player().getDifficult();
	PTR_GAMEMAIN->m_GameInstrumType[0] = (int)GameDB::getInstance()->getInstType(_option.get1Player().getInstID());

	log("+++++++++++gameplay  PTR_GAMEMAIN->m_SelectMusicIdx:%d   PTR_GAMEMAIN->m_SelectLevel[0]:%d   PTR_GAMEMAIN->m_GameInstrumType[0]:%d", PTR_GAMEMAIN->m_SelectMusicIdx, PTR_GAMEMAIN->m_SelectLevel[0], PTR_GAMEMAIN->m_GameInstrumType[0]);

	//////임시
	{
		switch (PTR_GAMEMAIN->m_SelectMusicIdx)
		{
		case 1294: case 1503: case 1527: case 1793: case 1441:
		{
			PTR_GAMEMAIN->m_isSampler = true;
		}
		break;

		case 22727: //case 22735: case 1490:
		{
			PTR_GAMEMAIN->m_GameMode = PlayMode::SYNC_PLAY;
		}
			break;
		}
	}
    

	if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::PIANO)
    {
		PTR_GAMEMAIN->m_SelectPiano = _option.get1Player().getInstID();
    }
	else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::GUITAR)
    {
		PTR_GAMEMAIN->m_SelectGuitar = _option.get1Player().getInstID();
    }
	else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::DRUM)
    {
		PTR_GAMEMAIN->m_SelectDrum = _option.get1Player().getInstID();
    }


    PTR_GAMEMAIN->m_isRecording = _option.getPlayConfig().getEnabledRec();

	log("++++++++++PlayGame::init()         musicID:%d        instType:%d          ", PTR_GAMEMAIN->m_SelectMusicIdx, PTR_GAMEMAIN->m_GameInstrumType);
   
	_option.getNoteRecord().init();
    
    _downloadManager = DownloadManager::create(CC_CALLBACK_0(PlayGame::onDownloadSuccess, this));
    CC_SAFE_RETAIN(_downloadManager);

    
    return true;
}

void PlayGame::SetStageData()
{
	//아이템
	std::vector<int> item = _option.get1Player().getItemIDs();
	log("item cnt = %d", item.size());

	for (int i = 0; i < item.size(); i++)
	{
		PTR_GAMEMAIN->m_HaveItem[i] = item[i];
		log("[%d] = %d", i, PTR_GAMEMAIN->m_HaveItem[i]);

		int num = PTR_GAMEMAIN->m_HaveItem[i] - 800;
		PTR_GAMEMAIN->m_ItemInfo[num].isHave = true;
	}


	if (_option.getSelectStageID() <= 0)
	{
		//클리어조건
		auto stageInfo = StageDB::getInstance()->getStageInfo(_option.getStageID());
		PTR_GAMEMAIN->m_gameClearContition[0] = stageInfo.score();
		PTR_GAMEMAIN->m_gameClearContition[1] = stageInfo.combo();
		PTR_GAMEMAIN->m_gameClearContition[2] = stageInfo.perfect();
		PTR_GAMEMAIN->m_gameClearContition[3] = stageInfo.great();
		PTR_GAMEMAIN->m_gameClearContition[4] = stageInfo.good();
		PTR_GAMEMAIN->m_gameClearContition[5] = stageInfo.fever();
		PTR_GAMEMAIN->m_gameClearContition[6] = stageInfo.miss();
	}

	///방해요소셋팅
	//if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_HINDRANCE].isHave == false)
	{

		if (_option.getSelectStageID() <= 0)
		{
			auto slotInfo = StageDB::getInstance()->getStageInfo(_option.getStageID());


			PTR_GAMEMAIN->m_Hindrance[0].type = getHindranceType(slotInfo.disturb1());
			PTR_GAMEMAIN->m_Hindrance[0].condition = slotInfo.disturb1_start();
			PTR_GAMEMAIN->m_Hindrance[0].value[0] = slotInfo.disturb1_setting_a();
			PTR_GAMEMAIN->m_Hindrance[0].value[1] = slotInfo.disturb1_setting_b();

			PTR_GAMEMAIN->m_Hindrance[1].type = getHindranceType(slotInfo.disturb2());
			PTR_GAMEMAIN->m_Hindrance[1].condition = slotInfo.disturb2_start();
			PTR_GAMEMAIN->m_Hindrance[1].value[0] = slotInfo.disturb2_setting_a();
			PTR_GAMEMAIN->m_Hindrance[1].value[1] = slotInfo.disturb2_setting_b();

			PTR_GAMEMAIN->m_Hindrance[2].type = getHindranceType(slotInfo.disturb3());
			PTR_GAMEMAIN->m_Hindrance[2].condition = slotInfo.disturb3_start();
			PTR_GAMEMAIN->m_Hindrance[2].value[0] = slotInfo.disturb3_setting_a();
			PTR_GAMEMAIN->m_Hindrance[2].value[1] = slotInfo.disturb3_setting_b();


			PTR_GAMEMAIN->m_Hindrance[3].type = getHindranceType(slotInfo.disturb4());
			PTR_GAMEMAIN->m_Hindrance[3].condition = slotInfo.disturb4_start();
			PTR_GAMEMAIN->m_Hindrance[3].value[0] = slotInfo.disturb4_setting_a();
			PTR_GAMEMAIN->m_Hindrance[3].value[1] = slotInfo.disturb4_setting_b();

			PTR_GAMEMAIN->m_Hindrance[4].type = getHindranceType(slotInfo.disturb5());
			PTR_GAMEMAIN->m_Hindrance[4].condition = slotInfo.disturb5_start();
			PTR_GAMEMAIN->m_Hindrance[4].value[0] = slotInfo.disturb5_setting_a();
			PTR_GAMEMAIN->m_Hindrance[4].value[1] = slotInfo.disturb5_setting_b();

			//즉시 발생하는 방해요소 시작
			if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_HINDRANCE].isHave == false)
			{
				for (int i = 0; i < 5; i++)
				{
					log("+++++++++++++++++++  PTR_GAMEMAIN->m_Hindrance[%d].type = %d condition:%d ", i, PTR_GAMEMAIN->m_Hindrance[i].type, PTR_GAMEMAIN->m_Hindrance[i].condition);
					if (PTR_GAMEMAIN->m_Hindrance[i].condition == 1 && PTR_GAMEMAIN->m_Hindrance[i].type >= 0)
					{
						if (PTR_GAMEMAIN->m_Hindrance[i].type >= HINDRANCE_ILLUSION)
						{
							PTR_GAMEMAIN->m_isHindrance[PTR_GAMEMAIN->m_Hindrance[i].type] = true;

							PTR_GAMEMAIN->m_GameSpeed = 2; //일루션과 슈퍼일루션이 나올경우 고정됨
						}
					}
				}
			}
		}
	}
}


int PlayGame::getHindranceType(int num)
{
	switch (num)
	{
	case 1007:
		return HINDRANCE_HIDDEN;
		break;

	case 1008:
		return HINDRANCE_WIPER;
		break;

	case 1009:
		return HINDRANCE_SUDDEN;
		break;

	case 1010:
		return HINDRANCE_MIST;
		break;

	case 1011:
		return HINDRANCE_BLINK;
		break;

	case 1012:
		return HINDRANCE_SHUTTER;
		break;

	case 1013:
		return HINDRANCE_BIG_NOTE;
		break;

	case 1014:
		return HINDRANCE_SMALL_NOTE;
		break;

	case 1015:
		return HINDRANCE_SKAN;
		break;

	case 1016:
		return HINDRANCE_ILLUSION;
		break;

	case 1017:
		return HINDRANCE_SUPER_ILLUSION;
		break;
	}

	return -1;
}

void PlayGame::load(const std::function<void(Screen*)> &callback)
{
    _callback = callback;
    
    this->addDownloadBgmAndData();
    this->addDownloadInstumentAudio();
    this->addDownloadNoteFx();
    

	switch (PTR_GAMEMAIN->m_SelectMusicIdx)
	{
	case 22753: //딜팽이
	//case 1235: case 22721:
		this->addDownloadMR();
		break;
	}

    _downloadManager->download();
}


void PlayGame::addDownloadMR()
{

	char urlString[512] = {'\0'};

	if (PTR_GAMEMAIN->m_SelectMusicIdx == 22753)
	{
		sprintf(urlString, "http://222.106.118.83:8080/MusicianBurningWeb/user-resources/SING_ALONG/5770/22753.m4a");
	}
	/*
	else
	{
		sprintf(urlString, "http://222.106.118.83:8080/MusicianBurningWeb/user-resources/SING_ALONG/3173/%d.m4a", PTR_GAMEMAIN->m_SelectMusicIdx);
	}
	*/

	//std::string packageUrl = StringUtils::format(urlString);
	std::string packageUrl = urlString;

	std::string storagePath = StringUtils::format("%s/musics/%d/",
		FileUtils::getInstance()->getWritablePath().c_str(),
		PTR_GAMEMAIN->m_SelectMusicIdx);

	if (FileUtils::getInstance()->createDirectory(storagePath))
	{
		auto it = GameDB::getInstance()->getMusicInfo(PTR_GAMEMAIN->m_SelectMusicIdx);
		MusicInfo musicInfo = it->second;

		char ver[255] = { '\0' };
		sprintf(ver, "%d", musicInfo.music_base_info().music_version());

		_downloadManager->addFile(packageUrl.c_str(), storagePath.c_str(), ver);
	}
}

void PlayGame::addDownloadBgmAndData()
{
	

    std::string packageUrl = StringUtils::format("%s/%d.%s",
												CDN_SERVER_ROOT,
                                                 PTR_GAMEMAIN->m_SelectMusicIdx,
                                                 "zip");

    std::string storagePath = StringUtils::format("%s/musics/%d/",
                                                  FileUtils::getInstance()->getWritablePath().c_str(),
                                                  PTR_GAMEMAIN->m_SelectMusicIdx);
    
    if(FileUtils::getInstance()->createDirectory(storagePath))
    {
		auto it = GameDB::getInstance()->getMusicInfo(PTR_GAMEMAIN->m_SelectMusicIdx);
		MusicInfo musicInfo = it->second;

		char ver[255] = { '\0' };
		sprintf(ver, "%d", musicInfo.music_base_info().music_version());
		_downloadManager->addFile(packageUrl.c_str(), storagePath.c_str(), ver);
    }
}

void PlayGame::addDownloadInstumentAudio()
{
	auto it = GameDB::getInstance()->getMusicInfo(PTR_GAMEMAIN->m_SelectMusicIdx);
	MusicInfo musicInfo = it->second;
        
	int n = musicInfo.music_inst_infos_size();
	int instID = 0;
	InstType instType;
    
	PTR_GAMEMAIN->m_trackCount = n;

	for (int i = 0; i < n; i++)
	{
		const MusicInstInfo& instInfo = musicInfo.music_inst_infos(i);

		PTR_GAMEMAIN->m_MusicInfoInstType[i] = instInfo.inst_type();

		//if (_option.getInstID() == instInfo.inst_id())
		if (PTR_GAMEMAIN->m_GameInstrumType[0] == PTR_GAMEMAIN->m_MusicInfoInstType[i])
		{
			instID = _option.get1Player().getInstID();
		}
		else
		{
			instID = instInfo.inst_id();
            instType = (InstType)instInfo.inst_type();
            
			if (instType == InstType::PIANO)
			{
				PTR_GAMEMAIN->m_SelectPiano = instID;
			}
			else if (instType == InstType::GUITAR)
			{
				PTR_GAMEMAIN->m_SelectGuitar = instID;
			}
			else if (instType == InstType::DRUM)
			{
				PTR_GAMEMAIN->m_SelectDrum = instID;
			}
		}
#if(CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		std::string platform = "android";
#elif(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#ifdef MIXING
		std::string platform = "ios";
#else
		std::string platform = "android";
#endif
#else
		std::string platform = "ios";
#endif
		std::string packageUrl = StringUtils::format("%s/res/%s/%d.%s",
			CDN_SERVER_ROOT,
			platform.c_str(),
			instID,
			"zip");

		std::string versionFileUrl = StringUtils::format("%s/res/%s/%d.%s",
			CDN_SERVER_ROOT,
			platform.c_str(),
			instID,
			"txt");

		std::string storagePath = StringUtils::format("%s/instruments/%d/",
			FileUtils::getInstance()->getWritablePath().c_str(),
			instID);

		if (FileUtils::getInstance()->createDirectory(storagePath))
		{
			_downloadManager->addFile(packageUrl.c_str(), storagePath.c_str(), versionFileUrl.c_str());

		}
	}
}

void PlayGame::addDownloadNoteFx()
{

    auto it = GameDB::getInstance()->getMusicInfo(_option.getMusicID());
    const MusicInfo& musicInfo = it->second;
    const MusicBaseInfo& musicBaseInfo = musicInfo.music_base_info();
    

	if (musicBaseInfo.is_exist_fx())
	{
		PTR_GAMEMAIN->m_isFxSound = true;
#if(CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		std::string platform = "android";
#elif(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
#ifdef MIXING
		std::string platform = "ios";
#else
		std::string platform = "android";
#endif
#else
		std::string platform = "ios";
#endif

		std::string packageUrl = StringUtils::format("%s/res/%s/%d_FX.%s",
			CDN_SERVER_ROOT,
			platform.c_str(),
			PTR_GAMEMAIN->m_SelectMusicIdx,
			"zip");


		std::string storagePath = StringUtils::format("%s/musics/%d/",
			FileUtils::getInstance()->getWritablePath().c_str(),
			PTR_GAMEMAIN->m_SelectMusicIdx);

		if (FileUtils::getInstance()->createDirectory(storagePath))
		{
			auto it = GameDB::getInstance()->getMusicInfo(PTR_GAMEMAIN->m_SelectMusicIdx);
			MusicInfo musicInfo = it->second;

			char ver[255] = { '\0' };
			sprintf(ver, "%d", musicInfo.music_base_info().music_version());

			_downloadManager->addFile(packageUrl.c_str(), storagePath.c_str(), ver);
		}
	}
}

void PlayGame::onDownloadSuccess()
{
    this->requestStartGame();
}

void PlayGame::requestStartGame()
{
	switch (PTR_GAMEMAIN->m_GameMode)
    {
            
		case PlayMode::PREMIUM:
		case PlayMode::PREVIEW_PLAY:
		case PlayMode::SYNC_PLAY:
            this->requestPremiumStartGame();
            break;

		case PlayMode::STAGE:
			this->requestStageStartGame();
			break;
            
		case PlayMode::PVP:
            this->requestPVPStartGame();
            break;
            
        default:
            break;
    }
}

void PlayGame::requestStageStartGame()
{
	auto req = std::make_shared<StageStartGameReq>();
    if (_option.getSelectStageID() > 0)
    {
        req->set_stage_id(_option.getSelectStageID());
        req->set_is_select_stage(true);
    }
    else
    {
        req->set_stage_id(_option.getStageID());
        req->set_is_select_stage(false);
    }
	
	std::vector<int> item = _option.get1Player().getItemIDs();
	for (int i = 0; i < item.size(); i++)
	{
		req->add_use_item_ids(item[i]);
	}


	ProtobufTaskManager::getInstance()->addRequest(req, [this](const char* buf, int len)
	{
		StageStartGameResp resp;
		resp.ParseFromArray(buf, len);


		StartGameRespInfo * gameInfo = resp.release_start_game_resp_info();

		//각 딜레이 값 셋팅
		//PTR_GAMEMAIN->m_PlayDelay = gameInfo->device_delay();
		PTR_GAMEMAIN->m_PlayDelay = gameInfo->real_delay();
		PTR_GAMEMAIN->m_EarphoneDelay = gameInfo->earphone_delay();
		PTR_GAMEMAIN->m_ResumeDelay = gameInfo->resume_delay();

		//판정 관련 데이타 셋팅
		int itemVal = 0;
		if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_JUDGE].isHave == true)	
			PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_JUDGE].data;

		PTR_GAMEMAIN->m_JudgeRange[0] = ((float)gameInfo->perfect_judge_range()) + ((float)(itemVal*0.01f));
		PTR_GAMEMAIN->m_JudgeRange[1] = ((float)gameInfo->great_judge_range()) + ((float)(itemVal * 0.01f));
		PTR_GAMEMAIN->m_JudgeRange[2] = ((float)gameInfo->cool_judge_range()) + ((float)(itemVal * 0.01f));
		PTR_GAMEMAIN->m_JudgeRange[3] = ((float)gameInfo->good_judge_range()) + ((float)(itemVal * 0.01f));
		PTR_GAMEMAIN->m_JudgeRange[4] = ((float)gameInfo->good_judge_range()) + ((float)(itemVal * 0.01f));


		//판정 score값
		PTR_GAMEMAIN->m_JudgeScore[0] = gameInfo->perfect_judge_score();
		PTR_GAMEMAIN->m_JudgeScore[1] = gameInfo->great_judge_score();
		PTR_GAMEMAIN->m_JudgeScore[2] = gameInfo->cool_judge_score();
		PTR_GAMEMAIN->m_JudgeScore[3] = gameInfo->good_judge_score();
		PTR_GAMEMAIN->m_JudgeScore[4] = gameInfo->long_judge_score();

		PTR_GAMEMAIN->m_JudgeScoreFever[0] = gameInfo->fever_perfect_judge_score();
		PTR_GAMEMAIN->m_JudgeScoreFever[1] = gameInfo->fever_great_judge_score();
		PTR_GAMEMAIN->m_JudgeScoreFever[2] = gameInfo->fever_cool_judge_score();
		PTR_GAMEMAIN->m_JudgeScoreFever[3] = gameInfo->fever_good_judge_score();
		PTR_GAMEMAIN->m_JudgeScoreFever[4] = gameInfo->fever_long_judge_score();


		//하이라이트 설정
		PTR_GAMEMAIN->m_Highlight[0].startPosition = gameInfo->highlight_0_start();
		PTR_GAMEMAIN->m_Highlight[0].endPosition = gameInfo->highlight_0_end();

		PTR_GAMEMAIN->m_Highlight[1].startPosition = gameInfo->highlight_1_start();
		PTR_GAMEMAIN->m_Highlight[1].endPosition = gameInfo->highlight_1_end();

		PTR_GAMEMAIN->m_Highlight[2].startPosition = gameInfo->highlight_2_start();
		PTR_GAMEMAIN->m_Highlight[2].endPosition = gameInfo->highlight_2_end();

	});

	ProtobufTaskManager::getInstance()->execute([this](){ this->_callback(this); });
}

void PlayGame::requestPremiumStartGame()
{ 
    auto req = std::make_shared<PremiumStartGameReq>();
    req->set_music_id(_option.getMusicID());
    req->set_inst_difficult(_option.get1Player().getDifficult());
	req->set_inst_type(PTR_GAMEMAIN->m_GameInstrumType[0]);
    
    auto studioOwnerID = _option.getStudioOwnerID();
    if (studioOwnerID > 0)
    {
        req->set_music_other_user_id(studioOwnerID);
    }

	std::vector<int> item = _option.get1Player().getItemIDs();
	for (int i = 0; i < item.size(); i++)
	{
		req->add_use_item_ids(item[i]);
	}
    
    ProtobufTaskManager::getInstance()->addRequest(req, [this](const char* buf, int len)
    {
        PremiumStartGameResp resp;
        resp.ParseFromArray(buf, len);

		StartGameRespInfo * gameInfo = resp.release_start_game_resp_info();

        //각 딜레이 값 셋팅
		//PTR_GAMEMAIN->m_PlayDelay = gameInfo->device_delay();
		PTR_GAMEMAIN->m_PlayDelay = gameInfo->real_delay();
		PTR_GAMEMAIN->m_EarphoneDelay = gameInfo->earphone_delay();
		PTR_GAMEMAIN->m_ResumeDelay = gameInfo->resume_delay();
        
        //판정 관련 데이타 셋팅
		
		PTR_GAMEMAIN->m_JudgeRange[0] = ((float)gameInfo->perfect_judge_range());
		PTR_GAMEMAIN->m_JudgeRange[1] = ((float)gameInfo->great_judge_range());
		PTR_GAMEMAIN->m_JudgeRange[2] = ((float)gameInfo->cool_judge_range());
		PTR_GAMEMAIN->m_JudgeRange[3] = ((float)gameInfo->good_judge_range());
		PTR_GAMEMAIN->m_JudgeRange[4] = ((float)gameInfo->good_judge_range());
	

		//판정 score값
		PTR_GAMEMAIN->m_JudgeScore[0] = gameInfo->perfect_judge_score();
		PTR_GAMEMAIN->m_JudgeScore[1] = gameInfo->great_judge_score();
		PTR_GAMEMAIN->m_JudgeScore[2] = gameInfo->cool_judge_score();
		PTR_GAMEMAIN->m_JudgeScore[3] = gameInfo->good_judge_score();
		PTR_GAMEMAIN->m_JudgeScore[4] = gameInfo->long_judge_score();
		
		PTR_GAMEMAIN->m_JudgeScoreFever[0] = gameInfo->fever_perfect_judge_score();
		PTR_GAMEMAIN->m_JudgeScoreFever[1] = gameInfo->fever_great_judge_score();
		PTR_GAMEMAIN->m_JudgeScoreFever[2] = gameInfo->fever_cool_judge_score();
		PTR_GAMEMAIN->m_JudgeScoreFever[3] = gameInfo->fever_good_judge_score();
		PTR_GAMEMAIN->m_JudgeScoreFever[4] = gameInfo->fever_long_judge_score();

		//하이라이트 설정
		/*
		PTR_GAMEMAIN->m_Highlight[0].startPosition = gameInfo->highlight_0_start();
		PTR_GAMEMAIN->m_Highlight[0].endPosition = gameInfo->highlight_0_end();

		PTR_GAMEMAIN->m_Highlight[1].startPosition = gameInfo->highlight_1_start();
		PTR_GAMEMAIN->m_Highlight[1].endPosition = gameInfo->highlight_1_end();

		PTR_GAMEMAIN->m_Highlight[2].startPosition = gameInfo->highlight_2_start();
		PTR_GAMEMAIN->m_Highlight[2].endPosition = gameInfo->highlight_2_end();
		*/
		UserDB::getInstance()->setSocialPointCurCount(resp.result_social_point());
		
    });
    
    ProtobufTaskManager::getInstance()->execute([this](){ this->_callback(this); });
}

void PlayGame::requestPVPStartGame()
{
    auto pvpStartReq = std::make_shared<PvpStartGameReq>();
	pvpStartReq->set_music_id(_option.getMusicID());
	pvpStartReq->set_inst_difficult(_option.get1Player().getDifficult());
	pvpStartReq->set_inst_type(PTR_GAMEMAIN->m_GameInstrumType[0]);

	std::vector<int> item = _option.get1Player().getItemIDs();
	for (int i = 0; i < item.size(); i++)
	{
		pvpStartReq->add_use_item_ids(item[i]);
	}

	ProtobufTaskManager::getInstance()->addRequest(pvpStartReq, [this](const char* buf, int len)
	{
		PremiumStartGameResp resp;
		resp.ParseFromArray(buf, len);

		StartGameRespInfo * gameInfo = resp.release_start_game_resp_info();

		//각 딜레이 값 셋팅
		//PTR_GAMEMAIN->m_PlayDelay = gameInfo->device_delay();
		PTR_GAMEMAIN->m_PlayDelay = gameInfo->real_delay();
		PTR_GAMEMAIN->m_EarphoneDelay = gameInfo->earphone_delay();
		PTR_GAMEMAIN->m_ResumeDelay = gameInfo->resume_delay();

		//판정 관련 데이타 셋팅

		PTR_GAMEMAIN->m_JudgeRange[0] = ((float)gameInfo->perfect_judge_range());
		PTR_GAMEMAIN->m_JudgeRange[1] = ((float)gameInfo->great_judge_range());
		PTR_GAMEMAIN->m_JudgeRange[2] = ((float)gameInfo->cool_judge_range());
		PTR_GAMEMAIN->m_JudgeRange[3] = ((float)gameInfo->good_judge_range());
		PTR_GAMEMAIN->m_JudgeRange[4] = ((float)gameInfo->good_judge_range());
		

		//판정 score값
		PTR_GAMEMAIN->m_JudgeScore[0] = gameInfo->perfect_judge_score();
		PTR_GAMEMAIN->m_JudgeScore[1] = gameInfo->great_judge_score();
		PTR_GAMEMAIN->m_JudgeScore[2] = gameInfo->cool_judge_score();
		PTR_GAMEMAIN->m_JudgeScore[3] = gameInfo->good_judge_score();
		PTR_GAMEMAIN->m_JudgeScore[4] = gameInfo->long_judge_score();

		PTR_GAMEMAIN->m_JudgeScoreFever[0] = gameInfo->fever_perfect_judge_score();
		PTR_GAMEMAIN->m_JudgeScoreFever[1] = gameInfo->fever_great_judge_score();
		PTR_GAMEMAIN->m_JudgeScoreFever[2] = gameInfo->fever_cool_judge_score();
		PTR_GAMEMAIN->m_JudgeScoreFever[3] = gameInfo->fever_good_judge_score();
		PTR_GAMEMAIN->m_JudgeScoreFever[4] = gameInfo->fever_long_judge_score();


		//하이라이트 설정
		/*
		PTR_GAMEMAIN->m_Highlight[0].startPosition = gameInfo->highlight_0_start();
		PTR_GAMEMAIN->m_Highlight[0].endPosition = gameInfo->highlight_0_end();

		PTR_GAMEMAIN->m_Highlight[1].startPosition = gameInfo->highlight_1_start();
		PTR_GAMEMAIN->m_Highlight[1].endPosition = gameInfo->highlight_1_end();

		PTR_GAMEMAIN->m_Highlight[2].startPosition = gameInfo->highlight_2_start();
		PTR_GAMEMAIN->m_Highlight[2].endPosition = gameInfo->highlight_2_end();
		*/
		UserDB::getInstance()->setSocialPointCurCount(resp.result_social_point());

	});

	long long id = _option.get2Player().getUserID();

    auto downloadStackSaveReq = std::make_shared<DownloadStackSaveReq>();
	downloadStackSaveReq->set_target_user_id(id);
	downloadStackSaveReq->set_music_id(_option.getMusicID());
	downloadStackSaveReq->set_inst_difficult(_option.get1Player().getDifficult());
	downloadStackSaveReq->set_inst_type(PTR_GAMEMAIN->m_GameInstrumType[0]);

	ProtobufTaskManager::getInstance()->addRequest(downloadStackSaveReq, [this](const char* buf, int len)
	{
		DownloadStackSaveResp resp;
		resp.ParseFromArray(buf, len);

		_option.getNoteRecord().parse(resp.stacksavedata().c_str());

	});

	ProtobufTaskManager::getInstance()->execute([this](){ this->_callback(this); });
}

void PlayGame::lazyInit()
{
#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
	//auto reverbConfig = ConfigManager::getInstance()->getReverbConfig();
	//IZAudioEngine::getInstance()->setReverbSettings(reverbConfig->roomLevel, reverbConfig->decayTime, reverbConfig->reverbLevel);
#endif

	Vec2 origin = Director::getInstance()->getVisibleOrigin();


	//볼륨 적용
	{
		auto it = GameDB::getInstance()->getMusicInfo(PTR_GAMEMAIN->m_SelectMusicIdx);
		MusicInfo musicInfo = it->second;


		VOPlayConfig& playConfig = _option.getPlayConfig();
		playConfig.setVolume(VOPlayConfig::Audio::EFFECT, musicInfo.music_base_info().main_volume());
		playConfig.setVolume(VOPlayConfig::Audio::OTHERS, musicInfo.music_base_info().other_volume());
		playConfig.setVolume(VOPlayConfig::Audio::BGM, musicInfo.music_base_info().bgm_volume());

		//가사를 기본으로 보이게 설정
		/*
#if(CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID )
		playConfig.setEnabledLyrics(false);
#else
		playConfig.setEnabledLyrics(true);
#endif
		*/
		playConfig.setEnabledLyrics(true);


		int val = 0;
		for (int i = 0; i < PTR_GAMEMAIN->m_trackCount; i++)
		{
			const MusicInstInfo& instInfo = musicInfo.music_inst_infos(i);

			PTR_GAMEMAIN->m_MoveSpeed[i] = instInfo.move_speed();
		}
	}


	//touch
	auto touchListener = EventListenerTouchAllAtOnce::create();
	touchListener->onTouchesBegan = CC_CALLBACK_2(PlayGame::touchesBegan, this);
	touchListener->onTouchesMoved = CC_CALLBACK_2(PlayGame::touchesMove, this);
	touchListener->onTouchesEnded = CC_CALLBACK_2(PlayGame::touchesEnd, this);

	this->getEventDispatcher()->addEventListenerWithSceneGraphPriority(touchListener, this);

	PTR_GAMEMAIN->m_isEarphone = NativeDevice::getHeadsetStatus();



	m_BGLayer = PlayGame_BgView::create();
	m_BGLayer->setAnchorPoint(Point(0.0f, 0.0f));
	this->addChild(m_BGLayer, TAG_GAME_BG_LAYER, TAG_GAME_BG_LAYER);

	/*
	m_AniLayer = UIFullScreenAnimationLayer::create("video.jpg", "video.plist", "frame_%05d.png", 0.05f);
	this->addChild(m_AniLayer, TAG_GAME_ANI_LAYER, TAG_GAME_ANI_LAYER);
	*/
	

	m_NoteLayer = PlayGame_NoteView::create();
	m_NoteLayer->setAnchorPoint(Point(0.0f, 0.0f));
	this->addChild(m_NoteLayer, TAG_GAME_NOTE_LAYER, TAG_GAME_NOTE_LAYER);

	m_UILayer = PlayGame_UiView::create();
	this->addChild(m_UILayer, TAG_GAME_UI_LAYER, TAG_GAME_UI_LAYER);


	{
		auto s = Director::getInstance()->getWinSize();

		float farPlane = DEFAULT_FAR_PLANE;
		

		auto cameraBG = Camera::createPerspective(60, (GLfloat)s.width / s.height, 10, 1000);
		// set parameters for camera
		cameraBG->setPosition3D(Vec3(s.width / 2, s.height / 2, (s.height / 1.1566f)));
		cameraBG->lookAt(Vec3(s.width / 2, s.height / 2, 0), Vec3(0, 1, 0));
		cameraBG->setCameraFlag(CameraFlag::USER2);
		this->addChild(cameraBG); //add camera to the scene


		auto camera = Camera::createPerspective(60, (GLfloat)1280.0f / 720.0f, 10, farPlane);
		// set parameters for camera
		camera->setPosition3D(Vec3(640.0f, 360.0f, (720.0f / 1.1566f)));
		camera->lookAt(Vec3(640.0f, 360.0f, 0), Vec3(0, 1, 0));
		
		camera->setCameraFlag(CameraFlag::USER1);
		this->addChild(camera); //add camera to the scene
	}

	

	{
		char name[255] = { 0, };
		sprintf(name, "musics/%d/%d_lyrics.json", PTR_GAMEMAIN->m_SelectMusicIdx, PTR_GAMEMAIN->m_SelectMusicIdx);
		PTR_GAMEMAIN->Read_LyricsData(name, true);
        m_UILayer->loadLyriceData();
	}

	if (PTR_GAMEMAIN->m_isPreViewPlay == true)
	{
		m_OriginVolume_bgm = m_Volume_bgm;
		m_OriginVolume_main = m_Volume_main;
		m_OriginVolume_other = m_Volume_other;
	}



	//이펙트음 로딩
	/*
	{
		char name[512] = {'\0'};
		getEffectSoundPath("res/audio/Fever.m4a", name);
		IZAudioEngine::getInstance()->preloadEffect(name);
		getEffectSoundPath("res/audio/Disturb.m4a", name);
		IZAudioEngine::getInstance()->preloadEffect(name);
		getEffectSoundPath("res/audio/Count_S.m4a", name);
		IZAudioEngine::getInstance()->preloadEffect(name);
		getEffectSoundPath("res/audio/Count123.m4a", name);
		IZAudioEngine::getInstance()->preloadEffect(name);
		
	}
	*/
}


void PlayGame::getEffectSoundPath(char* name, char* fullPath)
{
	/*
	std::string m_cFullPathFileName;
	char m_filePath[256];

	sprintf(m_filePath, "%s", name);
	m_cFullPathFileName = m_filePath;
	//어플내에서 읽기
	std::string _pathKey = m_cFullPathFileName.c_str();
	_pathKey = FileUtils::sharedFileUtils()->fullPathForFilename(_pathKey.c_str());
	sprintf(m_filePath, "%s", _pathKey.c_str());
	log("getEffectSoundPath()                    %s", m_filePath);
	*/

	char _filePath[256] = { 0, };
	memcpy(_filePath, name, strlen(name));
	std::string pathKey = _filePath;
	log("__________________________________getEffectSoundPath()111 pathKey:%s", pathKey.c_str());
	pathKey = FileUtils::sharedFileUtils()->fullPathForFilename(pathKey.c_str());

	//memcpy(fullPath, pathKey.c_str(), sizeof(pathKey.c_str()));
	sprintf(fullPath, "%s", pathKey.c_str());

	log("__________________________________getEffectSoundPath()222 name:%s   %s", name, pathKey.c_str());
}

void PlayGame::initVal()
{

	PTR_GAMEMAIN->m_measureCount = 0;

	PTR_GAMEMAIN->m_PauseButtonTime = 0;
	
	m_isPause = false;
	m_isGameEnd = false;
	m_isGameOver = false;

	//m_isFever = false;
	m_FeverState = 0;

	m_feverGauge[0] = 0;
	m_feverGauge[1] = 100;

	m_feverTime[0] = 0.0f;


	m_isCounting = false;

	m_iScore[0] = m_iScore[1] =0;

	PTR_GAMEMAIN->m_isGameError = false;
	PTR_GAMEMAIN->m_isFxSound = false;

	PTR_GAMEMAIN->m_isLoadingEffectSound = false;


	PTR_GAMEMAIN->m_PlayDelay = 0;
	PTR_GAMEMAIN->m_EarphoneDelay = 0;
	PTR_GAMEMAIN->m_ResumeDelay = 0;

	PTR_GAMEMAIN->m_isEarphone = false;

	m_isRewind = false;
	m_isReStart = false;
	m_TestAutoPaly = false;
	m_TestFreePaly = false;
	m_NotGameOver = false;

	m_Volume_bgm = 0.0f;
	m_Volume_main = 0.0f;
	m_Volume_other = 0.0f;


	m_baseFeverVal = 0.0f;
	m_baseFeverTime = 0.0f;


	PTR_GAMEMAIN->m_SelectChannel = -1;

	PTR_GAMEMAIN->m_BaseMoveSpeed = 0;

	//PTR_GAMEMAIN->m_PlayTime = 0;
	PTR_GAMEMAIN->m_PlayNoteCnt = 0;

	PTR_GAMEMAIN->m_ComboCnt[0] = 0;
	PTR_GAMEMAIN->m_ComboCnt[1] = 0;

	PTR_GAMEMAIN->m_ComboCnt_2[0] = 0;
	PTR_GAMEMAIN->m_ComboCnt_2[1] = 0;

	PTR_GAMEMAIN->m_HiddenSuccessCnt = 0;


	PTR_GAMEMAIN->m_JudgeRange[0] = 20.0f;
	PTR_GAMEMAIN->m_JudgeRange[1] = 30.0f;
	PTR_GAMEMAIN->m_JudgeRange[2] = 60.0f;
	PTR_GAMEMAIN->m_JudgeRange[3] = 100.0f;
	PTR_GAMEMAIN->m_JudgeRange[4] = 200.0f;

	PTR_GAMEMAIN->m_SelectPiano = -1;
	PTR_GAMEMAIN->m_SelectGuitar = -1;
	PTR_GAMEMAIN->m_SelectDrum = -1;


	for (int k = 0; k < 3; k++)
	{
		PTR_GAMEMAIN->m_MusicInfoInstType[k] = -1;
	}


	for (int i = 0; i < 2; i++)
	{
		for (int k = 0; k < 6; k++)
		{
			PTR_GAMEMAIN->m_JudgeCount[i][k] = 0;
			PTR_GAMEMAIN->m_JudgeCountFever[i][k] = 0;
		}
	}


	m_feverStartTime = 0;


	PTR_GAMEMAIN->m_JudgeNoteCount[0] = 0;
	PTR_GAMEMAIN->m_JudgeNoteCount[1] = 0;

	
	for (int k = 0; k < MAX_JUDGE_NOTE_CNT; k++)
	{
		PTR_GAMEMAIN->m_judgeNoteData[0].judgeType[k] = NOTE_MISS;
		PTR_GAMEMAIN->m_judgeNoteData[1].judgeType[k] = NOTE_MISS;

		PTR_GAMEMAIN->m_judgeNoteData[0].fever[k] = false;
		PTR_GAMEMAIN->m_judgeNoteData[1].fever[k] = false;
	}


	memset(PTR_GAMEMAIN->m_filePath, 0, sizeof(PTR_GAMEMAIN->m_filePath));
	memset(PTR_GAMEMAIN->m_filePath_Voice, 0, sizeof(PTR_GAMEMAIN->m_filePath_Voice));
	PTR_GAMEMAIN->m_isVoice = false;

	PTR_GAMEMAIN->m_isPreViewPlay = false;


	PTR_GAMEMAIN->m_isSampler = false;

#ifdef	PLAY_NOTE_GUIDE
	memset(PTR_GAMEMAIN->m_noteLineData, -1, sizeof(PTR_GAMEMAIN->m_noteLineData));
	memset(PTR_GAMEMAIN->m_noteLineDataIdx, 0, sizeof(PTR_GAMEMAIN->m_noteLineDataIdx));
#endif

	//시작 테스트
	PTR_GAMEMAIN->m_isStartGameTime = false;
	PTR_GAMEMAIN->m_isStartMRTime = false;
	PTR_GAMEMAIN->m_StartGameTime = 0;
	PTR_GAMEMAIN->m_isSyncButton = true;



	//방해요소 관련 변구

	for (int i = 0; i < 15; i++)
	{
		PTR_GAMEMAIN->m_isHindrance[i] = false;
	}


	PTR_GAMEMAIN->m_illusion_speed = 2.0f;


	//아이템
	for (int i = 0; i < 8; i++)
	{
		PTR_GAMEMAIN->m_ItemInfo[i].isHave = false;
		PTR_GAMEMAIN->m_ItemInfo[i].data = 0;
	}

	for (int i = 0; i < 5; i++)
	{
		PTR_GAMEMAIN->m_HaveItem[i] = 0;
	}

	
	PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_LIFEGAUGE].data = 30;
	PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_JUDGE].data = 3;
	PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_SCORE].data = 5;
	PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_MISS].data = 10;
	


	for (int i = 0; i < 3; i++)
	{
		PTR_GAMEMAIN->m_Highlight[i].startPosition = 0;
		PTR_GAMEMAIN->m_Highlight[i].endPosition = 0;
		PTR_GAMEMAIN->m_Highlight[i].isStart = false;
	}


	for (int i = 0; i < 5; i++)
	{
		PTR_GAMEMAIN->m_Hindrance[i].type = -1;
		PTR_GAMEMAIN->m_Hindrance[i].condition = 1;
		PTR_GAMEMAIN->m_Hindrance[i].value[0] = 0;
		PTR_GAMEMAIN->m_Hindrance[i].value[1] = 0;
	}



	for (int i = 0; i < 7; i++)
	{
		PTR_GAMEMAIN->m_gameClearContition[i] = 0;
		PTR_GAMEMAIN->m_gameClearContitionClear[i] = false;
	}



	m_feverTotalTime = 0;
}


void PlayGame::onEnter()
{
	Screen::onEnter();

	if (PTR_GAMEMAIN->m_isGameError == true)
	{

		return;
	}

	if (PTR_GAMEMAIN->m_isPreViewPlay == true)
	{
		auto it = GameDB::getInstance()->getMusicInfo(PTR_GAMEMAIN->m_SelectMusicIdx);
		MusicInfo musicInfo = it->second;
		MusicBaseInfo musicBaseInfo = musicInfo.music_base_info();

		int sampleTime = musicBaseInfo.sample_start_time();
		int time = 0;
		if (sampleTime > 0)
		{
			time = sampleTime / 1000;
			time = time * 1000;
		}
		else
		{
			time = PTR_GAMEMAIN->m_PreViewStartTime / 1000;
			time = time * 1000;
		}
		time -= 1000;
		if (time < 0)
			time = 0;

		PTR_GAMEMAIN->m_PreViewStartTime = time;
		m_TestAutoPaly = true;
		PTR_GAMEMAIN->m_PreViewEndTime = time + 20000;

		m_NoteLayer->noteClearForPreview(PTR_GAMEMAIN->m_PreViewStartTime);
	}
	else
	{
		PTR_GAMEMAIN->m_PreViewStartTime = 0;

#if(CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		//m_TestAutoPaly = true;
#endif

	}

	if (PTR_GAMEMAIN->m_GameMode == PlayMode::SYNC_PLAY)
	{
		m_TestAutoPaly = true;
	}

	


	if (PTR_GAMEMAIN->m_isVideoRec)
	{
#ifdef VIDEO_RECORD
		//THCCommon::startCameraPreview();
#endif
	}
    
    TouchManager::getInstance()->setTouchType(TouchManager::TouchType::MULTIPLE);
    
    IZAudioEngine::getInstance()->allocPlayer(16);
    
    this->addCustomEventListener(APP_DID_ENTER_BG, CC_CALLBACK_1(PlayGame::onDidEnterBG, this));
    this->addCustomEventListener(APP_WILL_ENTER_FG, CC_CALLBACK_1(PlayGame::onWillEnterFG, this));

	this->addCustomEventListener(APP_WILL_RESIGN_ACTIVE, CC_CALLBACK_1(PlayGame::onWillResignActive, this));
	this->addCustomEventListener(APP_DID_BECOME_ACTIVE, CC_CALLBACK_1(PlayGame::onDisBecomeActive, this));

    
    if (PTR_GAMEMAIN->m_GameMode == PlayMode::PVP )
    {
        m_UILayer->initPVP_Gauge();
    }

    
	//시작시 초기화
	IZAudioEngine::getInstance()->stopAllAudios();
	IZAudioEngine::getInstance()->stopAllEffects();

    IZAudioEngine::getInstance()->setVolumes(m_Volume_bgm*100.0f, m_Volume_other, m_Volume_main);
	LoadingManager::getInstance()->show();
	schedule(schedule_selector(PlayGame::PreLoading), 0.0f);
	PTR_GAMEMAIN->m_isLoadingEffectSound = true;
}



void PlayGame::PreLoading(float pt)
{
	if (m_NoteLayer->loadEffectSound() == true)
	{
		PTR_GAMEMAIN->m_isLoadingEffectSound = false;
		m_UILayer->LoadingDone();


		for (int i = 0; i < 3; i++)
		{
			if (PTR_GAMEMAIN->m_Highlight[i].startPosition > 0)
			{
				m_NoteLayer->setHightlight(i);
			}
		}


		unschedule(schedule_selector(PlayGame::PreLoading));
		initCountScene(0, 1.0f);
		LoadingManager::getInstance()->hide();
	}
}

void PlayGame::setLoadingText(char* str)
{
	m_UILayer->setLoadingText(str);
}



void PlayGame::onExit()
{
	Screen::onExit();
    /*
    IZAudioEngine::getInstance()->stopAudio(_bgmID);
	if (PTR_GAMEMAIN->m_isVoice == true)
	{
		IZAudioEngine::getInstance()->stopAudio(_voiceID);
	}
     */
    IZAudioEngine::getInstance()->stopMR();

	m_BGLayer->removeAllChildrenWithCleanup(true);

	//m_AniLayer->removeAllChildrenWithCleanup(true);

	m_NoteLayer->removeAllChildrenWithCleanup(true);


	m_UILayer->removeAllChildrenWithCleanup(true);
	removeAllChildrenWithCleanup(true);

    CC_SAFE_RELEASE_NULL(_downloadManager);
    
    IZAudioEngine::getInstance()->freePlayer();
    
    this->removeAllEventListener();
}


void PlayGame::onDidEnterBG(EventCustom* event)
{
    if (PTR_GAMEMAIN->m_isLoadingEffectSound == true)
    {
    }
    else
    {
		if (m_isCounting == true)
		{
			unschedule(schedule_selector(PlayGame::CountingScene));
		}
		else
		{
			if (m_isGameOver == false)
			{
				setPause();
				openPlayPausePopup();
			}
		}
    }
}
void PlayGame::onWillEnterFG(EventCustom* event)
{
    log("+++++++++++++onWillEnterFG()");
	if (m_isCounting == true)
	{
		schedule(schedule_selector(PlayGame::CountingScene), 1.0f);
	}
	else
	{
		if (m_isGameEnd == false)
		{
			//startResume();
		}
	}
}

void PlayGame::onWillResignActive(EventCustom* event)
{
    if (PTR_GAMEMAIN->m_isLoadingEffectSound == true)
    {
    }
    else
    {
		if (m_isCounting == true)
		{
			unschedule(schedule_selector(PlayGame::CountingScene));
		}
		else
		{
			setPause();
			openPlayPausePopup();
		}
    }
}
void PlayGame::onDisBecomeActive(EventCustom* event)
{
    log("+++++++++++++onDisBecomeActive()");
	if (m_isGameEnd == false)
	{
		//startResume();
	}
}

void PlayGame::update(float dt)
{
#ifdef NEW_SYNC_PROC
	long long time = 0;

	m_updataTick++;

	if (m_updataTick > 1000000)
	{
		m_updataTick = 1000;
	}

	if (PTR_GAMEMAIN->m_isStartMRTime == false)
	{
		if (m_updataTick >= (100 + PTR_GAMEMAIN->m_MySyncValue))
		{
			PTR_GAMEMAIN->m_isStartMRTime = true;
            IZAudioEngine::getInstance()->startMR();
            if(m_isPause)
                IZAudioEngine::getInstance()->pauseMR();
            /*
			_bgmID = IZAudioEngine::getInstance()->playAudio(PTR_GAMEMAIN->m_filePath, m_Volume_bgm, false);
			
			if (m_isPause == true)
			{
				IZAudioEngine::getInstance()->setAudioCurrentTime(_bgmID, PTR_GAMEMAIN->m_NowMusicPlayTime);
			}
			else
			{
				if (PTR_GAMEMAIN->m_PreViewStartTime > 0)
					IZAudioEngine::getInstance()->setAudioCurrentTime(_bgmID, PTR_GAMEMAIN->m_PreViewStartTime);
			}
			if (PTR_GAMEMAIN->m_isVoice == true)
			{
				_voiceID = IZAudioEngine::getInstance()->playAudio(PTR_GAMEMAIN->m_filePath_Voice, m_Volume_bgm, false);
				if (m_isPause == true)
				{
					IZAudioEngine::getInstance()->setAudioCurrentTime(_voiceID, PTR_GAMEMAIN->m_NowMusicPlayTime);
				}
				else
				{
					if (PTR_GAMEMAIN->m_PreViewStartTime > 0)
						IZAudioEngine::getInstance()->setAudioCurrentTime(_voiceID, PTR_GAMEMAIN->m_PreViewStartTime);
				}
			}
            */
			
			log("update()    start MR   tick :%d !!!!!!!!!!!!!!!!!!!!!!!!", m_updataTick);
		}
	}

	if (m_updataTick == 100)
	{
		if (m_isPause == true)
		{
			PTR_GAMEMAIN->m_MusicPlayTimeUM = (PTR_GAMEMAIN->getUMicroSecond() - (PTR_GAMEMAIN->m_NowMusicPlayTime * 1000));

			if (m_FeverState > 0)
			{
				m_feverTime[0] += (MOVE_PLAY_TIME / 1000.0f);

				if (m_feverTime[0] > m_feverTime[1])
					m_feverTime[0] = m_feverTime[1];

				m_feverStartTime = (PTR_GAMEMAIN->getUMicroSecond() / 1000);
				schedule(schedule_selector(PlayGame::FeverEnd), 0.1f);

				if (m_FeverState == 1)
					m_NoteLayer->pauseFeverEffect(false);
			}
			m_isPause = false;
		}
		else
		{
			//PTR_GAMEMAIN->m_MusicPlayTimeUM = (PTR_GAMEMAIN->getUMicroSecond() - (PTR_GAMEMAIN->m_PreViewStartTime * 1000));
			PTR_GAMEMAIN->m_MusicPlayTimeUM = PTR_GAMEMAIN->getUMicroSecond();
		}
		log("update()    start tick :%d !!!!!!!!!!!!!!!!!!!!!!!!    PTR_GAMEMAIN->m_MusicPlayTimeUM : %lld   noTime:%lld", m_updataTick, PTR_GAMEMAIN->m_MusicPlayTimeUM, PTR_GAMEMAIN->m_NowMusicPlayTime);
		PTR_GAMEMAIN->m_isSyncButton = true;
	}

	if (m_updataTick >= 100)
#endif
	{
		

		int percent = m_UILayer->setPlayProgressBar(m_NoteLayer->m_pTimeUpdate);

		if (percent >= 100)
		{
			if (m_isGameEnd == false)
			{
				log("$$$$$$$$$$$$$$$$$$$$$$   END GAME!!!!!!!!!!!!!!!!");
				m_isGameEnd = true;

				if (PTR_GAMEMAIN->m_isRecording)
				{
					IZAudioEngine::getInstance()->stopRecord();
				}

				if (PTR_GAMEMAIN->m_isVideoRec)
				{
#ifdef VIDEO_RECORD
					//THCCommon::stopVideoRecord();
#endif
				}
				this->requestEndGame();
				return;
			}
		}


		if (m_isPause || m_isGameEnd)
			return;


		m_NoteLayer->ProC(dt);
		m_UILayer->updateEffectObject(dt);

		if (PTR_GAMEMAIN->m_isLyrics == true)
		{
			m_UILayer->DrawLyrics(m_NoteLayer->m_pTimeUpdate);
		}

		if (m_FeverState > 0)
		{
			/*
			if (m_FeverState == 1)
			{
			m_NoteLayer->FeverEffectProC();
			}
			*/

			if (m_FeverState == 1)
			{
				if (m_NoteLayer->feverEffectChange() == true)
				{
					m_FeverState = 2;
					m_NoteLayer->startFeverEffect(m_FeverState);
				}
			}
			/*
			else if (m_FeverState == 2)
			{
				m_NoteLayer->FeverEffectProC();
			}
			*/
		
		}


		if (PTR_GAMEMAIN->m_isPreViewPlay == true)
		{
			if (m_NoteLayer->m_pTimeUpdate >= PTR_GAMEMAIN->m_PreViewEndTime)
			{
				this->requestEndGame();
				return;
			}
			if (m_NoteLayer->m_pTimeUpdate > PTR_GAMEMAIN->m_PreViewEndTime - PREVIEW_FADE_OUT_TIME)
			{
				if (!getChildByTag(PREVIEW_FADE_LAYER_TAG))
				{
					onPreViewPlayFadeout(CC_CALLBACK_1(PlayGame::endViewPlayFadeout, this));
				}
				// FADE 시간이 되면 음량을 계산해서 줄인다.

				m_Volume_bgm = (PTR_GAMEMAIN->m_PreViewEndTime - m_NoteLayer->m_pTimeUpdate) / PREVIEW_FADE_OUT_TIME;
				m_Volume_bgm *= m_OriginVolume_bgm; // _playOption.getPlayConfig().getVolume(PlayConfig::Audio::BGM);

				m_Volume_main = (PTR_GAMEMAIN->m_PreViewEndTime - m_NoteLayer->m_pTimeUpdate) / PREVIEW_FADE_OUT_TIME;
				m_Volume_main *= m_OriginVolume_main;// _playOption.getPlayConfig().getVolume(PlayConfig::Audio::EFFECT);

				m_Volume_other = (PTR_GAMEMAIN->m_PreViewEndTime - m_NoteLayer->m_pTimeUpdate) / PREVIEW_FADE_OUT_TIME;
				m_Volume_other *= m_OriginVolume_other;// _playOption.getPlayConfig().getVolume(PlayConfig::Audio::OTHERS);

				IZAudioEngine::getInstance()->setAudioVolume(_bgmID, m_Volume_bgm);
				if (PTR_GAMEMAIN->m_isVoice == true)
				{
					IZAudioEngine::getInstance()->setAudioVolume(_voiceID, 1.0f);
				}

				CCLOG("m_Volume_bgm = %2.f , m_Volume_main = %2.f, m_Volume_other = %2.f", m_Volume_bgm, m_Volume_main, m_Volume_other);


			}
		}
	}
}


void PlayGame::onPreViewPlayFadeout(ccHandleFadeEndCallback _callback)
{
	LayerColor* pFadeLayer(LayerColor::create(Color4B::BLACK));
	pFadeLayer->setOpacity(0);

	pFadeLayer->runAction(Sequence::create(FadeIn::create(PREVIEW_FADE_OUT_TIME * 0.001f), CCCallFuncN::create(_callback), NULL));

	addChild(pFadeLayer, PREVIEW_FADE_LAYER_TAG, PREVIEW_FADE_LAYER_TAG);

	//if (m_UILayer)
	//	m_UILayer->hidePauseBtn();

}

void PlayGame::onViewPlayFadeout(ccHandleFadeEndCallback _callback)
{
	LayerColor* pFadeLayer(LayerColor::create(Color4B::BLACK));
	pFadeLayer->setOpacity(0);

	pFadeLayer->runAction(Sequence::create(FadeIn::create(.15f), CCCallFuncN::create(_callback), NULL));

	addChild(pFadeLayer, PREVIEW_FADE_LAYER_TAG, PREVIEW_FADE_LAYER_TAG);

	//if (m_UILayer)
	//	m_UILayer->hidePauseBtn();

}
void PlayGame::endViewPlayFadeout(Node* pNode)
{
	/*
	ScreenManager::getInstance()->replaceScreen(LoadingScreen::create(MusicListScreen::create(getPlayOption().getSelectedCategoryID(), getPlayOption().getMusicID()
		, getPlayOption().getInstID(), getPlayOption().getDifficult()), getPlayOption()));
		*/
	return;
}

void PlayGame::requestEndGame()
{

	this->unscheduleUpdate();

	//if (m_isGameEnd == false)
	{
		IZAudioEngine::getInstance()->stopAudio(_bgmID);
		if (PTR_GAMEMAIN->m_isVoice == true)
		{
			IZAudioEngine::getInstance()->stopAudio(_voiceID);
		}
	}

	if (m_FeverState > 0)
	{
		m_FeverState = 0;
		m_NoteLayer->endFeverEffect();
		unschedule(schedule_selector(PlayGame::FeverEnd));
	}

	if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::PIANO)
	{
		_option.getNoteRecord().setInstType((int)InstType::PIANO);
		_option.getNoteRecord().setInstID(PTR_GAMEMAIN->m_SelectPiano);
	}
	else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::GUITAR)
	{
		_option.getNoteRecord().setInstType((int)InstType::GUITAR);
		_option.getNoteRecord().setInstID(PTR_GAMEMAIN->m_SelectGuitar);
	}
	else if (PTR_GAMEMAIN->m_GameInstrumType[0] == (int)InstType::DRUM)
	{
		_option.getNoteRecord().setInstType((int)InstType::DRUM);
		_option.getNoteRecord().setInstID(PTR_GAMEMAIN->m_SelectDrum);
	}
	_option.getNoteRecord().setNoteCount(PTR_GAMEMAIN->m_noteCount);

//    UserPlayData& userPlayData = GameDB::getInstance()->getUserPlayData();


//	if (PTR_GAMEMAIN->m_GameMode == GAME_CHALLENGE || PTR_GAMEMAIN->m_GameMode == GAME_RIVAL)
//	{
//		if (m_UILayer->m_PvpGauge[0] == m_UILayer->m_PvpGauge[1])
//		{
//			userPlayData.getGameData().setOutcome(OutcomeType::DRAW);
//		}
//		else if (m_UILayer->m_PvpGauge[0] > m_UILayer->m_PvpGauge[1])
//		{
//			userPlayData.getGameData().setOutcome(OutcomeType::WIN);
//		}
//		else
//		{
//			userPlayData.getGameData().setOutcome(OutcomeType::LOSE);
//		}
//
//	}
    


#ifdef SOUNDTEAM_TEST_BUILD
	if (PTR_GAMEMAIN->m_GameMode == PlayMode::SYNC_PLAY)
	{
		ScreenManager::getInstance()->replaceScreen(HomeScreen::create());
		return;
	}
	else if (PTR_GAMEMAIN->m_GameMode == PlayMode::STAGE)
	{
		ScreenManager::getInstance()->replaceScreen(StageListScreen::create());
	}
	else
	{
		ScreenManager::getInstance()->replaceScreen(MusicListScreen::create(_option.getCategoryID(), _option.getMusicID(), (int)_option.getPlayMode(), _option.get1Player().getInstID(), _option.get1Player().getDifficult()));
	}
#else
    switch(PTR_GAMEMAIN->m_GameMode)
    {
		case PlayMode::PREVIEW_PLAY:
			ScreenManager::getInstance()->popScreen();
            break;
       
		case PlayMode::PREMIUM:
            this->requestPremiumEndGame();
            break;
            
		case PlayMode::PVP:
            this->requestPVPEndGame();
            break;

		case PlayMode::STAGE:
			this->requestStageEndGame();
			break;
            
        default:
            break;
    }
#endif
}

void PlayGame::requestStageEndGame()
{
	auto req = std::make_shared<StageEndGameReq>();
	if (_option.getSelectStageID() > 0)
    {
        req->set_stage_id(_option.getSelectStageID());
        req->set_is_select_stage(true);
    }
    else
    {
        req->set_stage_id(_option.getStageID());
        req->set_is_select_stage(false);
    }
    
	req->set_inst_id(_option.get1Player().getInstID());

	req->set_miss_count(PTR_GAMEMAIN->m_JudgeCount[0][NOTE_MISS]);
	req->set_good_count(PTR_GAMEMAIN->m_JudgeCount[0][NOTE_GOOD]);
	req->set_cool_count(PTR_GAMEMAIN->m_JudgeCount[0][NOTE_COOL]);
	req->set_great_count(PTR_GAMEMAIN->m_JudgeCount[0][NOTE_GREAT]);
	req->set_perfect_count(PTR_GAMEMAIN->m_JudgeCount[0][NOTE_PERFECT]);
	req->set_long_count(PTR_GAMEMAIN->m_JudgeCount[0][NOTE_LONG_TAIL]);

	req->set_max_combo(PTR_GAMEMAIN->m_ComboCnt[1]);
	req->set_fever_count(0);

	req->set_fever_good_count(PTR_GAMEMAIN->m_JudgeCountFever[0][NOTE_GOOD]);
	req->set_fever_cool_count(PTR_GAMEMAIN->m_JudgeCountFever[0][NOTE_COOL]);
	req->set_fever_great_count(PTR_GAMEMAIN->m_JudgeCountFever[0][NOTE_GREAT]);
	req->set_fever_perfect_count(PTR_GAMEMAIN->m_JudgeCountFever[0][NOTE_PERFECT]);
	req->set_fever_long_count(PTR_GAMEMAIN->m_JudgeCountFever[0][NOTE_LONG_TAIL]);

	req->set_fever_time(m_feverTotalTime);

	req->set_is_game_over(m_isGameOver);

	req->set_select_stage_music_id(_option.getMusicID());
	req->set_select_stage_inst_difficult((int)_option.get1Player().getDifficult());
	req->set_select_stage_inst_type(PTR_GAMEMAIN->m_GameInstrumType[0]);


	for (int i = 0; i < 6; i++)
	{
		log(" requestPremiumEndGame()  m_JudgeCount[%d]=%d  m_JudgeCountFever[%d]=%d ", i, PTR_GAMEMAIN->m_JudgeCount[0][i], i, PTR_GAMEMAIN->m_JudgeCountFever[0][i]);
	}

	ProtobufTaskManager::getInstance()->addRequest(req, [this](const char* buf, int len)
	{

		StageEndGameResp resp;
		resp.ParseFromArray(buf, len);

		VOResult result(resp.is_new_record(), resp.perfect_count(), resp.great_count(), resp.cool_count(), resp.good_count(), resp.miss_count(), resp.score(), resp.high_score(), 0, 
			resp.rank(), resp.clear_state(), 0, resp.max_combo(), resp.bonus_score(), resp.harmony_score(), resp.is_score_clear(), resp.is_combo_clear(), resp.is_perfect_clear(), resp.is_great_clear(),
			resp.is_good_clear(), resp.is_fever_clear(), resp.is_miss_clear(), resp.is_all_clear(), resp.cash_price(), resp.survey_sale_price());
		_option.setResultFor1P(result);
	});
    ProtobufTaskManager::getInstance()->addRequest(std::make_shared<UserInfoReq>());
	ProtobufTaskManager::getInstance()->execute([this]()
	{
		ScreenManager::getInstance()->pushScreen(PlayResultScreen::create(&this->_option));
	});
}

void PlayGame::requestPremiumEndGame()
{   
    auto req = std::make_shared<PremiumEndGameReq>();
    req->set_music_id(_option.getMusicID());
	req->set_inst_type(PTR_GAMEMAIN->m_GameInstrumType[0]);
    req->set_inst_id(_option.get1Player().getInstID());
    req->set_inst_difficult((int)_option.get1Player().getDifficult());
    
	req->set_miss_count(PTR_GAMEMAIN->m_JudgeCount[0][NOTE_MISS]);
	req->set_good_count(PTR_GAMEMAIN->m_JudgeCount[0][NOTE_GOOD]);
	req->set_cool_count(PTR_GAMEMAIN->m_JudgeCount[0][NOTE_COOL]);
	req->set_great_count(PTR_GAMEMAIN->m_JudgeCount[0][NOTE_GREAT]);
	req->set_perfect_count(PTR_GAMEMAIN->m_JudgeCount[0][NOTE_PERFECT]);
	req->set_long_count(PTR_GAMEMAIN->m_JudgeCount[0][NOTE_LONG_TAIL]);

	req->set_max_combo(PTR_GAMEMAIN->m_ComboCnt[1]);
	req->set_fever_count(0);

	req->set_fever_good_count(PTR_GAMEMAIN->m_JudgeCountFever[0][NOTE_GOOD]);
	req->set_fever_cool_count(PTR_GAMEMAIN->m_JudgeCountFever[0][NOTE_COOL]);
	req->set_fever_great_count(PTR_GAMEMAIN->m_JudgeCountFever[0][NOTE_GREAT]);
	req->set_fever_perfect_count(PTR_GAMEMAIN->m_JudgeCountFever[0][NOTE_PERFECT]);
	req->set_fever_long_count(PTR_GAMEMAIN->m_JudgeCountFever[0][NOTE_LONG_TAIL]);

	req->set_is_game_over(m_isGameOver);

	for (int i = 0; i < 6; i++)
	{
		log(" requestPremiumEndGame()  m_JudgeCount[%d]=%d  m_JudgeCountFever[%d]=%d ", i, PTR_GAMEMAIN->m_JudgeCount[0][i], i, PTR_GAMEMAIN->m_JudgeCountFever[0][i]);
	}

	ProtobufTaskManager::getInstance()->addRequest(req, [this](const char* buf, int len)
    {
		
		PremiumEndGameResp resp;
		resp.ParseFromArray(buf, len);
        
		VOResult result(resp.is_new_record(), resp.perfect_count(), resp.great_count(), resp.cool_count(), resp.good_count(), resp.miss_count(), resp.score(), resp.high_score(), 0, resp.rank(), resp.clear_state(), 0, resp.max_combo(), resp.bonus_score(), resp.harmony_score(),
			false, false, false, false, false, false, false, false, 0, 0);
		_option.setResultFor1P(result);
    });
    ProtobufTaskManager::getInstance()->addRequest(std::make_shared<UserInfoReq>());
	ProtobufTaskManager::getInstance()->execute([this]()
	{
		ScreenManager::getInstance()->pushScreen(PlayResultScreen::create(&this->_option));
	});
}

void PlayGame::requestPVPEndGame()
{
	bool isPvpWin = false;

	if (m_UILayer->m_PvpGauge[0] > m_UILayer->m_PvpGauge[1])
	{
		isPvpWin = true;
	}
	
    auto req = std::make_shared<PvpEndGameReq>();
	req->set_music_id(_option.getMusicID());
	
	for (int i = 0; i < 2; i++)
	{
		EndGameReqData *endData = req->add_end_game_req_datas();

		endData->set_inst_type(PTR_GAMEMAIN->m_GameInstrumType[0]);
		endData->set_inst_difficult((int)_option.get1Player().getDifficult());

		endData->set_miss_count(PTR_GAMEMAIN->m_JudgeCount[i][NOTE_MISS]);
		endData->set_good_count(PTR_GAMEMAIN->m_JudgeCount[i][NOTE_GOOD]);
		endData->set_cool_count(PTR_GAMEMAIN->m_JudgeCount[i][NOTE_COOL]);
		endData->set_great_count(PTR_GAMEMAIN->m_JudgeCount[i][NOTE_GREAT]);
		endData->set_perfect_count(PTR_GAMEMAIN->m_JudgeCount[i][NOTE_PERFECT]);
		endData->set_long_count(PTR_GAMEMAIN->m_JudgeCount[i][NOTE_LONG_TAIL]);

		if(i == 0)
			endData->set_max_combo(PTR_GAMEMAIN->m_ComboCnt[1]);
		else
			endData->set_max_combo(PTR_GAMEMAIN->m_ComboCnt_2[1]);
		endData->set_fever_count(0);

		endData->set_fever_good_count(PTR_GAMEMAIN->m_JudgeCountFever[i][NOTE_GOOD]);
		endData->set_fever_cool_count(PTR_GAMEMAIN->m_JudgeCountFever[i][NOTE_COOL]);
		endData->set_fever_great_count(PTR_GAMEMAIN->m_JudgeCountFever[i][NOTE_GREAT]);
		endData->set_fever_perfect_count(PTR_GAMEMAIN->m_JudgeCountFever[i][NOTE_PERFECT]);
		endData->set_fever_long_count(PTR_GAMEMAIN->m_JudgeCountFever[i][NOTE_LONG_TAIL]);


		log("endGame  m_JudgeCount[%d][PREFECT]=%d  ", i, PTR_GAMEMAIN->m_JudgeCount[i][NOTE_PERFECT]);
		// 승패 판단
		OutcomeType winstate = OutcomeType::WIN;
		if (m_UILayer->m_PvpGauge[0] > m_UILayer->m_PvpGauge[1])
		{
			if (i == 0)
			{
				winstate = OutcomeType::WIN;
			}
			else
			{
				winstate = OutcomeType::LOSE;
			}
		}
		else if (m_UILayer->m_PvpGauge[0] < m_UILayer->m_PvpGauge[1])
		{
			if (i == 0)
			{
				winstate = OutcomeType::LOSE;
			}
			else
			{
				winstate = OutcomeType::WIN;
			}
		}
		else
		{
			winstate = OutcomeType::DRAW;
		}
		endData->set_win_state((int)winstate);
	}

	req->set_is_game_over(m_isGameOver);

	ProtobufTaskManager::getInstance()->addRequest(req, [this](const char* buf, int len)
	{

		PvpEndGameResp resp;
		resp.ParseFromArray(buf, len);

		for (int i = 0; i < resp.end_game_resp_datas_size(); i++)
		{
			EndGameRespData endDta = resp.end_game_resp_datas(i);
			VOResult result(endDta.is_new_record(), endDta.perfect_count(), endDta.great_count(), endDta.cool_count(), endDta.good_count(), endDta.miss_count(), endDta.score(), endDta.high_score(), 0, endDta.rank(), endDta.clear_state(), endDta.win_state(), endDta.max_combo(), endDta.bonus_score(), endDta.harmony_score(),
				false, false, false, false, false, false, false, false, 0, 0);

			if (i == 0)
			{
				_option.setResultFor1P(result);
			}
			else
			{
				_option.setResultFor2P(result);
			}
		}
	});
    ProtobufTaskManager::getInstance()->addRequest(std::make_shared<UserInfoReq>());
	ProtobufTaskManager::getInstance()->execute([this]()
	{
		ScreenManager::getInstance()->pushScreen(PlayResultScreen::create(&this->_option));
	});

}

void PlayGame::initCountScene(int _index, float _tick)
{
	m_isCounting = true;
	m_iCountingCnt = _index;
	schedule(schedule_selector(PlayGame::CountingScene), _tick);
	
	if (PTR_GAMEMAIN->m_GameMode == PlayMode::SYNC_PLAY)
	{
		m_UILayer->setSyncControlVisible(true);
	}

	if (PTR_GAMEMAIN->m_GameMode == PlayMode::STAGE || PTR_GAMEMAIN->m_GameMode == PlayMode::PREMIUM)
	{
		m_UILayer->setLifeProgressBar(0);
	}

	/*
	if (m_isPause == true)
	{
		PopupManager::getInstance()->closeAll();
		setResume();
	}
	else
	{
		unschedule(schedule_selector(PlayGame::CountingScene));

#ifdef NEW_SYNC_PROC
		m_updataTick = 0;

		//main Schedule
		scheduleUpdate();
#else
		startGameNow();
#endif
		m_isCounting = false;
	}
	*/
}

void PlayGame::CountingScene(float pt)
{
	m_iCountingCnt++;
	m_UILayer->deleteCountingSprite();

	log(" CountingScene()    m_iCountingCnt=%d ", m_iCountingCnt);
	if (m_iCountingCnt < 5)
	{
		m_UILayer->initCountingSprite(m_iCountingCnt);


		if (m_isPause == false)
		{
#ifndef NEW_SYNC_PROC
	#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS )
	#else
				if (m_iCountingCnt == 1)
				{
					_bgmID = IZAudioEngine::getInstance()->playAudio(PTR_GAMEMAIN->m_filePath, 0.0f, false);

					if (PTR_GAMEMAIN->m_isVoice == true)
					{
						_voiceID = IZAudioEngine::getInstance()->playAudio(PTR_GAMEMAIN->m_filePath_Voice, 0.0f, false);
					}
				}
				else if (m_iCountingCnt == 4)
				{
					IZAudioEngine::getInstance()->pauseAudio(_bgmID);
					if (PTR_GAMEMAIN->m_isVoice == true)
					{
						IZAudioEngine::getInstance()->pauseAudio(_voiceID);
					}
				}
	#endif
#endif
		}

		//카운트 사운드
		/*
		if (m_iCountingCnt< 4)
		{
			IZAudioEngine::getInstance()->playAudio("res/audio/Count123.m4a");
		}
		else
		{
			IZAudioEngine::getInstance()->playAudio("res/audio/Count_S.m4a");
		}
		*/
	}
	else
	{
		if (PTR_GAMEMAIN->m_GameMode == PlayMode::SYNC_PLAY)
		{
			m_UILayer->setSyncControlVisible(true);
		}

		if (m_isPause == true)
		{
			PopupManager::getInstance()->closeAll();
			setResume();
		}
		else
		{
            unschedule(schedule_selector(PlayGame::CountingScene));

#ifdef NEW_SYNC_PROC
			m_updataTick = 0;

			//main Schedule
			scheduleUpdate();
#else
			startGameNow();
#endif

			if (PTR_GAMEMAIN->m_GameMode == PlayMode::STAGE)
			{
				if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_HINDRANCE].isHave == true)
					m_UILayer->ShieldOn();


				{
					int count = 0;
					for (int i = 0; i < 5; i++)
					{
						if (PTR_GAMEMAIN->m_Hindrance[i].type >= 0)
						{
							count++;
						}
					}
					if (count>0)
						m_UILayer->startDisturbanceIcon(1);
				}

				
				if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_HINDRANCE].isHave == false)
				{
					int Hindcount = 0;
					//즉시 발생하는 방해요소 시작
					for (int i = 0; i < 5; i++)
					{
						if (PTR_GAMEMAIN->m_Hindrance[i].condition == 1 && PTR_GAMEMAIN->m_Hindrance[i].type >= 0)
						{
							PTR_GAMEMAIN->m_isHindrance[PTR_GAMEMAIN->m_Hindrance[i].type] = true;

							m_NoteLayer->startHidrance(i, -1);
							Hindcount++;
						}
						else if (PTR_GAMEMAIN->m_Hindrance[i].condition == 2 && (PTR_GAMEMAIN->m_Hindrance[i].type == HINDRANCE_BIG_NOTE || PTR_GAMEMAIN->m_Hindrance[i].type == HINDRANCE_SMALL_NOTE))
						{
							float _scale = (PTR_GAMEMAIN->m_Hindrance[i].value[0] / 100.0f);
							for (int hi = 0; hi < 3; hi++)
							{
								if (PTR_GAMEMAIN->m_Highlight[hi].startPosition > 0)
									m_NoteLayer->settingNoteSize(true, hi, _scale);
							}
						}
					}

					if (Hindcount > 0)
					{
						//IZAudioEngine::getInstance()->playAudio("res/audio/Disturb.m4a");
					}
				}
			}
		}
		m_isCounting = false;
	}
}

void PlayGame::startGameNow()
{

#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS )

	//main Schedule
	scheduleUpdate();

    /*
	_bgmID = IZAudioEngine::getInstance()->playAudio(PTR_GAMEMAIN->m_filePath, m_Volume_bgm, false);
	if (PTR_GAMEMAIN->m_isVoice == true)
	{
		_voiceID = IZAudioEngine::getInstance()->playAudio(PTR_GAMEMAIN->m_filePath_Voice, 1.0f, false);
	}

	long long time = PTR_GAMEMAIN->getUMicroSecond();

	PTR_GAMEMAIN->m_MusicPlayTimeUM = time - (PTR_GAMEMAIN->m_PreViewStartTime * 1000);
	IZAudioEngine::getInstance()->setAudioCurrentTime(_bgmID, PTR_GAMEMAIN->m_PreViewStartTime);
	if (PTR_GAMEMAIN->m_isVoice == true)
	{
		IZAudioEngine::getInstance()->setAudioCurrentTime(_voiceID, PTR_GAMEMAIN->m_PreViewStartTime);
	}
     */
    IZAudioEngine::getInstance()->startMR();
#else
	/*
	long long time = PTR_GAMEMAIN->getUMicroSecond();


	PTR_GAMEMAIN->m_MusicPlayTimeUM = (time - (PTR_GAMEMAIN->m_PlayDelay * 1000) - (PTR_GAMEMAIN->m_PreViewStartTime * 1000));
	log("++++startGameNow()   PTR_GAMEMAIN->m_MySyncValue:%d ", PTR_GAMEMAIN->m_MySyncValue);

	//main Schedule
	scheduleUpdate();

	_bgmID = IZAudioEngine::getInstance()->playAudio(PTR_GAMEMAIN->m_filePath, m_Volume_bgm, false);
	if (PTR_GAMEMAIN->m_isVoice == true)
	{
		_voiceID = IZAudioEngine::getInstance()->playAudio(PTR_GAMEMAIN->m_filePath_Voice, m_Volume_bgm, false);
	}

	IZAudioEngine::getInstance()->setAudioCurrentTime(_bgmID, PTR_GAMEMAIN->m_PreViewStartTime);
	if (PTR_GAMEMAIN->m_isVoice == true)
	{
		IZAudioEngine::getInstance()->setAudioCurrentTime(_voiceID, PTR_GAMEMAIN->m_PreViewStartTime);
	}
	*/
	
	long long time = PTR_GAMEMAIN->getUMicroSecond();


	PTR_GAMEMAIN->m_MusicPlayTimeUM = (time - (PTR_GAMEMAIN->m_PlayDelay * 1000) - (PTR_GAMEMAIN->m_PreViewStartTime * 1000));
	log("++++startGameNow()   PTR_GAMEMAIN->m_MySyncValue:%d ", PTR_GAMEMAIN->m_MySyncValue);

	//main Schedule
	scheduleUpdate();

	IZAudioEngine::getInstance()->setAudioCurrentTime(_bgmID, PTR_GAMEMAIN->m_PreViewStartTime);
	IZAudioEngine::getInstance()->setAudioVolume(_bgmID, m_Volume_bgm);
	IZAudioEngine::getInstance()->resumeAudio(_bgmID);

	if (PTR_GAMEMAIN->m_isVoice == true)
	{
		IZAudioEngine::getInstance()->setAudioCurrentTime(_voiceID, PTR_GAMEMAIN->m_PreViewStartTime);
		IZAudioEngine::getInstance()->setAudioVolume(_voiceID, 1.0f);
		IZAudioEngine::getInstance()->resumeAudio(_voiceID);
	}

#endif
	if (PTR_GAMEMAIN->m_isRecording)
	{
		IZAudioEngine::getInstance()->startRecord("voice.m4a");
	}

	if (PTR_GAMEMAIN->m_isVideoRec)
	{
#ifdef VIDEO_RECORD
		//THCCommon::startVideoRecord();
#endif
	}
}


void PlayGame::startResume()
{
	if (m_isPause == true)
	{
		log("___________________________ startResume()");
        PopupManager::getInstance()->closeAll();
        
		//m_NoteLayer->setResumeNoteClear();

		initCountScene(0, 1.0f);
	}
}

void PlayGame::setResume()
{
	if (m_isPause == true)
	{
		log("___________________________ setResume()");

		unschedule(schedule_selector(PlayGame::CountingScene));



		PTR_GAMEMAIN->m_isStartGameTime = false;
		PTR_GAMEMAIN->m_isStartMRTime = false;
		m_updataTick = 0;
		//main Schedule
		scheduleUpdate();

#ifdef NEW_SYNC_PROC
		return;
#endif

		if (m_isRewind == true)
		{
			PTR_GAMEMAIN->m_NowMusicPlayTime -= (7000);
			if (PTR_GAMEMAIN->m_NowMusicPlayTime < 0)
				PTR_GAMEMAIN->m_NowMusicPlayTime = 0;
		}


		m_isPause = false;
#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS )
		scheduleUpdate();
        /*
		_bgmID = IZAudioEngine::getInstance()->playAudio(PTR_GAMEMAIN->m_filePath, m_Volume_bgm, false);
		if (PTR_GAMEMAIN->m_isVoice == true)
		{
			_voiceID = IZAudioEngine::getInstance()->playAudio(PTR_GAMEMAIN->m_filePath_Voice, 1.0f, false);
		}

		long long time = PTR_GAMEMAIN->getUMicroSecond();
		PTR_GAMEMAIN->m_MusicPlayTimeUM = time- (PTR_GAMEMAIN->m_NowMusicPlayTime*1000);
		IZAudioEngine::getInstance()->setAudioCurrentTime(_bgmID, PTR_GAMEMAIN->m_NowMusicPlayTime);
		if (PTR_GAMEMAIN->m_isVoice == true)
		{
			IZAudioEngine::getInstance()->setAudioCurrentTime(_voiceID, PTR_GAMEMAIN->m_NowMusicPlayTime);
		}
        */
        IZAudioEngine::getInstance()->resumeMR();
		
		PTR_GAMEMAIN->m_MusicPlayTimeUM += (10*1000);
#else
		long long time = PTR_GAMEMAIN->getUMicroSecond();


		PTR_GAMEMAIN->m_PauseButtonTime = time;

#ifdef NEW_SYNC_PROC
		PTR_GAMEMAIN->m_MusicPlayTimeUM = (time - ((PTR_GAMEMAIN->m_MySyncValue * 1000)/2) - (PTR_GAMEMAIN->m_NowMusicPlayTime * 1000));
#else
		PTR_GAMEMAIN->m_MusicPlayTimeUM = time - (PTR_GAMEMAIN->m_NowMusicPlayTime * 1000);
#endif
		//main Schedule
		scheduleUpdate();

		PTR_GAMEMAIN->m_MusicPlayTimeUM = time - (PTR_GAMEMAIN->m_ResumeDelay * 1000)-(PTR_GAMEMAIN->m_NowMusicPlayTime * 1000);

		//main Schedule
		scheduleUpdate();
        /*
		IZAudioEngine::getInstance()->setAudioCurrentTime(_bgmID, PTR_GAMEMAIN->m_NowMusicPlayTime);
		IZAudioEngine::getInstance()->setAudioVolume(_bgmID, m_Volume_bgm);
		IZAudioEngine::getInstance()->resumeAudio(_bgmID);

		if (PTR_GAMEMAIN->m_isVoice == true)
		{
			IZAudioEngine::getInstance()->setAudioCurrentTime(_voiceID, PTR_GAMEMAIN->m_NowMusicPlayTime);
			IZAudioEngine::getInstance()->setAudioVolume(_voiceID, 1.0f);
			IZAudioEngine::getInstance()->resumeAudio(_voiceID);
		}
        */
        IZAudioEngine::getInstance()->resumeMR();
		//PTR_GAMEMAIN->m_MusicPlayTimeUM -= (PTR_GAMEMAIN->m_ResumeDelay * 1000);
#endif


		if (PTR_GAMEMAIN->m_isRecording)
		{
			IZAudioEngine::getInstance()->resumeRecord();
		}


		if (m_FeverState > 0)
		{
			//m_feverTime[0] += (MOVE_PLAY_TIME / 1000.0f);

			if (m_feverTime[0] > m_feverTime[1])
				m_feverTime[0] = m_feverTime[1];

			m_feverStartTime = (PTR_GAMEMAIN->getUMicroSecond() / 1000);

			schedule(schedule_selector(PlayGame::FeverEnd), 0.1f);

			if (m_FeverState == 1)
				m_NoteLayer->pauseFeverEffect(false);
		}

		if (PTR_GAMEMAIN->m_GameMode == PlayMode::STAGE)
		{
			m_NoteLayer->pauseHidrance(false);
		}
	}
}

void PlayGame::openPlayPausePopup()
{
	if (PTR_GAMEMAIN->m_GameMode == PlayMode::PREVIEW_PLAY )
	{
		auto popup = PausePopup::create(3); //PreView Pause
		popup->setResume(CC_CALLBACK_2(PlayGame::onResumeEvent, this));
		popup->setExit(CC_CALLBACK_2(PlayGame::onExitEvent, this));
		PopupManager::getInstance()->open(popup);
	}
	else if (PTR_GAMEMAIN->m_GameMode == PlayMode::STAGE)
	{
		auto popup = PausePopup::create(4); //Stage Pause
		popup->setResume(CC_CALLBACK_2(PlayGame::onResumeEvent, this));
		popup->setExit(CC_CALLBACK_2(PlayGame::onExitEvent, this));
		PopupManager::getInstance()->open(popup);
	}
	else
	{
		auto popup = PausePopup::create(0); //base Pause
		popup->setRestart(CC_CALLBACK_2(PlayGame::onRestartEvent, this));
		popup->setResume(CC_CALLBACK_2(PlayGame::onResumeEvent, this));
		popup->setExit(CC_CALLBACK_2(PlayGame::onExitEvent, this));
		PopupManager::getInstance()->open(popup);
	}
}

void PlayGame::setPause()
{
	if (m_isPause == false )
	{ 
		if (PTR_GAMEMAIN->m_GameMode == PlayMode::SYNC_PLAY)
		{
			m_UILayer->setSyncControlVisible(false);
		}


		log("___________________________ setPause()");
		this->unscheduleUpdate();

		if (m_FeverState > 0)
		{
			long long time = (PTR_GAMEMAIN->getUMicroSecond() / 1000);
			unschedule(schedule_selector(PlayGame::FeverEnd));
		}

		//현재 플레이 시간 저장
		PTR_GAMEMAIN->m_NowMusicPlayTime = (PTR_GAMEMAIN->getUMicroSecond() - PTR_GAMEMAIN->m_MusicPlayTimeUM) / 1000;

		
#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS )	
        /*
		IZAudioEngine::getInstance()->stopAudio(_bgmID);
		if (PTR_GAMEMAIN->m_isVoice == true)
		{
			IZAudioEngine::getInstance()->stopAudio(_voiceID);
		}
        */
        IZAudioEngine::getInstance()->stopMR();
#else
	#ifdef NEW_SYNC_PROC
			IZAudioEngine::getInstance()->stopAudio(_bgmID);
			if (PTR_GAMEMAIN->m_isVoice == true)
			{
				IZAudioEngine::getInstance()->stopAudio(_voiceID);
			}
	#else
			IZAudioEngine::getInstance()->pauseAudio(_bgmID);
			if (PTR_GAMEMAIN->m_isVoice == true)
			{
				IZAudioEngine::getInstance()->pauseAudio(_voiceID);
			}
	#endif
#endif

		IZAudioEngine::getInstance()->stopAllEffects();
   
		if (PTR_GAMEMAIN->m_isRecording == false)
		{
			/*
			int time = PTR_GAMEMAIN->m_NowMusicPlayTime / 1000;

			PTR_GAMEMAIN->m_NowMusicPlayTime = (time * 1000);
		
			//PTR_GAMEMAIN->m_NowMusicPlayTime -= (MOVE_PLAY_TIME);

			if (PTR_GAMEMAIN->m_NowMusicPlayTime < 0)
				PTR_GAMEMAIN->m_NowMusicPlayTime = 0;
				*/

			log("+++++++++++setPause()    PTR_GAMEMAIN->m_NowMusicPlayTime : %lld ", PTR_GAMEMAIN->m_NowMusicPlayTime);
		}
        
		if (PTR_GAMEMAIN->m_isRecording)
		{
			IZAudioEngine::getInstance()->pauseRecord();
		}
		

		//초기화
		{
			m_NoteLayer->deleteEffectObject();
		}



		if (m_FeverState == 1)
		{
			m_NoteLayer->pauseFeverEffect(true);
		}

		m_isPause = true;


		if (PTR_GAMEMAIN->m_GameMode == PlayMode::STAGE)
		{
			m_NoteLayer->pauseHidrance(true);
		}

	}
}



void PlayGame::GotoResult()
{
	auto _layer = (Layer*)this->getChildByTag(TAG_GAME_NOTE_LAYER);
	auto _noteLayer = (PlayGame_NoteView*)_layer->getChildByTag(1);
    if (PTR_GAMEMAIN->m_isRecording)
    {
		IZAudioEngine::getInstance()->stopRecord();
    }
    /*
	IZAudioEngine::getInstance()->stopAudio(_bgmID);

	if (PTR_GAMEMAIN->m_isVoice == true)
	{
		IZAudioEngine::getInstance()->stopAudio(_voiceID);
	}
     */
    IZAudioEngine::getInstance()->stopMR();
    this->requestEndGame();
}

void PlayGame::touchesBegan(const std::vector<cocos2d::Touch*>& pTouches, cocos2d::Event* pEvnet)
{
	for (auto &touch : pTouches)
	{
		auto _location = touch->getLocation();

		
		if (PTR_GAMEMAIN->m_GameMode == PlayMode::PREMIUM || PTR_GAMEMAIN->m_GameMode == PlayMode::STAGE)
		{
			if ((_location.x >= 0 && _location.x <= 200) &&
				(_location.y >= PTR_GAMEMAIN->m_WinSize.height - 200 && _location.y <= PTR_GAMEMAIN->m_WinSize.height))
			{
				if (m_isPause == false)
				{
					if (m_NotGameOver)
					{
						m_NotGameOver = false;
						m_UILayer->setTestInfo(0);
					}
					else
					{
						m_NotGameOver = true;
						m_UILayer->setTestInfo(1);
					}
				}
			}
			else if ((_location.x >= PTR_GAMEMAIN->m_WinSize.width - 200 && _location.x <= PTR_GAMEMAIN->m_WinSize.width) &&
				(_location.y >= PTR_GAMEMAIN->m_WinSize.height - 300 && _location.y <= PTR_GAMEMAIN->m_WinSize.height - 100))
			{
				if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_AUTOPLAY].isHave == false)
				{
					if (m_isPause == false)
					{
						if (m_TestAutoPaly)
						{
							m_TestAutoPaly = false;
							m_UILayer->setTestInfo(0);
						}
						else
						{
							m_TestAutoPaly = true;
							m_UILayer->setTestInfo(1);
						}
					}
				}
			}
		}
	}
}

void PlayGame::touchesMove(const std::vector<cocos2d::Touch*>& pTouches, cocos2d::Event* pEvnet)
{
	for (auto &touch : pTouches)
	{
		auto _location = touch->getLocation();
	}
}
void PlayGame::touchesEnd(const std::vector<cocos2d::Touch*>& pTouches, cocos2d::Event* pEvnet)
{
	for (auto &touch : pTouches)
	{
		auto _location = touch->getLocation();
	}
}



void PlayGame::setScore(NoteObject * note, int type)
{
	if (m_isGameEnd == true)
		return;

	if (PTR_GAMEMAIN->m_isPreViewPlay == true)
		return;

//	float speed = m_NoteLayer->getDubbleSpeedValue();



	float playTime = m_NoteLayer->m_pTimeUpdate;


	if (m_NoteLayer->isChordNote(note) == true)
	{
		if (note->m_childSize[0] <= 0)
		{
			//판정 안해준다
			return;
		}
	}

	if (type == NOTE_MISS)
	{
		if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_MISS].isHave == true)
		{
			if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_MISS].data > 0)
			{
				type = NOTE_PERFECT;
				PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_MISS].data -= 1;
				m_UILayer->setItemMissCount(PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_MISS].data);
			}
		}

		PTR_GAMEMAIN->m_judgeNoteData[0].judgeType[note->m_idx] = type;
		PTR_GAMEMAIN->m_judgeNoteData[0].judgeRate[note->m_idx] = 0;

		if (m_FeverState > 0)
			PTR_GAMEMAIN->m_judgeNoteData[0].fever[note->m_idx] = true;
		else
			PTR_GAMEMAIN->m_judgeNoteData[0].fever[note->m_idx] = false;
	}
	else
	{
		type = getJudgeType(note->m_time, m_NoteLayer->m_pTimeUpdate, 0, note->m_idx);

		if (m_TestAutoPaly == true)
		{
			type = NOTE_PERFECT;
		}



		if (m_NoteLayer->isMuteNote(note) == true)
		{
			type = NOTE_PERFECT;
		}


		/*
		//드럼효과
		if (PTR_GAMEMAIN->m_GameInstrumType == (int)InstType::DRUM)
		{
			if (note->m_NoteImageProp > 1)
			{
				m_BGLayer->setDrumEffect(note->m_NoteImageProp);
			}
		}
		*/

	}

	if (PTR_GAMEMAIN->m_GameMode != PlayMode::PVP)
	{
		m_UILayer->setJudgeMentText(type, note->m_idx, m_FeverState, -1, 0);
	}


	if (PTR_GAMEMAIN->m_GameMode == PlayMode::SYNC_PLAY )
	{
		return;
	}


	//int baseScore = 100000 / PTR_GAMEMAIN->m_JudgeNoteCount[0];
	
	float baseFeverVal = 0.0f;
	float baseFeverTime = 0.0f;

	

	int tmp = PTR_GAMEMAIN->m_JudgeScore[type];

	//점수계산

	int addScoreVal = 0;
	if (PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_SCORE].isHave == true)
	{
		addScoreVal = PTR_GAMEMAIN->m_ItemInfo[GAME_ITEM_SCORE].data;
	}

	if (type != NOTE_MISS)
	{
		if (m_FeverState > 0)
			m_iScore[0] += (PTR_GAMEMAIN->m_JudgeScoreFever[type] + (PTR_GAMEMAIN->m_JudgeScoreFever[type] * (float)(addScoreVal * 0.01f)));
		else
			m_iScore[0] += (PTR_GAMEMAIN->m_JudgeScore[type] + (PTR_GAMEMAIN->m_JudgeScore[type] * (float)(addScoreVal * 0.01f)));
	}

	switch (type)
	{
	case NOTE_PERFECT:
		baseFeverVal = (float)(m_NoteLayer->getBPM()/2.0f) / (float)(PTR_GAMEMAIN->m_JudgeNoteCount[0]/2.25f);
		baseFeverTime = ((float)(m_NoteLayer->getBPM() * 2.0f) / PTR_GAMEMAIN->m_JudgeNoteCount[0]) / 3.9f;
		break;
	case NOTE_GREAT:
		baseFeverVal = (float)(m_NoteLayer->getBPM() / 2.0f) / (float)(PTR_GAMEMAIN->m_JudgeNoteCount[0] / 1.7f);
		baseFeverTime = ((m_NoteLayer->getBPM() * 2.0f) / PTR_GAMEMAIN->m_JudgeNoteCount[0]) / 5.1f;
		break;
	case NOTE_COOL:
		baseFeverVal = (float)(m_NoteLayer->getBPM() / 2.0f) / (float)(PTR_GAMEMAIN->m_JudgeNoteCount[0] / 1.4f);
		baseFeverTime = ((m_NoteLayer->getBPM() * 2.0f) / PTR_GAMEMAIN->m_JudgeNoteCount[0]) / 6.6f;
		break;
	case NOTE_GOOD:
		baseFeverVal = (float)(m_NoteLayer->getBPM() / 2.0f) / (float)(PTR_GAMEMAIN->m_JudgeNoteCount[0] / 1.05f);
		baseFeverTime = ((m_NoteLayer->getBPM() * 2.0f) / PTR_GAMEMAIN->m_JudgeNoteCount[0]) / 9.0f;
		break;
	case NOTE_MISS:
		float  totlaFeverTime = (600.f / (float)m_NoteLayer->getBPM()) + (float)(m_UILayer->m_time*0.01f);
		baseFeverTime = -(totlaFeverTime/2.3f);

		float feverGauge = (600.f / (float)m_NoteLayer->getBPM()) + (float)(m_UILayer->m_time*0.01f);
		baseFeverVal = -(feverGauge*0.005f);
		break;
	}


	if (note->m_noteType == NOTE_TYPE_NORMAL_LONG || note->m_noteType == NOTE_TYPE_DRAG_LONG)
	{
		m_baseFeverVal = baseFeverVal;
		m_baseFeverTime = baseFeverTime;
	}

	m_UILayer->setPlayScore(m_iScore[0], 0);

	if (type <= NOTE_GOOD)
	{
		setComboCnt(m_NoteLayer->isHiddenNote(note));
	}
	else
	{
		PTR_GAMEMAIN->m_ComboCnt[0] = 0;
	}

	if(m_FeverState > 0) //피버일때
	{
		if (type == NOTE_PERFECT || type == NOTE_GREAT)
		{
			PTR_GAMEMAIN->m_JudgeCount[0][NOTE_PERFECT] += 1;
			PTR_GAMEMAIN->m_JudgeCountFever[0][NOTE_PERFECT] += 1;
		}
		else if (type == NOTE_COOL)
		{
			PTR_GAMEMAIN->m_JudgeCount[0][NOTE_GREAT] += 1;
			PTR_GAMEMAIN->m_JudgeCountFever[0][NOTE_GREAT] += 1;
		}
		else
		{
			PTR_GAMEMAIN->m_JudgeCount[0][type] += 1;
			PTR_GAMEMAIN->m_JudgeCountFever[0][type] += 1;
		}
	}
	else
	{
		PTR_GAMEMAIN->m_JudgeCount[0][type] += 1;
	}

	float tmpGauge[5] = { 1.2f, 0.4f, 0.2f, 0.1f, -3.6f };
	float tmpFeverGauge[5] = { 1.6f, 0.7f, 0.4f, 0.2f, -3.6f };

	if (_option.getPlayMode() == PlayMode::PREMIUM || _option.getPlayMode() == PlayMode::STAGE)
	{
		if (m_NotGameOver == false)
		{
			if (m_FeverState > 0)
			{
				m_UILayer->setLifeProgressBar(tmpFeverGauge[type]);
			}
			else
			{
				m_UILayer->setLifeProgressBar(tmpGauge[type]);
			}
	
			if (m_TestFreePaly == false)
			{
				if (m_UILayer->m_LifeGauge <= 0) //GAME OVER
				{
					m_isGameEnd = true;
					m_isGameOver = true;
					//GotoResult();
					{
						this->unscheduleUpdate();

						if (m_FeverState > 0)
						{
							unschedule(schedule_selector(PlayGame::FeverEnd));
						}
                        /*
						IZAudioEngine::getInstance()->stopAudio(_bgmID);

						if (PTR_GAMEMAIN->m_isVoice == true)
						{
							IZAudioEngine::getInstance()->stopAudio(_voiceID);
						}
                         */
                        IZAudioEngine::getInstance()->stopMR();

						IZAudioEngine::getInstance()->playAudio("res/audio/Gameover.mp3", 1.0f, false);


						if (PTR_GAMEMAIN->m_GameMode == PlayMode::STAGE)
						{
							auto popup = PausePopup::create(5);
							popup->setExit(CC_CALLBACK_2(PlayGame::onExitEvent, this));
							PopupManager::getInstance()->open(popup);
						}
						else
						{
							auto popup = PausePopup::create(2);
							popup->setRestart(CC_CALLBACK_2(PlayGame::onRestartEvent, this));
							popup->setExit(CC_CALLBACK_2(PlayGame::onExitEvent, this));
							PopupManager::getInstance()->open(popup);
						}
					}
					return;
				}
			}
		}
		else
		{
			if (type != NOTE_MISS)
			{
				if (m_FeverState > 0)
				{
					m_UILayer->setLifeProgressBar(tmpFeverGauge[type]);
				}
				else
				{
					m_UILayer->setLifeProgressBar(tmpGauge[type]);
				}
			}
		}
	}
	 
	//피버 데이타수치 적용
	addFeverData(baseFeverVal, baseFeverTime);
	


	//PVP판정 하기
	if (PTR_GAMEMAIN->m_GameMode == PlayMode::PVP || PTR_GAMEMAIN->m_GameMode == PlayMode::CHALLENGE)
	{
		if (m_NoteLayer->isMuteNote(note) == true)
			return;

		int pvpType = NOTE_MISS;

		int time = _option.getNoteRecord().getTime(note->m_idx);
	
		pvpType = getJudgeType(note->m_time,  time, 1, note->m_idx);
		
		PTR_GAMEMAIN->m_JudgeCount[1][pvpType] += 1;

		if (pvpType != NOTE_MISS)
		{
			if (note->m_noteType == NOTE_TYPE_NORMAL_LONG || note->m_noteType == NOTE_TYPE_DRAG_LONG)
			{
				PTR_GAMEMAIN->m_JudgeCount[1][pvpType] += 1;
			}
		}
		
		
		//PVP콤보 카운트 보여주기
		m_UILayer->setPvpCombo(type, pvpType);

		if (_option.getNoteRecord().getFever(note->m_idx) != false)
			m_iScore[1] += PTR_GAMEMAIN->m_JudgeScoreFever[pvpType];
		else
			m_iScore[1] += PTR_GAMEMAIN->m_JudgeScore[pvpType];

		m_UILayer->setPlayScore(m_iScore[1], 1);

		float tmpValus = 0;
		float scaleSize = 0.0f;
        

		int firstRate = PTR_GAMEMAIN->m_judgeNoteData[0].judgeRate[note->m_idx];
		int secondRate = PTR_GAMEMAIN->m_judgeNoteData[1].judgeRate[note->m_idx];
        
        int firstScore = PTR_GAMEMAIN->m_JudgeScore[type]+(PTR_GAMEMAIN->m_judgeNoteData[0].judgeRate[note->m_idx]/20);
        int secondScore = PTR_GAMEMAIN->m_JudgeScore[pvpType]+(PTR_GAMEMAIN->m_judgeNoteData[1].judgeRate[note->m_idx]/20);

		//결과
		if (type == pvpType) //비겼을때 같은 등급에 파정비율로 처리
		{
			if (type == NOTE_MISS)
			{
				m_UILayer->setJudgeMentText(type, note->m_idx, m_FeverState, 1, 0);
				m_UILayer->setJudgeMentText(pvpType, note->m_idx, m_FeverState, 1, 1);
			}
			else
			{
				if (firstRate > secondRate)
				{
					m_UILayer->setJudgeMentText(type, note->m_idx, m_FeverState, 0, 0);
					m_UILayer->setJudgeMentText(pvpType, note->m_idx, m_FeverState, 2, 1);

					tmpValus = firstScore - secondScore;
					m_UILayer->setPVP_Gauge(0, tmpValus);
					m_UILayer->setPVP_Gauge(1, -(tmpValus));					
				}
				else if (firstRate < secondRate)
				{
					m_UILayer->setJudgeMentText(type, note->m_idx, m_FeverState, 2, 0);
					m_UILayer->setJudgeMentText(pvpType, note->m_idx, m_FeverState, 0, 1);


					tmpValus = secondScore - firstScore;
					m_UILayer->setPVP_Gauge(0, -(tmpValus));
					m_UILayer->setPVP_Gauge(1, tmpValus);
				}
				else
				{
					m_UILayer->setJudgeMentText(type, note->m_idx, m_FeverState, 1, 0);
					m_UILayer->setJudgeMentText(pvpType, note->m_idx, m_FeverState, 1, 1);
				}
			}
		}
		else if (type < pvpType)
		{
			m_UILayer->setJudgeMentText(type, note->m_idx, m_FeverState, 0, 0);
			m_UILayer->setJudgeMentText(pvpType, note->m_idx, m_FeverState, 2, 1);


            tmpValus = firstScore - secondScore;
			m_UILayer->setPVP_Gauge(0, tmpValus);
			m_UILayer->setPVP_Gauge(1, -(tmpValus));
		}
		else if (type > pvpType)
		{
			m_UILayer->setJudgeMentText(type, note->m_idx, m_FeverState, 2, 0);
			m_UILayer->setJudgeMentText(pvpType, note->m_idx, m_FeverState, 0, 1);

            tmpValus = secondScore - firstScore;
			m_UILayer->setPVP_Gauge(0, -(tmpValus));
			m_UILayer->setPVP_Gauge(1, tmpValus);
		}

		//log("judge:%d/%d  rate:%d/%d", type, pvpType, firstRate, secondRate);
	}



	if (PTR_GAMEMAIN->m_GameMode == PlayMode::STAGE)
	{
		m_UILayer->setSatageClearCondition(false, m_iScore[0], PTR_GAMEMAIN->m_ComboCnt[1], (m_feverTotalTime / 1000), (PTR_GAMEMAIN->m_JudgeCount[0][NOTE_MISS]), PTR_GAMEMAIN->m_JudgeCount[0][NOTE_PERFECT]);

		/*
		if (UserDefault::getInstance()->getBoolForKey("tutorial_stage_ingame_pause", false)==false)
		{
			if (type == NOTE_MISS)
			{
				if (_option.getStageID() < 4)
				{
					setPause();

					auto popup = PausePopup::create(6); //PreView Pause
					popup->setResume(CC_CALLBACK_2(PlayGame::onResumeEvent, this));
					popup->setExit(CC_CALLBACK_2(PlayGame::onExitEvent, this));
					PopupManager::getInstance()->open(popup);

					UserDefault::getInstance()->setBoolForKey("tutorial_stage_ingame_pause", true);
					UserDefault::getInstance()->flush();
				}
			}
		}
		*/
	}


#ifdef TEST_GAME_INFO
	m_UILayer->setMusicInfo(PTR_GAMEMAIN->m_JudgeCount[0][NOTE_PERFECT], PTR_GAMEMAIN->m_JudgeCount[0][NOTE_MISS], (m_feverTotalTime / 1000));
#endif

}


void PlayGame::addFeverData(float feverGauge, float feverTime)
{
	m_feverGauge[1] = (600.f / (float)m_NoteLayer->getBPM()) + (float)(m_UILayer->m_time*0.01f);
	if (m_FeverState == 0)
	{
		m_feverGauge[0] += feverGauge;

		if (m_feverGauge[0] < 0)
			m_feverGauge[0] = 0;

		float tmpVal = ((float)(m_feverGauge[0] * 100.0f) / m_feverGauge[1]);
		m_UILayer->setFeverProgressBar(m_FeverState, tmpVal, 100);

		if (m_feverGauge[0] >= m_feverGauge[1])
		{
			//IZAudioEngine::getInstance()->playAudio("res/audio/Fever.m4a");
			m_FeverState = 1;
			m_feverGauge[0] = 0.0f;
			m_feverTime[0] = m_feverTime[1] = (600 / m_NoteLayer->getBPM()) + (m_UILayer->m_time*0.01f);

			m_UILayer->startFeverGaugeEffect();
			m_NoteLayer->startFeverEffect(m_FeverState);
			m_feverStartTime = (PTR_GAMEMAIN->getUMicroSecond() / 1000);
			schedule(schedule_selector(PlayGame::FeverEnd), 0.1f);
		}
	}
	else
	{
		m_feverGauge[0] = 0.0f;
		m_feverTime[0] += feverTime;
		if (m_feverTime[0] > m_feverTime[1])
		{
			m_feverTime[0] = m_feverTime[1];
		}
	}
}


void PlayGame::addLongNoteFever()
{

	if (PTR_GAMEMAIN->m_isPreViewPlay == true)
		return;


	if (m_FeverState == 0) 
	{
		addFeverData(m_baseFeverVal/ 8.0f, 0);

		//m_iScore[0] += PTR_GAMEMAIN->m_JudgeScore[4];
		//m_UILayer->setPlayScore(m_iScore[0], 0);
		PTR_GAMEMAIN->m_JudgeCount[0][NOTE_LONG_TAIL] += 1;
	}
	else
	{
		addFeverData(0, m_baseFeverTime/16.0f);

		//m_iScore[0] += PTR_GAMEMAIN->m_JudgeScoreFever[4];
		//m_UILayer->setPlayScore(m_iScore[0], 0);

		PTR_GAMEMAIN->m_JudgeCount[0][NOTE_LONG_TAIL] += 1;
		PTR_GAMEMAIN->m_JudgeCountFever[0][NOTE_LONG_TAIL] += 1;
	}
}

int PlayGame::getJudgeType(float noteTime, float playTime, int playNum, int idx)
{
	float judgeValue = 0.0f;
	int type = NOTE_MISS;
    int addPoint = 0;

	if (m_FeverState > 0)
	{
		judgeValue = 20.0f;
	}
    
    
    float time = 100.0f;
    
    if (noteTime <= 0)
    {
        type = NOTE_MISS;
    }
    else if (playTime >= (noteTime - PTR_GAMEMAIN->m_JudgeRange[NOTE_PERFECT]) && playTime <= (noteTime + PTR_GAMEMAIN->m_JudgeRange[NOTE_PERFECT]))
    {
        type = NOTE_PERFECT;
        
        time = abs(noteTime - playTime);
    }
    else if (playTime >= (noteTime - PTR_GAMEMAIN->m_JudgeRange[NOTE_GREAT]) && playTime <= (noteTime + PTR_GAMEMAIN->m_JudgeRange[NOTE_GREAT]))
    {
        type = NOTE_GREAT;
    }
    else if (playTime >= (noteTime - (PTR_GAMEMAIN->m_JudgeRange[NOTE_COOL] + judgeValue)) && playTime <= (noteTime + (PTR_GAMEMAIN->m_JudgeRange[NOTE_COOL] + judgeValue)))
    {
        type = NOTE_COOL;
    }
    else
    {
        type = NOTE_GOOD;
    }
    
    
    if( time < 10 )
        addPoint  = 5;
    else if( time < 20 )
        addPoint  = 4;
    else if( time < 30 )
        addPoint  = 3;
    else if( time < 40 )
        addPoint  = 2;
    else if( time < 50 )
        addPoint  = 1;
    else
        addPoint  = 0;
    
    PTR_GAMEMAIN->m_judgeNoteData[playNum].judgeType[idx] = type;
    PTR_GAMEMAIN->m_judgeNoteData[playNum].judgeRate[idx] = (addPoint*20);
    
    if (m_FeverState > 0)
        PTR_GAMEMAIN->m_judgeNoteData[playNum].fever[idx] = true;
    else
        PTR_GAMEMAIN->m_judgeNoteData[playNum].fever[idx] = false;
    
    //log("++++++ getJudgeType() type:%d addPoint = %d", type, addPoint);

    
    /*
	if (noteTime <= 0)
	{
		type = NOTE_MISS;
	}
	else if (noteTime >= (playTime - PTR_GAMEMAIN->m_JudgeRange[NOTE_PERFECT]) && noteTime <= (playTime + PTR_GAMEMAIN->m_JudgeRange[NOTE_PERFECT]))
	{
		type = NOTE_PERFECT;
	}
	else if (noteTime >= (playTime - PTR_GAMEMAIN->m_JudgeRange[NOTE_GREAT]) && noteTime <= (playTime + PTR_GAMEMAIN->m_JudgeRange[NOTE_GREAT]))
	{
		type = NOTE_GREAT;
	}
	else if (noteTime >= (playTime - (PTR_GAMEMAIN->m_JudgeRange[NOTE_COOL] + judgeValue)) && noteTime <= (playTime + (PTR_GAMEMAIN->m_JudgeRange[NOTE_COOL] + judgeValue)))
	{
		type = NOTE_COOL;
	}
	else
	{
		type = NOTE_GOOD;
	}


	int timeJudge = abs(noteTime - (playTime - (PTR_GAMEMAIN->m_JudgeRange[type])));
	int tempVal = ((timeJudge / (PTR_GAMEMAIN->m_JudgeRange[type])) * 100);

	if (type == NOTE_COOL || type == NOTE_GOOD)
	{
		timeJudge = abs(noteTime - (playTime - (PTR_GAMEMAIN->m_JudgeRange[type] + judgeValue)));
		tempVal = ((timeJudge / (PTR_GAMEMAIN->m_JudgeRange[type] + judgeValue)) * 100);
	}

	if (tempVal > 100)
		tempVal = 100;

	timeJudge = 100 - tempVal;

	PTR_GAMEMAIN->m_judgeNoteData[playNum].judgeType[idx] = type;
	PTR_GAMEMAIN->m_judgeNoteData[playNum].judgeRate[idx] = tempVal;

	if (m_FeverState > 0)
		PTR_GAMEMAIN->m_judgeNoteData[playNum].fever[idx] = true;
	else
		PTR_GAMEMAIN->m_judgeNoteData[playNum].fever[idx] = false;


     */
	return type;
}


void PlayGame::FeverEnd(float f)
{
	long long time = (PTR_GAMEMAIN->getUMicroSecond() / 1000);
	float ft = ((float)(time - m_feverStartTime) / 1000.0f);
	m_feverTime[0] -= ft;


	m_feverTotalTime += (time - m_feverStartTime);

	m_feverStartTime = time;

	if (m_feverTime[0] <= 0.0f)
	{
		m_FeverState = 0;
		m_feverTime[0] = 0.0f;
		m_NoteLayer->endFeverEffect();

		m_UILayer->setFeverProgressBar(m_FeverState, 0, 100);
		m_UILayer->endFeverGaugeEffect();
		
		unschedule(schedule_selector(PlayGame::FeverEnd));
		log("+++++++++++++ FEVER END!!! m_feverTotalTime = %d", m_feverTotalTime);
	}
	else
	{
		float tmpVal = ((float)(m_feverTime[0] * 100.0f) / m_feverTime[1]);
		m_UILayer->setFeverProgressBar(m_FeverState, 0, tmpVal);
	}
}

void PlayGame::setComboCnt(bool isHidden)
{	
	if (isHidden)
	{
		PTR_GAMEMAIN->m_HiddenSuccessCnt++;
	}
	else
	{	
		if (PTR_GAMEMAIN->m_ComboCnt[0] >= 1 && PTR_GAMEMAIN->m_ComboCnt[0] < 9999)
		{
			PTR_GAMEMAIN->m_ComboCnt[0]++;


			if (PTR_GAMEMAIN->m_ComboCnt[0] > PTR_GAMEMAIN->m_ComboCnt[1])
			{
				PTR_GAMEMAIN->m_ComboCnt[1] = PTR_GAMEMAIN->m_ComboCnt[0];

				m_UILayer->setMaxCombo(PTR_GAMEMAIN->m_ComboCnt[1]);
			}
		}
		else
		{
			PTR_GAMEMAIN->m_ComboCnt[0]++;
		}

		if (PTR_GAMEMAIN->m_ComboCnt[0] >= 2)
		{
			m_UILayer->setPlayCombo();
		}
		else if (PTR_GAMEMAIN->m_ComboCnt[0] == 0)
		{
			m_UILayer->HidePlayCombo();
		}
	}
}

/*
void PlayGame::onRewindEvent(Ref* sender)
{
    m_isRewind = true;
    getOptionData();
    startResume();
    PopupManager::getInstance()->closeAll();
}
*/

void PlayGame::onRestartEvent(Ref* sender, Widget::TouchEventType type)
{
	if (type == Widget::TouchEventType::ENDED)
	{
		//IZAudioEngine::getInstance()->playAudio("res/audio/Button_b.m4a");
		PopupManager::getInstance()->closeAll();

		if (PTR_GAMEMAIN->m_GameMode == PlayMode::SYNC_PLAY)
		{
			ScreenManager::getInstance()->replaceScreen(HomeScreen::create());
			return;
		}


		if (PTR_GAMEMAIN->m_isRecording)
		{
			IZAudioEngine::getInstance()->stopRecord();
		}

		this->unscheduleUpdate();

		if (m_isGameEnd == false)
		{
            /*
			IZAudioEngine::getInstance()->stopAudio(_bgmID);
			if (PTR_GAMEMAIN->m_isVoice == true)
			{
				IZAudioEngine::getInstance()->stopAudio(_voiceID);
			}
             */
            IZAudioEngine::getInstance()->stopMR();
		}

		if (PTR_GAMEMAIN->m_GameMode == PlayMode::STAGE)
		{
			ScreenManager::getInstance()->replaceScreen(StageListScreen::create());
			
		}
		else
		{
			ScreenManager::getInstance()->replaceScreen(MusicListScreen::create(_option.getCategoryID(), _option.getMusicID(), (int)_option.getPlayMode(), _option.get1Player().getInstID(), _option.get1Player().getDifficult()));
		}


		// 바로 재시작으로
		/*
		if (m_FeverState > 0)
		{
		m_FeverState = 0;
		m_NoteLayer->endFeverEffect();
		unschedule(schedule_selector(PlayGame::FeverEnd));
		}

		this->unscheduleUpdate();
		GameReStart();
		*/
	}
}

void PlayGame::GameReStartButton()
{
	this->unscheduleUpdate();

#if(CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    /*
	IZAudioEngine::getInstance()->stopAudio(_bgmID);
    if (PTR_GAMEMAIN->m_isVoice == true)
	{
		IZAudioEngine::getInstance()->stopAudio(_voiceID);
	}
     */
    IZAudioEngine::getInstance()->stopMR();
#else
	IZAudioEngine::getInstance()->stopAudio(_bgmID);
	if (PTR_GAMEMAIN->m_isVoice == true)
	{
		IZAudioEngine::getInstance()->stopAudio(_voiceID);
	}
#endif

	m_isGameEnd = false;
	m_NoteLayer->m_pTimeUpdate = 0;
	GameReStart();
}

void PlayGame::GameReStart()
{
	m_isReStart = true;

	m_isPause = false;
	PTR_GAMEMAIN->m_NowMusicPlayTime = 0;
	
	m_NoteLayer->setPreViewNote();

	PTR_GAMEMAIN->m_PreViewStartTime = 0;
	PTR_GAMEMAIN->m_MusicPlayTimeUM = 0;


	m_NoteLayer->m_pTimeUpdate = 0.0f;
	m_NoteLayer->m_pTimeUpdateBefore = 0.0f;

	
	m_FeverState = 0;
	m_feverGauge[0] = m_feverTime[0] = 0;  //0:cur 1:max
	m_feverGauge[1] = m_feverTime[1] = 100;

	m_FeverState = 0;
	m_UILayer->setFeverProgressBar(0, 0, 100);
	m_UILayer->setPlayProgressBar(0);
		
	m_isRewind = false;
	PTR_GAMEMAIN->m_PreViewStartTime = 0;

	m_UILayer->DrawLyrics(0);


	getOptionData();

	//임시 게임데이타 초기화
	{
		m_UILayer->m_LifeGauge = 100;
		m_iScore[0] = 0;
		PTR_GAMEMAIN->m_ComboCnt[0] = PTR_GAMEMAIN->m_ComboCnt[1] = 0;

		for (int k = 0; k < 2; k++)
		{
			for (int i = 0; i < 6; i++)
			{
				PTR_GAMEMAIN->m_JudgeCount[k][i] = 0;
				PTR_GAMEMAIN->m_JudgeCountFever[k][i] = 0;
			}
		}

		m_UILayer->setPlayScore(m_iScore[0], 0);
		m_UILayer->setMaxCombo(PTR_GAMEMAIN->m_ComboCnt[1]);
	}
	
#ifdef NEW_SYNC_PROC
    PTR_GAMEMAIN->m_isStartGameTime = false;
    PTR_GAMEMAIN->m_isStartMRTime = false;
    //PTR_GAMEMAIN->m_StartGameTime = PTR_GAMEMAIN->getUMicroSecond();
    m_updataTick = 0;
    //main Schedule
    scheduleUpdate();
#else
	//initCountScene(0, 1.0f);
	startGameNow();
#endif


	

}


void PlayGame::onResumeEvent(Ref* sender, Widget::TouchEventType type)
{
	if (type == Widget::TouchEventType::ENDED)
	{
		//IZAudioEngine::getInstance()->playAudio("res/audio/Button_b.m4a");

		getOptionData();
		startResume();
		PopupManager::getInstance()->closeAll();
	}
}

void PlayGame::onExitEvent(Ref* sender, Widget::TouchEventType type)
{
	if (type == Widget::TouchEventType::ENDED)
	{
		//IZAudioEngine::getInstance()->playAudio("res/audio/Button_b.m4a");

		m_isGameOver = true;

		PopupManager::getInstance()->closeAll();

        /*
		IZAudioEngine::getInstance()->stopAudio(_bgmID);

		if (PTR_GAMEMAIN->m_isVoice == true)
		{
			IZAudioEngine::getInstance()->stopAudio(_voiceID);
		}
         */
        IZAudioEngine::getInstance()->stopMR();

		if (PTR_GAMEMAIN->m_GameMode == PlayMode::SYNC_PLAY)
		{
			ScreenManager::getInstance()->replaceScreen(HomeScreen::create());
			return;
		}
		else if (PTR_GAMEMAIN->m_GameMode == PlayMode::STAGE)
		{
			ScreenManager::getInstance()->replaceScreen(StageListScreen::create());
		}
		else
		{
			ScreenManager::getInstance()->popScreen();
			//ScreenManager::getInstance()->replaceScreen(MusicListScreen::create(_option.getCategoryID(), _option.getMusicID(), (int)_option.getPlayMode(), _option.get1Player().getInstID(), _option.get1Player().getDifficult()));
		}

		//GotoResult();
	}
}


void PlayGame::getOptionData()
{
    VOPlayConfig& playConfig = _option.getPlayConfig();
    float tmp = SettingData::getInstance()->m_VoiceBgm; // playConfig.getVolume(VOPlayConfig::Audio::BGM);
	m_Volume_bgm = tmp/100.0f;

    m_Volume_main = SettingData::getInstance()->m_InstrumentTone; // playConfig.getVolume(VOPlayConfig::Audio::EFFECT);
    m_Volume_other = SettingData::getInstance()->m_BgmInstrument; // playConfig.getVolume(VOPlayConfig::Audio::OTHERS);
    PTR_GAMEMAIN->m_isDrawLyrics = SettingData::getInstance()->m_isLyricsShow; //playConfig.getEnabledLyrics();
	PTR_GAMEMAIN->m_GameSpeed = SettingData::getInstance()->m_GameSpeed;
	log("+++++++++++++++++++++ volum  M:%f   O:%f   B:%f", m_Volume_main, m_Volume_other, m_Volume_bgm);
}

VOOption& PlayGame::getOption()
{
	return _option;
}

void PlayGame::GameError(int type)
{
	PTR_GAMEMAIN->m_isGameError = true;
	switch (type)
	{
	case 0: //NTM데이타로딩에 문제 있을 경우 발생
	{
			  auto popup = UIAlertPopup::create();
			  popup->setMessage("Data loading ERROR!");
			  popup->setConfirm("OK", CC_CALLBACK_2(PlayGame::onGoHome, this));
			  PopupManager::getInstance()->open(popup);
			  //게임화면 전으로 이동 시키기
	}
		break;

	case 1: //그룹안에 노트갯수가 300개가 넘을 경우 발생
	{
			  auto popup = UIAlertPopup::create();
			  popup->setMessage("Group child size error:300 over!!!");
			  popup->setConfirm("OK", CC_CALLBACK_2(PlayGame::onGoHome, this));
			  PopupManager::getInstance()->open(popup);
			  //게임화면 전으로 이동 시키기
	}
		break;

	case 2:  //FX음은 있는데 곡정보에 FX음이없음일경우 문제 발생
	{
			  auto popup = UIAlertPopup::create();
			  popup->setMessage("FX SOUND INFO ERROR!!!");
			  popup->setConfirm("OK", CC_CALLBACK_2(PlayGame::onGoHome, this));
			  PopupManager::getInstance()->open(popup);
			  //게임화면 전으로 이동 시키기
	}
		break;

	}
}

void PlayGame::onGoHome(Ref* sender, Widget::TouchEventType type)
{
	if (type == Widget::TouchEventType::ENDED)
	{
		PopupManager::getInstance()->closeAll();
		ProtobufTaskManager::getInstance()->removeAllRequest();
		ScreenManager::getInstance()->replaceScreen(LogoScreen::create());
	}
}
