#define _GNU_SOURCE
#include <arpa/inet.h>
#include <net/if.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define STACK_SIZE (1024 * 1024)

static char stack[STACK_SIZE];

int setip(char *name, char *addr, char *netmask) {
    struct ifreq ifr;
    int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

    strncpy(ifr.ifr_name, name, IFNAMSIZ);

    ifr.ifr_addr.sa_family = AF_INET;
    inet_pton(AF_INET, addr, ifr.ifr_addr.sa_data + 2);
    ioctl(fd, SIOCSIFADDR, &ifr);

    inet_pton(AF_INET, netmask, ifr.ifr_addr.sa_data + 2);
    ioctl(fd, SIOCSIFNETMASK, &ifr);

    // get flags
    ioctl(fd, SIOCGIFFLAGS, &ifr);
    strncpy(ifr.ifr_name, name, IFNAMSIZ);
    ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
    // set flags
    ioctl(fd, SIOCSIFFLAGS, &ifr);

    return 0;
}

int child(void *arg) {
    char c;
    sleep(1);
    sethostname("myhost", 6);
    int status;
    // status = mount("/usr/lib", "fs/usr/lib", "ext4", MS_BIND | MS_RDONLY,
    // ""); if (status != 0) {
    //     perror("mount error: ");
    // }
    // status = chroot("fs/");
    // if (status != 0) {
    //     perror("chroot error: ");
    // }

    // status = chdir("/");
    // if (status != 0) {
    //     perror("chdir error: ");
    // }
    // // status = mount("proc", "/proc", "proc", 0, NULL);
    // if (status != 0) {
    //     perror("mount error: ");
    // }

    setip("veth1", "10.0.0.15", "255.0.0.0");

    // execlp("/bin/python", "/bin/python", "/http_recorder.py", (char *)NULL);
    execlp("/bin/bash", "/bin/bash", (char *)NULL);
    perror("error: ");
    // execlp("/bin/cat", "/bin/cat", "123", (char *)NULL);
    printf("execlp failed\n");

    return 1;
}

int main() {
    char buf[255];
    pid_t pid = clone(child, stack + STACK_SIZE,
                      CLONE_NEWNET | CLONE_NEWUTS | CLONE_NEWIPC |
                          CLONE_NEWPID | CLONE_NEWNS | SIGCHLD,
                      NULL);

    sprintf(buf,
            "sudo ip link add name veth0 type veth peer name veth1 netns %d",
            pid);

    system(buf);
    setip("veth0", "10.0.0.13", "255.0.0.0");
    waitpid(pid, NULL, 0);
    return 0;
}
