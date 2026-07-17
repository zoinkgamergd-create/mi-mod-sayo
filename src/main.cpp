#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <string>
#include <chrono>
#include <algorithm>
#include <format> // Nativo de C++20 para reemplazar a fmt

using namespace geode::prelude;

// Función ultra-optimizada para reproducir y reutilizar las animaciones sin consumir CPU adicional
void playVideoAnimation(const std::string& animName, GJBaseGameLayer* layer) {
    const int VIDEO_TAG = 80085; // Identificador único para el sprite de la animación

    // Buscamos la animación precargada en la memoria caché del juego
    auto animation = CCAnimationCache::sharedAnimationCache()->animationByName(animName.c_str());
    if (!animation) return;

    auto animate = CCAnimate::create(animation);
    auto sequence = CCSequence::create(
        animate,
        CCRemoveSelf::create(), // Se elimina automáticamente al terminar la secuencia
        nullptr
    );

    // REUTILIZACIÓN DE MEMORIA (Antilag):
    if (auto sprite = static_cast<CCSprite*>(layer->getChildByTag(VIDEO_TAG))) {
        sprite->stopAllActions(); 
        sprite->runAction(sequence); 
    } 
    // Si es el primer clic o la animación anterior ya había finalizado, creamos el sprite
    else {
        auto firstFrame = static_cast<CCSpriteFrame*>(animation->getFrames()->objectAtIndex(0));
        auto newSprite = CCSprite::createWithSpriteFrame(firstFrame);
        newSprite->setTag(VIDEO_TAG);

        // Centrar en pantalla y escalar proporcionalmente (Aspect-Ratio)
        auto winSize = CCDirector::sharedDirector()->getWinSize();
        newSprite->setPosition(winSize / 2);
        
        float scaleX = winSize.width / newSprite->getContentSize().width;
        float scaleY = winSize.height / newSprite->getContentSize().height;
        newSprite->setScale(std::max(scaleX, scaleY));

        newSprite->runAction(sequence);
        layer->addChild(newSprite, 9999); // Dibujar en la capa superior por encima del gameplay
    }
}

class $modify(MyBaseGameLayer, GJBaseGameLayer) {
    struct Fields {
        // Registros de tiempo usando steady_clock para medir intervalos de microsegundos de forma precisa
        std::chrono::steady_clock::time_point m_lastP1Click = std::chrono::steady_clock::now() - std::chrono::hours(1);
        std::chrono::steady_clock::time_point m_lastP2Click = std::chrono::steady_clock::now() - std::chrono::hours(1);
    };

    void pushButton(PlayerButton btn, bool isPlayer2) {
        // INICIALIZACIÓN PEREZOSA (Evita errores de compilación al no usar init())
        static bool initialized = false;
        if (!initialized) {
            initialized = true;

            // 1. Precarga de las 90 imágenes directamente en la memoria de la GPU con std::format
            for (int i = 0; i < 30; i++) {
                CCTextureCache::sharedTextureCache()->addImage(std::format("p1_frame_{:02d}.png", i).c_str());
                CCTextureCache::sharedTextureCache()->addImage(std::format("p2_frame_{:02d}.png", i).c_str());
                CCTextureCache::sharedTextureCache()->addImage(std::format("dual_frame_{:02d}.png", i).c_str());
            }

            // 2. Definición del creador de animaciones a 30fps
            auto cacheAnimation = [](const std::string& prefix, const std::string& animName) {
                auto animFrames = CCArray::create();
                for (int i = 0; i < 30; i++) {
                    std::string fileName = std::format("{}_{:02d}.png", prefix, i);
                    auto texture = CCTextureCache::sharedTextureCache()->textureForKey(fileName.c_str());
                    if (texture) {
                        auto rect = CCRect{ 0, 0, texture->getContentSize().width, texture->getContentSize().height };
                        auto frame = CCSpriteFrame::createWithTexture(texture, rect);
                        animFrames->addObject(frame);
                    }
                }
                // 30 FPS = ~0.033s por cuadro de animación
                auto animation = CCAnimation::createWithSpriteFrames(animFrames, 1.0f / 30.0f);
                CCAnimationCache::sharedAnimationCache()->addAnimation(animation, animName.c_str());
            };

            // Almacenamos las tres animaciones estructuradas en la caché global del motor
            cacheAnimation("p1_frame", "p1_anim");
            cacheAnimation("p2_frame", "p2_anim");
            cacheAnimation("dual_frame", "dual_anim");
        }

        GJBaseGameLayer::pushButton(btn, isPlayer2);
        
        // Filtrar clics para actuar solo ante comandos de Salto
        if (btn != PlayerButton::Jump) return;

        // Comprobación de seguridad: ¿El nivel actual está configurado como 'Two Player Mode'?
        bool isTwoPlayerLevel = false;
        if (this->m_levelSettings) {
            isTwoPlayerLevel = this->m_levelSettings->m_twoPlayerMode;
        }

        if (isTwoPlayerLevel) {
            // === LÓGICA DE DETECCIÓN DUAL (Mano Izquierda, Derecha y Simultáneo) ===
            auto now = std::chrono::steady_clock::now();
            const auto simultaneousThreshold = std::chrono::milliseconds(50); // Margen de coincidencia física

            if (!isPlayer2) { // Acción Jugador 1
                m_fields->m_lastP1Click = now;
                auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_fields->m_lastP2Click);
                
                if (timeDiff < simultaneousThreshold) {
                    playVideoAnimation("dual_anim", this); // Acción combinada (Ambas manos)
                } else {
                    playVideoAnimation("p1_anim", this); // Solo Mano Izquierda
                }
            } 
            else { // Acción Jugador 2
                m_fields->m_lastP2Click = now;
                auto timeDiff = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_fields->m_lastP1Click);
                
                if (timeDiff < simultaneousThreshold) {
                    playVideoAnimation("dual_anim", this); // Acción combinada (Ambas manos)
                } else {
                    playVideoAnimation("p2_anim", this); // Solo Mano Derecha
                }
            }
        } 
        else {
            // === LÓGICA SPAM INDEPENDIENTE (1 Player - Multi-teclado/Sayo) ===
            if (!isPlayer2) {
                playVideoAnimation("p1_anim", this); // Tecla izquierda Sayo (Mano 1)
            } else {
                playVideoAnimation("p2_anim", this); // Tecla derecha Sayo (Mano 2)
            }
        }
    }
};
