module FontPsfExporter
  def self.export(input_file : String) : String
    var_name = File.basename(input_file, File.extname(input_file)).downcase.gsub(/[^a-z0-9_]/, "_")

    file_bytes = File.read(input_file).bytes

    glyph_start_offset = 0
    char_height = 8

    if file_bytes[0] == 0x36 && file_bytes[1] == 0x04
      # PSF1 Header
      glyph_start_offset = 4
      char_height = file_bytes[3].to_i
    elsif file_bytes[0] == 0x72 && file_bytes[1] == 0xb5 && file_bytes[2] == 0x4a && file_bytes[3] == 0x86
      # PSF2 Header
      glyph_start_offset = (file_bytes[16] | (file_bytes[17] << 8) | (file_bytes[18] << 16) | (file_bytes[19] << 24)).to_i
      char_height = (file_bytes[24] | (file_bytes[25] << 8) | (file_bytes[26] << 16) | (file_bytes[27] << 24)).to_i
    end

    glyphs = file_bytes[glyph_start_offset..-1]

    font_spacing = char_height

    # Build 128 chars x 16 rows, padded with zeros for FontData compatibility
    font_data = Array.new(128) { Array.new(16, 0_u16) }

    128.times do |ascii_val|
      char_height.times do |row|
        byte_offset = (ascii_val * char_height) + row
        byte_val = byte_offset < glyphs.size ? glyphs[byte_offset].to_u16 : 0_u16
        font_data[ascii_val][row] = byte_val
      end
    end

    # PSF glyphs are always 8 pixels wide
    format_binary = ->(val : UInt16) {
      b = val.to_s(2).rjust(8, '0')
      "0b#{b[0..3]}'#{b[4..7]}"
    }

    String.build do |str|
      str << "        inline constexpr FontData #{var_name} = {\n"
      str << "            .size = #{char_height},\n"
      str << "            .spacing = #{font_spacing},\n"
      str << "            .data = {\n"
      font_data.each_with_index do |rows, char_idx|
        str << "                {\n"
        rows.each_with_index do |row_val, r_idx|
          str << "                    #{format_binary.call(row_val)}"
          str << "," if r_idx < 15
          str << "\n"
        end
        str << "                },"
        str << " // ASCII #{char_idx}\n"
      end
      str << "            }\n"
      str << "        };\n\n"
    end
  end
end
