// small test and development map - use about 32 objects with it

std::vector<std::string> vWallSpriteFiles = {
    "sprites/Rock-wall.png",
    "../sprites/new wall_brd.png",
    "textures 128x128/Bricks/Bricks_01-128x128.png",
    "textures 128x128/Bricks/Bricks_02-128x128.png",
//    "sprites/Rock-door.rbg.png",
    "sprites/Rock-gate-closed2.rbg.png",
    "sprites/wall01.png",
    "sprites/Rock-window.rbg.png",
    "sprites/Rock-barred-window.rbg.png"
};

std::vector<std::string> vCeilSpriteFiles = {
    "../sprites/ceiling_texture.png",
    "textures 128x128/Wood/Wood_03-128x128.png",
    "textures 128x128/Wood/Wood_05-128x128.png",
    "textures 128x128/Wood/Wood_13-128x128.png",
    "sprites/wood.png",
    "sprites/greystone.png",
    "sprites/floor2.png",
    "../sprites/wood.png",
};

std::vector<std::string> vRoofSpriteFiles = {
    "../sprites/roof texture.png",
    "textures 128x128/Roofs/Roofs_07-128x128.png",
    "textures 128x128/Roofs/Roofs_11-128x128.png",
    "textures 128x128/Roofs/Roofs_19-128x128.png",
    "../sprites/wood.png",
    "sprites/floor2.png",
    "sprites/wood.png",
    "../sprites/ceiling_texture.png",
};

std::vector<std::string> vFloorSpriteFiles = {
    "textures 128x128/Grass/Grass_02-128x128.png",
    "../sprites/grass_texture.png",
};

std::vector<std::string> vObjectSpriteFiles = {
    "sprites/elf-girl_stationary-front.rbg.png",    // elf girl

    "sprites/barrel.rbg.png",                       // stationary objects
    "sprites/pillar.rbg.png",

    "sprites/bush_object_01.rbg.png",               // bushes
    "sprites/bush_object_02.rbg.png",
    "sprites/bush_object_03.rbg.png",
    "sprites/bush_object_04.rbg.png",

    "sprites/tree_object_01.rbg.png",               // trees
    "sprites/tree_object_02.rbg.png",
    "sprites/tree_object_03.rbg.png",
    "sprites/tree_object_04.rbg.png",
    "sprites/tree_object_05.rbg.png",
    "sprites/tree_object_06.rbg.png",
    "sprites/tree_object_07.rbg.png",
    "sprites/tree_object_08.rbg.png",
};


// map dimensions
int glbMapX = 16;
int glbMapY = 16;


std::vector<std::string> vMap_level = {

//   0         1         2                       LEVEL 0
//   012345678901234567890
    "!!!!!!$$$$$$$$.."    // 0 0
    "!....!.........#"    //   1
    "!....!.........#"    //   2
    "!!!.!!.........."    //   3
    "@..............."    //   4
    "@..............."    //   5
    "@..............#"    //   6
    "@..............#"    //   7
    "@............###"    //   8
    "@.%.........##.#"    //   9
    "@..........##..#"    // 1 0
    "@.....###$##...#"    //   1
    "@.....#........#"    //   2
    "@.....+........#"    //   3
    "......#........#"    //   4
    ".###.#####*####.",   //   5

//   0         1         2                       LEVEL 1
//   012345678901234567890
    "!!!!!!.........."    // 0 0
    "!....!.........."    //   1
    "!....!.........Q"    //   2
    "!!!!!!.........H"    //   3
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
