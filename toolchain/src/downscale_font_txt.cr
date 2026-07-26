# Usage: crystal downscale_font.cr input_font.txt output_font.txt
class FontDownscaler
  def initialize(@input_path : String, @output_path : String)
  end

  def run
    lines = File.read_lines(@input_path)
    output_buffer = IO::Memory.new

    i = 0
    while i < lines.size
      line = lines[i]

      # Check if this line is a character header, e.g., CHAR: '1' (ASCII 49)
      if line.starts_with?("CHAR:")
        header = line
        i += 1

        # Collect the next 16 lines as the font grid block
        grid_lines = [] of String
        16.times do
          if i < lines.size
            grid_lines << lines[i]
            i += 1
          end
        end

        if grid_lines.size == 16
          # Downscale 16x16 -> 8x8 using Max Pooling (OR logic)
          new_grid = crop_grid(grid_lines)

          output_buffer.puts(header)
          new_grid.each { |gl| output_buffer.puts(gl) }
          output_buffer.puts("") # Blank line separator
        else
          # Fallback if file ends unexpectedly
          output_buffer.puts(header)
          grid_lines.each { |gl| output_buffer.puts(gl) }
        end
      else
        # Copy any other lines (like file headers or comments) as-is
        output_buffer.puts(line)
        i += 1
      end
    end

    File.write(@output_path, output_buffer.to_s)
    puts "Successfully downscaled font: #{@output_path}"
  end

  private def downscale_grid(lines : Array(String)) : Array(String)
    new_grid = Array.new(8) { "" }

    8.times do |y|
      row_str = ""
      8.times do |x|
        has_pixel = false

        2.times do |dy|
          2.times do |dx|
            src_x = (x * 2) + dx
            src_y = (y * 2) + dy
            if src_y < lines.size && src_x < lines[src_y].size && lines[src_y][src_x] == '#'
              has_pixel = true
            end
          end
        end

        row_str += has_pixel ? '#' : '.'
      end
      new_grid[y] = row_str
    end

    new_grid
  end

  # Inside the FontDownscaler class, replace downscale_grid with:
  private def crop_grid(lines : Array(String)) : Array(String)
    new_grid = Array.new(8) { "" }

    8.times do |y|
      row_str = ""
      8.times do |x|
        # Sample directly from the center 8x8 window of the 16x16 grid (offset by +4)
        src_x = x + 2
        src_y = y + 4

        pixel = '.'
        if src_y < lines.size && src_x < lines[src_y].size
          pixel = lines[src_y][src_x]
        end
        row_str += pixel
      end
      new_grid[y] = row_str
    end

    new_grid
  end
end

if ARGV.size < 2
  STDERR.puts "Usage: crystal downscale_fonts.cr <input_file>.txt <output_file>.txt"
  exit 1
end

FontDownscaler.new(ARGV[0], ARGV[1]).run
