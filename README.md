# cocos2dx-animated-tile-support
A new `TMXTiledmapEx` class that supports animated tile of TMX tiledmap in layer-level. Animated tiles are still rendered with `SpriteBatchNode`.

Now this work has beeb merged into Cocos2d-x, it can support animated tiles nativly, with another way to tick the animation.

## How To Use

Add `TMXPropertyAnimation` into the seconed enum of CCTMXXMLParser.h

Then just copy everything into your project and use `pm::TMXTiledmapEx` in the same way like `cocos2d::TMXTiledmap`.

## ShowCase

![In Tiled editor](https://github.com/DEAGS3000/cocos2dx-animated-tile-support/raw/master/showcase-tiled.gif)

![In App](https://github.com/DEAGS3000/cocos2dx-animated-tile-support/raw/master/showcase-app.gif)



