#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <string>
#include <chrono>
#include <algorithm>

using namespace geode::prelude;

// Función optimizada para reproducir y reutilizar las animaciones
void playVideoAnimation(const std::string& animName, GJBaseGameLayer* layer) {
    const int VIDEO_TAG = 80085;

    auto animation = CCAnimationCache::sharedAnimationCache()->animationByName(animName.c_str());
    if (!animation) return;

    auto animate = CCAnimate::create(animation);
    auto sequence = CCSequence::create(
        animate,
        CCRemoveSelf::create(),
        nullptr
    );

    if (auto sprite = static_cast<CCSprite*>(layer->getChildByTag(VIDEO_TAG))) {
        sprite->stopAllActions(); 
        sprite->runAction(sequence); 
    } 
    else {
        auto firstFrame = static_cast<CCSpriteFrame*>(animation->getFrames()->objectAtIndex(0));
        auto newSprite = CCSprite::createWithSpriteFrame(firstFrame);
        newSprite->setTag(VIDEO_TAG);

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        newSprite->setPosition(winSize / 2);
        
        float scaleX = winSize.width / newSprite->getContentSize().width;
        float scaleY = winSize.height / newSprite->getContentSize().height;
        newSprite->setScale(std::max(scaleX, scaleY));

        newSprite->runAction(sequence);
        layer->addChild(newSprite, 9999);
    }
}

class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    struct Fields {
        std::chrono::steady_clock::time_point m_lastP1Click = std::chrono::steady_clock::now() - std::chrono::hours(1);
        std::chrono::steady_clock::time_point m_lastP2Click = std::chrono::steady_clock::now() - std::chrono::hours(1);
        bool m_initialized = false;
    };

    void pushButton(PlayerButton btn, bool isPlayer2) {
        GJBaseGameLayer::pushButton(btn, isPlayer2);
        
        if (btn != PlayerButton::Jump) return;

        // Inicialización usando el motor nativo del juego (Inmune a fallos)
        if (!m_fields->m_initialized) {
            m_fields->m_initialized = true;

            for (int i = 0; i < 30; i++) {
                const char* p1File = CCString::createWithFormat("p1_frame_%02d.png", i)->getCString();
                const char* p2File = CCString::createWithFormat("p2_frame_%02d.png", i)->getCString();
                const char* dualFile = CCString::createWithFormat("dual_frame_%02d.png", i)->getCString();

                CCTextureCache::sharedTextureCache()->addImage(p1File);
                CCTextureCache::sharedTextureCache()->addImage(p2File);
                CCTextureCache::sharedTextureCache()->addImage(dualFile);
            }

            auto cacheAnimation = [](const char* prefix, const char* animName) {
                auto animFrames = CCArray::create();
                for (int i = 0; i < 30; i++) {
                    const char* fileName = CCString::createWithFormat("%s_%02d.png", prefix, i)->getCString();
                    auto texture = CCTextureCache::sharedTextureCache()->textureForKey(fileName);
                    if (texture) {
                        auto rect = CCRect{ 0, 0, texture->getContentSize().width, texture->getContentSize().height };
                        auto frame = CCSpriteFrame::createWithTexture(texture, rect);
                        animFrames->addObject(frame);
                    }
                }
                auto animation = CCAnimation::createWithSpriteFrames(animFrames, 1.0f / 30.0f);
                CCAnimationCache::sharedAnimationCache()->addAnimation(animation, animName);
            };

            cacheAnimation("p1_frame", "p1_anim");
            cacheAnimation("p2_frame", "p2_anim");
            cacheAnimation("dual_frame", "dual_anim");
        }

        bool isTwoPlayerLevel = false;
        if (this->m_levelSettings) {
            isTwoPlayerLevel = this->m_levelSettings->m_twoPlayerMode;
        }

        if (isTwoPlayerLevel) {
            auto now = std::chrono::steady_clock::now();
            const auto simultaneousThreshold = std::chrono::milliseconds(50);

            if (!isPlayer2) {
                m_fields->m_lastP1Click = now;
                auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_fields->m_lastP2Click);
                
                if (timeDiff < simultaneousThreshold) {
                    playVideoAnimation("dual_anim", this);
                } else {
                    playVideoAnimation("p1_anim", this);
                }
            } 
            else {
                m_fields->m_lastP2Click = now;
                auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_fields->m_lastP1Click);
                
                if (timeDiff < simultaneousThreshold) {
                    playVideoAnimation("dual_anim", this);
                } else {
                    playVideoAnimation("p2_anim", this);
                }
            }
        } 
        else {
            if (!isPlayer2) {
                playVideoAnimation("p1_anim", this);
            } else {
                playVideoAnimation("p2_anim", this);
            }
        }
    }
};
