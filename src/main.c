#include <signal.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

static struct termios term;
static volatile char run = 1;

char t_set() {
    if (tcgetattr(STDIN_FILENO, &term) < 0) { return 1; }

    cc_t tmp = term.c_cc[VSUSP];
    term.c_cc[VSUSP] = _POSIX_VDISABLE;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0) { return 2; }

    term.c_cc[VSUSP] = tmp;
    return 0;
}

char t_restore() {
    return (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0);
}

void signalHandler(int sig) {
    if (sig == -1) {
        signal(SIGINT, signalHandler);
        signal(SIGHUP, signalHandler);
        signal(SIGKILL, signalHandler);
        signal(SIGQUIT, signalHandler);
        signal(SIGTERM, signalHandler);
        signal(SIGTSTP, signalHandler);
        return;
    }
    signal(sig, signalHandler);
}

void usr1handler(__attribute__ ((unused)) int _) { run = 0; }

int main() {
    signal(SIGUSR1, usr1handler);
    if (t_set()) {
        puts("failed to set terminal attributes");
        return 1;
    }
    signalHandler(-1); // init
    while(run) {}
    if (t_restore()) {
        puts("failed to restore terminal attributes");
        return 2;
    }
    return 0;
}
