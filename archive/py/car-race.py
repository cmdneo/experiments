import sys
import time
import random
import string
import itertools
from term import Term, Colors, setcurs, getkey


# TODO
class GameState:
    pass


class Road:
    def __init__(self, pattern, n):
        self.pattern = pattern
        self.n = n
        self.last_draw = time.time()
        self.flip = False
        self.tsize = None
        self.road = []

    def _update(self):
        self.road = []
        for unit in self.pattern:
            gap = (self.tsize[0] - self.n) // self.n
            self.road.append(f"{' ' * gap}{unit}" * (self.n - 1))

    def draw(self, ss, speed):
        if time.time() - self.last_draw > 1 / speed:
            self.last_draw = time.time()
            self.flip = not self.flip

        if self.tsize != ss.tsize:
            self.tsize = ss.tsize
            self._update()

        tmp_road = self.road
        if self.flip:
            tmp_road = tmp_road[::-1]

        for i, line in enumerate(itertools.cycle(tmp_road)):
            if i > ss.tsize[1]:
                break
            ss.addstr(line, 0, i)


class Thing:
    HOLE = [["╭───╮", "│   │", "╰───╯"], [5, 3]]

    def __init__(self, thing, pos=[0, 0]):
        self.thing = thing[0]
        self.size = thing[1]
        self.pos = pos

    def draw(self, ss, fg=Colors.Red):
        for i, line in enumerate(self.thing):
            ss.addstr(line, self.pos[0], self.pos[1] + i, fg=fg, bold=True)

    @staticmethod
    def draw_next_frame(speed, last_time, ss, things):
        popped = 0
        for thing in things:
            thing.pos[1] += speed * (time.time() - last_time)
            if thing.pos[1] > ss.tsize[1]:
                popped += 1
                things.remove(thing)
            else:
                thing.draw(ss)

        return popped


# Width: 6, Height: 5
class Car:
    SIZE = [6, 5]
    CAR = [" ╭──╮", "╭╯┅┅╰╮", "┃╭──╮┃", "┠┼──┼┨", "╚╧══╧╝"]

    def __init__(self, tsize):
        self.pos = [tsize[0] // 2 - Car.SIZE[0] // 2, tsize[1] - Car.SIZE[1]]
        self.size = Car.SIZE

    def reposition(self, tsize):
        self.pos = [tsize[0] // 2 - Car.SIZE[0] // 2, tsize[1] - Car.SIZE[1]]

    def control(self, key, move, screen_size):
        if (key == Term.UP or key == "w") and self.pos[1] > 1:
            self.pos[1] -= move
        elif (key == Term.DOWN or key == "s") and self.pos[1] + self.size[1] <= screen_size[1]:
            self.pos[1] += move
        elif (key == Term.LEFT or key == "a") and self.pos[0] > 1:
            self.pos[0] -= move
        elif (
            (key == Term.RIGHT or key == "d") and self.pos[0] + self.size[0] <= screen_size[0]
        ):
            self.pos[0] += move

    def draw(self, ss):
        for i, line in enumerate(Car.CAR):
            ss.addstr(line, self.pos[0], self.pos[1] + i)

    def detect_collision(self, things):
        for thing in things:
            if (
                int(thing.pos[0]) + thing.size[0] > self.pos[0]
                and int(thing.pos[1]) + thing.size[1] > self.pos[1]
                and int(thing.pos[0]) < self.pos[0] + self.size[0]
                and int(thing.pos[1]) < self.pos[1] + self.size[1]
            ):
                return True

        return False


def main(ss):
    setcurs(0)
    ss.start()

    # gs = GameState(ss)
    car = Car(ss.tsize)
    things = []
    road = Road(["█", " "], 3)
    last_time = time.time()
    speed = 5
    score = 0
    thing_prob = 0.05
    paused = False

    while True:
        ss.clear()

        key = getkey()

        if key == " ":
            paused = not paused
            last_time = time.time()
        if paused:
            continue

        if random.random() < thing_prob:
            things.append(
                Thing(
                    Thing.HOLE,
                    pos=[random.randint(0, ss.tsize[0] - Thing.HOLE[1][0]), 0],
                )
            )

        road.draw(ss, speed)
        score += Thing.draw_next_frame(speed, last_time, ss, things)
        speed = 5 + 0.01 * score
        car.control(key, 1, ss.tsize)
        car.draw(ss)
        ss.addstr(f"Score: {score}", xcenter=True, bold=True, fg=Colors.Green)
        if car.detect_collision(things):
            ss.addstr(
                "<CRAHSED| Hit SPACE |CRASHED>",
                0,
                2,
                xcenter=True,
                bg=Colors.Red,
                bold=True,
            )
            car.reposition(ss.tsize)
            score = 0
            things = []
            paused = True

        ss.update()
        last_time = time.time()
        time.sleep(0.05)


if __name__ == "__main__":
    ss = Term()
    try:
        main(ss)
    except KeyboardInterrupt:
        sys.exit(130)
    finally:
        ss.endwin()
