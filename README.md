# Project
Something like a screen locker but for tty.

# Usage
To unlock, you can either type your password, or send `SIGUSR1` to ttylock process.

# Setup
Before using it, you need to run ```sudo sh -c 'echo "auth required pam_unix.so" > /etc/pam.d/ttylock'```

