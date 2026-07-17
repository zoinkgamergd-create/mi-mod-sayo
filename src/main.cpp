#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <string>
#include <chrono>
#include <algorithm>

using namespace geode::prelude;

// Función optimizada para reproducir y reutilizar las animaciones
void playVideoAnimation(const std::string& animName, GJBaseGameLayer* layer) {
    const int VIDEO_TAG = 80085;

    // Sintaxis oficial de Geode v3 (Inmune a fallos)
    auto animationCache = CCAnimationCache::get();
    if (!animationCache) return;

    auto animation = animationCache->animationByName(animName.c_str());
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
        // Obtenemos el primer frame de manera segura usando std::string estándar
        std::string firstFrameName = "p1_frame_00.png";
        if (animName == "p2_anim") firstFrameName = "p2_frame_00.png";
        else if (animName == "dual_anim") firstFrameName = "dual_frame_00.png";

        auto textureCache = CCTextureCache::get();
        if (!textureCache) return;

        auto texture = textureCache->textureForKey(firstFrameName.c_str());
        if (!texture) return;

        auto rect = CCRect{ 0, 0, texture->getContentSize().width, texture->getContentSize().height };
        auto newSprite = CCSprite::createWithTexture(texture, rect);
        newSprite->setTag(VIDEO_TAG);

        auto director = CCDirector::get();
        if (!director) return;
        auto winSize = director->getWinSize();
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

        // Inicialización usando concatenación estándar compatible con cualquier compilador
        if (!m_fields->m_initialized) {
            m_fields->m_initialized = true;

            auto textureCache = CCTextureCache::get();
            if (textureCache) {
                for (int i = 0; i < 30; i++) {
                    std::string num = (i < 10) ? "0" + std::to_string(i) : std::to_string(i);
                    
                    std::string p1File = "p1_frame_" + num + ".png";
                    std::string p2File = "p2_frame_" + num + ".png";
                    std::string dualFile = "dual_frame_" + num + ".png";

                    textureCache->addImage(p1File.c_str());
                    textureCache->addImage(p2File.c_str());
                    textureCache->addImage(dualFile.c_str());
                }
            }

            auto cacheAnimation = [](const std::string& prefix, const std::string& animName) {
                auto animFrames = CCArray::create();
                auto textureCache = CCTextureCache::get();
                if (!textureCache) return;

                for (int i = 0; i < 30; i++) {
                    std::string num = (i < 10) ? "0" + std::to_string(i) : std::to_string(i);
                    std::string fileName = prefix + "_" + num + ".png";
                    
                    auto texture = textureCache->textureForKey(fileName.c_str());
                    if (texture) {
                        auto rect = CCRect{ 0, 0, texture->getContentSize().width, texture->getContentSize().height };
                        auto frame = CCSpriteFrame::createWithTexture(texture, rect);
                        animFrames->addObject(frame);
                    }
                }
                auto animation = CCAnimation::createWithSpriteFrames(animFrames, 1.0f / 30.0f);
                auto animationCache = CCAnimationCache::get();
                if (animationCache) {
                    animationCache->addAnimation(animation, animName.c_str());
                }
            };

            cacheAnimation("p1_frame", "p1_anim");
            cacheAnimation("p2_frame", "p2_anim");
            cacheAnimation("dual_frame", "dual_anim");
        }

        bool isTwoPlayerLevel = false;
        if (m_levelSettings) {
            isTwoPlayerLevel = m_levelSettings->m_twoPlayerMode;
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
