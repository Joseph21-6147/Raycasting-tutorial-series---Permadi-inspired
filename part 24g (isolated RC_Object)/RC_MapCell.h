#ifndef RC_MAPCELL_H
#define RC_MAPCELL_H

#include "RC_Face.h"

//////////////////////////////////  MAP CELL BLUEPRINTS  //////////////////////////////////////

/* For faces and map cells you can define blueprints, that are used to build up the map. So the face blueprints are the
 * components for dressing the map cell blueprints, which in turn are used to define the map.
 *
 * This way you can define a character based map and have all kinds of behaviour in it: textured, animated
 *
 * See also the description in RC_Map.h
 */

// ==============================/  definition of MapCellBluePrint  /==============================

// A "MapCellBluePrint" object is a combination of
//   + a character identifying that map cell in the map definition
//   + a specific height
//   + 6 faces (EAST through BOTTOM, see constants in RC_Face.h) containing indexes into the vFaceBluePrintLib
//   + a number of flags denoting the characteristics of the map cell
typedef struct sMapCellBluePrintStruct {
    char  cID;
    float fHeight;
    int   nFaces[ FACE_NR_OF ];    // values index into vFaceBluePrintLib !

    bool bPermeable = false;       // can player move through the map cell?
    bool bEmpty     = false;
} MapCellBluePrint;

// This list contains the data to initialize the map cell blueprint library
extern std::vector<MapCellBluePrint> vInitMapCellBluePrints;
// The library of map cells is modeled as a std::map, for fast (O(n log n)) searching
extern std::map<char, MapCellBluePrint> mMapCellBluePrintLib;
// Convenience function to add one map cell configuration - enables error checking on input data
void AddMapCellBluePrint( MapCellBluePrint &rMBP );
// Uses the data from vInitMapCellBluePrints to populate the library of map cells (mMapCellBluePrintLib)
// This construction decouples the blue print definition from its use, and enables error checking
// on the blue print data
void InitMapCellBluePrints();

// return a reference to the map cell in the library having id cID
MapCellBluePrint &GetMapCellBluePrint( char cID );

//////////////////////////////////  RC_MapCell   //////////////////////////////////////////

/* An RC_MapCell object is either empty (in which case it's just a placeholder to prevent nullptrs), or it consists
 * of 6 faces (East, North, West, South, Top, Bottom). These faces are modeled by RC_Face objects.
 */

// ==============================/  class RC_MapCell  /==============================

class RC_MapCell {

protected:
    int x, y, layer;           // the tile coordinate and layer of this block in the map
    char id      = '.';        // identifying character in the map
    float height = 0.0f;       // for empty blocks height must be 0.0f and vice versa
    bool bEmpty  = true;

    // below members have no meaning for empty blocks
    RC_Face *pFaces[ FACE_NR_OF ] = { nullptr };   // array of pointers to faces
    bool bPermeable = false;                      // can player pass through the cell?

public:
    RC_MapCell();
    virtual ~RC_MapCell();

    virtual void Init( int px, int py, int l );

    int GetX();
    int GetY();
    int GetLayer();

    void SetX( int px );
    void SetY( int py );
    void SetLayer( int nLayer );

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
