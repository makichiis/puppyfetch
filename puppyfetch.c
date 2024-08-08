#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>

const char* puppy = ""\
"  /^ ^\\    \n" \
" / 0 0 \\   \n" \
" V\\ Y /V   \n" \
"  / - \\    \n" \
" /    |    \n" \
"V__) ||    \n";

// https://man7.org/linux/man-pages/man7/hostname.7.html
#define HOSTNAME_BUFSZ _SC_HOST_NAME_MAX  
// just betting on https://bbs.archlinux.org/viewtopic.php?id=251984 and 
// https://unix.stackexchange.com/questions/220542/linux-unix-username-length-limit
#define USERNAME_BUFSZ _SC_LOGIN_NAME_MAX  

#define PROC_CPUINFO_PATH "/proc/cpuinfo"
#define PROC_MEMINFO_PATH "/proc/meminfo"

static const char* this_path;

void get_pkgs_info(char* buf, size_t max_size);

/**
 * @brief Retrieve a formatted summary of the system's processor(s)
 * and their respective clock speeds. The string returned should be 
 * freed after this function is called.
 */ 
void get_cpuinfo_model(char* buf, size_t max_size);

/**
 * @brief Retrieve a formatted summary of the system's memory
 * usage vs. its total memory. The string returned should be 
 * freed after this function is called.
 */ 
void get_meminfo_usage(char* buf, size_t max_size);

void get_os(char* buf, size_t max_size);

const char* art = "";

#define arrlen(arr) (sizeof arr / sizeof *arr)


// pgks: dpkg -l | wc -l

struct meta {
    const char* name;
    const char* value;
};

#define meta_null(meta) (meta.name == NULL)

#define gotoxy(x, y) printf("\033[%d;%dH", (y), (x))

int main(int argc, const char** argv) {
    this_path = argv[0];
    char username[USERNAME_BUFSZ];
    char hostname[HOSTNAME_BUFSZ];
    char packages_summary[64];
    char cpuinfo_summary[64];
    char meminfo_summary[64];
    char os_name[64];

    getlogin_r(username, sizeof username);
    gethostname(hostname, sizeof hostname);
    //get_pkgs_info(packages_summary, sizeof packages_summary);
    get_cpuinfo_model(cpuinfo_summary, sizeof cpuinfo_summary);
    get_meminfo_usage(meminfo_summary, sizeof meminfo_summary);
    get_os(os_name, sizeof os_name);

    struct utsname name;
    if (uname(&name) == -1) {
        fprintf(stderr, "%s: uname() failed with errno %d\n", 
                this_path, errno);
        exit(1);
    }

    struct meta lines[6] = {
        { "os     ", os_name },
        { "cpu    ", cpuinfo_summary }, 
        { "kernel ", name.release },
        { "server ", getenv("XDG_SESSION_TYPE") },
        //{ "pkgs   ", packages_summary }, 
        { "memory ", meminfo_summary },
    };

    printf("%s\n", puppy);

    int written = printf("%s@%s\n", username, hostname);

    for (size_t i = 0; i < arrlen(lines); ++i) {
        if (!meta_null(lines[i])) printf("%s %s\n", 
                                lines[i].name,
                                lines[i].value);
    }
}

void get_pkgs_info(char* buf, size_t max_size) {
    FILE* fp = popen("dpkg -l", "r");
    
    // Would it be faster to just read from the stream directly?
    // especially if im using io_uring which will paralellize
    // this anyway.
    
    ssize_t dpkg_count = -5; // exclude header rows 

    char pbuf[256];
    ssize_t res = 0;
    while (1) {
        res = read(fileno(fp), pbuf, sizeof pbuf);
        
        if (res == -1) {
            fprintf(stderr, "%s: Pipe broken with errno %d\n",
                    this_path, errno);
            break;
        }
        
        if (res == 0) 
            break;

        for (ssize_t i = 0; i < res; ++i)
            if (pbuf[i] == '\n')
                ++dpkg_count;
    }
    pclose(fp);

    snprintf(buf, max_size, "dpkg (%ld)", dpkg_count);
}

// ref: AMD Ryzen 5 4600H with Radeon Graphics (12) @ 3.000GHz

#define NEXT_LINE(pbuf) {\
    for (size_t i = 0; i < sizeof pbuf; ++i)\
        \
}

static inline const char* pf__next_line(const char* buf, const char* end) {
    while (buf != end) {
        if (*buf == '\n')
            return buf+1;
        ++buf;
    }

    return end;
}

// I have no idea how to do this on a system level without 
// platform-specific kernel bindings 

// Cant use parsable rows becuase they exclude the mode name 
#define CPU_QUERY "lscpu | grep -E 'CPU\\(s\\):|Model name:|max MHz:' | tr -s ' ' | cut -d ':' -f 2"

void get_cpuinfo_model(char* buf, size_t max_size) {
    // if this sucks (formatting fails on other systems) 
    // i'll get around to changing it 
    
    FILE* pipe = popen(CPU_QUERY, "r");

    char pbuf[2048]; // fits all relevant information 
    
    read(fileno(pipe), pbuf, sizeof pbuf);
    pclose(pipe);

    // god forbid any of these be platform-dependent
    // the rationale is in the bash output of `CPU_QUERY`
    // I need to test if this is preferrable to in-program
    // parsing of lscpu output.

    const char* pbufp = pbuf;

    int cpu_count;
    sscanf(pbufp+1, "%d", &cpu_count);
    
    char cpu_model[128];
    pbufp = pf__next_line(pbufp, pbuf + 2048);
    sscanf(pbufp+1, "%s", cpu_model);

    int speed_mhz;
    pbufp = pf__next_line(pbufp, pbuf + 2048);
    sscanf(pbufp, "%d", &speed_mhz);
    double speed_ghz = ((double)speed_mhz) / 1000;

    snprintf(buf, max_size, "%s %dc @ %.4lf GHz", cpu_model, cpu_count, speed_ghz);
}

// MemUsed = Memtotal + Shmem - MemFree - Buffers - Cached - SReclaimable
// NOTE: Memory is off. Needs to be patched.

void get_meminfo_usage(char* buf, size_t max_size) {
    FILE* fs = fopen("/proc/meminfo", "r");

    long long mem_total;
    long long mem_used = 0;

    char param_name[64];
    long long value;
    while (1) {
        int res = fscanf(fs, "%s %lld", param_name, &value);
        if (res == EOF) {
            break;
        }

        if (strcmp(param_name, "MemTotal:") == 0) {
            mem_total = value;
            mem_used = mem_total;
        } else if (strcmp(param_name, "Shmem:") == 0) {
            mem_used += value;
        } else if (strcmp(param_name, "MemFree:") == 0) {
            mem_used -= value;
        } else if (strcmp(param_name, "Buffers:") == 0) {
            mem_used -= value;
        } else if (strcmp(param_name, "Cached") == 0) {
            mem_used -= value;
        } else if (strcmp(param_name, "SReclaimable:") == 0) {
            mem_used -= value;
        } else if (strcmp(param_name, "Cached:") == 0) {
            mem_used -= value;
        }
    }

    fclose(fs);

    long long memused_mib = mem_used / 1049;
    long long memtotal_mib = mem_total / 1049;

    snprintf(buf, max_size, "%lld mib / %lld mib", memused_mib, memtotal_mib);
}

// refer to pfetch source for reference.
// Using only lsb_release for now since I only care about
// a few systems.

// lsb-release -sd is slow (8.5ms)
#define OS_NAME_QUERY "cat /etc/os-release | grep PRETTY_NAME | cut -d '\"' -f 2"

void get_os(char* buf, size_t max_size) {
    FILE* pipe = popen(OS_NAME_QUERY, "r");

    ssize_t bytes_read = read(fileno(pipe), buf, 256);
    if (bytes_read == -1) {
        perror("BAD PIPE.");
        exit(1);
    }
    buf[bytes_read-1] = '\0'; // truncate newline

    pclose(pipe);
}

