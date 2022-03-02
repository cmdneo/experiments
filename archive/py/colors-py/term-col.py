import colorsys
import shutil

w = shutil.get_terminal_size().columns

for x in range(-180, 180):
	d = x / 180
	r, g, b = colorsys.hsv_to_rgb(d, 1, 1)
	r = 255 * r
	b *= 255
	g = 255 * g
	print(f"\x1b[48;2;{int(r)};{int(g)};{int(b)}m{' ' * w}\x1b[0m")
