# toolchain/src/export/psf_to_txt.cr
# Usage: crystal pack_psf_to_txt.cr font.psf font_editable.txt

filepath = ARGV[0]
outpath = ARGV[1]
file_bytes = File.read(filepath).bytes

# Parse PSF1/PSF2 header to find raw glyph bytes
glyph_start_offset = 0
char_height = 8
if file_bytes[0] == 0x36 && file_bytes[1] == 0x04
  glyph_start_offset = 4
  char_height = file_bytes[3].to_i
end

glyphs = file_bytes[glyph_start_offset..-1]

output = IO::Memory.new

fonts_header = String.build do |str|
  str << "# Master 8x8 Pixel Font Definition File\n"
  str << "# Format: Each character maps to its ASCII character representation, using '.' for empty and '#' for filled pixels.\n"
  str << "\n"
  str << "# --- METADATA - REQUIRED\n"
  str << "# size: 8\n"
  str << "# spacing: 8\n"
  str << "\n"
  str << "\n"
end

output.puts(fonts_header)

128.times do |ascii_val|
  # CHAR: '!' (ASCII 33)
  output.puts("# CHAR: '#{ascii_val >= 32 ? ascii_val.chr : ' '}' (ASCII #{ascii_val})")
  char_height.times do |row|
    byte_offset = (ascii_val * char_height) + row
    b = byte_offset < glyphs.size ? glyphs[byte_offset] : 0_u8
    # Convert byte to string of '.' and '#'
    output.puts(b.to_s(2).rjust(8, '0').tr("01", ".#"))
  end

  output.puts
  output.puts
end

File.write(outpath, output.to_s)
puts "Exported editable text grid to #{outpath}"
