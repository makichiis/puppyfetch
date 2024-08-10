#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>

#include "ANSI-color-codes.h"

#define ERR_OS_BROKEN_STREAM 25

const char* puppy = 
"  /^ ^\\    \n" \
" / 0 0 \\   \n" \
" V\\ Y /V   \n" \
"  / - \\    \n" \
" /    |    \n" \
"V__) ||    \n";

/**
 * @brief Get the square width of a string whose rows are delimited by newlines.
 * The square width refers to the longest "line" (newline-delimited substring) 
 * in `art`.
 * @return The length of the largest line in the newline-separated string.
 */ 
size_t get_art_square_width(const char* art);

/**
 * @brief Write up to `total_width` ASCII characters to stdout, or until a 
 * newline is read. If the number of ASCII characters written is less than 
 * `total_width`, `2 + total_width - written` whitespace characters will be written 
 * to stdout. If NULL is passed, `2 + total_width` characters are written to stdout.
 * @return A pointer to 1 + the address of the newline scanned, or NULL if a 
 * null-terminating character ('\0') is read.
 */ 
const char* art_drawline(const char* art_cursor, size_t total_width);

// https://man7.org/linux/man-pages/man7/hostname.7.html
#define HOSTNAME_BUFSZ _SC_HOST_NAME_MAX  
// just betting on https://bbs.archlinux.org/viewtopic.php?id=251984 and 
// https://unix.stackexchange.com/questions/220542/linux-unix-username-length-limit
#define USERNAME_BUFSZ _SC_LOGIN_NAME_MAX  

#define PROC_CPUINFO_PATH "/proc/cpuinfo"
#define PROC_MEMINFO_PATH "/proc/meminfo"

static const char* this_path;

// deprecated 
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

/**
 * @brief Get OS name.
 */  
void get_os(char* buf, size_t max_size);

#define arrlen(arr) (sizeof arr / sizeof *arr)

struct info_entry {
    const char* name;
    const char* value;
};

#define is_entry_null(meta) (meta.name == NULL)

// TODO: RAM doesnt report properly
// TODO: Generalize param skips (rather than harcoding SKIP_LINEs)
// in case of position-dependence breaking
// TODO: Configurable options ?
// TODO: Colors

int main(int argc, const char** argv) {
    this_path = argv[0];
    char username[USERNAME_BUFSZ];
    char hostname[HOSTNAME_BUFSZ];
    char cpuinfo_summary[64];
    char meminfo_summary[64];
    char os_name[64];

    getlogin_r(username, sizeof username);
    gethostname(hostname, sizeof hostname);
    get_cpuinfo_model(cpuinfo_summary, sizeof cpuinfo_summary);
    get_meminfo_usage(meminfo_summary, sizeof meminfo_summary);
    get_os(os_name, sizeof os_name);

    struct utsname name;
    if (uname(&name) == -1) {
        fprintf(stderr, "%s: uname() failed with errno %d\n", 
                this_path, errno);
        exit(1);
    }

    char userbuf[USERNAME_BUFSZ+HOSTNAME_BUFSZ + 64]; // 64 for color padding
    snprintf(userbuf, sizeof userbuf, HMAG "%s" reset "@" HBLU "%s" reset, username, hostname);

    struct info_entry lines[6] = {
        { "", userbuf },
        { HBLU "os     " reset, os_name },
        { HMAG "cpu    " reset, cpuinfo_summary }, 
        { BWHT "kernel " reset, name.release },
        { HMAG "server " reset, getenv("XDG_SESSION_TYPE") },
        { HBLU "memory " reset, meminfo_summary },
    };

    const char* art = puppy;
    const char* art_cursor = art;
    size_t width = get_art_square_width(art);
    
    for (size_t i = 0; i < arrlen(lines); ++i) {
        art_cursor = art_drawline(art_cursor, width);
        if (!is_entry_null(lines[i]))
            printf("%s%s", lines[i].name, lines[i].value);
        puts("");
    }
}

#define max(a, b) (a > b ? a : b)

size_t get_art_square_width(const char* art) {
    size_t max_width = 0;
    size_t width = 0;

    while (*art) {
        if (*art == '\n') {
            max_width = max(max_width, width);
            width = 0;
        } else {
            ++width;
        }

        ++art;
    }

    return max_width;
}

const char* art_drawline(const char* art_cursor, size_t total_width) {
    size_t written = 0;

    while (*art_cursor && *art_cursor != '\n') {
        putc(*art_cursor++, stdout);
        ++written;
    }

    while (written < total_width) {
        putc(' ', stdout);
        ++written;
    }

    return 1 + art_cursor;
}

// deprecated
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

#define SKIP_LINE "%*[^\n]\n"
#define SCAN_CPUINFO_KEY "%[^:]:"

void get_cpuinfo_model(char* buf, size_t max_size) {
    FILE* fs = fopen("/proc/cpuinfo", "r");

    char key[32];
    char model_prefix[32];
    int threads;
    int clkkhz;

    // skip to model name
    fscanf(fs, SKIP_LINE SKIP_LINE SKIP_LINE SKIP_LINE SCAN_CPUINFO_KEY "%s", key, model_prefix);
    // skip to siblings
    fscanf(fs, SKIP_LINE SKIP_LINE SKIP_LINE SKIP_LINE\
           SKIP_LINE SKIP_LINE SCAN_CPUINFO_KEY "%d", key, &threads);

    // cpuinfo doesnt HAVE INFO ABOUT CPU CLOCK MAX 
    fs = freopen("/sys/devices/system/cpu/cpu0/cpufreq/bios_limit", "r", fs);
    fscanf(fs, "%d", &clkkhz);

    fclose(fs);

    snprintf(buf, max_size, "%s %dt @ %.4lf GHz", model_prefix, threads, ((double)clkkhz) / 1000000.0);
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
            break;
        } else if (strcmp(param_name, "Cached:") == 0) {
            mem_used -= value;
        }
    }

    fclose(fs);

    long long memused_mib = mem_used / 1049;
    long long memtotal_mib = mem_total / 1049;

    snprintf(buf, max_size, "%lld mib / %lld mib", memused_mib, memtotal_mib);
}

#define OS_RELEASE_GUARD_LEN 20 

void get_os(char* buf, size_t max_size) {
    FILE* fs = fopen("/etc/os-release", "r");

    char keybuf[256] = {};
    
    size_t guard = OS_RELEASE_GUARD_LEN;
    while (strcmp("PRETTY_NAME", keybuf) != 0) {
        buf[max_size-1] = 0;
        fscanf(fs, "%255[^=]=%*[\"]%31s", keybuf, buf);
        fscanf(fs, "%*[^\n]\n");
        if (guard-- == 0) {
            fprintf(stderr, "%s: /etc/os-release read failed.\n", this_path);
            exit(ERR_OS_BROKEN_STREAM);
        }
    }

    if (buf[max_size-1] != '\0') {
        perror("Shit broke here and I'm gonna quit while we're ahead in case this is an exploit.");
        exit(ERR_OS_BROKEN_STREAM);
    }

    fclose(fs);
}

