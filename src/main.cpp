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

    // Método seguro y nativo para eliminar el sprite al terminar
    void removeAnimSprite(CCNode* sender) {
        if (sender) {
            sender->removeFromParentAndCleanup(true);
        }
    }

    // Reproducción optimizada integrada directamente en la clase
    void playVideoAnimation(const std::string& animName) {
        const int VIDEO_TAG = 80085;

        auto animation = CCAnimationCache::get()->animationByName(animName.c_str());
        if (!animation) return;

        auto animate = CCAnimate::create(animation);

        if (auto sprite = static_cast<CCSprite*>(this->getChildByTag(VIDEO_TAG))) {
            sprite->stopAllActions(); 
            auto sequence = CCSequence::create(
                animate,
                CCCallFuncN::create(this, callfuncN_selector(MyBaseGameLayer::removeAnimSprite)),
                nullptr
            );
            sprite->runAction(sequence); 
        } 
        else {
            std::string firstFrameName = "p1_frame_00.png";
            if (animName == "p2_anim") firstFrameName = "p2_frame_00.png";
            else if (animName == "dual_anim") firstFrameName = "dual_frame_00.png";

            // Intentamos obtener el frame de la caché de Geode o del disco si hace falta
            CCSpriteFrame* frame = CCSpriteFrameCache::get()->spriteFrameByName(firstFrameName.c_str());
            if (!frame) {
                auto texture = CCTextureCache::get()->textureForKey(firstFrameName.c_str());
                if (texture) {
                    auto rect = CCRect{ 0, 0, texture->getContentSize().width, texture->getContentSize().height };
                    frame = CCSpriteFrame::createWithTexture(texture, rect);
                }
            }

            if (!frame) return;

            auto newSprite = CCSprite::createWithSpriteFrame(frame);
            newSprite->setTag(VIDEO_TAG);

            auto winSize = CCDirector::get()->getWinSize();
            newSprite->setPosition(winSize / 2);
            
            float scaleX = winSize.width / newSprite->getContentSize().width;
            float scaleY = winSize.height / newSprite->getContentSize().height;
            newSprite->setScale(std::max(scaleX, scaleY));

            auto sequence = CCSequence::create(
                animate,
                CCCallFuncN::create(this, callfuncN_selector(MyBaseGameLayer::removeAnimSprite)),
                nullptr
            );

            newSprite->runAction(sequence);
            this->addChild(newSprite, 9999);
        }
    }

    void pushButton(PlayerButton btn, bool isPlayer2) {
        GJBaseGameLayer::pushButton(btn, isPlayer2);
        
        if (btn != PlayerButton::Jump) return;

        // Inicialización con doble vía de carga (Caché de Geode o imágenes sueltas)
        if (!m_fields->m_initialized) {
            m_fields->m_initialized = true;

            auto cacheAnimation = [](const std::string& prefix, const std::string& animName) {
                auto animFrames = CCArray::create();
                auto textureCache = CCTextureCache::get();
                auto frameCache = CCSpriteFrameCache::get();

                for (int i = 0; i < 30; i++) {
                    std::string num = (i < 10) ? "0" + std::to_string(i) : std::to_string(i);
                    std::string fileName = prefix + "_" + num + ".png";
                    
                    auto frame = frameCache->spriteFrameByName(fileName.c_str());
                    if (!frame && textureCache) {
                        auto texture = textureCache->addImage(fileName.c_str());
                        if (texture) {
                            auto rect = CCRect{ 0, 0, texture->getContentSize().width, texture->getContentSize().height };
                            frame = CCSpriteFrame::createWithTexture(texture, rect);
                            frameCache->addSpriteFrame(frame, fileName.c_str());
                        }
                    }
                    
                    if (frame) {
                        animFrames->addObject(frame);
                    }
                }
                
                if (animFrames->count() > 0) {
                    auto animation = CCAnimation::createWithSpriteFrames(animFrames, 1.0f / 30.0f);
                    CCAnimationCache::get()->addAnimation(animation, animName.c_str());
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
                    this->playVideoAnimation("dual_anim");
                } else {
                    this->playVideoAnimation("p1_anim");
                }
            } 
            else {
                m_fields->m_lastP2Click = now;
                auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_fields->m_lastP1Click);
                
                if (timeDiff < simultaneousThreshold) {
                    this->playVideoAnimation("dual_anim");
                } else {
                    this->playVideoAnimation("p2_anim");
                }
            }
        } 
        else {
            if (!isPlayer2) {
                this->playVideoAnimation("p1_anim");
            } else {
                this->playVideoAnimation("p2_anim");
            }
        }
    }
};
