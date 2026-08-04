require "process"
require "file"
require "dir"
require "random"

module ImageExporter
  ASEPRITE_CMD = "aseprite"
  BUILD_DIR    = "build"

  # __DIR__ points to "toolchain/src/export/images"
  # This resolves cleanly to the full path of "aseprite_to_bytes.lua"
  LUA_SCRIPT   = File.expand_path("aseprite_to_bytes.lua", __DIR__)

  struct FrameData
    property offset : Int32
    property len : Int32
    property width : Int32
    property height : Int32
    property duration_ms : Int32

    def initialize(@offset, @len, @width, @height, @duration_ms)
    end
  end

  struct TagData
    property name : String
    property from_frame : Int32
    property to_frame : Int32
    property loop : Bool

    def initialize(@name, @from_frame, @to_frame, @loop)
    end
  end

  struct SpriteData
    property name : String
    property palette : Array(String)
    property compressed_size : Int32
    property c_array_string : String
    property width : Int32
    property height : Int32
    property frames : Array(FrameData)
    property tags : Array(TagData)

    def initialize(@name, @palette, @compressed_size, @c_array_string, @width, @height, @frames, @tags)
    end
  end

  def self.process_sprite(sprite_path : String, master_palette_path : String = "") : SpriteData
    Dir.mkdir_p(BUILD_DIR) unless Dir.exists?(BUILD_DIR)

    raw_data_file = File.join(BUILD_DIR, "temp_#{Random::Secure.hex}.bin")
    palette_file  = File.join(BUILD_DIR, "temp_#{Random::Secure.hex}.txt")
    meta_file     = File.join(BUILD_DIR, "temp_#{Random::Secure.hex}.meta")

    begin
      base_name   = File.basename(sprite_path, File.extname(sprite_path))
      symbol_name = base_name.downcase.gsub(/[^a-z0-9_]/, "_")

      args = [
        "-b", sprite_path,
        "--script-param", "filename=#{raw_data_file}",
        "--script-param", "palette=#{palette_file}",
        "--script-param", "meta=#{meta_file}",
      ]

      # Pass master palette path to Lua so color_to_index uses the canonical GPL palette
      if !master_palette_path.empty? && File.exists?(master_palette_path)
        args += ["--script-param", "master_palette=#{File.expand_path(master_palette_path)}"]
      end

      args += ["--script", LUA_SCRIPT]

      res = Process.run(
        ASEPRITE_CMD,
        args,
        output: Process::Redirect::Inherit,
        error: Process::Redirect::Inherit
      )

      unless res.success? && File.exists?(raw_data_file) && File.exists?(palette_file) && File.exists?(meta_file)
        STDERR.puts "\n❌ Error: Aseprite export pipeline failed for #{sprite_path}."
        STDERR.puts "   Lua script target: #{LUA_SCRIPT}"
        exit(1)
      end

      # Parse Meta File
      width = 0
      height = 0
      frame_count = 0
      tag_count = 0
      raw_frame_metas = [] of Tuple(Int32, Int32, Int32, Int32)
      parsed_tags = [] of TagData

      File.each_line(meta_file) do |line|
        parts = line.strip.split
        next if parts.empty?

        case parts[0]
        when "HEADER"
          width = parts[1].to_i
          height = parts[2].to_i
          frame_count = parts[3].to_i
          tag_count = parts[4].to_i
        when "FRAME"
          f_idx = parts[1].to_i
          dur_ms = parts[2].to_i
          r_off = parts[3].to_i
          r_len = parts[4].to_i
          raw_frame_metas << {f_idx, dur_ms, r_off, r_len}
        when "TAG"
          tag_name = parts[1]
          from_f = parts[2].to_i
          to_f = parts[3].to_i
          is_loop = parts[4].to_i == 1
          parsed_tags << TagData.new(tag_name, from_f, to_f, is_loop)
        end
      end

      raw_pixels = File.open(raw_data_file, &.gets_to_end).to_slice

      # Per-Frame RLE Compression
      rle_bytes = Bytes.new(raw_pixels.size * 2)
      rle_idx = 0
      parsed_frames = [] of FrameData

      raw_frame_metas.each do |f_idx, dur_ms, r_off, r_len|
        frame_pixels = raw_pixels[r_off, r_len]
        frame_rle_offset = rle_idx

        if frame_pixels.size > 0
          current_color = frame_pixels[0]
          run_length = 1

          (1...frame_pixels.size).each do |i|
            pixel = frame_pixels[i]
            if pixel == current_color && run_length < 255
              run_length += 1
            else
              rle_bytes[rle_idx] = run_length.to_u8; rle_idx += 1
              rle_bytes[rle_idx] = current_color;    rle_idx += 1
              current_color = pixel
              run_length = 1
            end
          end
          rle_bytes[rle_idx] = run_length.to_u8; rle_idx += 1
          rle_bytes[rle_idx] = current_color;    rle_idx += 1
        end

        frame_rle_len = rle_idx - frame_rle_offset
        parsed_frames << FrameData.new(frame_rle_offset, frame_rle_len, width, height, dur_ms)
      end

      # Parse Palette
      colors = [] of String
      File.each_line(palette_file) do |line|
        next if line.starts_with?("#") || line.strip.empty?
        next unless line.split.first?.try &.to_i?
        parts = line.split
        if parts.size >= 4
          colors << "0x%02X%02X%02X%02X" % {parts[3].to_i, parts[0].to_i, parts[1].to_i, parts[2].to_i}
        elsif parts.size >= 3
          colors << "0xFF%02X%02X%02X" % {parts[0].to_i, parts[1].to_i, parts[2].to_i}
        end
      end

      SpriteData.new(
        name: symbol_name,
        palette: colors,
        compressed_size: rle_idx,
        c_array_string: rle_bytes[0...rle_idx].map(&.to_s).join(", "),
        width: width,
        height: height,
        frames: parsed_frames,
        tags: parsed_tags
      )
    ensure
      File.delete(raw_data_file) if File.exists?(raw_data_file)
      File.delete(palette_file) if File.exists?(palette_file)
      File.delete(meta_file) if File.exists?(meta_file)
    end
  end
end
