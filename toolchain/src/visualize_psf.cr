# visualize_psf.cr
# Usage: crystal visualize_psf.cr font.psf

if ARGV.empty?
  STDERR.puts "Usage: crystal visualize_psf.cr <font.psf> [--preview]"
  exit 1
end

filepath = ARGV.empty? ? "font.psf" : ARGV[0]
file_bytes = File.read(filepath).bytes

glyph_start_offset = 0
char_width = 8
char_height = 16

if file_bytes[0] == 0x36 && file_bytes[1] == 0x04
  # PSF1 Header
  glyph_start_offset = 4
  char_height = file_bytes[3]
elsif file_bytes[0] == 0x72 && file_bytes[1] == 0xb5 && file_bytes[2] == 0x4a && file_bytes[3] == 0x86
  # PSF2 Header
  glyph_start_offset = file_bytes[16] | (file_bytes[17] << 8) | (file_bytes[18] << 16) | (file_bytes[19] << 24)
  char_height = file_bytes[24] | (file_bytes[25] << 8) | (file_bytes[26] << 16) | (file_bytes[27] << 24)
else
  glyph_start_offset = 0
  char_height = 8
end

glyphs = file_bytes[glyph_start_offset..-1]

# Terminal debug
puts "--- DEBUG: First 32 bytes of glyph data ---"
glyphs[0..31].each_slice(8) do |row_bytes|
  puts row_bytes.map { |b| b.to_s(2).rjust(8, '0').tr("01", ".#") }.join(" ")
end
puts "--------------------------------------------"

bytes_per_glyph = char_height
num_chars = glyphs.size // bytes_per_glyph

puts "Detected PSF Font -> Height: #{char_height}px, Glyphs found: #{num_chars}, Header offset: #{glyph_start_offset} bytes"

cols = 16
rows = (num_chars + cols - 1) // cols
scale = 4
img_w = cols * char_width * scale  # 16 * 8 * 4 = 512px
img_h = rows * char_height * scale # 16 * 8 * 4 = 512px

output_ppm = IO::Memory.new
output_ppm.puts("P3")
output_ppm.puts("#{img_w} #{img_h}")
output_ppm.puts("255")

pixel_count = 0
on_pixel_count = 0

# Loop over EVERY PIXEL of the final image buffer (img_h x img_w)
img_h.times.each do |py|
  char_y = py // scale
  r_idx = char_y % char_height
  c_row = char_y // char_height

  img_w.times.each do |px|
    char_x = px // scale
    c_col = char_x // char_width
    c_idx = c_row * cols + c_col

    pixel_is_on = false
    if c_idx < num_chars
      byte_offset = (c_idx * bytes_per_glyph) + r_idx
      if byte_offset < glyphs.size
        b = glyphs[byte_offset]
        bit = 7 - (char_x % char_width)
        pixel_is_on = (b & (1 << bit)) != 0
      end
    end

    pixel_count += 1
    if pixel_is_on
      on_pixel_count += 1
      output_ppm.puts("0 255 200") # Cyan text
    else
      output_ppm.puts("20 10 30")   # Dark background
    end
  end
end

Dir.mkdir_p("tmp")
preview_filename = "tmp/font_preview.ppm"
File.write(preview_filename, output_ppm.to_s)
puts "DEBUG: Wrote #{pixel_count} pixels (#{on_pixel_count} active font pixels)."
puts "Generated tmp/font_preview.ppm successfully!"

# print "Open preview image? [Y/n]: "
# input = (gets || "").strip.downcase

# if input.empty? || input == "y"
#   system("open #{preview_filename}")
# end

# Check if "--preview" was passed anywhere in the command-line arguments
should_preview = ARGV.includes?("--preview")

if should_preview
  puts "Opening in Mac Preview..."
  Dir.mkdir_p("tmp")
  # Generate your PPM visualizer image into "tmp/font_preview.ppm" etc.
  # ...
  system("open #{preview_filename}")
end
