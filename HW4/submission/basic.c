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

char buf[255];
char veth_parent[] = "veth-parent";
char veth_child[] = "veth-child";
char veth_parent_ip[] = "10.0.0.1";
char veth_child_ip[] = "10.0.0.2";

static char stack[STACK_SIZE];

int child(void *arg) {
    char c;

    memset(buf, 0, 255);
    sprintf(buf, "ip addr add %s/24 dev %s", veth_child_ip, veth_child);
    system(buf);

    memset(buf, 0, 255);
    sprintf(buf, "ip link set %s up", veth_child);
    system(buf);

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

    system("ip route add default via 10.0.0.13");

    execlp("python3", "python3", "http_recorder.py", (char *)NULL);
    // execlp("/bin/bash", "/bin/bash", (char *)NULL);
    perror("error: ");
    // execlp("/bin/cat", "/bin/cat", "123", (char *)NULL);
    printf("execlp failed\n");

    return 1;
}

int main() {
    pid_t pid = clone(child, stack + STACK_SIZE,
                      CLONE_NEWNET | CLONE_NEWUTS | CLONE_NEWIPC |
                          CLONE_NEWPID | CLONE_NEWNS | SIGCHLD,
                      NULL);

    memset(buf, 0, 255);
    sprintf(buf, "sudo ip link add name %s type veth peer name %s netns %d",
            veth_parent, veth_child, pid);
    system(buf);

    memset(buf, 0, 255);
    sprintf(buf, "ip addr add %s/24 dev %s", veth_parent_ip, veth_parent);
    system(buf);

    memset(buf, 0, 255);
    sprintf(buf, "ip link set %s up", veth_parent);
    system(buf);

    system("echo 1 > /proc/sys/net/ipv4/ip_forward");
    system("iptables -A FORWARD -o eth0 -i veth-default -j ACCEPT");
    system("iptables -A FORWARD -i eth0 -o veth-default -j ACCEPT");
    system("iptables -t nat -A POSTROUTING -s 10.0.0.15 -o eth0 -j MASQUERADE");
    execlp("/usr/bin/python3", "usr/bin/python3", "http_recorder.py",
           (char *)NULL);
    waitpid(pid, NULL, 0);
    return 0;
}
