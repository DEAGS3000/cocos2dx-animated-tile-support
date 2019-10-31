//
// Created by 张诚伟 on 2019/10/30.
//

#ifndef POKEMON_COCOS2DX_TMXTILEANIMATIONTICKER_H
#define POKEMON_COCOS2DX_TMXTILEANIMATIONTICKER_H
#include "TMXLayerEx.h"

namespace pm
{
    class TMXLayerEx;

    class TMXAnimationTask
    {
    public:
        TMXAnimationTask(TMXLayerEx *layer, TMXAnimation *animation, Vec2 &tilePos);
        void tick();
        void update(float dt);

        TMXLayerEx *_layer;
        Vec2 _tilePos;
        TMXAnimation *_animation;
        uint32_t _currentFrame;
        uint32_t _frameCount;
        float _timeElapsed;
    };

    class TMXTileAnimationTicker
    {
    public:
        float _timeElapsed;  // 单位为秒
        bool _started = false;
        std::vector<TMXAnimationTask> _tasks;
        TMXLayerEx* _layer;

        TMXTileAnimationTicker(TMXLayerEx *layer);
        void start(TMXLayerEx *layer);
        void update(float dt);
        void addTile();
    };
}




#endif //POKEMON_COCOS2DX_TMXTILEANIMATIONTICKER_H
