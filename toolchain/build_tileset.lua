-- ============================================================================
-- NOTE: Developer Seed / Generator Utility Script
-- ============================================================================
-- This script is NOT invoked by Taskfile.yml, pack_assets.cr, or any build steps.
-- It was used as a procedural generator to programmatically author the initial
-- binary asset at `assets/images/tileset.aseprite` with 5 tagged 16x16 frames:
--   1. "floor"
--   2. "wall"
--   3. "water"
--   4. "stone"
--   5. "dirt"
--
-- How to run (manually re-generate tileset.aseprite):
--   aseprite -b --script toolchain/build_tileset.lua
-- ============================================================================

local sprite = Sprite(16, 16)

-- Colors (RGBA)
local color_floor = Color{ r=34, g=32, b=52, a=255 }
local color_wall  = Color{ r=68, g=64, b=104, a=255 }
local color_water = Color{ r=35, g=60, b=120, a=255 }
local color_stone = Color{ r=155, g=173, b=183, a=255 }
local color_dirt  = Color{ r=69, g=40, b=60, a=255 }

local function fill_img(img, bg_color)
    for y = 0, 15 do
        for x = 0, 15 do
            img:drawPixel(x, y, bg_color)
        end
    end
    local border = Color{ r=25, g=25, b=25, a=255 }
    for i = 0, 15 do
        img:drawPixel(i, 0, border)
        img:drawPixel(0, i, border)
    end
end

-- Frame 1: floor
local img1 = Image(16, 16)
fill_img(img1, color_floor)
sprite.cels[1].image = img1

-- Helper to add frame with image
local function add_tile_frame(color)
    local frame = sprite:newFrame()
    local img = Image(16, 16)
    fill_img(img, color)
    sprite:newCel(sprite.layers[1], #sprite.frames, img)
end

add_tile_frame(color_wall)
add_tile_frame(color_water)
add_tile_frame(color_stone)
add_tile_frame(color_dirt)

-- Create tags
local t1 = sprite:newTag(1, 1)
t1.name = "floor"

local t2 = sprite:newTag(2, 2)
t2.name = "wall"

local t3 = sprite:newTag(3, 3)
t3.name = "water"

local t4 = sprite:newTag(4, 4)
t4.name = "stone"

local t5 = sprite:newTag(5, 5)
t5.name = "dirt"

sprite:saveAs("assets/images/tileset.aseprite")
