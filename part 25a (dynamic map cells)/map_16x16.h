// test and development map

// declaration of the sprite files that are loaded into the sprite libraries
// There are libs for wall sprites, ceiling and roof sprites, floor sprites and object sprites

// Their use is a bit different:
//   * wall, ceiling and roof sprites are used in defining the face blue prints
//   * floor sprites are referenced by a map directly
//   * object sprites are put in an object sprites lib

std::vector<std::string> vWallSpriteFiles = {
    "../sprites/Rock-wall.png",
    "../sprites/new wall_brd.png",
    "../textures 128x128/Bricks/Bricks_01-128x128.png",
    "../textures 128x128/Bricks/Bricks_02-128x128.png",
//    "../sprites/Rock-door.rbg.png",
//    "../sprites/Rock-gate-closed2.rbg.png",
    "../sprites/Gate-animation+wink.rbg.png",
    "../sprites/Brick-wall.png",
    "../sprites/Rock-window.rbg.png",
    "../sprites/Rock-barred-window.rbg.png"
};

std::vector<std::string> vCeilSpriteFiles = {
    "../sprites/ceiling_texture.png",
    "../textures 128x128/Wood/Wood_03-128x128.png",
    "../textures 128x128/Wood/Wood_05-128x128.png",
    "../textures 128x128/Wood/Wood_13-128x128.png",
    "../sprites/wood.png",
    "../sprites/greystone.png",
    "../sprites/floor2.png",
    "../sprites/wood.png",
};

std::vector<std::string> vRoofSpriteFiles = {
    "../sprites/roof texture.png",
    "../textures 128x128/Roofs/Roofs_07-128x128.png",
    "../textures 128x128/Roofs/Roofs_11-128x128.png",
    "../textures 128x128/Roofs/Roofs_19-128x128.png",
    "../sprites/wood.png",
    "../sprites/floor2.png",
    "../sprites/wood.png",
    "../sprites/ceiling_texture.png",
};

std::vector<std::string> vFlorSpriteFiles = {
    "../textures 128x128/Grass/Grass_02-128x128.png",
    "../sprites/grass_texture.png",
};

std::vector<std::string> vObjtSpriteFiles = {
    "../sprites/elf-girl_stationary-front.rbg.png",    // elf girl

    "../sprites/barrel.rbg.png",                       // stationary objects
    "../sprites/pillar.rbg.png",

    "../sprites/bush-object-01.rbg.png",               // bushes
    "../sprites/bush-object-02.rbg.png",
    "../sprites/bush-object-03.rbg.png",
    "../sprites/bush-object-04.rbg.png",
    "../sprites/tree-object-01.rbg.png",               // trees
    "../sprites/tree-object-02.rbg.png",
    "../sprites/tree-object-03.rbg.png",
    "../sprites/tree-object-04.rbg.png",
    "../sprites/tree-object-05.rbg.png",
    "../sprites/tree-object-06.rbg.png",
    "../sprites/tree-object-07.rbg.png",
    "../sprites/tree-object-08.rbg.png",
};


// map dimensions
int glbMapX = 16;
int glbMapY = 16;


std::vector<std::string> vMap_layer = {

//   0         1         2                       LEVEL 0
//   012345678901234567890
    "!!!!!!%%%%%%%%.."    // 0 0
    "!....!.........#"    //   1
    "!....!.........#"    //   2
    "!!!$!!.........."    //   3
    "@..............."    //   4
    "@........x......"    //   5
    "@..............#"    //   6
    "@..............#"    //   7
    "@............&&#"    //   8
    "@.$.........&&.#"    //   9
    "@..........&&..#"    // 1 0
    "@.....&&&$&&...#"    //   1
    "@.....#........#"    //   2
    "@.....+........#"    //   3
    "......#........#"    //   4
    ".###.#####*####.",   //   5

//   0         1         2                       LEVEL 1
//   012345678901234567890
    "!!!!!!.........."    // 0 0
    "!....!.........."    //   1
    "!....!.........Q"    //   2
    "!!!!!..........H"    //   3
    "...............T"    //   4
    "...............#"    //   5
    "...............#"    //   6
    "...............#"    //   7
    "...............#"    //   8
    "..%%............"    //   9
    "...............!"    // 1 0
    "................"    //   1
    "...............!"    //   2
    "................"    //   3
    "...............!"    //   4
    "..##..##.######.",   //   5

//   0         1         2                       LEVEL 2
//   012345678901234567890
    "!..............."    // 0 0
    "................"    //   1
    "................"    //   2
    "................"    //   3
    "................"    //   4
    "...............H"    //   5
    "...............#"    //   6
    "................"    //   7
    "................"    //   8
    "...%............"    //   9
    "................"    // 1 0
    "................"    //   1
    "................"    //   2
    "................"    //   3
    "................"    //   4
    "...####......#..",   //   5

////   0         1         2                       LEVEL 3
////   012345678901234567890
//    "!..............."    // 0 0
//    "................"    //   1
//    "................"    //   2
//    "................"    //   3
//    "................"    //   4
//    "................"    //   5
//    "................"    //   6
//    "................"    //   7
//    "................"    //   8
//    "..%%............"    //   9
//    "................"    // 1 0
//    "................"    //   1
//    "................"    //   2
//    "................"    //   3
//    "................"    //   4
//    "....##..........",   //   5
};
