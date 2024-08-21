#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <linux/limits.h>

#include "ANSI-color-codes.h"

#define ERR_OS_BROKEN_STREAM 25

const char* puppy = 
"  /^ ^\\    \n" \
" / 0 0 \\   \n" \
" V\\ Y /V   \n" \
"  / - \\    \n" \
" /    |    \n" \
"V__) ||    \n";

// TODO: Fix formatting
/*const char* puppy = 
"      ╱‾‾‾‾╲       \n"
"     /  ╱╲  \\          \n"
"/‾‾\\―\\  ╲╱  /―/‾‾\\      \n"
"\\__/  ╲____╱  \\__/     \n";*/

struct vendor_mapping {
    const char* vendor;
    const char* prettier;
};

// prettify certain vendor names 
// TODO: Contributions welcome
static struct vendor_mapping mappings[] = {
    { "AuthenticAMD", /* --> */ "AMD" },
    { "GenuineIntel", /* --> */ "Intel" },
};

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

#define CONFIG_DIR ".config/puppyfetch"

/**
 * @brief Returns a pointer to a statically defined path to this 
 * program's config folder. Linux-only, with 4096 PATH_MAX.
 * @note This function is not thread safe. I don't see that ever
 * becoming a problem, but if it does I will probably add a 
 * mutex.
 */ 
const char* config_path();

#define arrlen(arr) (sizeof arr / sizeof *arr)

struct info_entry {
    const char* name;
    const char* value;
};

#define is_entry_null(meta) (meta.name == NULL || meta.value == NULL)

// TODO: RAM doesnt report properly
// TODO: Generalize param skips (rather than harcoding SKIP_LINEs)
// in case of position-dependence breaking
// TODO: Better parsers/buffer safety
// TODO: More descriptive exit codes etc
// TODO: Configurable options ?
// TODO: Colors
// TODO: Look into cpuid to do more native-fetching of CPU info
//        maybe not if i want to support some ARM chips, though

int main(int argc, const char** argv) {
    this_path = argv[0];
    //char username[USERNAME_BUFSZ];
    const char* username = "<could not be determined>";
    char hostname[HOSTNAME_BUFSZ];
    char cpuinfo_summary[64] = {};
    char meminfo_summary[64] = {};
    char os_name[64] = {};

    //getlogin_r(username, sizeof username);
    username = getenv("USER");
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

    // TODO: Color procedurally
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

    const char* config_dir = config_path();
    bool use_config = config_dir != NULL;

    if (use_config) {
        struct stat statbuf; 
        if (stat(config_dir, &statbuf) == -1) {
            mkdir(config_dir, 0700);
        }
    }

    for (size_t i = 0; i < arrlen(lines); ++i) {
        while (is_entry_null(lines[i]) || strlen(lines[i].value) == 0)
            ++i;
        if (i >= arrlen(lines)) break;

        art_cursor = art_drawline(art_cursor, width);
        printf("%s%s\n", lines[i].name, lines[i].value);
    }
    
    while (*art_cursor) {
        art_cursor = art_drawline(art_cursor, width);
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

// I have no idea how to do this on a system level without 
// platform-specific kernel bindings 

#define SKIP_LINE "%*[^\n]\n"
#define END_LINE "%*[\n]"
#define SCAN_CPUINFO_KEY "%[^:]:"

// TODO: As it turns out, this is architecture-dependent. I need to figure out how to 
// use cpuinfo.h to fetch this info without reading cpuinfo.
// Platforms that need patches:
//  - Various Raspberry PI devices 
//  - Android devices 
//      - OnePlus6T 

bool architecture_is_arm() {
#ifdef __arm__ 
    return true;
#endif 
    return false;
}

void get_cpuinfo_model_arm(char* buf, size_t max_size) {
    FILE* fs = fopen("/proc/cpuinfo", "r");
    if (!fs) {
        return;
    }

    char key[32];
    char arch[32];
    // TODO: cores maybe? cant compute threads. 
    
    fscanf(fs, SCAN_CPUINFO_KEY "%s\n", key, arch);
    snprintf(buf, max_size, "%s", arch);

    fclose(fs);
}

void get_cpuinfo_model(char* buf, size_t max_size) {
    if (architecture_is_arm()) {
        get_cpuinfo_model_arm(buf, max_size);
        return;
    }

    FILE* fs = fopen("/proc/cpuinfo", "r");
    if (!fs) {
        return;
    }

    char key[32];
    char vendor[32];
    int threads;
    int clkkhz;

    // skip to model name
    fscanf(fs, SKIP_LINE SCAN_CPUINFO_KEY "%s\n", key, vendor);
    // skip to siblings
    fscanf(fs, SKIP_LINE SKIP_LINE SKIP_LINE SKIP_LINE \
           SKIP_LINE SKIP_LINE SKIP_LINE SKIP_LINE \
           SCAN_CPUINFO_KEY "%d", key, &threads);

    // vendor mapping 
    for (size_t i = 0; 
            (i < sizeof mappings / sizeof (struct vendor_mapping)) 
            && mappings[i].vendor != NULL; ++i) {
        if (strcmp(mappings[i].vendor, vendor) == 0) {
            strcpy(vendor, mappings[i].prettier);
            break;
        }
    }

    // cpuinfo doesnt HAVE INFO ABOUT CPU CLOCK MAX 
    fs = freopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r", fs);
    if (!fs) {
        snprintf(buf, max_size, "%s %dt", vendor, threads);
        return;
    }
    fscanf(fs, "%d", &clkkhz);

    fclose(fs);

    snprintf(buf, max_size, "%s %dt @ %.4lf GHz", vendor, threads, ((double)clkkhz) / 1000000.0);
}

// MemUsed = Memtotal + Shmem - MemFree - Buffers - Cached - SReclaimable
// NOTE: Memory is off. Needs to be patched.

void get_meminfo_usage(char* buf, size_t max_size) {
    FILE* fs = fopen("/proc/meminfo", "r");
    if (!fs) {
        return;
    }

    long long mem_total = 0;
    long long mem_available = 0;
    long long mem_used = 0;

    char param_name[64];
    long long value;
    while (1) {
        int res = fscanf(fs, "%s %lld", param_name, &value);
        if (res == EOF) {
            break;
        }

        // Generalized because structure may vary between systems

        if (strcmp(param_name, "MemTotal:") == 0) {
            mem_total = value;
        } else if (strcmp(param_name, "MemAvailable:") == 0) {
            mem_available = value;
        }
    }

    fclose(fs);
    mem_used = mem_total - mem_available;

    long long memused_mib = (double)mem_used / 1049; // approximate. Precise
    // would be mem_used * 0.00095367431640625
    long long memtotal_mib = (double)mem_total / 1049; // same here obv

    snprintf(buf, max_size, "%lld mib / %lld mib", memused_mib, memtotal_mib);
}

#define OS_RELEASE_GUARD_LEN 20 

void get_os(char* buf, size_t max_size) {
    FILE* fs = fopen("/etc/os-release", "r");
    if (!fs) {
        return;
    }

    char keybuf[256] = {};
    
    size_t guard = OS_RELEASE_GUARD_LEN;
    while (strcmp("PRETTY_NAME", keybuf) != 0) {
        buf[max_size-1] = 0;
        fscanf(fs, "%255[^=]=%*[\"]%31[^\"]", keybuf, buf);
        fscanf(fs, SKIP_LINE);
        if (guard-- == 0) {
            fprintf(stderr, "%s: /etc/os-release read failed.\n", this_path);
            exit(ERR_OS_BROKEN_STREAM);
        }
    }

    if (buf[max_size-1] != '\0') {
        perror("Unexpected buffer overwrite.\n");
        exit(ERR_OS_BROKEN_STREAM);
    }

    fclose(fs);
}

#define STR_EMPTY(str) (!*str)

const char* config_path() {
    static char config_dir[PATH_MAX] = {'\0'};

    if (STR_EMPTY(config_dir)) {
        char* home_dir = getenv("HOME");
        if (!home_dir) {
            // fail quietly. config will not be 
            // used if not found. Add verbose arg?
            return NULL;
        }
        size_t home_dir_sz = strlen(home_dir); // null byte will be overwritten
        strncpy(config_dir, home_dir, PATH_MAX);
        strncpy(config_dir + home_dir_sz, "/" CONFIG_DIR, PATH_MAX - home_dir_sz);
    }

    return config_dir;
}

