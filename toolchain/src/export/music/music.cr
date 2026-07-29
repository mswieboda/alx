module MusicExporter
  def self.export(input_file : String) : String
    bytes = File.read(input_file).to_slice
    base_name = File.basename(input_file, File.extname(input_file))
    var_name = base_name.downcase.gsub(/[^a-zA-Z0-9_]/, "_")

    String.build do |str|
      str << "        inline const uint8_t #{var_name}[] = {\n"
      bytes.each_with_index do |byte, index|
        str << "            " if index % 12 == 0
        str << sprintf("0x%02x", byte)
        str << ", " unless index == bytes.size - 1

        str << "\n" if (index + 1) % 12 == 0 || index == bytes.size - 1
      end
      str << "        };\n"
      str << "        inline const size_t #{var_name}_len = #{bytes.size};\n\n"
    end
  end
end
