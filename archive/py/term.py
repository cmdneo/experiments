import sys
import tty
import select
import shutil
import string
import termios
import textwrap


class Term:
    ANSI_ENDS = string.ascii_letters + "~<>="
    BACKSPACE = "\x7f"
    CLEAR_LINE = "\x15"
    DELETE = "\x1b[3~"
    HOME = "\x1b[H"
    END = "\x1b[F"
    UP = "\x1b[A"
    DOWN = "\x1b[B"
    RIGHT = "\x1b[C"
    LEFT = "\x1b[D"

    def __init__(self):
        self.buf = ""
        self.old_buf = ""
        self.fd = sys.stdin.fileno()
        self.old_settings = termios.tcgetattr(self.fd)
        self.win_started = False
        try:
            tsize = shutil.get_terminal_size()
        except OSError:
            print("Error! Output not to a terminal.", file=sys.stderr)
            self.tsize = [-1,-1]
        else:
            self.tsize = (tsize.columns, tsize.lines)

    def addstr(self, text, x=0, y=0, xcenter=False, **kwargs):
        if xcenter:
            x = self.tsize[0] // 2 - len(text) // 2
        if y <= self.tsize[1]:
            text = Colors.format(text, **kwargs)
            self.buf += f"\x1b[{int(y)};{int(x)}H{text}"

    def clear(self):
        self.old_buf = self.buf
        self.buf = "\x1b[2J"

    def update(self):
        if self.buf != self.old_buf:
            sys.stdout.write(self.buf)
            sys.stdout.flush()
        try:
            tsize = shutil.get_terminal_size()
        except OSError:
            self.tsize = [-1,-1]
        else:
            self.tsize = (tsize.columns, tsize.lines)

    def start(self):
        tty.setcbreak(self.fd)
        self.win_started = True

    def endwin(self):
        setcurs(1)
        if self.win_started:
            print("\x1b[2J")
            self.win_started = False
            termios.tcsetattr(self.fd, termios.TCSADRAIN, self.old_settings)


class Colors:
    Black = 0
    Maroon = 1
    Green = 2
    Olive = 3
    Navy = 4
    Purple = 5
    Teal = 6
    Silver = 7
    Grey = 8
    Red = 9
    Lime = 10
    Yellow = 11
    Blue = 12
    Fuchsia = 13
    Aqua = 14
    White = 15
    NavyBlue = 17
    DarkBlue = 18
    DarkGreen = 22
    DarkCyan = 36
    LightSeaGreen = 37
    DarkTurquoise = 44
    MediumSpringGreen = 49
    DarkRed = 52
    BlueViolet = 57
    SteelBlue = 67
    CornflowerBlue = 69
    CadetBlue = 72
    CadetBlue = 73
    MediumTurquoise = 80
    DarkRed = 88
    DarkMagenta = 90
    DarkMagenta = 91
    DarkViolet = 92
    Purple = 93
    LightSlateGrey = 103
    MediumPurple = 104
    LightSlateBlue = 105
    DarkSeaGreen = 108
    LightGreen = 119
    MediumVioletRed = 126
    DarkViolet = 128
    Purple = 129
    IndianRed = 131
    MediumOrchid = 134
    DarkGoldenrod = 136
    RosyBrown = 138
    DarkKhaki = 143
    LightSteelBlue = 147
    GreenYellow = 154
    IndianRed = 167
    Orchid = 170
    Violet = 177
    Tan = 180
    HotPink = 205
    HotPink = 206
    DarkOrange = 208
    LightCoral = 210
    SandyBrown = 215
    attrs = {"bold": 1, "faint": 2, "italic": 3, "underline": 4}

    @staticmethod
    def format(
        text,
        fg=White,
        bg=Black,
        bold=False,
        faint=False,
        italic=False,
        underline=False,
    ):
        fg_ansi = f"\x1b[38;5;{fg}m"
        bg_ansi = f"\x1b[48;5;{bg}m"

        if isinstance(fg, (list, tuple)) and len(fg) == 3:
            fg_ansi = f"\x1b[38;2;{fg[0]};{fg[1]};{fg[2]}m"
        if isinstance(bg, (list, tuple)) and len(bg) == 3:
            bg_ansi = f"\x1b[48;2;{fg[0]};{fg[1]};{fg[2]}m"

        attrs_ansi = (
            "\x1b[1m" * bold
            + "\x1b[2m" * faint
            + "\x1b[3m" * italic
            + "\x1b[4m" * underline
            + fg_ansi
            + bg_ansi
        )

        return f"{attrs_ansi}{text}\x1b[0m"


class Window:
    UL = "\u250c"
    LL = "\u2514"
    UR = "\u2510"
    LR = "\u2518"
    H = "\u2500"
    V = "\u2502"
    B = " "

    def __init__(self, title, text, width, height, margins=[0, 1, 0, 1]):
        self.title = title
        self.width = width
        self.height = height
        self.margins = margins
        self.text = text

    def _format_text(self, text_len):
        lines = self.text.split("\n")
        final_text = []

        for line in lines:
            wrapped = textwrap.wrap(line, text_len)
            for w in wrapped:
                final_text.append(w + " " * (self.width - len(w)))
        return final_text

    def draw_box(self, x, y):
        text_len = self.width - (self.margins[1] + self.margins[3] + 2)

        proc_text = _format_text(self.text.rstrip(), text_len)
        redc = self.width - len(title) - 5
        lower = f"{Window.LL}{Window.H * (self.width - 2)}{Window.LR}"
        upper = f"{Window.UL}{Window.H} {title} {Window.H * redc}{Window.UR}"

        print(upper)
        print(
            f"{Window.V}{Window.B * (self.width - 2)}{Window.V}\n"
            * self.margins[0],
            end="",
        )
        for line in proc_text:
            print(
                Window.V
                + Window.B * self.margins[1]
                + line
                + Window.B * self.margins[3]
                + Window.B * (text_len - len(line))
                + Window.V
            )
        print(
            f"{Window.V}{Window.B * (self.width - 2)}{Window.V}\n"
            * self.margins[2],
            end="",
        )
        print(lower)


def setcurs(vis):
    sys.stdout.write(f"\x1b[?25{'h' if vis else 'l'}")
    sys.stdout.flush()


def getkey(blocked=False):
    # Non-blocking
    if not (
        blocked
        or select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], [])
    ):
        return ""

    c = sys.stdin.read(1)
    ansi_state = 0

    if c == "\x1b":
        ansi_state = 1
    while ansi_state:
        cnext = sys.stdin.read(1)
        c += cnext
        if cnext in Term.ANSI_ENDS:
            ansi_state = 0

    return c


def getch():
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)

    try:
        tty.setcbreak(sys.stdin.fileno())
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)

    return ch
