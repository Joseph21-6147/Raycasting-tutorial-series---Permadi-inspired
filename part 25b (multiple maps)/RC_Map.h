#ifndef RC_MAP_H
#define RC_MAP_H

//////////////////////////////////  RC_Map   //////////////////////////////////////////

/* The game map is an RC_Map object, which is a 3D grid having a width (x-size), a height (y-size)
 * and a nr of layers (z-size). The structure is divided in discrete cells (in 2D we would call it
 * tiled, here "blocked" is more appropriate), it's components (blocks) are modeled by RC_MapCell objects.
 *
 * A map cell object is either empty, or it is not empty, in which case it contains six faces.
 *
 * In it's most basic form, a face is just a texture. It can be more advanced, like an animated face.
 * Either way it uses sprites from the sprite lists for walls, roofs, and ceilings. These sprite lists
 * are defined in the map definition file.
 */

#include "RC_MapCell.h"

// ==============================/  class RC_Map  /==============================


class RC_Map {

private:
    int nMapID = -1;     // the map ID is also its index in the vMaps[] vector
    int nMapX  =  0;     // dimensions for the map
    int nMapY  =  0;
    int nMapZ  =  0;

    std::vector<std::vector<RC_MapCell *>> bMaps;  // all map cell is stored per tile in an RC_MapCell type (derived) object
    olc::Sprite *pFloorSpritePtr = nullptr;        // a pointer to the sprite that is used as floor texture
    olc::Pixel SkyColour = olc::CYAN;              // the colour that is used to paint the sky

public:

    RC_Map();

    ~RC_Map();

    // First initialize the map calling this method ...
    void InitMap( int nID, olc::Sprite *floorTxtr = nullptr, olc::Pixel skyCol = olc::CYAN );
    // ... then add nSizeZ (at least 1) layers to it using this method
    void AddLayer( std::vector<std::string> &sUserMap, std::vector<olc::Sprite *> &vWallTextures,
                                                       std::vector<olc::Sprite *> &vCeilTextures,
                                                       std::vector<olc::Sprite *> &vRoofTextures );

    // method to clean up the object before it gets out of scope
    void FinalizeMap();

    // getters for map width and height
    int GetID();
    int GetWidth();
    int GetHeight();
    // returns the diagonal length of the map - useful for setting max distance value
    float DiagonalLength();
    float DiagonalLength3D();
    // returns current number of layers in this map object - use fMaps as representative model
    int NrOfLayers();

    // returns whether (x, y) is within map boundaries
    bool IsInBounds( float x, float y );
    // returns whether (x, y, z) is within map boundaries (z is the layer height)
    bool IsInBounds( float x, float y, float z );

    // getter for (cumulated) cell height at coordinates (x, y)
    // Note - there's no intuitive meaning for this method in maps with holes
    float CellHeight( int x, int y );
    // getter for obtaining height value of the cell at layer, coordinates (x, y)
    float CellHeightAt( int x, int y, int layer );

    // getter for obtaining the character value of the cell at layer, coordinates (x, y)
    char CellValueAt( int x, int y, int layer );

    // getter for obtaining the pointer to the associated block of the cell at layer, coordinates (x, y)
    RC_MapCell *MapCellPtrAt( int x, int y, int layer );

    // collision detection on the map:
    // Note that int( fH ) denotes layer to check, and (fH - int( fH )) denotes height to check within that layer
    // and fR is the radius of the object (considered as a pillar shape)
    bool Collides( float fX, float fY, float fH, float fR, float fVX, float fVY );

    void SetFloorSpritePtr( olc::Sprite *pSpritePtr );
    olc::Sprite *GetFloorSpritePtr();

    void SetSkyColour( olc::Pixel col );
    olc::Pixel GetSkyColour();
};

#endif // RC_MAP_H
