from audioop import mul
import random
import sys
import pyglet

from copy import deepcopy
from functools import partial
from pyglet import window, shapes, text

# TODO Add changing config of block when rotation not possible
# at present anchor position, due to no space on some sides


class Point:
    def __init__(self, x: int, y: int) -> None:
        self.x = x
        self.y = y

    def __add__(self, other: "Point") -> "Point":
        return Point(self.x + other.x, self.y + other.y)

    def __sub__(self, other: "Point") -> "Point":
        return Point(self.x - other.x, self.y - other.y)

    def __mul__(self, other: "Point") -> "Point":
        return Point(self.x * other.x, self.y * other.y)

    def __rmul__(self, scalar: int) -> "Point":
        return Point(self.x * scalar, self.y * scalar)

    def __str__(self) -> str:
        return self.__repr__()

    def __eq__(self, other: "Point") -> bool:
        return self.x == other.x and self.y == other.y

    def __get__(self, n: int) -> int:
        if n == 0:
            return self.x
        elif n == 1:
            return self.y
        raise IndexError

    def __repr__(self) -> str:
        return f"Point({self.x}, {self.y})"


class Block:
    # Zeroth index must be black
    COLORS = [
        (0, 0, 0),
        (0, 255, 255), (255, 255, 0), (127, 0, 127), (0, 255, 0),
        (255, 0, 0), (0, 0, 255), (255, 127, 0), (127, 127, 127),
    ]

    def __init__(self, states, anchor=(0, 0), colid=0) -> None:
        self.states = [[Point(pos[0], pos[1]) for pos in s] for s in states]
        self.at = 0
        self.current = self.states[0]
        self.anchor = anchor
        self.colid = colid % len(Block.COLORS)
        # self.batch = pyglet.graphics.Batch()

    def next_config(self):
        self.at += 1
        self.current = self.states[self.at % len(self.states)]

    def prev_config(self):
        self.at -= 1
        self.current = self.states[self.at % len(self.states)]


# Blocks and their all configurations
# Each block has 4 square tiles
NTILES = 4
# The lazy way...
S_SQ = [
    [(0, 0), (0, 1), (1, 0), (1, 1)],
]
S_I = [
    [(-1, 0), (0, 0), (1, 0), (2, 0)],
    [(0, -1), (0, 0), (0, 1), (0, 2)],
]
S_S = [
    [(-1, -1), (0, -1), (0, 0), (1, 0)],
    [(0, 1), (0, 0), (1, 0), (1, -1)],
]
S_Z = [
    [(-1, 0), (0, 0), (0, -1), (1, -1)],
    [(0, -1), (0, 0), (1, 0), (1, 1)],
]
S_J = [
    [(-1, 0), (0, 0), (1, 0), (1, -1)],
    [(-1, -1), (0, -1), (0, 0), (0, 1)],
    [(-1, 1), (-1, 0), (0, 0), (1, 0)],
    [(1, 1), (0, 1), (0, 0), (0, -1)],
]
S_L = [
    [(-1, -1), (-1, 0), (0, 0), (1, 0)],
    [(-1, 1), (0, 1), (0, 0), (0, -1)],
    [(-1, 0), (0, 0), (1, 0), (1, 1)],
    [(1, 0), (0, 0), (-1, 0), (-1, 1)],
]
S_T = [
    [(-1, 0), (0, 0), (1, 0), (0, -1)],
    [(-1, 0), (0, 0), (0, 1), (0, -1)],
    [(-1, 0), (0, 0), (1, 0), (0, 1)],
    [(+1, 0), (0, 0), (0, 1), (0, -1)],
]

BLOCKS = [Block(blk, colid=i+1) for i, blk
          in enumerate([S_SQ, S_I, S_J, S_L, S_Z, S_S, S_T])]
perr = partial(print, file=sys.stderr)


class ColorCycler:
    def __init__(self, start=(0, 0, 0), end=(255, 255, 255), steps=255) -> None:
        self.r, self.g, self.b = start
        self.start = start
        self.end = end
        self.steps = steps
        self.at = 0
        self.dir = 1

    def next(self) -> tuple[int, int, int]:
        if self.at == self.steps:
            self.dir = -1
        elif self.at == 0:
            self.dir = 1

        f = (self.at / self.steps) ** 0.5
        self.r = (self.start[0] * f + self.end[0] * (1 - f)) // 1
        self.g = (self.start[1] * f + self.end[1] * (1 - f)) // 1
        self.b = (self.start[2] * f + self.end[2] * (1 - f)) // 1

        self.at += self.dir

        return (self.r, self.g, self.b)

    @staticmethod
    def complement(color: tuple[int, int, int]) -> tuple[int, int, int]:
        return ((255 - color[0]), (255 - color[1]), (255 - color[2]))


class Arena:
    def __init__(self, w: int, h: int) -> None:
        self.w = w
        self.h = h
        self.buf = [[0 for _ in range(w)] for _ in range(h)]

    def _is_inside(self, pt: Point) -> bool:
        return 0 <= pt.x < self.w and 0 <= pt.y < self.h

    def _neigh_count(self, x: int, y: int) -> int:
        neighs = 0
        if x > 0:
            neighs += bool(self.buf[y][x-1])
        if x < self.w - 1:
            neighs += bool(self.buf[y][x+1])
        if y > 0:
            neighs += bool(self.buf[y-1][x])
        if y < self.h - 1:
            neighs += bool(self.buf[y+1][x])

        return neighs

    def _drop_tile(self, x, y):
        while y > 0 and not self.buf[y - 1][x]:
            tmp = self.buf[y][x]
            self.buf[y][x] = 0
            self.buf[y - 1][x] = tmp
            y -= 1

    def _clear_row(self, y: int):
        self.buf[y:] = self.buf[y+1:] + [[0 for _ in range(len(self.buf[y]))]]
        for yr, row in enumerate(self.buf[y:]):
            ya = yr + y
            for xa, _tile in enumerate(row):
                if self._neigh_count(xa, ya) == 0:
                    self._drop_tile(xa, ya)

    def will_fit(self, block: Block) -> bool:
        for rpos in block.current:
            tmp = rpos + block.anchor
            # Can be above max height when new block spawned
            if not (0 <= tmp.x < self.w and 0 <= tmp.y):
                return False
            elif tmp.y < self.h and self.buf[tmp.y][tmp.x]:
                return False

        return True

    def at_bottom(self, block: Block) -> bool:
        for rpos in block.current:
            tmp = rpos + block.anchor
            if tmp.y == 0:
                return True
            # Case when (tmp.y - 1) is less than 0 is handled above
            elif self._is_inside(tmp) and self.buf[tmp.y - 1][tmp.x]:
                return True

        return False

    def fill(self, block: Block, fval: int):
        for rpos in block.current:
            tmp = rpos + block.anchor
            if self._is_inside(tmp):
                self.buf[tmp.y][tmp.x] = fval

    def is_row_full(self, y: int) -> bool:
        return all(self.buf[y])

    def clear_full_rows(self) -> int:
        cnt = 0
        for y, row in enumerate(self.buf):
            if all(row):
                cnt += 1
                self._clear_row(y)

        return cnt

    def debug(self):
        for row in reversed(self.buf):
            for tile in row:
                perr("# " if tile else "- ", end="")
            perr()
        perr("\n", "=" * (self.w * 2))

    def debug_raw(self, extra: Block = None):
        pts = []
        if extra is not None:
            for rpos in extra.current:
                pts.append(rpos + extra.anchor)

        for y, row in enumerate(self.buf):
            for x, tile in enumerate(row):
                # do better... and use binary format
                sys.stdout.write("1" if tile or Point(x, y) in pts else "0")
        sys.stdout.flush()


# TODO Seperate window instance from Game-state machine
# ! Name clashes possible
class GState(window.Window):
    DOWN_TICKS = 30

    def __init__(self, w, h, size, title) -> None:
        # ! Must enable double_buffer, otherwise manual glFlush required
        config = pyglet.gl.Config(
            sample_buffers=1, samples=4, double_buffer=True)
        super(GState, self).__init__(
            w * size * 2, h * size, caption=title, config=config)

        self.w = w
        self.h = h
        self.size = size
        self.score = 0
        self.down_ticks = 0
        self.margin = 1
        self.paused = False

        self.arena = Arena(w, h)
        self.block = deepcopy(random.choice(BLOCKS))
        self.next_block = deepcopy(random.choice(BLOCKS))
        self.block.anchor = Point(w // 2, h)
        self.color = ColorCycler((128, 128, 255), (255, 255, 255))

    def on_draw(self):
        self.clear()
        # self.arena.debug_raw(self.block)
        tsq = shapes.Rectangle(
            0, 0,
            self.size - self.margin * 2, self.size - self.margin * 2,
        )
        tsq.anchor_position = (self.margin, self.margin)

        # Draw the fixed tiles in buffer
        for y, row in enumerate(self.arena.buf):
            for x, tile_col in enumerate(row):
                tsq.color = Block.COLORS[tile_col]
                tsq.x = x * self.size
                tsq.y = y * self.size
                if tile_col:
                    tsq.draw()

        # Draw the falling tile
        for pos in self.block.current:
            tsq.x = pos.x * self.size + self.block.anchor.x * self.size
            tsq.y = pos.y * self.size + self.block.anchor.y * self.size
            tsq.color = self.color.next()
            tsq.draw()

        # Draw the vertical seperator
        tsq.color = (127, 127, 127)
        for ny in range(self.h):
            tsq.position = (self.w * self.size, ny * self.size)
            tsq.draw()

        # Draw next tile
        tsq.color = Block.COLORS[self.next_block.colid]
        next_at = Point(self.width * 3 // 4,
                        self.height - self.size * 3)
        for pos in self.next_block.current:
            tmp = self.size * pos + next_at
            tsq.x, tsq.y = tmp.x, tmp.y
            tsq.draw()

        score = text.Label(
            f"Rows Cleared {self.score}",
            x=next_at.x, y=self.height // 2,
            font_size=self.size // 2, anchor_x="center"
        )
        score.draw()

    def on_key_press(self, sym, mods) -> None:
        k = window.key

        if sym == k.ESCAPE:
            self.close()
        if sym == k.UP:
            self.block.next_config()
            if not self.arena.will_fit(self.block):
                self.block.prev_config()
        elif sym == k.DOWN:
            # Handeled in update(), when down arrow key is hold down
            pass
        elif sym == k.RIGHT:
            self.block.anchor.x += 1
            if not self.arena.will_fit(self.block):
                self.block.anchor.x -= 1
        elif sym == k.LEFT:
            self.block.anchor.x -= 1
            if not self.arena.will_fit(self.block):
                self.block.anchor.x += 1
        elif sym == k.SPACE:
            self.paused = not self.paused

    def update(self, dt):
        if self.paused:
            return

        if window.key.DOWN in self.pressed_keys:
            if not self.arena.at_bottom(self.block):
                self.block.anchor.y -= 1

        self.down_ticks += 1

        # No further processing required if no descent
        if self.down_ticks != GState.DOWN_TICKS:
            return
        self.down_ticks = 0

        if self.arena.at_bottom(self.block):
            self.arena.fill(self.block, self.block.colid)
            self.block = self.next_block
            self.next_block = deepcopy(random.choice(BLOCKS))
            self.block.anchor = Point(self.w // 2, self.h)
            self.score += self.arena.clear_full_rows()
        else:
            self.block.anchor.y -= 1

        if not self.arena.will_fit(self.block):
            self.close()
            pyglet.app.exit()

            perr("=" * 10)
            perr([self.block.anchor + b for b in self.block.current])
            self.arena.debug()
            perr("GAME OVER!!1")
            return


if __name__ == "__main__":
    state = GState(10, 20, 40, "Tetris")
    pyglet.clock.schedule_interval(state.update, 1/60)
    pyglet.app.run()
