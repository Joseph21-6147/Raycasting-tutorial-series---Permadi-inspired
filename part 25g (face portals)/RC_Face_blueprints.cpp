#include "RC_Face.h"

// ==============================/  face blue print stuff   /==============================

// this list contains the data to initialise the face blue print library
std::vector<FaceBluePrint> vInitFaceBluePrints = {

    // +-------------------------------------------- ID of this blueprint - must be sequential since it is
    // |                                             also used as index into face blueprint library
    // |        +----------------------------------- face type (one of TYPE_FACE_WALL / _ROOF / _CEIL)
    // |        |          +------------------------ index into wall/roof/ceiling sprite list
    // |        |          |    +------------------- flags whether face is transparent
    // |        |          |    |      +------------ flags whether face is animated
    // |        |          |    |      |      +----- flags whether face is a portal
    // |        |          |    |      |      |
    // V        V          V    V      V      V

    {  0, TYPE_FACE_WALL,  0, false, false, false },
    {  1, TYPE_FACE_WALL,  1, false, false, false },
    {  2, TYPE_FACE_WALL,  2, false, false, false },
    {  3, TYPE_FACE_WALL,  3, false, false, false },
    {  4, TYPE_FACE_WALL,  4, true , true , false },    // animated gate blueprint
    {  5, TYPE_FACE_WALL,  5, false, false, false },
    {  6, TYPE_FACE_WALL,  6, true , false, false },    // transparent, but not animated
    {  7, TYPE_FACE_WALL,  7, true , false, false },
    {  8, TYPE_FACE_WALL,  8, true , false, true  },    // portal face
    {  9, TYPE_FACE_WALL,  0, false, false, false },    // fill out so that roof textures start at index 10

    { 10, TYPE_FACE_ROOF,  0, false, false, false },
    { 11, TYPE_FACE_ROOF,  1, false, false, false },
    { 12, TYPE_FACE_ROOF,  2, false, false, false },
    { 13, TYPE_FACE_ROOF,  3, true , false, false },
    { 14, TYPE_FACE_ROOF,  4, false, false, false },
    { 15, TYPE_FACE_ROOF,  5, false, false, false },
    { 16, TYPE_FACE_ROOF,  6, true , false, false },
    { 17, TYPE_FACE_ROOF,  7, false, false, false },
    { 18, TYPE_FACE_ROOF,  0, false, false, false },    // fill out so that ceiling textures start at index 20
    { 19, TYPE_FACE_ROOF,  0, false, false, false },

//    { 20, TYPE_FACE_ROOF,  3, true , false, false },    // for testing transparent ceilings
    { 20, TYPE_FACE_CEIL,  0, false, false, false },
    { 21, TYPE_FACE_CEIL,  1, false, false, false },
    { 22, TYPE_FACE_CEIL,  2, false, false, false },
    { 23, TYPE_FACE_CEIL,  3, false, false, false },
    { 24, TYPE_FACE_CEIL,  4, false, false, false },
    { 25, TYPE_FACE_CEIL,  5, false, false, false },
    { 26, TYPE_FACE_CEIL,  6, false, false, false },
    { 27, TYPE_FACE_CEIL,  7, false, false, false },
    { 28, TYPE_FACE_CEIL,  0, false, false, false },
    { 29, TYPE_FACE_CEIL,  0, false, false, false },
};

// ==============================/  end of file   /==============================
