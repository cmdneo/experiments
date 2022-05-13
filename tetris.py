import sys
import pyglet
import random

from copy import deepcopy
from typing import Generator
from itertools import repeat
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

    def __eq__(self, other: "Point") -> bool:
        return self.x == other.x and self.y == other.y

    def __getitem__(self, n: int) -> int:
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
        (69, 69, 69),
        (0, 255, 255), (255, 255, 0), (127, 0, 127), (0, 255, 0),
        (255, 0, 0), (0, 0, 255), (255, 127, 0), (127, 127, 127),
    ]
    T = list[list[tuple[int, int]]]

    def __init__(self, states: T, anchor: Point = None, colid=0) -> None:
        anchor = anchor or Point(0, 0)
        self.states = [[Point(pos[0], pos[1]) for pos in s] for s in states]
        self.at = 0
        self.current = tuple(self.states[0])
        self.anchor = anchor
        self.colid = colid % len(Block.COLORS)

    def next_config(self):
        self.at += 1
        self.current = tuple(self.states[self.at % len(self.states)])

    def prev_config(self):
        self.at -= 1
        self.current = tuple(self.states[self.at % len(self.states)])

    def tile_positions(self) -> Generator[Point, None, None]:
        for cur in self.current:
            yield cur + self.anchor

    def __bool__(self) -> bool:
        return self.colid == 0

    def __eq__(self, other: "Block") -> bool:
        return self.current == other.current \
            and self.anchor == other.anchor

    def __repr__(self) -> str:
        return f"({self.anchor} => {self.current})"


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
    def invert(color: tuple[int, int, int]) -> tuple[int, int, int]:
        return ((255 - color[0]), (255 - color[1]), (255 - color[2]))


class Arena:
    def __init__(self, w: int, h: int, gstate=None) -> None:
        self.w = w
        self.h = h
        self.gstate = gstate
        self.buf = [[0 for _ in range(w)] for _ in range(h)]

    def _is_inside(self, pt: Point) -> bool:
        return 0 <= pt.x < self.w and 0 <= pt.y < self.h

    def _tile_neighs(self, pt: Point, exclude: list[Point] = None) -> list[Point]:
        x, y = pt.x, pt.y
        neighs = []

        if x > 0 and self.buf[y][x-1]:
            neighs.append(Point(x-1, y))
        if x < self.w - 1 and self.buf[y][x+1]:
            neighs.append(Point(x + 1, y))
        if y > 0 and self.buf[y-1][x]:
            neighs.append(Point(x, y - 1))
        if y < self.h - 1 and self.buf[y+1][x]:
            neighs.append(Point(x, y + 1))

        if not exclude:
            return neighs
        return list(filter(lambda item: item not in exclude, neighs))

    def _floating_blocks_above(self, yabv: int) -> set[Block]:
        assert yabv < self.h - 1
        y = yabv + 1
        blocks = []
        visited = []

        # Find the disconnected subgraph(block)
        # It is disconnected if it is not connected to any non-floating tiles.
        # As per game logic all tiles with pos.y <= ya are non-floating
        # ----------- The Stack of Iterators Pattern -----------
        # An elegant pattern for eliminating recursion in graph algorithms
        # Refer => https://garethrees.org/2016/09/28/pattern/
        for x, base_tile in enumerate(self.buf[yabv]):
            if not (self.buf[y][x] != 0 and base_tile == 0):
                continue
            stk = [iter([Point(x, y)])]
            blk_tiles = []

            while stk:
                try:
                    tile = next(stk[-1])
                except StopIteration:
                    stk.pop()
                else:
                    if tile in visited:
                        break
                    if tile.y <= yabv:
                        blk_tiles.clear()
                        break
                    blk_tiles.append(tile)
                    visited.append(tile)
                    stk.append(iter(self._tile_neighs(tile, exclude=visited)))

            if blk_tiles:
                blocks.append(Block(
                    [[(pt.x - x, pt.y - y) for pt in blk_tiles]], anchor=Point(x, y)
                ))

        # import pprint
        # pprint.pp(blocks)
        return blocks

    def clear_row(self, y: int):
        # As there is nothing above the top row
        if y == self.h - 1:
            self.buf[y] = [[0 for _ in range(self.w)]]
            return

        self.buf[y:] = self.buf[y+1:] + [[0 for _ in range(self.w)]]
        # No floating tiles possible if cleared row was on floor
        if y == 0:
            return

        # Get and remove all floating blocks from the arena
        floating_blocks = self._floating_blocks_above(y - 1)
        fvals = [self.empty(blk) for blk in floating_blocks]
        # Bring them to bottom
        while not all(map(self.at_bottom, floating_blocks)):
            for blk in floating_blocks:
                if not self.at_bottom(blk):
                    blk.anchor.y -= 1
        # Fill them in the arena with updated positions
        for blk, fval in zip(floating_blocks, fvals):
            self.fill(blk, fval)

    def will_fit(self, block: Block) -> bool:
        for pos in block.tile_positions():
            # Can be above max-height when new block spawned
            if not (0 <= pos.x < self.w and 0 <= pos.y):
                return False
            elif pos.y < self.h and self.buf[pos.y][pos.x]:
                return False

        return True

    def at_bottom(self, block: Block) -> bool:
        for pos in block.tile_positions():
            if pos.y == 0:
                return True
            # Case when (tmp.y - 1) is less than 0 is handled above
            elif self._is_inside(pos) and self.buf[pos.y - 1][pos.x]:
                return True

        return False

    def fill(self, block: Block, fill: int | list[int]):
        try:
            iter(fill)
        except TypeError:
            fill = repeat(fill)
        else:
            assert len(fill) <= len(block.current)

        for fv, pos in zip(fill, block.tile_positions()):
            if self._is_inside(pos):
                self.buf[pos.y][pos.x] = fv

    def empty(self, block: Block) -> list[int]:
        ret = []
        for pos in block.tile_positions():
            if self._is_inside(pos):
                ret.append(self.buf[pos.y][pos.x])
                self.buf[pos.y][pos.x] = 0

        return ret

    def get_full_rows(self, y: int) -> list[int]:
        ret = []
        for y, row in enumerate(self.buf):
            if all(row):
                ret.append(y)

        return ret

    def clear_full_rows(self) -> int:
        cnt = 0
        for y, row in enumerate(self.buf):
            if all(row):
                cnt += 1
                self.clear_row(y)

        return cnt

    def debug(self):
        for row in reversed(self.buf):
            for tile in row:
                perr("# " if tile else "- ", end="")
            perr()
        perr("=" * (self.w * 2))
        perr(self.buf)

    def debug_raw(self, extra: Block = None):
        pts = []
        if extra is not None:
            for pos in extra.tile_positions():
                pts.append(pos)

        for y, row in enumerate(self.buf):
            for x, tile in enumerate(row):
                # do better...
                sys.stdout.write("1" if tile or Point(x, y) in pts else "0")
        sys.stdout.flush()


# TODO Use smooth animations instead of instant transitions and rearchitecture
class GState:
    DOWN_TICKS = 30

    def __init__(self, w, h, size, title) -> None:
        # ! Must enable double_buffer, otherwise manual glFlush required
        config = pyglet.gl.Config(sample_buffers=1, samples=4,
                                  double_buffer=True)
        self.win = window.Window(
            (w + 8) * size, h * size,
            caption=title, config=config
        )
        self.kbd = window.key.KeyStateHandler()
        self.win.push_handlers(self)
        self.win.push_handlers(self.kbd)

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
        self.win.clear()
        # self.arena.debug_raw(self.block)
        tsq = shapes.Rectangle(
            0, 0,
            self.size - self.margin * 2, self.size - self.margin * 2,
        )
        tsq.anchor_position = (self.margin, self.margin)

        # Draw the fixed tiles in buffer
        for y, row in enumerate(self.arena.buf):
            for x, tile_colid in enumerate(row):
                tsq.color = Block.COLORS[tile_colid]
                tsq.x = x * self.size
                tsq.y = y * self.size
                if tile_colid:
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
        next_at = Point(self.win.width * 3 // 4,
                        self.win.height - self.size * 3)
        for pos in self.next_block.current:
            tmp = self.size * pos + next_at
            tsq.x, tsq.y = tmp.x, tmp.y
            tsq.draw()

        score = text.Label(
            f"Rows Cleared {self.score}",
            x=next_at.x, y=self.win.height // 2,
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

        if self.kbd[window.key.DOWN]:
            if not self.arena.at_bottom(self.block):
                self.block.anchor.y -= 1

        self.down_ticks += 1

        # No further processing required if no descent
        if self.down_ticks != GState.DOWN_TICKS:
            return
        self.down_ticks = 0

        tmp = self.arena.clear_full_rows()
        while tmp:
            self.score += tmp
            tmp = self.arena.clear_full_rows()

        if self.arena.at_bottom(self.block):
            self.arena.fill(self.block, self.block.colid)
            self.block = self.next_block
            self.next_block = deepcopy(random.choice(BLOCKS))
            self.block.anchor = Point(self.w // 2, self.h)
        else:
            self.block.anchor.y -= 1

        if not self.arena.will_fit(self.block):
            self.close()
            perr("GAME OVER!!1")

            return

    def close(self, debug=True):
        if debug:
            self.arena.debug()
            perr()
            perr("=" * (self.w * 2))
            perr([self.block.anchor + b for b in self.block.current])

        self.win.close()
        pyglet.app.exit()


if __name__ == "__main__":
    state = GState(10, 20, 40, "Tetris")
    pyglet.clock.schedule_interval(state.update, 1/60)
    pyglet.app.run()
