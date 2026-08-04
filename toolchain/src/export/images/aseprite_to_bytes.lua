local sprite = app.activeSprite
if not sprite then
    io.stderr:write("Error: No active sprite found.\n")
    os.exit(1)
end

local target_filename = app.params["filename"]
local palette_filename = app.params["palette"]
local meta_filename = app.params["meta"]

if not target_filename or not palette_filename then
    io.stderr:write("Error: Missing parameters.\n")
    os.exit(1)
end

local frame_count = #sprite.frames
local width = sprite.width
local height = sprite.height

-- Build color_to_index lookup from master palette (GPL file) if provided,
-- otherwise scan all rendered pixel colors across ALL frames to build a dynamic palette.
local color_to_index = {}
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
    -- Fallback: scan actual rendered pixel colors across all frames.
    local next_idx = 0
    for frame_idx = 1, frame_count do
        local img = Image(width, height, ColorMode.RGB)
        img:drawSprite(sprite, frame_idx)
        for y = 0, height - 1 do
            for x = 0, width - 1 do
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
end

-- 2. EXPORT RAW PIXEL DATA FOR ALL FRAMES SEQUENTIALLY
local file = io.open(target_filename, "wb")
if not file then
    io.stderr:write("Error: Could not open output file.\n")
    os.exit(1)
end

local frame_durations = {}

for frame_idx = 1, frame_count do
    local frame = sprite.frames[frame_idx]
    local duration_ms = math.floor((frame.duration or 0.1) * 1000.0 + 0.5)
    table.insert(frame_durations, duration_ms)

    local img = Image(width, height, ColorMode.RGB)
    img:drawSprite(sprite, frame_idx)

    for y = 0, height - 1 do
        for x = 0, width - 1 do
            local color_byte = 0
            local raw_pixel = img:getPixel(x, y)

            local r = app.pixelColor.rgbaR(raw_pixel)
            local g = app.pixelColor.rgbaG(raw_pixel)
            local b = app.pixelColor.rgbaB(raw_pixel)
            local a = app.pixelColor.rgbaA(raw_pixel)

            if a == 0 then
                color_byte = 255
            else
                local key = string.format("%d,%d,%d,%d", r, g, b, a)
                color_byte = color_to_index[key] or 0
            end

            file:write(string.char(color_byte))
        end
    end
end
file:close()

-- 3. EXPORT RAW PALETTE TEXT
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

-- 4. EXPORT METADATA FILE IF REQUESTED
if meta_filename then
    local meta_file = io.open(meta_filename, "w")
    if meta_file then
        local tag_count = #sprite.tags
        meta_file:write(string.format("HEADER %d %d %d %d\n", width, height, frame_count, tag_count))

        local frame_pixel_bytes = width * height
        for f_idx = 1, frame_count do
            local duration_ms = frame_durations[f_idx]
            local raw_offset = (f_idx - 1) * frame_pixel_bytes
            meta_file:write(string.format("FRAME %d %d %d %d\n", f_idx - 1, duration_ms, raw_offset, frame_pixel_bytes))
        end

        if tag_count > 0 then
            for _, tag in ipairs(sprite.tags) do
                local from_f = tag.fromFrame.frameNumber - 1
                local to_f = tag.toFrame.frameNumber - 1
                local is_loop = 1
                if tag.aniDir == AniDir.FORWARD_ONCE or tag.aniDir == AniDir.REVERSE_ONCE then
                    is_loop = 0
                end
                meta_file:write(string.format("TAG %s %d %d %d\n", tag.name, from_f, to_f, is_loop))
            end
        else
            meta_file:write(string.format("TAG default 0 %d 1\n", frame_count - 1))
        end

        meta_file:close()
    end
end
