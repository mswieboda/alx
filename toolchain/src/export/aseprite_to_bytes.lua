local sprite = app.activeSprite
if not sprite then
    io.stderr:write("Error: No active sprite found.\n")
    os.exit(1)
end

local target_filename = app.params["filename"]
local palette_filename = app.params["palette"]

if not target_filename or not palette_filename then
    io.stderr:write("Error: Missing parameters.\n")
    os.exit(1)
end

-- 1. DRAW SPRITE TO A FRESH RGB IMAGE BUFFER
-- This automatically flattens all visible layers into true RGBA values!
local img = Image(sprite.width, sprite.height, ColorMode.RGB)
img:drawSprite(sprite, 1) -- Draws frame 1 (1-based index in Lua)

-- Build color_to_index lookup from master palette (GPL file) if provided,
-- otherwise scan the flattened image pixels to build a dynamic palette.
local color_to_index = {}
-- index_to_rgba stores {r, g, b, a} for palette text export
local index_to_rgba = {}

local master_palette_file = app.params["master_palette"]
if master_palette_file then
    -- Parse GPL file: lines are "  R   G   B  A\tLabel"
    local f = io.open(master_palette_file, "r")
    if f then
        local idx = 0
        for line in f:lines() do
            local r, g, b, a = line:match("^%s*(%d+)%s+(%d+)%s+(%d+)%s+(%d+)")
            if r then
                local ri, gi, bi, ai = tonumber(r), tonumber(g), tonumber(b), tonumber(a)
                local key = string.format("%d,%d,%d,%d", ri, gi, bi, ai)
                if color_to_index[key] == nil then
                    color_to_index[key] = idx
                    index_to_rgba[idx] = {ri, gi, bi, ai}
                end
                idx = idx + 1
            else
                -- Try RGB-only line (no alpha column)
                local r3, g3, b3 = line:match("^%s*(%d+)%s+(%d+)%s+(%d+)%s*$")
                if r3 then
                    local ri, gi, bi = tonumber(r3), tonumber(g3), tonumber(b3)
                    local key = string.format("%d,%d,%d,255", ri, gi, bi)
                    if color_to_index[key] == nil then
                        color_to_index[key] = idx
                        index_to_rgba[idx] = {ri, gi, bi, 255}
                    end
                    idx = idx + 1
                end
            end
        end
        f:close()
    end
else
    -- Fallback: scan actual rendered pixel colors from the flattened image.
    -- This works correctly for RGB mode sprites with any colors, since
    -- sprite.palettes[1] is decorative in RGB mode and may not match pixels.
    -- Reserve index 255 for fully transparent pixels.
    local next_idx = 0
    for y = 0, sprite.height - 1 do
        for x = 0, sprite.width - 1 do
            local raw_pixel = img:getPixel(x, y)
            local r = app.pixelColor.rgbaR(raw_pixel)
            local g = app.pixelColor.rgbaG(raw_pixel)
            local b = app.pixelColor.rgbaB(raw_pixel)
            local a = app.pixelColor.rgbaA(raw_pixel)
            if a ~= 0 then
                local key = string.format("%d,%d,%d,%d", r, g, b, a)
                if color_to_index[key] == nil and next_idx < 255 then
                    color_to_index[key] = next_idx
                    index_to_rgba[next_idx] = {r, g, b, a}
                    next_idx = next_idx + 1
                end
            end
        end
    end
end

-- 2. EXPORT RAW PIXEL DATA
local file = io.open(target_filename, "wb")
if not file then
    io.stderr:write("Error: Could not open output file.\n")
    os.exit(1)
end

for y = 0, sprite.height - 1 do
    for x = 0, sprite.width - 1 do
        local color_byte = 0

        if x < img.width and y < img.height then
            local raw_pixel = img:getPixel(x, y)

            -- Extract true RGBA components from the pure RGB image buffer
            local r = app.pixelColor.rgbaR(raw_pixel)
            local g = app.pixelColor.rgbaG(raw_pixel)
            local b = app.pixelColor.rgbaB(raw_pixel)
            local a = app.pixelColor.rgbaA(raw_pixel)

            -- If pixel is fully transparent, treat it as color index 255
            -- (which defaults to 0x00FF00FF / Magenta transparent in our engine)
            if a == 0 then
                color_byte = 255
            else
                local key = string.format("%d,%d,%d,%d", r, g, b, a)
                color_byte = color_to_index[key] or 0
            end
        end

        file:write(string.char(color_byte))
    end
end
file:close()

-- 3. EXPORT RAW PALETTE TEXT
-- Always write from index_to_rgba so the palette file matches what the pixel
-- data actually references, regardless of color mode or whether GPL was used.
local pal_file = io.open(palette_filename, "w")
if pal_file then
    for i = 0, 254 do
        local c = index_to_rgba[i]
        if c then
            pal_file:write(string.format("%d %d %d %d\n", c[1], c[2], c[3], c[4]))
        else
            break
        end
    end
    pal_file:close()
end

