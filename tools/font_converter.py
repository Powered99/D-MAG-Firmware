import tkinter as tk
from tkinter import filedialog
from PIL import Image, ImageFont, ImageDraw
import re

# -------------------- Functions --------------------

def select_file(filetypes, title):
    """Open a file selection dialog and return the selected file path."""
    root = tk.Tk()
    root.withdraw()                   # Hide main window
    root.attributes('-topmost', True) # Make dialog appear on top
    file_path = filedialog.askopenfilename(title=title, filetypes=filetypes)
    root.destroy()                    # Destroy root immediately
    return file_path

def select_font_file():
    return select_file([("Font files", "*.ttf *.otf"), ("All files", "*.*")], 
                       "Select a TTF/OTF font file")

def select_c_file():
    return select_file([("C files", "*.c *.h"), ("All files", "*.*")],
                       "Select a C font array file")


# --- Fixed rendering helper ---
def draw_char_as_bitmap(char, font, char_width, char_height, scale=4):
    """
    Render a character at higher scale and downsample without non-uniform scaling.
    Steps:
      - Render into a large canvas (scale×).
      - Crop to the glyph bbox.
      - Uniformly scale the glyph so it fits into char_width×char_height (preserve aspect).
      - Center the scaled glyph into the target cell.
      - Convert to 1-bit image.
    """
    # Large canvas for high-res rendering
    large_w, large_h = char_width * scale, char_height * scale
    img_large = Image.new('L', (large_w, large_h), 0)
    draw_large = ImageDraw.Draw(img_large)

    # Draw the character at (0,0) so bbox is measured relative to that
    draw_large.text((0, 0), char, font=font, fill=255)

    # Get bounding box of the drawn glyph
    bbox = img_large.getbbox()
    if not bbox:
        # empty glyph -> return blank image
        return Image.new('1', (char_width, char_height), 0)

    # Crop to glyph
    glyph = img_large.crop(bbox)
    gw, gh = glyph.size

    # Compute uniform scale to fit glyph into target cell while preserving aspect ratio
    scale_x = char_width / gw
    scale_y = char_height / gh
    uniform_scale = min(scale_x, scale_y)

    # New size (at least 1 pixel)
    new_w = max(1, int(round(gw * uniform_scale)))
    new_h = max(1, int(round(gh * uniform_scale)))

    # Resize glyph using nearest (keep pixels crisp)
    glyph_resized = glyph.resize((new_w, new_h), Image.NEAREST)

    # Create target canvas (grayscale), paste centered
    target = Image.new('L', (char_width, char_height), 0)
    paste_x = (char_width - new_w) // 2
    paste_y = (char_height - new_h) // 2
    target.paste(glyph_resized, (paste_x, paste_y))

    # Convert to bilevel (1-bit) with threshold
    target_1 = target.point(lambda p: 255 if p > 128 else 0).convert('1')
    return target_1


def generate_font_array(font_path, char_width=8, char_height=8, first_char=32, last_char=126):
    """Generate displaylib_16 font array from a TTF/OTF font."""
    # Load slightly larger to maintain sharpness
    font = ImageFont.truetype(font_path, char_height * 4)
    font_bytes = []

    print(f"// Font generated from: {font_path}")
    print(f"static const std::array<uint8_t, {(last_char-first_char+1)*char_height + 4}> FontGenerated = {{")
    print(f"0x{char_width:02X}, 0x{char_height:02X}, 0x{first_char:02X}, 0x{last_char:02X},")

    for c in range(first_char, last_char+1):
        # Render high-res then scale down
        img = draw_char_as_bitmap(chr(c), font, char_width, char_height)

        char_bytes = []
        for y in range(char_height):
            byte = 0
            for x in range(char_width):
                pixel = img.getpixel((x, y))
                if pixel:
                    byte |= (1 << (7-x))
            char_bytes.append(byte)

        font_bytes.extend(char_bytes)
        hex_bytes = ','.join(f'0x{b:02X}' for b in char_bytes)
        print(f"{hex_bytes}, // '{chr(c)}'")

    print("};")
    return [char_width, char_height, first_char, last_char] + font_bytes


def parse_c_font_array(c_code):
    """Parse a C-style displaylib_16 font array into a Python list of bytes."""
    code_no_comments = re.sub(r"//.*", "", c_code)
    hex_values = re.findall(r"0x[0-9A-Fa-f]{2}", code_no_comments)
    return [int(h,16) for h in hex_values]


def visualize_font_array(font_array):
    """Visualize a displaylib_16 font array as ASCII art."""
    char_width = font_array[0]
    char_height = font_array[1]
    first_ascii = font_array[2]
    last_ascii = font_array[3]
    font_bytes = font_array[4:]
    num_chars = last_ascii - first_ascii + 1

    for i in range(num_chars):
        ascii_code = first_ascii + i
        char_bytes = font_bytes[i*char_height : (i+1)*char_height]
        print(f"\n// '{chr(ascii_code)}' (ASCII {ascii_code})")
        for b in char_bytes:
            row_str = ""
            for bit in range(7, 7-char_width, -1):
                row_str += "#" if b & (1 << bit) else "-"
            print(row_str)


# -------------------- Main Program --------------------

print("Choose an option:")
print("1 - Generate a C-style font array from a TTF/OTF font")
print("2 - Visualize an existing C-style font array")
choice = input("Enter 1 or 2: ")

if choice == "1":
    font_path = select_font_file()
    if not font_path:
        print("No file selected, exiting.")
        exit()

    try:
        font_height = int(input("Enter font height in px (default 8): ") or 8)
    except ValueError:
        font_height = 8

    try:
        font_width = int(input("Enter char width in px (default 8): ") or 8)
    except ValueError:
        font_width = 8

    font_array = generate_font_array(font_path, char_width=font_width, char_height=font_height)
    print("\n--- ASCII Art Preview of Entire Font ---")
    # visualize_font_array(font_array)

elif choice == "2":
    c_file_path = select_c_file()
    if not c_file_path:
        print("No file selected, exiting.")
        exit()

    with open(c_file_path, 'r') as f:
        c_code = f.read()

    font_array = parse_c_font_array(c_code)
    print("\n--- ASCII Art Visualization ---")
    visualize_font_array(font_array)

else:
    print("Invalid choice. Exiting.")