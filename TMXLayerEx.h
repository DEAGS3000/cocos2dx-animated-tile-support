#pragma once
#include <cocos2d.h>
#include "TMXTilesetInfoEx.h"
#include "TMXTiledMapEx.h"
#include "TMXTileAnimationTicker.h"

USING_NS_CC;



namespace pm
{
    class TMXTiledMapEx;
    class TMXMapInfoEx;
    //class TMXTilesetInfoEx;
    class TMXTileAnimationTicker;


	class TMXLayerInfoEx : public TMXLayerInfo
	{
    public:

	};

	class TMXLayerEx : public TMXLayer
	{
	public:
        static TMXLayerEx * create(TMXTilesetInfoEx *tilesetInfo, TMXLayerInfoEx *layerInfo, TMXMapInfoEx *mapInfo);

        //void startAllTileAnims();
        /** Initializes a TMXLayer with a tileset info, a layer info and a map info.
     *
     * @param tilesetInfo An tileset info.
     * @param layerInfo A layer info.
     * @param mapInfo A map info.
     * @return If initializes successfully, it will return true.
     */
        bool initWithTilesetInfo(TMXTilesetInfoEx *tilesetInfo, TMXLayerInfoEx *layerInfo, TMXMapInfoEx *mapInfo);
        void setupTilesEx();
        /** Sets the tile gid (gid = tile global id) at a given tile coordinate.
 * The Tile GID can be obtained by using the method "tileGIDAt" or by using the TMX editor -> Tileset Mgr +1.
 * If a tile is already placed at that position, then it will be removed.
 *
 * @param gid The tile gid.
 * @param tileCoordinate The tile coordinate.
 */
        void setTileGIDEx(uint32_t gid, const Vec2& tileCoordinate);

        /** Sets the tile gid (gid = tile global id) at a given tile coordinate.
         * The Tile GID can be obtained by using the method "tileGIDAt" or by using the TMX editor -> Tileset Mgr +1.
         * If a tile is already placed at that position, then it will be removed.
         * Use withFlags if the tile flags need to be changed as well.
         *
         * @param gid The tile gid.
         * @param tileCoordinate The tile coordinate.
         * @param flags The tile flags.
         */

        void setTileGIDEx(uint32_t gid, const Vec2& tileCoordinate, TMXTileFlags flags);

        /** Removes a tile at given tile coordinate.
         *
         * @param tileCoordinate The tile coordinate.
         */
        //void removeTileAt(const Vec2& tileCoordinate);

        /** Returns the position in points of a given tile coordinate.
         *
         * @param tileCoordinate The tile coordinate.
         * @return The position in points of a given tile coordinate.
         */
        //Vec2 getPositionAt(const Vec2& tileCoordinate);

        Rect getRectForGID(uint32_t gid);
        TMXTilesetInfoEx* getTileSetEx();
        void onEnter() override ;
        void onExit() override;
        void tickAnimationTiles(float dt);

        bool _hasAnimatedTiles = false;
        TMXTileAnimationTicker *_ticker = nullptr;

        //Map<uint32_t, Action*> _tileAnimationActionDict;
        //std::map<uint32_t, Vector<Sprite*>> _animTileDict;
        std::map<uint32_t, std::vector<Vec2>> _animTileCoordDict;
	protected:
        void setupTileSpriteEx(Sprite* sprite, const Vec2& pos, uint32_t gid);
        Sprite* reusedTileWithRectEx(const Rect& rect);
        /* optimization methods */
        Sprite* appendTileForGIDEx(uint32_t gid, const Vec2& pos);
        Sprite* insertTileForGIDEx(uint32_t gid, const Vec2& pos);
        Sprite* updateTileForGIDEx(uint32_t gid, const Vec2& pos);
        //Sprite* appendAnimTileForGID(uint32_t gid, const Vec2& pos);

        /** Tileset information for the layer */
        TMXTilesetInfoEx* _tileSet;

	};


}
