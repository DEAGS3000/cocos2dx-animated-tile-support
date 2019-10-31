#include "TMXLayerEx.h"
#include "Definitions.h"
#include "ContentManager.h"

namespace pm
{
    TMXLayerEx * TMXLayerEx::create(TMXTilesetInfoEx *tilesetInfo, TMXLayerInfoEx *layerInfo, TMXMapInfoEx *mapInfo)
    {
        TMXLayerEx *ret = new (std::nothrow) TMXLayerEx();
        if (ret->initWithTilesetInfo(tilesetInfo, layerInfo, mapInfo))
        {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool TMXLayerEx::initWithTilesetInfo(TMXTilesetInfoEx *tilesetInfo, TMXLayerInfoEx *layerInfo, TMXMapInfoEx *mapInfo)
    {
        // FIXME:: is 35% a good estimate ?
        Size size = layerInfo->_layerSize;
        float totalNumberOfTiles = size.width * size.height;
        float capacity = totalNumberOfTiles * 0.35f + 1; // 35 percent is occupied ?

        Texture2D *texture = nullptr;
        if( tilesetInfo )
        {
            texture = Director::getInstance()->getTextureCache()->addImage(tilesetInfo->_sourceImage);
        }

        if (nullptr == texture)
            return false;

        if (SpriteBatchNode::initWithTexture(texture, static_cast<ssize_t>(capacity)))
        {
            // layerInfo
            _layerName = layerInfo->_name;
            _layerSize = size;
            _tiles = layerInfo->_tiles;
            _opacity = layerInfo->_opacity;
            setProperties(layerInfo->getProperties());
            _contentScaleFactor = Director::getInstance()->getContentScaleFactor();

            // tilesetInfo
            _tileSet = tilesetInfo;
            CC_SAFE_RETAIN(_tileSet);

            // mapInfo
            _mapTileSize = mapInfo->getTileSize();
            _layerOrientation = mapInfo->getOrientation();
            _staggerAxis = mapInfo->getStaggerAxis();
            _staggerIndex = mapInfo->getStaggerIndex();
            _hexSideLength = mapInfo->getHexSideLength();

            // offset (after layer orientation is set);
            Vec2 offset = this->calculateLayerOffset(layerInfo->_offset);
            this->setPosition(CC_POINT_PIXELS_TO_POINTS(offset));

            _atlasIndexArray = ccCArrayNew(totalNumberOfTiles);

            float width = 0;
            float height = 0;
            if (_layerOrientation == TMXOrientationHex) {
                if (_staggerAxis == TMXStaggerAxis_X) {
                    height = _mapTileSize.height * (_layerSize.height + 0.5);
                    width = (_mapTileSize.width + _hexSideLength) * ((int)(_layerSize.width / 2)) + _mapTileSize.width * ((int)_layerSize.width % 2);
                } else {
                    width = _mapTileSize.width * (_layerSize.width + 0.5);
                    height = (_mapTileSize.height + _hexSideLength) * ((int)(_layerSize.height / 2)) + _mapTileSize.height * ((int)_layerSize.height % 2);
                }
            } else {
                width = _layerSize.width * _mapTileSize.width;
                height = _layerSize.height * _mapTileSize.height;
            }
            this->setContentSize(CC_SIZE_PIXELS_TO_POINTS(Size(width, height)));

            _useAutomaticVertexZ = false;
            _vertexZvalue = 0;

            return true;
        }
        return false;
    }

    // TMXLayerEx - setup Tiles
    void TMXLayerEx::setupTilesEx()
    {
        // TODO: 添加动画块应该就是在这个函数里
        // Optimization: quick hack that sets the image size on the tileset
        _tileSet->_imageSize = _textureAtlas->getTexture()->getContentSizeInPixels();

        // By default all the tiles are aliased
        // pros:
        //  - easier to render
        // cons:
        //  - difficult to scale / rotate / etc.
        _textureAtlas->getTexture()->setAliasTexParameters();

        //CFByteOrder o = CFByteOrderGetCurrent();

        // Parse cocos2d properties
        this->parseInternalProperties();

        for (int y=0; y < _layerSize.height; y++)
        {
            for (int x=0; x < _layerSize.width; x++)
            {
                int newX = x;
                // fix correct render ordering in Hexagonal maps when stagger axis == x
                if (_staggerAxis == TMXStaggerAxis_X && _layerOrientation == TMXOrientationHex)
                {
                    if (_staggerIndex == TMXStaggerIndex_Odd)
                    {
                        if (x >= _layerSize.width/2)
                            newX = (x - std::ceil(_layerSize.width/2)) * 2 + 1;
                        else
                            newX = x * 2;
                    } else {
                        // TMXStaggerIndex_Even
                        if (x >= static_cast<int>(_layerSize.width/2))
                            newX = (x - static_cast<int>(_layerSize.width/2)) * 2;
                        else
                            newX = x * 2 + 1;
                    }
                }

                int pos = static_cast<int>(newX + _layerSize.width * y);
                int gid = _tiles[ pos ];

                // gid are stored in little endian.
                // if host is big endian, then swap
                //if( o == CFByteOrderBigEndian )
                //    gid = CFSwapInt32( gid );
                /* We support little endian.*/

                // FIXME:: gid == 0 --> empty tile
                if (gid != 0)
                {
                    // FIXME: 我加的，添加动画块
                    //if(*(std::find_if(_tileSet->_animationInfo.begin(), _tileSet->_animationInfo.end(), [=] (const TMXAnimation* a)->bool { return a->_tileID == gid; } )))
                    if(_tileSet->_animationInfo[gid])
                    {
                            this->appendTileForGIDEx(gid, Vec2(newX, y));
                            _animTileCoordDict[gid].push_back(Vec2(newX, y));
                            this->_hasAnimatedTiles = true;
                    }
                    else
                        this->appendTileForGIDEx(gid, Vec2(newX, y));

                        //this->appendAnimTileForGID(gid, Vec2(newX, y));

                }
            }
        }
    }

//    Sprite *TMXLayerEx::appendAnimTileForGID(uint32_t gid, const Vec2 &pos)
//    {
//        if (gid != 0 && (static_cast<int>((gid & kTMXFlippedMask)) - _tileSet->_firstGid) >= 0)
//        {
//            Rect rect = _tileSet->getRectForGID(gid);
//            rect = CC_RECT_PIXELS_TO_POINTS(rect);
//
//            // Z could be just an integer that gets incremented each time it is called.
//            // but that wouldn't work on layers with empty tiles.
//            // and it is IMPORTANT that Z returns an unique and bigger number than the previous one.
//            // since _atlasIndexArray must be ordered because `bsearch` is used to find the GID for
//            // a given Z. (github issue #16512)
//            intptr_t z = getZForPos(pos);
//
//            // TODO: 动画块的重用与普通块的重用要分开
//            // FIXME: 如果使用下面这句，所有动画块会是同一个tile，说是为了优化，不知道怎么做的
//            // Sprite *tile = reusedTileWithRect(rect);
//            Sprite *tile = Sprite::createWithTexture(_textureAtlas->getTexture(), rect);
//            tile->setBatchNode(this);
//            tile->retain();
//
//            setupTileSprite(tile ,pos ,gid);
//
//            // 创建动画，如果已经有了就不创建了
//            if(!_tileAnimationActionDict.at(gid))
//            {
//                Vector<AnimationFrame *> animation_frames;
//                auto animation_info = _tileSet->_animationInfo[gid];
//                for(auto &frame : animation_info->_frames)
//                {
//                    // FIXME:: delayUnits先设为0
//                    animation_frames.pushBack(AnimationFrame::create(SpriteFrame::createWithTexture(_textureAtlas->getTexture(), rect), frame._tileID,
//                                                                     ValueMap()));
//                }
//                auto tile_animation = Animation::create(animation_frames, 0.001f);  //TODO: 这里还有个循环次数，没填。delayPerUnit记得改
//                auto action = RepeatForever::create(Animate::create(tile_animation));
//                //action->retain();
//                this->_tileAnimationActionDict.insert(gid, action);
//            }
//            // gid到对应的tile数组的map，如果没有，要创建
//            if(!_animTileDict.count(gid))
//            {
//                _animTileDict.insert(std::pair<uint32_t, Vector<Sprite*>>(gid, Vector<Sprite*>()));
//            }
//
//            _animTileDict[gid].pushBack(tile);
//            //tile->retain();
//
//
//            // FIXME: 启动tile动画的代码，因为还没有优雅的方式储存动画和tile的关系，就先在这里直接创建并启动了
//            //auto action = RepeatForever::create(Animate::create(tile_animation));
//            //_tileAnimationActions.pushBack(action);
//            //this->runAction(action);
//
//
//            // optimization:
//            // The difference between appendTileForGID and insertTileforGID is that append is faster, since
//            // it appends the tile at the end of the texture atlas
//            ssize_t indexForZ = _atlasIndexArray->num;
//
//            // don't add it using the "standard" way.
//            insertQuadFromSprite(tile, indexForZ);
//
//            // append should be after addQuadFromSprite since it modifies the quantity values
//            ccCArrayInsertValueAtIndex(_atlasIndexArray, (void*)z, indexForZ);
//
//            // Validation for issue #16512
//            CCASSERT(_atlasIndexArray->num == 1 ||
//                     _atlasIndexArray->arr[_atlasIndexArray->num-1] > _atlasIndexArray->arr[_atlasIndexArray->num-2], "Invalid z for _atlasIndexArray");
//
//            return tile;
//        }
//
//        return nullptr;
//    }


    // used only when parsing the map. useless after the map was parsed
    // since lot's of assumptions are no longer true
    Sprite * TMXLayerEx::appendTileForGIDEx(uint32_t gid, const Vec2& pos)
    {
        if (gid != 0 && (static_cast<int>((gid & kTMXFlippedMask)) - _tileSet->_firstGid) >= 0)
        {
            Rect rect = _tileSet->getRectForGID(gid);
            rect = CC_RECT_PIXELS_TO_POINTS(rect);

            // Z could be just an integer that gets incremented each time it is called.
            // but that wouldn't work on layers with empty tiles.
            // and it is IMPORTANT that Z returns an unique and bigger number than the previous one.
            // since _atlasIndexArray must be ordered because `bsearch` is used to find the GID for
            // a given Z. (github issue #16512)
            intptr_t z = getZForPos(pos);

            Sprite *tile = reusedTileWithRect(rect);

            setupTileSprite(tile ,pos ,gid);

            // optimization:
            // The difference between appendTileForGID and insertTileforGID is that append is faster, since
            // it appends the tile at the end of the texture atlas
            ssize_t indexForZ = _atlasIndexArray->num;

            // don't add it using the "standard" way.
            insertQuadFromSprite(tile, indexForZ);

            // append should be after addQuadFromSprite since it modifies the quantity values
            ccCArrayInsertValueAtIndex(_atlasIndexArray, (void*)z, indexForZ);

            // Validation for issue #16512
            CCASSERT(_atlasIndexArray->num == 1 ||
                     _atlasIndexArray->arr[_atlasIndexArray->num-1] > _atlasIndexArray->arr[_atlasIndexArray->num-2], "Invalid z for _atlasIndexArray");

            return tile;
        }

        return nullptr;
    }

    Sprite *TMXLayerEx::insertTileForGIDEx(uint32_t gid, const Vec2 &pos)
    {
        if (gid != 0 && (static_cast<int>((gid & kTMXFlippedMask)) - _tileSet->_firstGid) >= 0)
        {
            Rect rect = _tileSet->getRectForGID(gid);
            rect = CC_RECT_PIXELS_TO_POINTS(rect);

            intptr_t z = (intptr_t)((int) pos.x + (int) pos.y * _layerSize.width);

            Sprite *tile = reusedTileWithRect(rect);

            setupTileSprite(tile, pos, gid);

            // get atlas index
            ssize_t indexForZ = atlasIndexForNewZ(static_cast<int>(z));

            // Optimization: add the quad without adding a child
            this->insertQuadFromSprite(tile, indexForZ);

            // insert it into the local atlasindex array
            ccCArrayInsertValueAtIndex(_atlasIndexArray, (void*)z, indexForZ);

            // update possible children

            for(const auto &child : _children) {
                Sprite* sp = static_cast<Sprite*>(child);
                ssize_t ai = sp->getAtlasIndex();
                if ( ai >= indexForZ )
                {
                    sp->setAtlasIndex(ai+1);
                }
            }

            _tiles[z] = gid;
            return tile;
        }

        return nullptr;
    }

    Sprite *TMXLayerEx::updateTileForGIDEx(uint32_t gid, const Vec2 &pos)
    {
        // TODO: 或者可以用这个函数实现地图动画？
        Rect rect = _tileSet->getRectForGID(gid);
        rect = Rect(rect.origin.x / _contentScaleFactor, rect.origin.y / _contentScaleFactor, rect.size.width/ _contentScaleFactor, rect.size.height/ _contentScaleFactor);
        int z = (int)((int) pos.x + (int) pos.y * _layerSize.width);

        Sprite *tile = reusedTileWithRect(rect);

        setupTileSprite(tile ,pos ,gid);

        // get atlas index
        ssize_t indexForZ = atlasIndexForExistantZ(z);
        tile->setAtlasIndex(indexForZ);
        tile->setDirty(true);
        tile->updateTransform();
        _tiles[z] = gid;

        return tile;
    }

    void TMXLayerEx::setTileGIDEx(uint32_t gid, const Vec2& pos)
    {
        setTileGIDEx(gid, pos, (TMXTileFlags)0);
    }

    void TMXLayerEx::setTileGIDEx(uint32_t gid, const Vec2& pos, TMXTileFlags flags)
    {
        CCASSERT(pos.x < _layerSize.width && pos.y < _layerSize.height && pos.x >=0 && pos.y >=0, "TMXLayer: invalid position");
        CCASSERT(_tiles && _atlasIndexArray, "TMXLayer: the tiles map has been released");
        CCASSERT(gid == 0 || (int)gid >= _tileSet->_firstGid, "TMXLayer: invalid gid" );

        TMXTileFlags currentFlags;
        uint32_t currentGID = getTileGIDAt(pos, &currentFlags);

        if (currentGID != gid || currentFlags != flags)
        {
            uint32_t gidAndFlags = gid | flags;

            // setting gid=0 is equal to remove the tile
            if (gid == 0)
            {
                removeTileAt(pos);
            }
                // empty tile. create a new one
            else if (currentGID == 0)
            {
                insertTileForGIDEx(gidAndFlags, pos);
            }
                // modifying an existing tile with a non-empty tile
            else
            {
                int z = (int) pos.x + (int) pos.y * _layerSize.width;
                Sprite *sprite = static_cast<Sprite*>(getChildByTag(z));
                if (sprite)
                {
                    Rect rect = _tileSet->getRectForGID(gid);
                    rect = CC_RECT_PIXELS_TO_POINTS(rect);

                    sprite->setTextureRect(rect, false, rect.size);
                    if (flags)
                    {
                        setupTileSprite(sprite, sprite->getPosition(), gidAndFlags);
                    }
                    _tiles[z] = gidAndFlags;
                }
                else
                {
                    updateTileForGIDEx(gidAndFlags, pos);
                }
            }
        }
    }

    void TMXLayerEx::setupTileSpriteEx(Sprite* sprite, const Vec2& pos, uint32_t gid)
    {
        sprite->setPosition(getPositionAt(pos));
        sprite->setPositionZ((float)getVertexZForPos(pos));
        sprite->setAnchorPoint(Vec2::ZERO);
        sprite->setOpacity(_opacity);

        //issue 1264, flip can be undone as well
        sprite->setFlippedX(false);
        sprite->setFlippedY(false);
        sprite->setRotation(0.0f);
        sprite->setAnchorPoint(Vec2(0,0));

        // Rotation in tiled is achieved using 3 flipped states, flipping across the horizontal, vertical, and diagonal axes of the tiles.
        if (gid & kTMXTileDiagonalFlag)
        {
            // put the anchor in the middle for ease of rotation.
            sprite->setAnchorPoint(Vec2(0.5f,0.5f));
            sprite->setPosition(getPositionAt(pos).x + sprite->getContentSize().height/2,
                                getPositionAt(pos).y + sprite->getContentSize().width/2 );

            auto flag = gid & (kTMXTileHorizontalFlag | kTMXTileVerticalFlag );

            // handle the 4 diagonally flipped states.
            if (flag == kTMXTileHorizontalFlag)
            {
                sprite->setRotation(90.0f);
            }
            else if (flag == kTMXTileVerticalFlag)
            {
                sprite->setRotation(270.0f);
            }
            else if (flag == (kTMXTileVerticalFlag | kTMXTileHorizontalFlag) )
            {
                sprite->setRotation(90.0f);
                sprite->setFlippedX(true);
            }
            else
            {
                sprite->setRotation(270.0f);
                sprite->setFlippedX(true);
            }
        }
        else
        {
            if (gid & kTMXTileHorizontalFlag)
            {
                sprite->setFlippedX(true);
            }

            if (gid & kTMXTileVerticalFlag)
            {
                sprite->setFlippedY(true);
            }
        }
    }

    Sprite* TMXLayerEx::reusedTileWithRectEx(const Rect& rect)
    {
        if (! _reusedTile)
        {
            _reusedTile = Sprite::createWithTexture(_textureAtlas->getTexture(), rect);
            _reusedTile->setBatchNode(this);
            _reusedTile->retain();
        }
        else
        {
            // FIXME: HACK: Needed because if "batch node" is nil,
            // then the Sprite'squad will be reset
            _reusedTile->setBatchNode(nullptr);

            // Re-init the sprite
            _reusedTile->setTextureRect(rect, false, rect.size);

            // restore the batch node
            _reusedTile->setBatchNode(this);
        }

        return _reusedTile;
    }

//    void TMXLayerEx::startAllTileAnims()
//    {
//        // FIXME: 创建Action的工作本来应该在别处，先放到这里测试一下
//        for(auto &pair : _tileAnimationActionDict)
//        {
//            for(auto &sprite : _animTileDict[pair.first])
//            {
//                if(sprite==_animTileDict[pair.first].front())
//
//                // cocos会把tile优化掉。这里可以强行启用，但不够优雅。还是避免优化掉
//                this->appendChild(sprite);
//                //sprite->scheduleUpdate();
//                sprite->runAction(pair.second->clone());
//            }
//            // TODO: runaction的主体是谁？layer还是tile
//        }
//    }


    void TMXLayerEx::onEnter()
    {
        //ilog("[info] animation layer enter\n");
        //this->scheduleUpdate();
        //this->schedule(schedule_selector(TMXLayerEx::tickAnimationTiles));
        //TMXLayer::onEnter();
        if(!_hasAnimatedTiles) return;
        if(!_ticker)
            _ticker = new TMXTileAnimationTicker(this);
        _ticker->start(this);
    }

    Rect TMXLayerEx::getRectForGID(uint32_t gid)
    {
        return _tileSet->getRectForGID(gid);
    }

    void TMXLayerEx::tickAnimationTiles(float dt)
    {
        //ilog("[info] animation layer tick animation tiles\n");
        if(_ticker->_started)
            _ticker->update(dt);
    }

    void TMXLayerEx::onExit()
    {
        Node::onExit();
        if(!_hasAnimatedTiles) return;
        if(_ticker && _ticker->_started)
            _ticker->_started = false;
    }

    TMXTilesetInfoEx *TMXLayerEx::getTileSetEx()
    {
        return _tileSet;
    }
}

