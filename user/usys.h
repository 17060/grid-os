#ifndef USER_USYS_H
#define USER_USYS_H

#define SYS_EXIT        0
#define SYS_WRITE       1
#define SYS_DISC        2
#define SYS_VAULT_GET   3
#define SYS_CYCLES      4
#define SYS_READ        5
#define SYS_READ_LINE   6
#define SYS_GRIDFS_READ 7

static inline long sys_call(long number, long a1, long a2, long a3) {
    long ret;
    __asm__ volatile(
        "int $0x80"
        : "=a"(ret)
        : "a"(number), "D"(a1), "S"(a2), "d"(a3)
        : "rcx", "r11", "memory");
    return ret;
}

static inline void sys_write(const char *text) {
    (void)sys_call(SYS_WRITE, (long)text, 0, 0);
}

static inline void sys_exit(int code) {
    (void)sys_call(SYS_EXIT, code, 0, 0);
}

static inline void sys_disc(char *buffer, long cap) {
    (void)sys_call(SYS_DISC, (long)buffer, cap, 0);
}

static inline void sys_vault_get(const char *key, char *buffer, long cap) {
    (void)sys_call(SYS_VAULT_GET, (long)key, (long)buffer, cap);
}

static inline void sys_cycles(char *buffer, long cap) {
    (void)sys_call(SYS_CYCLES, (long)buffer, cap, 0);
}

static inline long sys_read_line(char *buffer, long cap) {
    return sys_call(SYS_READ_LINE, (long)buffer, cap, 0);
}

static inline long sys_gridfs_read(const char *path, char *buffer, long cap) {
    return sys_call(SYS_GRIDFS_READ, (long)path, (long)buffer, cap);
}

#endif
