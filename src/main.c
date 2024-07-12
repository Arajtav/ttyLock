#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <security/_pam_types.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <sys/ioctl.h>

static struct pam_conv conv = {
    misc_conv,
    NULL
};

static struct termios term;
static volatile char run = 1;

char t_set() {
    if (tcgetattr(STDIN_FILENO, &term) < 0) { return 1; }

    struct termios uterm;
    memcpy(&uterm, &term, sizeof(term));

    // just in case, even tho signals are ignored, perhaps better way would be to just memset uterm.c_cc with _POSIX_VDISABLE;
    uterm.c_cc[VSUSP] = _POSIX_VDISABLE;
    uterm.c_cc[VINTR] = _POSIX_VDISABLE;
    uterm.c_cc[VKILL] = _POSIX_VDISABLE;
    uterm.c_cc[VQUIT] = _POSIX_VDISABLE;
    uterm.c_cc[VSTOP] = _POSIX_VDISABLE;
    uterm.c_cc[VSWTC] = _POSIX_VDISABLE;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &uterm) < 0) { return 2; }

    return 0;
}

void t_restore() {
    if (tcsetattr(STDIN_FILENO, TCSANOW, &term) < 0) {
        fputs("failed to restore terminal\n", stderr);
    }
}

void signalHandler(int sig) {
    if (sig == -1) {
        signal(SIGINT, signalHandler);
        signal(SIGHUP, signalHandler);
        signal(SIGKILL, signalHandler); // tho i'm almost sure i can't catch SIGKILL
        signal(SIGQUIT, signalHandler);
        signal(SIGTERM, signalHandler);
        signal(SIGTSTP, signalHandler);
        return;
    }
    signal(sig, signalHandler);
}

void usr1handler(__attribute__ ((unused)) int _) { run = 0; }

int main() {
    int y = 1;
    int x = 1;
    struct winsize winsz;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &winsz) != 0) {
        fputs("failed to get terminal size\n", stderr);
    } else {
        y = (winsz.ws_row/2)+1;
        x = (winsz.ws_col/2)-4; // +1 - length of password divided by 2
    }

    signal(SIGUSR1, usr1handler);
    if (t_set()) {
        fputs("failed to set terminal attributes\n", stderr);
        return 1;
    }

    const char* user = getlogin();
    if (user == NULL) {
        fputs("failed to get current user name\n", stderr);
        return 3;
    }

    signalHandler(-1); // init

    pam_handle_t* pamh = NULL;
    int tmp;

    if ((tmp = pam_start("ttylock", user, &conv, &pamh)) != PAM_SUCCESS) {
        fprintf(stderr, "pam_start failed: %s\n", pam_strerror(pamh, tmp));
        t_restore();
        return 4;
    }

    write(STDOUT_FILENO, "\033[s\033[?1049h", 11);  // save cursor position, use alt buffer
    while(run) {
        write(STDOUT_FILENO, "\033[2J", 4); // clear screen
        printf("\033[%d;%dH", y, x); // move cursor
        fflush(stdout);
        if (pam_authenticate(pamh, 0) == PAM_SUCCESS) {
            break;
        } else {
            // for SIGUSR1 unlock, signals cause pam_authenticate to fail.
            if (!run) { break; }
            // note: x-6 is -5 if ioctl failed, but terminal since terminal can't do that, it's being clipped to 1. clipping that in here is redundant
            printf("\033[%dGauthentication failure", x-6);  // -6 is because of difference in length of "password: " and "authentication failure"
            fflush(stdout);
            sleep(1);
        }
    }

   if (pam_end(pamh, tmp) != PAM_SUCCESS) {
       fputs("pam_end failed\n", stderr);
       t_restore();
       return 5;
    }

    t_restore();

    write(STDOUT_FILENO, "\033[?1049l\033[u", 11);  // exit alt buffer, restore cursor
    puts("unlocked!");

    return 0;
}