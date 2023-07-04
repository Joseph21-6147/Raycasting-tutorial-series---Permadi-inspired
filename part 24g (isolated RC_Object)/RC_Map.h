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
    int nMapX;                // dimensions for the map
    int nMapY;

    std::vector<std::vector<RC_MapCell *>> bMaps;  // all info is stored per tile in an RC_MapCell type (derived) object

public:

    RC_Map();

    ~RC_Map();

    // First initialize the map calling this method ...
    void InitMap( int nSizeX, int nSizeY );

    // ... then add at least 1 layer to it using this method
    void AddLayer( const std::string &sUserMap, std::vector<olc::Sprite *> vWallTextures,
                                                std::vector<olc::Sprite *> vCeilTextures,
                                                std::vector<olc::Sprite *> vRoofTextures );

    // method to clean up the object before it gets out of scope
    void FinalizeMap();

    // getters for map width and height
    int Width();
    int Hight();

    // returns whether (x, y) is within map boundaries
    bool IsInBounds( float x, float y );

    // getter for (cumulated) cell height at coordinates (x, y)
    // Note - there's no intuitive meaning for this method in maps with holes
    float CellHeight( int x, int y );
    // getter for obtaining height value of the cell at level, coordinates (x, y)
    float CellHeightAt( int x, int y, int level );

    // getter for obtaining the character value of the cell at level, coordinates (x, y)
    char CellValueAt( int x, int y, int level );

    // getter for obtaining the pointer to the associated block (can return nullptr) of the cell at level, coordinates (x, y)
    RC_MapCell *MapCellPtrAt( int x, int y, int level );

    // returns the diagonal length of the map - useful for setting max distance value
    float DiagonalLength();

    // returns current number of layers in this map object - use fMaps as representative model
    int NrOfLayers();

    // collision detection on the map:
    // Note that int( fH ) denotes level to check, and (fH - int( fH )) denotes height to check within that level
    // and fR is the radius of the object (considered as a pillar shape)
    bool Collides( float fX, float fY, float fH, float fR, float fVX, float fVY );
};

#endif // RC_MAP_H
