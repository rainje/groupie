"""Generate groupie.ico — simple, readable at 16px."""
from PIL import Image, ImageDraw

def draw_icon(size):
    """Draw a simple tab group icon: 3 tabs on a dark background."""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    s = size / 256

    def r(x): return round(x * s)

    # Rounded background
    d.rounded_rectangle([r(8), r(8), r(248), r(248)], radius=r(40), fill='#1E293B')

    # Three tab shapes at the top
    # Active tab (bright blue, taller)
    d.rounded_rectangle([r(24), r(32), r(100), r(100)], radius=r(14), fill='#3B82F6')
    # Tab 2
    d.rounded_rectangle([r(108), r(48), r(172), r(100)], radius=r(12), fill='#475569')
    # Tab 3
    d.rounded_rectangle([r(180), r(48), r(232), r(100)], radius=r(12), fill='#475569')

    # Content area below tabs
    d.rounded_rectangle([r(24), r(100), r(232), r(224)], radius=r(0), fill='#0F172A')
    d.rounded_rectangle([r(24), r(196), r(232), r(224)], radius=r(14), fill='#0F172A')

    # Simple content lines (only for larger sizes)
    if size >= 48:
        d.rounded_rectangle([r(48), r(128), r(180), r(140)], radius=r(4), fill='#1E293B')
        d.rounded_rectangle([r(48), r(152), r(140), r(164)], radius=r(4), fill='#1E293B')
        d.rounded_rectangle([r(48), r(176), r(200), r(188)], radius=r(4), fill='#1E293B')

    return img

sizes = [16, 24, 32, 48, 64, 128, 256]
img = draw_icon(256)
img.save(
    'groupie.ico',
    format='ICO',
    sizes=[(s, s) for s in sizes]
)
print("Generated groupie.ico")
