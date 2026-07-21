module FontExporter
  def self.export(input_file : String) : String
    var_name = File.basename(input_file, File.extname(input_file)).downcase.gsub(/[^a-z0-9_]/, "_")

    font_size = 16
    font_spacing = 10

    font_data = Array.new(128) { Array.new(16, 0_u16) }
    current_ascii = -1
    current_row = 0

    File.each_line(input_file) do |line|
      line = line.strip
      next if line.empty?

      if line.starts_with?("#")
        if line =~ /#\s*size:\s*(\d+)/i
          font_size = $1.to_i
        elsif line =~ /#\s*spacing:\s*(\d+)/i
          font_spacing = $1.to_i
        end
        next
      end

      if match = line.match(/\(ASCII\s+(\d+)\)/i) || line.match(/^ASCII\s+(\d+)/i) || line.match(/^(\d+)$/)
        current_ascii = match[1].to_i
        current_row = 0
        next
      elsif line.match(/^CHAR:/i) && (match = line.match(/\(ASCII\s+(\d+)\)/i))
        current_ascii = match[1].to_i
        current_row = 0
        next
      end

      if current_ascii >= 0 && current_ascii < 128 && current_row < 16
        # Bit 15 = leftmost pixel (idx 0), Bit 0 = rightmost pixel (idx 15)
        bitmask = line.chars.each_with_index.reduce(0_u16) do |acc, (char, idx)|
          break acc if idx >= 16
          (char == '#' || char == '1') ? (acc | (1_u16 << (15 - idx))) : acc
        end

        font_data[current_ascii][current_row] = bitmask
        current_row += 1
      end
    end

    # Helper function to format 16-bit uint into 0b0000'0000'0000'0000
    format_binary = ->(val : UInt16) {
      b = val.to_s(2).rjust(16, '0')
      "0b#{b[0..3]}'#{b[4..7]}'#{b[8..11]}'#{b[12..15]}"
    }

    String.build do |str|
      str << "        inline constexpr FontData #{var_name} = {\n"
      str << "            .size = #{font_size},\n"
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
