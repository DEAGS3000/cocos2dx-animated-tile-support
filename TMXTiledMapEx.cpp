#include "TMXTiledMapEx.h"
#include "2d/CCTMXXMLParser.h"
#include <unordered_map>
#include <sstream>
#include <string>

using namespace std;

namespace pm
{
	TMXTiledMapEx::TMXTiledMapEx()
	{
	}


	TMXTiledMapEx::~TMXTiledMapEx()
	{
	}

    TMXMapInfoEx *TMXMapInfoEx::create(const std::string &tmxFile)
    {
        TMXMapInfoEx *ret = new (std::nothrow) TMXMapInfoEx();
        if (ret->initWithTMXFile(tmxFile))
        {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    TMXTiledMapEx * TMXTiledMapEx::create(const std::string& tmxFile)
    {
        TMXTiledMapEx *ret = new (std::nothrow) TMXTiledMapEx();
        if (ret->initWithTMXFileEx(tmxFile))
        {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool TMXTiledMapEx::initWithTMXFileEx(const std::string& tmxFile)
    {
        CCASSERT(tmxFile.size()>0, "TMXTiledMap: tmx file should not be empty");

        _tmxFile = tmxFile;

        setContentSize(Size::ZERO);

        TMXMapInfoEx *mapInfo = TMXMapInfoEx::create(tmxFile);

        if (! mapInfo)
        {
            return false;
        }
        CCASSERT( !mapInfo->getTilesets().empty(), "TMXTiledMap: Map not found. Please check the filename.");
        buildWithMapInfoEx(mapInfo);

        return true;
    }

    void TMXMapInfoEx::startElement(void *, const char *name, const char **atts)
    {
            TMXMapInfoEx *tmxMapInfo = this;
            std::string elementName = name;
            ValueMap attributeDict;
            if (atts && atts[0])
            {
                for (int i = 0; atts[i]; i += 2)
                {
                    std::string key = atts[i];
                    std::string value = atts[i+1];
                    attributeDict.emplace(key, Value(value));
                }
            }
            if (elementName == "map")
            {
                std::string version = attributeDict["version"].asString();
                if ( version != "1.0")
                {
                    CCLOG("cocos2d: TMXFormat: Unsupported TMX version: %s", version.c_str());
                }
                std::string orientationStr = attributeDict["orientation"].asString();
                if (orientationStr == "orthogonal") {
                    tmxMapInfo->setOrientation(TMXOrientationOrtho);
                }
                else if (orientationStr  == "isometric") {
                    tmxMapInfo->setOrientation(TMXOrientationIso);
                }
                else if (orientationStr == "hexagonal") {
                    tmxMapInfo->setOrientation(TMXOrientationHex);
                }
                else if (orientationStr == "staggered") {
                    tmxMapInfo->setOrientation(TMXOrientationStaggered);
                }
                else {
                    CCLOG("cocos2d: TMXFomat: Unsupported orientation: %d", tmxMapInfo->getOrientation());
                }

                std::string staggerAxisStr = attributeDict["staggeraxis"].asString();
                if (staggerAxisStr == "x") {
                    tmxMapInfo->setStaggerAxis(TMXStaggerAxis_X);
                }
                else if (staggerAxisStr  == "y") {
                    tmxMapInfo->setStaggerAxis(TMXStaggerAxis_Y);
                }

                std::string staggerIndex = attributeDict["staggerindex"].asString();
                if (staggerIndex == "odd") {
                    tmxMapInfo->setStaggerIndex(TMXStaggerIndex_Odd);
                }
                else if (staggerIndex == "even") {
                    tmxMapInfo->setStaggerIndex(TMXStaggerIndex_Even);
                }


                float hexSideLength = attributeDict["hexsidelength"].asFloat();
                tmxMapInfo->setHexSideLength(hexSideLength);

                Size s;
                s.width = attributeDict["width"].asFloat();
                s.height = attributeDict["height"].asFloat();
                tmxMapInfo->setMapSize(s);

                s.width = attributeDict["tilewidth"].asFloat();
                s.height = attributeDict["tileheight"].asFloat();
                tmxMapInfo->setTileSize(s);



                // The parent element is now "map"
                tmxMapInfo->setParentElement(TMXPropertyMap);
            }
            else if (elementName == "tileset")
            {
                // If this is an external tileset then start parsing that
                std::string externalTilesetFilename = attributeDict["source"].asString();
                if (externalTilesetFilename != "")
                {
                    _externalTilesetFilename = externalTilesetFilename;

                    // Tileset file will be relative to the map file. So we need to convert it to an absolute path
                    if (_TMXFileName.find_last_of('/') != string::npos)
                    {
                        string dir = _TMXFileName.substr(0, _TMXFileName.find_last_of('/') + 1);
                        externalTilesetFilename = dir + externalTilesetFilename;
                    }
                    else
                    {
                        externalTilesetFilename = _resources + "/" + externalTilesetFilename;
                    }
                    externalTilesetFilename = FileUtils::getInstance()->fullPathForFilename(externalTilesetFilename);

                    _currentFirstGID = attributeDict["firstgid"].asInt();
                    if (_currentFirstGID < 0)
                    {
                        _currentFirstGID = 0;
                    }
                    _recordFirstGID = false;
                    _externalTilesetFullPath = externalTilesetFilename;
                    tmxMapInfo->parseXMLFile(externalTilesetFilename);
                }
                else
                {
                    TMXTilesetInfoEx *tileset = new (std::nothrow) TMXTilesetInfoEx();
                    tileset->_name = attributeDict["name"].asString();

                    if (_recordFirstGID)
                    {
                        // unset before, so this is tmx file.
                        tileset->_firstGid = attributeDict["firstgid"].asInt();

                        if (tileset->_firstGid < 0)
                        {
                            tileset->_firstGid = 0;
                        }
                    }
                    else
                    {
                        tileset->_firstGid = _currentFirstGID;
                        _currentFirstGID = 0;
                    }

                    tileset->_spacing = attributeDict["spacing"].asInt();
                    tileset->_margin = attributeDict["margin"].asInt();
                    Size s;
                    s.width = attributeDict["tilewidth"].asFloat();
                    s.height = attributeDict["tileheight"].asFloat();
                    tileset->_tileSize = s;

                    tmxMapInfo->getTilesets().pushBack(tileset);
                    tileset->release();
                }
            }
            else if (elementName == "tile")
            {
                if (tmxMapInfo->getParentElement() == TMXPropertyLayer)
                {
                    TMXLayerInfoEx* layer = tmxMapInfo->getLayers().back();
                    Size layerSize = layer->_layerSize;
                    uint32_t gid = static_cast<uint32_t>(attributeDict["gid"].asUnsignedInt());
                    int tilesAmount = layerSize.width*layerSize.height;

                    if (_xmlTileIndex < tilesAmount)
                    {
                        layer->_tiles[_xmlTileIndex++] = gid;
                    }
                }
                else
                {
                    // 当解析到一个不从属于Layer的Tile，计算出它的gid，获取它的属性，将它的Parent设为Tile？？？不懂
                    // 有property子标签的应该是tileset里面对tile的定义
                    TMXTilesetInfoEx* info = tmxMapInfo->getTilesets().back();
                    tmxMapInfo->setParentGID(info->_firstGid + attributeDict["id"].asInt());
                    tmxMapInfo->getTileProperties()[tmxMapInfo->getParentGID()] = Value(ValueMap());
                    tmxMapInfo->setParentElement(TMXPropertyTile);
                }
            }
            else if (elementName == "layer")
            {
                TMXLayerInfoEx *layer = new (std::nothrow) TMXLayerInfoEx();
                layer->_name = attributeDict["name"].asString();

                Size s;
                s.width = attributeDict["width"].asFloat();
                s.height = attributeDict["height"].asFloat();
                layer->_layerSize = s;

                Value& visibleValue = attributeDict["visible"];
                layer->_visible = visibleValue.isNull() ? true : visibleValue.asBool();

                Value& opacityValue = attributeDict["opacity"];
                layer->_opacity = opacityValue.isNull() ? 255 : (unsigned char)(255.0f * opacityValue.asFloat());

                float x = attributeDict["x"].asFloat();
                float y = attributeDict["y"].asFloat();
                layer->_offset.set(x, y);

                tmxMapInfo->getLayers().pushBack(layer);
                layer->release();

                // The parent element is now "layer"
                tmxMapInfo->setParentElement(TMXPropertyLayer);
            }
            else if (elementName == "objectgroup")
            {
                TMXObjectGroup *objectGroup = new (std::nothrow) TMXObjectGroup();
                objectGroup->setGroupName(attributeDict["name"].asString());
                Vec2 positionOffset;
                positionOffset.x = attributeDict["x"].asFloat() * tmxMapInfo->getTileSize().width;
                positionOffset.y = attributeDict["y"].asFloat() * tmxMapInfo->getTileSize().height;
                objectGroup->setPositionOffset(positionOffset);

                tmxMapInfo->getObjectGroups().pushBack(objectGroup);
                objectGroup->release();

                // The parent element is now "objectgroup"
                tmxMapInfo->setParentElement(TMXPropertyObjectGroup);
            }
            else if (elementName == "tileoffset")
            {
                TMXTilesetInfoEx* tileset = tmxMapInfo->getTilesets().back();

                double tileOffsetX = attributeDict["x"].asDouble();

                double tileOffsetY = attributeDict["y"].asDouble();

                tileset->_tileOffset = Vec2(tileOffsetX, tileOffsetY);

            }
            else if (elementName == "image")
            {
                TMXTilesetInfoEx* tileset = tmxMapInfo->getTilesets().back();

                // build full path
                std::string imagename = attributeDict["source"].asString();
                tileset->_originSourceImage = imagename;

                if (!_externalTilesetFullPath.empty())
                {
                    string dir = _externalTilesetFullPath.substr(0, _externalTilesetFullPath.find_last_of('/') + 1);
                    tileset->_sourceImage = dir + imagename;
                }
                else if (_TMXFileName.find_last_of('/') != string::npos)
                {
                    string dir = _TMXFileName.substr(0, _TMXFileName.find_last_of('/') + 1);
                    tileset->_sourceImage = dir + imagename;
                }
                else
                {
                    tileset->_sourceImage = _resources + (_resources.size() ? "/" : "") + imagename;
                }
            }
            else if (elementName == "data")
            {
                std::string encoding = attributeDict["encoding"].asString();
                std::string compression = attributeDict["compression"].asString();

                if (encoding == "")
                {
                    tmxMapInfo->setLayerAttribs(tmxMapInfo->getLayerAttribs() | TMXLayerAttribNone);

                    TMXLayerInfoEx* layer = tmxMapInfo->getLayers().back();
                    Size layerSize = layer->_layerSize;
                    int tilesAmount = layerSize.width*layerSize.height;

                    uint32_t *tiles = (uint32_t*) malloc(tilesAmount*sizeof(uint32_t));
                    // set all value to 0
                    memset(tiles, 0, tilesAmount*sizeof(int));

                    layer->_tiles = tiles;
                }
                else if (encoding == "base64")
                {
                    int layerAttribs = tmxMapInfo->getLayerAttribs();
                    tmxMapInfo->setLayerAttribs(layerAttribs | TMXLayerAttribBase64);
                    tmxMapInfo->setStoringCharacters(true);

                    if (compression == "gzip")
                    {
                        layerAttribs = tmxMapInfo->getLayerAttribs();
                        tmxMapInfo->setLayerAttribs(layerAttribs | TMXLayerAttribGzip);
                    } else
                    if (compression == "zlib")
                    {
                        layerAttribs = tmxMapInfo->getLayerAttribs();
                        tmxMapInfo->setLayerAttribs(layerAttribs | TMXLayerAttribZlib);
                    }
                    CCASSERT( compression == "" || compression == "gzip" || compression == "zlib", "TMX: unsupported compression method" );
                }
                else if (encoding == "csv")
                {
                    int layerAttribs = tmxMapInfo->getLayerAttribs();
                    tmxMapInfo->setLayerAttribs(layerAttribs | TMXLayerAttribCSV);
                    tmxMapInfo->setStoringCharacters(true);
                }
            }
            else if (elementName == "object")
            {
                TMXObjectGroup* objectGroup = tmxMapInfo->getObjectGroups().back();

                // The value for "type" was blank or not a valid class name
                // Create an instance of TMXObjectInfo to store the object and its properties
                ValueMap dict;
                // Parse everything automatically
                const char* keys[] = {"name", "type", "width", "height", "gid", "id"};

                for (const auto& key : keys)
                {
                    Value value = attributeDict[key];
                    dict[key] = value;
                }

                // But X and Y since they need special treatment
                // X
                int x = attributeDict["x"].asInt();
                // Y
                int y = attributeDict["y"].asInt();

                Vec2 p(x + objectGroup->getPositionOffset().x, _mapSize.height * _tileSize.height - y  - objectGroup->getPositionOffset().y - attributeDict["height"].asInt());
                p = CC_POINT_PIXELS_TO_POINTS(p);
                dict["x"] = Value(p.x);
                dict["y"] = Value(p.y);

                int width = attributeDict["width"].asInt();
                int height = attributeDict["height"].asInt();
                Size s(width, height);
                s = CC_SIZE_PIXELS_TO_POINTS(s);
                dict["width"] = Value(s.width);
                dict["height"] = Value(s.height);

                dict["rotation"] = attributeDict["rotation"].asDouble();

                // Add the object to the objectGroup
                objectGroup->getObjects().push_back(Value(dict));

                // The parent element is now "object"
                tmxMapInfo->setParentElement(TMXPropertyObject);
            }
            else if (elementName == "property")
            {
                if ( tmxMapInfo->getParentElement() == TMXPropertyNone )
                {
                    CCLOG( "TMX tile map: Parent element is unsupported. Cannot add property named '%s' with value '%s'",
                           attributeDict["name"].asString().c_str(), attributeDict["value"].asString().c_str() );
                }
                else if ( tmxMapInfo->getParentElement() == TMXPropertyMap )
                {
                    // The parent element is the map
                    Value value = attributeDict["value"];
                    std::string key = attributeDict["name"].asString();
                    tmxMapInfo->getProperties().emplace(key, value);
                }
                else if ( tmxMapInfo->getParentElement() == TMXPropertyLayer )
                {
                    // The parent element is the last layer
                    TMXLayerInfoEx* layer = tmxMapInfo->getLayers().back();
                    Value value = attributeDict["value"];
                    std::string key = attributeDict["name"].asString();
                    // Add the property to the layer
                    layer->getProperties().emplace(key, value);
                }
                else if ( tmxMapInfo->getParentElement() == TMXPropertyObjectGroup )
                {
                    // The parent element is the last object group
                    TMXObjectGroup* objectGroup = tmxMapInfo->getObjectGroups().back();
                    Value value = attributeDict["value"];
                    std::string key = attributeDict["name"].asString();
                    objectGroup->getProperties().emplace(key, value);
                }
                else if ( tmxMapInfo->getParentElement() == TMXPropertyObject )
                {
                    // The parent element is the last object
                    TMXObjectGroup* objectGroup = tmxMapInfo->getObjectGroups().back();
                    ValueMap& dict = objectGroup->getObjects().rbegin()->asValueMap();

                    std::string propertyName = attributeDict["name"].asString();
                    dict[propertyName] = attributeDict["value"];
                }
                else if ( tmxMapInfo->getParentElement() == TMXPropertyTile )
                {
                    ValueMap& dict = tmxMapInfo->getTileProperties().at(tmxMapInfo->getParentGID()).asValueMap();

                    std::string propertyName = attributeDict["name"].asString();
                    dict[propertyName] = attributeDict["value"];
                }
            }
            else if (elementName == "polygon")
            {
                // find parent object's dict and add polygon-points to it
                TMXObjectGroup* objectGroup = _objectGroups.back();
                ValueMap& dict = objectGroup->getObjects().rbegin()->asValueMap();

                // get points value string
                std::string value = attributeDict["points"].asString();
                if (!value.empty())
                {
                    ValueVector pointsArray;
                    pointsArray.reserve(10);

                    // parse points string into a space-separated set of points
                    stringstream pointsStream(value);
                    string pointPair;
                    while (std::getline(pointsStream, pointPair, ' '))
                    {
                        // parse each point combo into a comma-separated x,y point
                        stringstream pointStream(pointPair);
                        string xStr, yStr;

                        ValueMap pointDict;

                        // set x
                        if (std::getline(pointStream, xStr, ','))
                        {
                            int x = atoi(xStr.c_str()) + (int)objectGroup->getPositionOffset().x;
                            pointDict["x"] = Value(x);
                        }

                        // set y
                        if (std::getline(pointStream, yStr, ','))
                        {
                            int y = atoi(yStr.c_str()) + (int)objectGroup->getPositionOffset().y;
                            pointDict["y"] = Value(y);
                        }

                        // add to points array
                        pointsArray.push_back(Value(pointDict));
                    }

                    dict["points"] = Value(pointsArray);
                }
            }
            else if (elementName == "polyline")
            {
                // find parent object's dict and add polyline-points to it
                TMXObjectGroup* objectGroup = _objectGroups.back();
                ValueMap& dict = objectGroup->getObjects().rbegin()->asValueMap();

                // get points value string
                std::string value = attributeDict["points"].asString();
                if (!value.empty())
                {
                    ValueVector pointsArray;
                    pointsArray.reserve(10);

                    // parse points string into a space-separated set of points
                    stringstream pointsStream(value);
                    string pointPair;
                    while (std::getline(pointsStream, pointPair, ' '))
                    {
                        // parse each point combo into a comma-separated x,y point
                        stringstream pointStream(pointPair);
                        string xStr, yStr;

                        ValueMap pointDict;

                        // set x
                        if (std::getline(pointStream, xStr, ','))
                        {
                            int x = atoi(xStr.c_str()) + (int)objectGroup->getPositionOffset().x;
                            pointDict["x"] = Value(x);
                        }

                        // set y
                        if (std::getline(pointStream, yStr, ','))
                        {
                            int y = atoi(yStr.c_str()) + (int)objectGroup->getPositionOffset().y;
                            pointDict["y"] = Value(y);
                        }

                        // add to points array
                        pointsArray.push_back(Value(pointDict));
                    }

                    dict["polylinePoints"] = Value(pointsArray);
                }
            }
            else if(elementName == "animation")
            {
                    TMXTilesetInfoEx* info = tmxMapInfo->getTilesets().back();
                    info->_animationInfo[tmxMapInfo->getParentGID()] = new TMXAnimation(tmxMapInfo->getParentGID());
                    //tmxMapInfo->setParentGID(info->_firstGid + attributeDict["id"].asInt());
                    //tmxMapInfo->getTileProperties()[tmxMapInfo->getParentGID()] = Value(ValueMap());
                    tmxMapInfo->setParentElement(TMXPropertyAnimation);
            }
            else if(elementName == "frame")
            {
                TMXTilesetInfoEx* info = tmxMapInfo->getTilesets().back();
                auto animInfo = info->_animationInfo[tmxMapInfo->getParentGID()];
                // calculate gid of frame
                animInfo->_frames.push_back(TMXFrame(info->_firstGid + attributeDict["tileid"].asInt(), attributeDict["duration"].asFloat()));
//                if (tmxMapInfo->getParentElement() == TMXPropertyAnimation)
//                {
//                    // TODO: 将frame添加到Animation
//                    // frame有两个属性：tileid和duration
//                    // tileid和id是一样的，是相对于tileset的firstid的偏移量
//                    // frame和tile的实际存储方式一样，都是gid，组装的时候会根据gid获取贴图
//                    TMXLayerInfoEx* layer = tmxMapInfo->getLayers().back();
//                    Size layerSize = layer->_layerSize;
//                    uint32_t gid = static_cast<uint32_t>(attributeDict["gid"].asUnsignedInt());
//                    int tilesAmount = layerSize.width*layerSize.height;
//
//                    if (_xmlTileIndex < tilesAmount)
//                    {
//                        layer->_tiles[_xmlTileIndex++] = gid;
//                    }
//                }
//                else
//                {
//                    // 当解析到一个不从属于Layer的Tile，计算出它的gid，获取它的属性，将它的Parent设为Tile？？？不懂
//                    // 有property子标签的应该是tileset里面对tile的定义
//                    TMXTilesetInfoEx* info = tmxMapInfo->getTilesets().back();
//                    tmxMapInfo->setParentGID(info->_firstGid + attributeDict["id"].asInt());
//                    tmxMapInfo->getTileProperties()[tmxMapInfo->getParentGID()] = Value(ValueMap());
//                    tmxMapInfo->setParentElement(TMXPropertyTile);
//                }
            }

    }

    const Vector<TMXTilesetInfoEx *> &TMXMapInfoEx::getTilesets() const
    {
        return _tilesets;
    }

    void TMXMapInfoEx::endElement(void* /*ctx*/, const char *name)
    {
        TMXMapInfoEx *tmxMapInfo = this;
        std::string elementName = name;

        if (elementName == "data")
        {
            if (tmxMapInfo->getLayerAttribs() & TMXLayerAttribBase64)
            {
                tmxMapInfo->setStoringCharacters(false);

                TMXLayerInfoEx* layer = tmxMapInfo->getLayers().back();

                std::string currentString = tmxMapInfo->getCurrentString();
                unsigned char *buffer;
                auto len = base64Decode((unsigned char*)currentString.c_str(), (unsigned int)currentString.length(), &buffer);
                if (!buffer)
                {
                    CCLOG("cocos2d: TiledMap: decode data error");
                    return;
                }

                if (tmxMapInfo->getLayerAttribs() & (TMXLayerAttribGzip | TMXLayerAttribZlib))
                {
                    unsigned char *deflated = nullptr;
                    Size s = layer->_layerSize;
                    // int sizeHint = s.width * s.height * sizeof(uint32_t);
                    ssize_t sizeHint = s.width * s.height * sizeof(unsigned int);

                    ssize_t CC_UNUSED inflatedLen = ZipUtils::inflateMemoryWithHint(buffer, len, &deflated, sizeHint);
                    CCASSERT(inflatedLen == sizeHint, "inflatedLen should be equal to sizeHint!");

                    free(buffer);
                    buffer = nullptr;

                    if (!deflated)
                    {
                        CCLOG("cocos2d: TiledMap: inflate data error");
                        return;
                    }

                    layer->_tiles = reinterpret_cast<uint32_t*>(deflated);
                }
                else
                {
                    layer->_tiles = reinterpret_cast<uint32_t*>(buffer);
                }

                tmxMapInfo->setCurrentString("");
            }
            else if (tmxMapInfo->getLayerAttribs() & TMXLayerAttribCSV)
            {
                unsigned char *buffer;

                TMXLayerInfoEx* layer = tmxMapInfo->getLayers().back();

                tmxMapInfo->setStoringCharacters(false);
                std::string currentString = tmxMapInfo->getCurrentString();

                vector<string> gidTokens;
                istringstream filestr(currentString);
                string sRow;
                while(getline(filestr, sRow, '\n')) {
                    string sGID;
                    istringstream rowstr(sRow);
                    while (getline(rowstr, sGID, ',')) {
                        gidTokens.push_back(sGID);
                    }
                }

                // 32-bits per gid
                buffer = (unsigned char*)malloc(gidTokens.size() * 4);
                if (!buffer)
                {
                    CCLOG("cocos2d: TiledMap: CSV buffer not allocated.");
                    return;
                }

                uint32_t* bufferPtr = reinterpret_cast<uint32_t*>(buffer);
                for(const auto& gidToken : gidTokens) {
                    auto tileGid = (uint32_t)strtoul(gidToken.c_str(), nullptr, 10);
                    *bufferPtr = tileGid;
                    bufferPtr++;
                }

                layer->_tiles = reinterpret_cast<uint32_t*>(buffer);

                tmxMapInfo->setCurrentString("");
            }
            else if (tmxMapInfo->getLayerAttribs() & TMXLayerAttribNone)
            {
                _xmlTileIndex = 0;
            }
        }
        else if (elementName == "map")
        {
            // The map element has ended
            tmxMapInfo->setParentElement(TMXPropertyNone);
        }
        else if (elementName == "layer")
        {
            // The layer element has ended
            tmxMapInfo->setParentElement(TMXPropertyNone);
        }
        else if (elementName == "objectgroup")
        {
            // The objectgroup element has ended
            tmxMapInfo->setParentElement(TMXPropertyNone);
        }
        else if (elementName == "object")
        {
            // The object element has ended
            tmxMapInfo->setParentElement(TMXPropertyNone);
        }
        else if (elementName == "tileset")
        {
            _recordFirstGID = true;
        }
        else if (elementName == "animation")
        {
            tmxMapInfo->setParentElement(TMXPropertyNone);
        }
    }

    const Vector<TMXLayerInfoEx *> &TMXMapInfoEx::getLayers() const
    {
	    return _layers;
    }

    Vector<TMXLayerInfoEx *> &TMXMapInfoEx::getLayers()
    {
        return _layers;
    }

    void TMXMapInfoEx::setLayers(const Vector<TMXLayerInfoEx *> &layers)
    {
	    _layers = layers;
    }

    bool TMXMapInfoEx::initWithTMXFileEx(const std::string &tmxFile)
    {
        internalInit(tmxFile, "");
        return parseXMLFile(_TMXFileName);
    }

    Vector<TMXTilesetInfoEx *> &TMXMapInfoEx::getTilesets()
    {
        return _tilesets;
    }

    void TMXMapInfoEx::setTilesets(const Vector<TMXTilesetInfoEx *> &tilesets)
    {
            _tilesets = tilesets;
    }


    void TMXTiledMapEx::buildWithMapInfoEx(TMXMapInfoEx *mapInfo)
    {
        _mapSize = mapInfo->getMapSize();
        _tileSize = mapInfo->getTileSize();
        _mapOrientation = mapInfo->getOrientation();

        _objectGroups = mapInfo->getObjectGroups();

        _properties = mapInfo->getProperties();

        _tileProperties = mapInfo->getTileProperties();

        int idx = 0;

        auto& layers = mapInfo->getLayers();
        for (const auto &layerInfo : layers) {
            // TODO: 这里其实可以提供一个选项，是否生成不可见图层
            if (layerInfo->_visible) {
                TMXLayerEx *child = parseLayer(layerInfo, mapInfo);
                if (child == nullptr) {
                    idx++;
                    continue;
                }
                addChild(child, idx, idx);
                // update content size with the max size
                const Size& childSize = child->getContentSize();
                Size currentSize = this->getContentSize();
                currentSize.width = std::max(currentSize.width, childSize.width);
                currentSize.height = std::max(currentSize.height, childSize.height);
                this->setContentSize(currentSize);

                idx++;
            }
        }
        _tmxLayerNum = idx;
    }

    TMXLayerEx *TMXTiledMapEx::parseLayer(TMXLayerInfoEx *layerInfo, TMXMapInfoEx *mapInfo)
    {
        TMXTilesetInfoEx *tileset = tilesetForLayer(layerInfo, mapInfo);
        if (tileset == nullptr)
            return nullptr;

        TMXLayerEx *layer = TMXLayerEx::create(tileset, layerInfo, mapInfo);

        if (nullptr != layer)
        {
            // tell the layerinfo to release the ownership of the tiles map.
            layerInfo->_ownTiles = false;
            layer->setupTilesEx();
        }

        return layer;
    }

    TMXTilesetInfoEx *TMXTiledMapEx::tilesetForLayer(TMXLayerInfoEx *layerInfo, TMXMapInfoEx *mapInfo)
    {
	    // FIXME: 在这个函数中，如果一个图层是空图层，就会被优化掉
        auto height = static_cast<uint32_t>(layerInfo->_layerSize.height);
        auto width  = static_cast<uint32_t>(layerInfo->_layerSize.width);
        auto& tilesets = mapInfo->getTilesets();

        for (auto iter = tilesets.crbegin(), end = tilesets.crend(); iter != end; ++iter)
        {
            TMXTilesetInfoEx* tileset = *iter;

            if (tileset)
            {
                for (uint32_t y = 0; y < height; y++)
                {
                    for (uint32_t x = 0; x < width; x++)
                    {
                        auto pos = x + width * y;
                        auto gid = layerInfo->_tiles[ pos ];

                        // FIXME:: gid == 0 --> empty tile
                        if (gid != 0)
                        {
                            // Optimization: quick return
                            // if the layer is invalid (more than 1 tileset per layer)
                            // an CCAssert will be thrown later
                            if (tileset->_firstGid < 0 ||
                                (gid & kTMXFlippedMask) >= static_cast<uint32_t>(tileset->_firstGid))
                                return tileset;
                        }
                    }
                }
            }
        }

        // If all the tiles are 0, return empty tileset
        CCLOG("cocos2d: Warning: TMX Layer '%s' has no tiles", layerInfo->_name.c_str());

        return nullptr;
    }

    void TMXTiledMapEx::update(float dt)
    {
        //Node::update(dt);
        TMXTiledMap::update(dt);
        for (auto& child : _children)
        {
            TMXLayerEx* layer = dynamic_cast<TMXLayerEx*>(child);
            if(layer && layer->_hasAnimatedTiles)
            {
                layer->tickAnimationTiles(dt);
            }
        }
    }

    void TMXTiledMapEx::onEnter()
    {
	    Node::onEnter();
	    scheduleUpdate();
    }


    TMXAnimation::TMXAnimation(uint32_t tileID)
    {
	    _tileID = tileID;
    }

    TMXFrame::TMXFrame(uint32_t tileID, float duration)
    {
	    _tileID = tileID; // 实际上是gid
	    _duration = duration;
    }
}

