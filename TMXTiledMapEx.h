#pragma once
#include "cocos2d.h"
#include "TMXLayerEx.h"
#include "TMXTilesetInfoEx.h"

USING_NS_CC;



namespace pm
{
    class TMXLayerEx;
    class TMXLayerInfoEx;



	class TMXMapInfoEx : public TMXMapInfo
	{
	public:
        static TMXMapInfoEx * create(const std::string& tmxFile);
        /// Layers
        const Vector<TMXLayerInfoEx*>& getLayers() const;
        Vector<TMXLayerInfoEx*>& getLayers();
        void setLayers(const Vector<TMXLayerInfoEx*>& layers);

        /// tilesets
        const Vector<TMXTilesetInfoEx*>& getTilesets() const;
        Vector<TMXTilesetInfoEx*>& getTilesets();
        void setTilesets(const Vector<TMXTilesetInfoEx*>& tilesets);

        /** initializes a TMX format with a  tmx file */
        bool initWithTMXFileEx(const std::string& tmxFile);
	private:
        void startElement(void* /*ctx*/, const char *name, const char **atts) override;
        void endElement(void *ctx, const char *name) override;

    protected:
        /// Layers
        Vector<TMXLayerInfoEx*> _layers;

/// tilesets
        Vector<TMXTilesetInfoEx*> _tilesets;

    };

	class TMXTiledMapEx : public TMXTiledMap
	{
	public:
        static TMXTiledMapEx* create(const std::string& tmxFile);
		TMXTiledMapEx();
		~TMXTiledMapEx();

        bool initWithTMXFileEx(const std::string& tmxFile);
        //void runTileAnimations();
        void onEnter() override;
        void update(float dt) override;
		//Vector<Sprite*> animation_tiles_;

    protected:
        TMXTilesetInfoEx * tilesetForLayer(TMXLayerInfoEx *layerInfo, TMXMapInfoEx *mapInfo);
        TMXLayerEx * parseLayer(TMXLayerInfoEx *layerInfo, TMXMapInfoEx *mapInfo);
        void buildWithMapInfoEx(TMXMapInfoEx* mapInfo);

	};

}


