#include "RC_Face.h"

// ==============================/  face blue print stuff   /==============================

// this list contains the data to initialise the face blue print library
std::vector<FaceBluePrint> vInitFaceBluePrints = {

    // +---------------------------------- ID of this blueprint - must be sequential since it is
    // |                                   also used as index into face blueprint library
    // |        +------------------------- face type (one of TYPE_FACE_WALL / _ROOF / _CEIL)
    // |        |          +-------------- index into wall/roof/ceiling sprite list
    // |        |          |    +--------- flags whether face is transparent
    // |        |          |    |      +-- flags whether face is animated
    // |        |          |    |      |
    // V        V          V    V      V

    {  0, TYPE_FACE_WALL,  0, false, false },
    {  1, TYPE_FACE_WALL,  1, false, false },
    {  2, TYPE_FACE_WALL,  2, false, false },
    {  3, TYPE_FACE_WALL,  3, false, false },
    {  4, TYPE_FACE_WALL,  4, true , true  },    // animated gate blueprint
    {  5, TYPE_FACE_WALL,  5, false, false },
    {  6, TYPE_FACE_WALL,  6, true , false },    // transparent, but not animated
    {  7, TYPE_FACE_WALL,  7, true , false },
    {  8, TYPE_FACE_WALL,  0, false, false },    // fill out so that roof textures start at index 10
    {  9, TYPE_FACE_WALL,  0, false, false },

    { 10, TYPE_FACE_ROOF,  0, false, false },
    { 11, TYPE_FACE_ROOF,  1, false, false },
    { 12, TYPE_FACE_ROOF,  2, false, false },
    { 13, TYPE_FACE_ROOF,  3, false, false },
    { 14, TYPE_FACE_ROOF,  4, false, false },
    { 15, TYPE_FACE_ROOF,  5, false, false },
    { 16, TYPE_FACE_ROOF,  6, false, false },
    { 17, TYPE_FACE_ROOF,  7, false, false },
    { 18, TYPE_FACE_ROOF,  0, false, false },    // fill out so that ceiling textures start at index 20
    { 19, TYPE_FACE_ROOF,  0, false, false },

    { 20, TYPE_FACE_CEIL,  0, false, false },
    { 21, TYPE_FACE_CEIL,  1, false, false },
    { 22, TYPE_FACE_CEIL,  2, false, false },
    { 23, TYPE_FACE_CEIL,  3, false, false },
    { 24, TYPE_FACE_CEIL,  4, false, false },
    { 25, TYPE_FACE_CEIL,  5, false, false },
    { 26, TYPE_FACE_CEIL,  6, false, false },
    { 27, TYPE_FACE_CEIL,  7, false, false },
    { 28, TYPE_FACE_CEIL,  0, false, false },
    { 29, TYPE_FACE_CEIL,  0, false, false },
};


// The library of faces is modeled as a vector, and can be indexed directly
std::vector<FaceBluePrint> vFaceBluePrintLib;

// Convenience function to add one face configuration
void AddFaceBluePrint( int nID, int nType, int nIndex, bool bT, bool bA ) {
    if (nID != (int)vFaceBluePrintLib.size()) {
        std::cout << "ERROR - AddFaceBluePrint() --> add order violated, id passed = " << nID << " and should have been " << (int)vFaceBluePrintLib.size() << std::endl;
    }
    FaceBluePrint sTmpFaceBP = { nID, nType, nIndex, bT, bA };
    vFaceBluePrintLib.push_back(sTmpFaceBP );
}

// Put all face configs you need into this function. Here the relation between the face index in the face blueprint lib
// and the sprite for that face are made, as well as the animation abilities of this face.
void InitFaceBluePrints() {
    // just add all elements from the input data one by one to the library of face blueprints
    for (auto &elt : vInitFaceBluePrints) {
        AddFaceBluePrint(
            elt.nID,
            elt.nFaceType,
            elt.nFaceIndex,
            elt.bTransparent,
            elt.bAnimated
        );
    }
}

// ==============================/  end of file   /==============================
