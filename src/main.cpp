#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <string>
#include <chrono>
#include <algorithm>

using namespace geode::prelude;

class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    struct Fields {
        std::chrono::steady_clock::time_point m_lastP1Click = std::chrono::steady_clock::now() - std::chrono::hours(1);
        std::chrono::steady_clock::time_point m_lastP2Click = std::chrono::steady_clock::now() - std::chrono::hours(1);
        bool m_initialized = false;
    };

    void removeAnimSprite(CCNode* sender) {
        if (sender) sender->removeFromParentAndCleanup(true);
    }

    void playVideoAnimation(const std::string& animName) {
        const int VIDEO_TAG = 80085;
        auto animation = CCAnimationCache::get()->animationByName(animName.c_str());
        if (!animation) return;

        auto animate = CCAnimate::create(animation);

        if (auto sprite = static_cast<CCSprite*>(this->getChildByTag(VIDEO_TAG))) {
            sprite->stopAllActions(); 
            sprite->runAction(CCSequence::create(animate, CCCallFuncN::create(this, callfuncN_selector(MyBaseGameLayer::removeAnimSprite)), nullptr)); 
        } else {
            std::string firstFrameName = (animName == "p2_anim") ? "p2_frame_00.png" : ((animName == "dual_anim") ? "dual_frame_00.png" : "p1_frame_00.png");
            CCSpriteFrame* frame = CCSpriteFrameCache::get()->spriteFrameByName(firstFrameName.c_str());
            if (!frame) return;

            auto newSprite = CCSprite::createWithSpriteFrame(frame);
            newSprite->setTag(VIDEO_TAG);
            auto winSize = CCDirector::get()->getWinSize();
            newSprite->setPosition(winSize / 2);
            newSprite->setScale(std::max(winSize.width / newSprite->getContentSize().width, winSize.height / newSprite->getContentSize().height));
            newSprite->runAction(CCSequence::create(animate, CCCallFuncN::create(this, callfuncN_selector(MyBaseGameLayer::removeAnimSprite)), nullptr));
            this->addChild(newSprite, 9999);
        }
    }

    void pushButton(PlayerButton btn, bool isPlayer2) {
        GJBaseGameLayer::pushButton(btn, isPlayer2);
        if (btn != PlayerButton::Jump) return;

        if (!m_fields->m_initialized) {
            m_fields->m_initialized = true;
            auto cacheAnimation = [](const std::string& prefix, const std::string& animName) {
                auto animFrames = CCArray::create();
                for (int i = 0; i < 30; i++) {
                    std::string fileName = prefix + "_" + ((i < 10) ? "0" + std::to_string(i) : std::to_string(i)) + ".png";
                    if (auto frame = CCSpriteFrameCache::get()->spriteFrameByName(fileName.c_str())) animFrames->addObject(frame);
                }
                if (animFrames->count() > 0) CCAnimationCache::get()->addAnimation(CCAnimation::createWithSpriteFrames(animFrames, 1.0f / 30.0f), animName.c_str());
            };
            cacheAnimation("p1_frame", "p1_anim");
            cacheAnimation("p2_frame", "p2_anim");
            cacheAnimation("dual_frame", "dual_anim");
        }

        bool isTwoPlayer = m_levelSettings && m_levelSettings->m_twoPlayerMode;
        auto now = std::chrono::steady_clock::now();
        if (isTwoPlayer) {
            if (!isPlayer2) {
                m_fields->m_lastP1Click = now;
                this->playVideoAnimation((std::chrono::duration_cast<std::chrono::milliseconds>(now - m_fields->m_lastP2Click) < std::chrono::milliseconds(50)) ? "dual_anim" : "p1_anim");
            } else {
                m_fields->m_lastP2Click = now;
                this->playVideoAnimation((std::chrono::duration_cast<std::chrono::milliseconds>(now - m_fields->m_lastP1Click) < std::chrono::milliseconds(50)) ? "dual_anim" : "p2_anim");
            }
        } else {
            this->playVideoAnimation(isPlayer2 ? "p2_anim" : "p1_anim");
        }
    }
};
