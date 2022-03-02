import sys
import shutil
import termios
import time
import tty
from collections import namedtuple
from dataclasses import dataclass


class Surface:
    def __init__(self, fd=None):
        if fd is None:
            fd = sys.stdout.fd()
        self.fd = fd
        self.old_settings = None
        self.tsize = Point2D(100, 60)
        self.windows = []
        self.last_drawn_at = time.time()
        self.dt = 0

    def start(self):
        self.update_data()
        self.old_settings = termios.tcgetattr(self.fd)
        tty.setcbreak(self.fd)

    def end(self):
        termios.tcsetattr(self.fd, termios.TCSADRAIN, self.old_settings)

    def update_data(self):
        tmp = shutil.get_terminal_size(fallback=(100, 60))
        self.tsize.x, self.tsize.y = tmp.columns, tmp.lines

    def add_window(self, window):
        self.windows.append(window)
        self.windows.sort(key=lambda item: item.z_index)

    def draw(self):
        self.update_data()
        for win in self.windows:
            win.draw()

        tmp_t = time.time()
        self.dt = tmp_t - self.last_drawn_at
        self.last_drawn_at = tmp_t

    def __del__(self):
        self.end()


class Window:
    def __init__(surface, pos, size, z_index=0):
        self.surface = surface
        self.pos = pos
        self.size = size
        self.z_index = z_index

        self.surface.add_window(self)

    def draw(self, *args, **kwargs):
        raise NotImplementedError


@dataclass
class Point2D:
    x: int
    y: int
    
    def __add__(self, other):
        return Point2D(self.x + other.x, self.y + other.y)

    def __sub__(self, other):
        return Point2D(self.x - other.x, self.y - other.y)


# TESTING TODO REMOVE TESTING
class NoteWindow(Window):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)



if __name__ == "__main__":
    try:
        pt = Point2D(100, 200)
        print(pt)
    finally:
        pass#surface.end()
