"""Generate groupie.ico from the minimal tabs design (proposal 3)."""
from PIL import Image, ImageDraw

def draw_icon(size):
    """Draw the minimal tabs icon at the given size."""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    s = size / 256  # scale factor

    def r(x): return round(x * s)

    # Window body
    d.rounded_rectangle([r(24), r(64), r(232), r(232)], radius=r(16), fill='#1E293B')
    # Tab bar background
    d.rounded_rectangle([r(24), r(64), r(232), r(104)], radius=r(16), fill='#334155')
    d.rectangle([r(24), r(88), r(232), r(104)], fill='#334155')

    # Active tab (elevated, brighter)
    d.rounded_rectangle([r(32), r(56), r(104), r(104)], radius=r(10), fill='#3B82F6')
    # Active tab dot
    d.ellipse([r(62), r(74), r(74), r(86)], fill='#FFFFFF')

    # Inactive tab 2
    d.rounded_rectangle([r(110), r(68), r(166), r(100)], radius=r(8), fill='#475569')
    d.ellipse([r(133), r(79), r(143), r(89)], fill='#94A3B8')

    # Inactive tab 3
    d.rounded_rectangle([r(172), r(68), r(228), r(100)], radius=r(8), fill='#475569')
    d.ellipse([r(195), r(79), r(205), r(89)], fill='#94A3B8')

    # Content area
    d.rounded_rectangle([r(36), r(116), r(220), r(220)], radius=r(10), fill='#0F172A')

    # Content lines
    d.rounded_rectangle([r(52), r(136), r(172), r(140)], radius=r(2), fill='#1E293B')
    d.rounded_rectangle([r(52), r(148), r(142), r(152)], radius=r(2), fill='#1E293B')
    d.rounded_rectangle([r(52), r(160), r(192), r(164)], radius=r(2), fill='#1E293B')
    d.rounded_rectangle([r(52), r(172), r(112), r(176)], radius=r(2), fill='#1E293B')

    return img

# Generate the largest size and let Pillow create all sizes
img = draw_icon(256)
img.save(
    'groupie.ico',
    format='ICO',
    sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
)

print("Generated groupie.ico with 7 sizes")
