#ifndef RC_MAPCELL_H
#define RC_MAPCELL_H

#include "RC_Face.h"

//////////////////////////////////  MAP CELL BLUEPRINTS  //////////////////////////////////////

/* For faces and map cells you can define blueprints, that are used to define the map. So the face blueprints are the
 * components for defining the block blueprints, which are used to define the map.
 *
 * This way you can define a character based map and have all kinds of behaviour in it: textured, animated
 *
 * See also the description in RC_Map.h
 */

// ==============================/  definition of MapCellBluePrint  /==============================

// A "MapCellBluePrint" object is a combination of
//   + a character identifying that block in the map definition
//   + a specific height
//   + 6 faces (EAST through BOTTOM, see constants above) containing indexes into the vFaceBluePrintLib
//   + a (number of) flag(s) denoting the characteristics of the map cell
typedef struct sMapCellBluePrintStruct {
    char  cID;
    float fHeight;
    int   nFaces[ FACE_NR_OF ];    // values index into vFaceBluePrintLib !

    bool  bPermeable   = false;    // can player move through the block?
} MapCellBluePrint;
// The library of blocks is modeled as a map, for fast (O(n log n)) searching
extern std::map<char, MapCellBluePrint> mMapCellBluePrintLib;

// Convenience function to add one block configuration
void AddMapCellBluePrint( char cID, float fH, int nE, int nN, int nW, int nS, int nT, int nB, bool bP = false );

// Put all block configs you need into this function. Here the relation between the identifying character
// and the sprites per face are made, as well as the height of the block.
void InitMapCellBluePrints();

// return a reference to the block in the library having id cID
MapCellBluePrint &GetMapCellBluePrint( char cID );

// getters for the members of the MapCellBluePrint
char  GetMapCellBPID(     MapCellBluePrint &b );
float GetMapCellBPHeight( MapCellBluePrint &b );
int   GetMapCellBPFaceIx( MapCellBluePrint &b, int nFace );

//////////////////////////////////  RC_MapCell   //////////////////////////////////////////

/* An RC_MapCell object is either empty (in which case it's just a placeholder to prevent nullptrs), or it consists
 * of 6 faces (East, North, West, South, Top, Bottom). These faces are modeled by RC_Face objects.
 */

// ==============================/  class RC_MapCell  /==============================


class RC_MapCell {
protected:

    int x, y, level;           // the tile coordinate and level of this block in the map
    char id      = '.';   // identifying character in the map
    float height = 0.0f;  // for empty blocks height must be 0.0f and vice versa
    bool bEmpty  = true;

    // below members have no meaning for empty blocks
    RC_Face *pFaces[ FACE_NR_OF ] = { nullptr };   // array of pointers to faces
    bool bPermeable   = false;

public:
    RC_MapCell();
    virtual ~RC_MapCell();

    virtual void Init( int px, int py, int l );

    void SetX( int px );
    void SetY( int py );
    void SetLevel( int nLevel );

    int GetX();
    int GetY();
    int GetLevel();

    virtual void Update( float fElapsedTime, bool &bPermFlag );

    // if not overriden, this is an empty block and sampling always returns olc::BLANK
    virtual olc::Pixel Sample( int nFaceIx, float sX, float sY );

    char GetID();
    void SetID( char cID );

    float GetHeight();
    void  SetHeight( float fH );

    virtual bool IsEmpty();
    bool IsPermeable();

    void SetEmpty( bool bParam = true );
    void SetPermeable( bool bParam = true );

    void SetFacePtr( int nFaceIx, RC_Face *pFace );
    RC_Face *GetFacePtr( int nFaceIx );
};


#endif // RC_MAPCELL_H
