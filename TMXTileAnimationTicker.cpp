//
// Created by 张诚伟 on 2019/10/30.
//

#include "TMXTileAnimationTicker.h"
#include "ContentManager.h"

namespace pm
{

    void TMXTileAnimationTicker::start(TMXLayerEx *layer)
    {
        _started = true;
    }

    void TMXTileAnimationTicker::update(float dt)
    {
        if(!_started || _tasks.empty())
            return;
        for(auto &task : _tasks)
        {
            task.update(dt);
        }
    }

    TMXTileAnimationTicker::TMXTileAnimationTicker(TMXLayerEx *layer)
    {
        _layer = layer;
        for(const auto &p : _layer->_animTileCoordDict)
        {
            for(auto tilePos : p.second)
            {
                _tasks.push_back(TMXAnimationTask(_layer, _layer->getTileSetEx()->_animationInfo[p.first], tilePos));
            }
        }
    }

    void TMXAnimationTask::tick()
    {
        //ilog("[animation tile] ticked\n");
        _currentFrame = (_currentFrame + 1) % _frameCount;
        _layer->setTileGIDEx(_animation->_frames[_currentFrame]._tileID, _tilePos);
        _timeElapsed = 0.0f;
        // TODO: 在指定间隔后调用本函数

//        _layer->scheduleOnce([=](float dt){
//            tick();
//            }, _animation->_frames[_currentFrame]._duration / 1000.0f, "");
    }

    TMXAnimationTask::TMXAnimationTask(TMXLayerEx *layer, TMXAnimation *animation, Vec2 & tilePos)
    {
        _layer = layer;
        _animation = animation;
        _currentFrame = 0;
        _frameCount = _animation->_frames.size();
        _tilePos = tilePos;
        _timeElapsed = 0.0f;
    }

    void TMXAnimationTask::update(float dt)
    {
        _timeElapsed += dt;
        if(_timeElapsed*1000.0f >= _animation->_frames[_currentFrame]._duration)
            tick();
    }
}
