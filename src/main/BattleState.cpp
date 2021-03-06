#include "BattleState.h"
#include "Alien.h"
#include "HumanAgent.h"
#include "Car.h"
#include "Entity.h"
#include "MenuState.h"
#include "SceneLoader.h"
#include "AIDivideAreaManager.h"
#include "EntityManager.h"
#include "Monster.h"
#include "MonsterAIAgent.h"
#include "EntityManager.h"
#include "ConfigurationManager.h"
#include "RecordManager.h"
#include "AnimationState.h"

#include <Graphics/CameraComponent.hpp>
#include <Graphics/LightComponent.hpp>
#include <Graphics/MeshComponent.hpp>
#include <Physics/PhysicsBodyComponent.hpp>
#include <Core/ResourceManager.hpp>
#include <Scene/Game.hpp>
#include <Gui/GuiRootWindow.hpp>
#include <Gui/GuiManager.hpp>
#include <Scene/StateManager.hpp>
#include <Logic/ScriptComponent.hpp>
#include <Logic/ScriptManager.hpp>

#include <OgreProcedural.h>

#include <iostream>

BattleState::BattleState(const QString stage_name) 
    : mQuestionLabel(nullptr),
      mDialogLabel(nullptr),
	  mPickUpCrystalBar(nullptr),
      mTotalEnemyNum(0),
      mRemainEnemyNum(0),
      mTotalCrystalNum(0),
      mObtainedCrystalNum(0),
      mStage(stage_name),
      mNextStage(""),
      mSceneParam1(0.0),
      mSceneParam2(0.0),
      mCrystalBarPosition(0),
      mHasPaused(false),
      mQAShowed(false) {}

void BattleState::onInitialize() {
   
    dt::ScriptManager::get()->loadScript("scripts/" + mStage + ".js");
    dt::Node* script_node = new dt::Node("script_node");
    script_node->addComponent(new dt::ScriptComponent(mStage + ".js", "state_script", true));

    AIDivideAreaManager::get()->beforeLoadScene(mSceneParam1, mSceneParam2);

    EntityManager::get()->beforeLoadScene();
    auto scene = addScene(SceneLoader::loadScene(mStage + ".scene"));
    scene->addChildNode(script_node);

    //scene->getPhysicsWorld()->setShowDebug(true);
    dt::GuiRootWindow& root_win = dt::GuiManager::get()->getRootWindow();

    auto health_img1 = root_win.addChildWidget<dt::GuiImageBox>(new dt::GuiImageBox("health_one"));
    auto health_img2 = root_win.addChildWidget<dt::GuiImageBox>(new dt::GuiImageBox("health_ten"));
    auto health_img3 = root_win.addChildWidget<dt::GuiImageBox>(new dt::GuiImageBox("health_hundred"));
    auto ammo_img1 = root_win.addChildWidget<dt::GuiImageBox>(new dt::GuiImageBox("ammo_one"));
    auto ammo_img2 = root_win.addChildWidget<dt::GuiImageBox>(new dt::GuiImageBox("ammo_ten"));
    auto ammo_img3 = root_win.addChildWidget<dt::GuiImageBox>(new dt::GuiImageBox("ammo_hundred"));
    auto clip_img1 = root_win.addChildWidget<dt::GuiImageBox>(new dt::GuiImageBox("clip_one"));
    auto clip_img2 = root_win.addChildWidget<dt::GuiImageBox>(new dt::GuiImageBox("clip_ten"));
    auto clip_img3 = root_win.addChildWidget<dt::GuiImageBox>(new dt::GuiImageBox("clip_hundred"));
    auto front_sight = root_win.addChildWidget<dt::GuiImageBox>(new dt::GuiImageBox("front_sight"));
    auto answer1 = root_win.addChildWidget<dt::GuiButton>(new dt::GuiButton("answer1"));
    auto answer2 = root_win.addChildWidget<dt::GuiButton>(new dt::GuiButton("answer2"));
    auto answer3 = root_win.addChildWidget<dt::GuiButton>(new dt::GuiButton("answer3"));
    auto answer4 = root_win.addChildWidget<dt::GuiButton>(new dt::GuiButton("answer4"));
    auto question = root_win.addChildWidget<dt::GuiEditBox>(new dt::GuiEditBox("question"));
    auto dialog = root_win.addChildWidget<dt::GuiLabel>(new dt::GuiLabel("dialog"));

    mResumeButton = root_win.addChildWidget<dt::GuiButton>(new dt::GuiButton("resume_button")).get();
    mSaveButton = root_win.addChildWidget<dt::GuiButton>(new dt::GuiButton("save_button")).get();
    mLoadButton = root_win.addChildWidget<dt::GuiButton>(new dt::GuiButton("load_button")).get();
    mReturnMenuButton = root_win.addChildWidget<dt::GuiButton>(new dt::GuiButton("return_menu_button")).get();
    mExitButton = root_win.addChildWidget<dt::GuiButton>(new dt::GuiButton("exit_button")).get();
    mQARescueButton = root_win.addChildWidget<dt::GuiButton>(new dt::GuiButton("qa")).get();
    mPickUpCrystalBar = root_win.addChildWidget<dt::GuiProgressBar>(new dt::GuiProgressBar("pick_up_crystal_bar")).get();

    mPickUpCrystalBar->setProgressRange(101);
    mPickUpCrystalBar->setProgressPosition(0);
    mPickUpCrystalBar->setVisible(false);

    mHealthHUD.push_back(health_img3.get());
    mHealthHUD.push_back(health_img2.get());
    mHealthHUD.push_back(health_img1.get());
    mAmmoHUD.push_back(ammo_img3.get());
    mAmmoHUD.push_back(ammo_img2.get());
    mAmmoHUD.push_back(ammo_img1.get());
    mFrontSight = front_sight.get();
    mClipNumHUD.push_back(clip_img3.get());
    mClipNumHUD.push_back(clip_img2.get());
    mClipNumHUD.push_back(clip_img1.get());
    mAnswerButtons.push_back(answer1.get());
    mAnswerButtons.push_back(answer2.get());
    mAnswerButtons.push_back(answer3.get());
    mAnswerButtons.push_back(answer4.get());
    mQuestionLabel = question.get();
    mDialogLabel = dialog.get();

	Alien *pAlien = EntityManager::get()->getHuman();
	connect(pAlien, SIGNAL(sAmmoClipChange(uint16_t, uint16_t)), this, SLOT(__onAmmoClipChange(uint16_t, uint16_t)));
	connect(pAlien, SIGNAL(sHealthChanged(uint16_t)), this, SLOT(__onHealthChanged(uint16_t)));

    __onHealthChanged(pAlien->getCurHealth());
    __onAmmoChanged(0);
    __onClipNumChanged(0);

    Weapon* weapon = pAlien->getCurWeapon();

    if (weapon != nullptr) {
        __onAmmoChanged(weapon->getCurAmmo());
        __onClipNumChanged(weapon->getCurClip());
    }

    for (uint8_t i = 0 ; i < mAnswerButtons.size() ; ++i) {
        mAnswerButtons[i]->getMyGUIWidget()->eventMouseButtonClick += MyGUI::newDelegate(this, &BattleState::__onAnswerButtonClick);
    }

    __hideQA();

    mResumeButton->setCaption(QString::fromLocal8Bit("返回游戏"));
    mSaveButton->setCaption(QString::fromLocal8Bit("保存游戏"));
    mLoadButton->setCaption(QString::fromLocal8Bit("读取游戏"));
    mReturnMenuButton->setCaption(QString::fromLocal8Bit("返回主菜单"));
    mExitButton->setCaption(QString::fromLocal8Bit("退出游戏"));
    mQARescueButton->setCaption("QA-Rescue");

    mResumeButton->setVisible(false);
    mSaveButton->setVisible(false);
    mLoadButton->setVisible(false);
    mReturnMenuButton->setVisible(false);
    mExitButton->setVisible(false);
    mQARescueButton->setVisible(false);

    auto qa = ConfigurationManager::getInstance()->getQASetting();

    mQARescueButton->getMyGUIWidget()->setEnabled(qa.getIsQAEnable());

    mResumeButton->getMyGUIWidget()->eventMouseButtonClick += MyGUI::newDelegate(this, &BattleState::__onClick);
    mSaveButton->getMyGUIWidget()->eventMouseButtonClick += MyGUI::newDelegate(this, &BattleState::__onClick);
    mLoadButton->getMyGUIWidget()->eventMouseButtonClick += MyGUI::newDelegate(this, &BattleState::__onClick);
    mReturnMenuButton->getMyGUIWidget()->eventMouseButtonClick += MyGUI::newDelegate(this, &BattleState::__onClick);
    mExitButton->getMyGUIWidget()->eventMouseButtonClick += MyGUI::newDelegate(this, &BattleState::__onClick);
    mQARescueButton->getMyGUIWidget()->eventMouseButtonClick += MyGUI::newDelegate(this, &BattleState::__onClick);

    mFrontSight->setImageTexture("FrontSight.png");

    mDialogLabel->getMyGUIWidget()->setAlign(MyGUI::Align::Left);

    MyGUI::EditBox* edit_box = dynamic_cast<MyGUI::EditBox*>(mQuestionLabel->getMyGUIWidget());

    edit_box->setEditMultiLine(true);
    edit_box->setEditWordWrap(true);
    edit_box->setEditStatic(true);

    MyGUI::TextBox* text_box = dynamic_cast<MyGUI::TextBox*>(mDialogLabel->getMyGUIWidget());
    text_box->setTextAlign(MyGUI::Align::Left);

    __resetGui();

    dt::GuiManager::get()->setMouseCursorVisible(false);

    EntityManager::get()->afterLoadScene(scene.get(), mStage);

    connect(dt::InputManager::get(), SIGNAL(sPressed(dt::InputManager::InputCode, const OIS::EventArg&)),
                               this, SLOT(__onKeyPressed(dt::InputManager::InputCode, const OIS::EventArg&)));

}

void BattleState::onDeinitialize() {
    disconnect(dt::InputManager::get(), SIGNAL(sPressed(dt::InputManager::InputCode, const OIS::EventArg&)),
                               this, SLOT(__onKeyPressed(dt::InputManager::InputCode, const OIS::EventArg&)));

    /*Alien* pAlien = EntityManager::get()->getHuman();

    disconnect(pAlien, SIGNAL(sAmmoClipChange(uint16_t, uint16_t)), this, SLOT(__onAmmoClipChange(uint16_t, uint16_t)));
	disconnect(pAlien, SIGNAL(sHealthChanged(uint16_t)), this, SLOT(__onHealthChanged(uint16_t)));*/
}


void BattleState::updateStateFrame(double simulation_frame_time) {
	//拾起水晶进度条过程
	if(mCrystalBarPosition > 0) {
        mPickUpCrystalBar->setVisible(true);
		mPickUpCrystalBar->setProgressPosition(mCrystalBarPosition);
		//if(mCrystalBarPosition > 5.0) {
		//mCrystalBarPosition = 0.0;
			
        if (mCrystalBarPosition >= 100) {
			++mObtainedCrystalNum;
			setObtainedCrystalNum(mObtainedCrystalNum);
            mCrystalBarPosition = 0;
		}
	} else {
        mPickUpCrystalBar->setVisible(false);
    }
}


BattleState::BattleState(uint16_t tot_enemy_num, uint16_t tot_crystal_num):
		mQuestionLabel(nullptr),
		mDialogLabel(nullptr),
		mTotalEnemyNum(tot_enemy_num),
		mRemainEnemyNum(tot_enemy_num),
		mTotalCrystalNum(tot_crystal_num),
		mObtainedCrystalNum(0) {
}

void BattleState::win() {
    auto state_mgr = dt::StateManager::get();
    emit sVictory();

    if (mNextStage != "") {
        state_mgr->setNewState(new AnimationState("videos/win.avi", 4.0, new BattleState(mNextStage)));
    } else {
        state_mgr->setNewState(new AnimationState("videos/final.avi", 4.0, new MenuState()));
    }
}

QString BattleState::getBattleStateName() const {
	return QString("BattleState");
}

dt::GuiLabel* BattleState::getDialogLabel() {
	return mDialogLabel;
}

void BattleState::setDialogLabel(dt::GuiLabel* dialog_label) {
	if (dialog_label) {
		mDialogLabel = dialog_label;
	}
}

int BattleState::getTotalEnemyNum() const {
	return mTotalEnemyNum;
}

void BattleState::setTotalEnemyNum(int total_enemy_num) {
	mTotalEnemyNum = total_enemy_num;
}

int BattleState::getRemainEnemyNum() const {
	return mRemainEnemyNum;
}

void BattleState::setRemainEnemyNum(int remain_enemy_num) {
	mRemainEnemyNum = remain_enemy_num;
}

int BattleState::getTotalCrystalNum() const {
	return mTotalCrystalNum;
}

void BattleState::setTotalCrystalNum(int total_crystal_num) {
	mTotalCrystalNum = total_crystal_num;
}

int BattleState::getObtainedCrystalNum() const {
	return mObtainedCrystalNum;
}

void BattleState::setObtainedCrystalNum(int obtained_crystal_num) {
	mObtainedCrystalNum = obtained_crystal_num;
}

dt::GuiEditBox* BattleState::getQuestionLabel() {
	return mQuestionLabel;
}

void BattleState::setQuestionLabel(dt::GuiEditBox* edit_box) {
	if (edit_box) {
		mQuestionLabel = edit_box;
	}
}

// Slots

void BattleState::__onTriggerText(uint16_t text_id) {
	mQuestionLabel->show();
}

void BattleState::__onHealthChanged(uint16_t cur_health) {
    __changeDigits(mHealthHUD, cur_health);
}

void BattleState::__onAmmoChanged(uint16_t cur_ammo) {
    __changeDigits(mAmmoHUD, cur_ammo);
}

void BattleState::__onClipNumChanged(uint16_t cur_num) {
    __changeDigits(mClipNumHUD, cur_num);
}

void BattleState::__onAmmoClipChange(uint16_t cur_ammo, uint16_t cur_clip) {
	__changeDigits(mAmmoHUD, cur_ammo);
	__changeDigits(mClipNumHUD, cur_clip);
}

void BattleState::__onGetCrystal() {

}

void BattleState::__onTriggerQA() {
	getQuestionLabel()->show();
}

void BattleState::__onAnswerButtonClick(MyGUI::Widget* sender) {
    uint8_t answer;

    if (sender->getName() == "Gui.answer1") {
        answer = 1;
    } else if (sender->getName() == "Gui.answer2") {
        answer = 2;
    } else if (sender->getName() == "Gui.answer3") {
        answer = 3;
    } else {
        answer = 4;
    }

    if (mQuestion.evaluate(answer)) {
        Alien* player = EntityManager::get()->getHuman();

        if (player && player->getCurHealth() > 0) {
            player->setCurHealth(player->getCurHealth() + 30);
        }
    }

    __hideQA();
}

QString BattleState::getNextStage() const {
    return mNextStage;
}

void BattleState::setNextStage(const QString next_stage) {
    mNextStage = next_stage;
}

QString BattleState::getStage() const {
    return mStage;
}

void BattleState::setStage(const QString stage) {
    mStage = stage;
}

void BattleState::__resetGui() {
    dt::GuiRootWindow& root_win = dt::GuiManager::get()->getRootWindow();
    auto coordination = root_win.getMyGUIWidget()->getAbsoluteCoord();

    mDialogLabel->setCaption("");

    int gap_h_large = (float)coordination.width / 15.0f;
    int gap_v_large = (float)coordination.height / 15.0f;
    int size_h_large = (float)coordination.width / 10.0f;
    int size_v_large = (float)coordination.height / 10.0f;

    int gap_h_medium = (float)coordination.width / 30.0f;
    int gap_v_medium = (float)coordination.height / 30.0f;
    int size_h_medium = (float)coordination.width / 13.0f;
    int size_v_medium = (float)coordination.height / 13.0f;

    int gap_h_small = (float)coordination.width / 100.0f;
    int gap_v_small = (float)coordination.height / 100.0f;
    int size_h_small = (float)coordination.width / 40.0f;
    int size_v_small = (float)coordination.height / 40.0f;

    mFrontSight->getMyGUIWidget()->setAlign(MyGUI::Align::Center);
    mFrontSight->setSize((int)(gap_h_medium * 1.5f), (int)(gap_h_medium * 1.5f));
    mFrontSight->getMyGUIWidget()->setPosition(coordination.width / 2 - mFrontSight->getMyGUIWidget()->getSize().width / 2,
        coordination.height / 2 - mFrontSight->getMyGUIWidget()->getSize().height /2);

    mQuestionLabel->setSize(4.0f / 13.0f + 0.1f, 1.0f / 3.0f);
    mQuestionLabel->setPosition((int)(coordination.width / 2.0f - mQuestionLabel->getMyGUIWidget()->getAbsoluteRect().width() / 2.0f), 
        (int)(coordination.height / 2.0f - mQuestionLabel->getMyGUIWidget()->getAbsoluteRect().height() / 2.0f));

    for (uint8_t i = 0 ; i < 4 ; ++i) {
        mAnswerButtons[i]->setPosition((int)(coordination.right() / 2.0f - (2 * size_h_medium + 1.5 * gap_h_medium) + i * (size_h_medium + gap_h_medium)), 
            (int)(coordination.height / 2.0f + mQuestionLabel->getMyGUIWidget()->getAbsoluteRect().height() / 2 + gap_v_medium));
        mAnswerButtons[i]->setSize(1.0f / 13.0f, 1.0f / 13.0f);
    }

    for (uint8_t i = 0 ; i < 3 ; ++i) {
        mHealthHUD[i]->setSize(1.0f / 30.0f, 1.0f / 15.0f);
        mAmmoHUD[i]->setSize(1.0f / 30.0f, 1.0f / 15.0f);
        mClipNumHUD[i]->setSize(1.0f / 50.0f, 1.0f / 25.0f);

        mClipNumHUD[i]->setPosition(coordination.width - (3 - i) * (mClipNumHUD[0]->getMyGUIWidget()->getSize().width - gap_h_small * 3 / 8) - gap_h_small,
            coordination.height - 3 * gap_v_medium);
        mAmmoHUD[i]->setPosition(coordination.width - (3 - i) * (mAmmoHUD[0]->getMyGUIWidget()->getSize().width - gap_h_small / 2) - gap_h_small * 3 / 8
            - (coordination.width - mClipNumHUD[0]->getMyGUIWidget()->getPosition().left), coordination.height - 3 * gap_v_medium);
        mHealthHUD[i]->setPosition(i * (mHealthHUD[0]->getMyGUIWidget()->getSize().width - gap_h_small / 2) + gap_h_small / 2, coordination.height - 3 * gap_v_medium);
    }

    mDialogLabel->setSize(300, size_v_small);
    mDialogLabel->setPosition(mHealthHUD[0]->getMyGUIWidget()->getPosition().left, mHealthHUD[0]->getMyGUIWidget()->getPosition().top - mHealthHUD[0]->getMyGUIWidget()->
        getSize().height - gap_v_small / 2);

    mResumeButton->setSize(0.2f, 0.05f);
    mSaveButton->setSize(0.2f, 0.05f);
    mLoadButton->setSize(0.2f, 0.05f);
    mReturnMenuButton->setSize(0.2f, 0.05f);
    mExitButton->setSize(0.2f, 0.05f);
    mQARescueButton->setSize(0.2f, 0.05f);

    mResumeButton->setPosition(coordination.right() / 2 - mResumeButton->getMyGUIWidget()->getSize().width / 2, 
        coordination.bottom() / 2 - 2.5 * mResumeButton->getMyGUIWidget()->getSize().height - 2 * gap_v_medium);
    mSaveButton->setPosition(coordination.right() / 2 - mSaveButton->getMyGUIWidget()->getSize().width / 2, 
        coordination.bottom() / 2 - 1.5 * mResumeButton->getMyGUIWidget()->getSize().height - 1 * gap_v_medium);
    mLoadButton->setPosition(coordination.right() / 2 - mLoadButton->getMyGUIWidget()->getSize().width / 2, 
        coordination.bottom() / 2 - 0.5 * mResumeButton->getMyGUIWidget()->getSize().height);
    mReturnMenuButton->setPosition(coordination.right() / 2 - mReturnMenuButton->getMyGUIWidget()->getSize().width / 2, 
        coordination.bottom() / 2 + 0.5 * mResumeButton->getMyGUIWidget()->getSize().height + 1 * gap_v_medium);
    mExitButton->setPosition(coordination.right() / 2 - mExitButton->getMyGUIWidget()->getSize().width / 2, 
        coordination.bottom() / 2 + 1.5 * mResumeButton->getMyGUIWidget()->getSize().height + 2 * gap_v_medium);
    mQARescueButton->setPosition(coordination.right() / 2 - mExitButton->getMyGUIWidget()->getSize().width / 2, 
        coordination.bottom() / 2 + 2.5 * mResumeButton->getMyGUIWidget()->getSize().height + 3 * gap_v_medium);

    mPickUpCrystalBar->setSize(0.27f, 0.05f);
    mPickUpCrystalBar->setPosition(coordination.right() /2 - mPickUpCrystalBar->getMyGUIWidget()->getSize().width / 2,
        (int)(coordination.bottom() * 0.9f));
}

void BattleState::__hideQA() {
    for (uint8_t i = 0 ; i < 4 ; ++i) {
        mAnswerButtons[i]->setVisible(false);
    }

    mQuestionLabel->setVisible(false);

    dt::GuiManager::get()->setMouseCursorVisible(false);

    mQAShowed = false;
}

void BattleState::setQA(Question question) {
    mQuestionLabel->setVisible(true);
    mQuestionLabel->setCaption(question.getQuestion());

    auto answers = question.getAnswers();

    for (uint8_t i = 0 ; i < answers.size() ; ++i) {
        mAnswerButtons[i]->setVisible(true);
        mAnswerButtons[i]->setCaption(answers[i]);
    }

    dt::GuiManager::get()->setMouseCursorVisible(true);

    mQAShowed = true;
}

void BattleState::__changeDigits(std::vector<dt::GuiImageBox*>& pics, uint16_t number) {
    uint16_t digit;
    uint16_t factor;

    for (uint8_t i = 0 ; i < 3 ; ++i) {
        factor = pow(10.0, (int)(2 - i));

        digit = number / factor;
        number %= factor;

        pics[i]->setImageTexture(dt::Utils::toString(digit) + ".png");
    }
}

void BattleState::setSceneParam1(double param1) {
    mSceneParam1 = param1;
}

void BattleState::setSceneParam2(double param2) {
    mSceneParam2 = param2;
}

void BattleState::__showMenu() {
    if (!mHasPaused) {
        mHasPaused = true;

        mResumeButton->setVisible(true);
        mSaveButton->setVisible(true);
        mLoadButton->setVisible(true);
        mReturnMenuButton->setVisible(true);
        mExitButton->setVisible(true);
        mFrontSight->setVisible(false);
        mQARescueButton->setVisible(true);

        dt::GuiManager::get()->setMouseCursorVisible(true);

        EntityManager::get()->getHuman()->findChildNode(Agent::AGENT)->disable();
    }
}

void BattleState::__hideMenu() {
    if (mHasPaused) {
        mHasPaused = false;

        mResumeButton->setVisible(false);
        mSaveButton->setVisible(false);
        mLoadButton->setVisible(false);
        mReturnMenuButton->setVisible(false);
        mExitButton->setVisible(false);
        mFrontSight->setVisible(true);
        mQARescueButton->setVisible(false);

        dt::GuiManager::get()->setMouseCursorVisible(false);

        EntityManager::get()->getHuman()->findChildNode(Agent::AGENT)->enable();
    }
}

void BattleState::__onKeyPressed(dt::InputManager::InputCode code, const OIS::EventArg& event) {
    if (code == dt::InputManager::KC_ESCAPE) {
        if (!mQAShowed) {
            if (mHasPaused) {
                __hideMenu();
            } else {
                __showMenu();
            }
        } else {
            __hideQA();
        }
    }
}

void BattleState::__onClick(MyGUI::Widget* sender) {
    if (sender->getName() == "Gui.resume_button") {
        __hideMenu();
    } else if (sender->getName() == "Gui.save_button") {
        RecordManager* record_mgr = RecordManager::get();

        record_mgr->save(this);
        __hideMenu();
    } else if (sender->getName() == "Gui.load_button") {
    } else if (sender->getName() == "Gui.return_menu_button") {
        dt::StateManager::get()->setNewState(new MenuState());
    } else if (sender->getName() == "Gui.exit_button") {
        exit(0);
    } else if (sender->getName() == "Gui.qa") {
        mQuestion = QAManager::getInstance()->getRandomQuestion();

        __hideMenu();
        setQA(mQuestion);
    }
}

void BattleState::__onUnlockCrystalProgressChanged(uint16_t percent) {
    if (percent >= 0 && percent <= 100) {
        mCrystalBarPosition = percent;
    }
}

void BattleState::fail() {
    dt::StateManager::get()->setNewState(new AnimationState("videos/fail.avi", 4, new MenuState()));
}