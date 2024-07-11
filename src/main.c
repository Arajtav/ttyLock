#include <signal.h>

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

int main() {
    signalHandler(-1); // init
    while(1) {}
    return 0;
}
