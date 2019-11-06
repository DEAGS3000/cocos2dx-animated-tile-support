#pragma once
#include "cocos2d.h"

USING_NS_CC;

namespace pm
{
    struct TMXFrame
    {
        TMXFrame(uint32_t tileID, float duration);
        uint32_t _tileID;
        float _duration;  // 毫秒
        // TODO: 应当把每个帧的sprite单独缓存一份实例供复用
    };

    struct TMXAnimation
    {
        TMXAnimation(uint32_t  tileID);
        uint32_t _tileID;
        Vec3 _tilePosition;
        std::vector<TMXFrame> _frames;
    };


    class TMXTilesetInfoEx : public TMXTilesetInfo
    {
    public:
        TMXTilesetInfoEx();
        ~TMXTilesetInfoEx();

        // 这里需要一个数据结构存储动画块信息
        // 存储frame信息
        // 存储animation信息
        //std::vector<TMXAnimation*> _animationInfo;
        std::map<uint32_t, TMXAnimation*> _animationInfo;
    };
}



