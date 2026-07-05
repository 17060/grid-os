#include "usys.h"

#define W 16
#define H 10

static char board[H][W + 1];
static int px = 2;
static int py = 2;
static int rx = 13;
static int ry = 7;
static int alive = 1;
static int moves = 0;

static void draw(void) {
    char line[W + 3];
    int x;
    int y;

    sys_write("\n");
    sys_write("+");
    for (x = 0; x < W; ++x) {
        sys_write("-");
    }
    sys_write("+\n");

    for (y = 0; y < H; ++y) {
        line[0] = '|';
        for (x = 0; x < W; ++x) {
            if (x == px && y == py) {
                line[x + 1] = '@';
            } else if (x == rx && y == ry) {
                line[x + 1] = 'R';
            } else if (board[y][x]) {
                line[x + 1] = '#';
            } else {
                line[x + 1] = '.';
            }
        }
        line[W + 1] = '|';
        line[W + 2] = '\0';
        sys_write(line);
        sys_write("\n");
    }

    sys_write("+");
    for (x = 0; x < W; ++x) {
        sys_write("-");
    }
    sys_write("+\n");
}

static void mark_trail(int x, int y) {
    board[y][x] = 1;
}

static int crash(int x, int y) {
    if (x < 0 || y < 0 || x >= W || y >= H) {
        return 1;
    }
    if (board[y][x] && !(x == px && y == py)) {
        return 1;
    }
    return 0;
}

static void recognizer_step(void) {
    if ((moves & 1) == 0) {
        if (rx > px) {
            rx--;
        } else if (rx < px) {
            rx++;
        }
    } else {
        if (ry > py) {
            ry--;
        } else if (ry < py) {
            ry++;
        }
    }
    mark_trail(rx, ry);
    if (rx == px && ry == py) {
        alive = 0;
    }
}

void _start(void) {
    int x;
    int y;
    char controls[96];

    for (y = 0; y < H; ++y) {
        for (x = 0; x < W; ++x) {
            board[y][x] = 0;
        }
    }

    sys_write("LIGHT CYCLE v2 // Flynn arena\n");
    sys_write("@ = you   R = recognizer drone   # = energy wall\n");
    sys_write("WASD to move. Outrun the Recognizer. q to quit.\n");

    while (alive) {
        mark_trail(px, py);
        draw();
        sys_write("move> ");
        if (sys_call(SYS_READ_LINE, (long)controls, 8, 0) < 0) {
            continue;
        }

        if (controls[0] == 'q') {
            break;
        }

        if (controls[0] == 'w') {
            py--;
        } else if (controls[0] == 's') {
            py++;
        } else if (controls[0] == 'a') {
            px--;
        } else if (controls[0] == 'd') {
            px++;
        }

        if (crash(px, py)) {
            alive = 0;
        }

        recognizer_step();
        if (crash(rx, ry)) {
            sys_write("Recognizer derezzed itself. You win.\n");
            sys_exit(0);
        }

        moves++;
    }

    sys_write("Derezzed! End of line.\n");
    sys_exit(0);
}
