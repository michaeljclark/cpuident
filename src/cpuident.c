#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpuident.h"

static int opt_help = 0;
static int opt_info = 0;
static int opt_env = 0;
static int opt_color = 0;

#if HAS_X86_CPUID

enum { x86_reg_eax, x86_reg_ebx, x86_reg_ecx, x86_reg_edx };

typedef struct {
    const char *name;
    int l, c, r, b;
    const char* desc;
} x86_cpu_feature;

static x86_cpu_feature cpu_feature[] = {
#define feature(name,l,c,r,b,d) { #name, l, c, x86_reg_ ## r, b, d },
#include "cpuident.inc"
#undef feature
    { NULL }
};

static int extract_bits(int val, int offset, int length)
{
    return (val >> offset) & ((1 << length) - 1);
}

static const char* cache_type_str(int type)
{
    switch(type) {
    case 1: return "data";
    case 2: return "inst";
    case 3: return "unified";
    }
    return "";
}

static void x86_dump_cache_info()
{
    int leaf[4], i = 0;
    const char *d = "-------";
    printf("%-8s%-8s%8s%8s%8s%8s%8s%8s\n",
        "level", "type", "share", "line", "part", "assoc", "sets", "size");
    printf("%-8s%-8s%8s%8s%8s%8s%8s%8s\n",
        d, d, d, d, d, d, d, d);
    do {
        __x86_cpuidex(leaf, 4, i);
        int cache_type = extract_bits(leaf[x86_reg_eax],0,5);
        int cache_level = extract_bits(leaf[x86_reg_eax],5,3);
        int cache_sharing = extract_bits(leaf[x86_reg_eax],14,12) + 1;
        int cache_cores = extract_bits(leaf[x86_reg_eax],26,6) + 1;
        int line_size = extract_bits(leaf[x86_reg_ebx],0,12) + 1;
        int partitions = extract_bits(leaf[x86_reg_ebx],12,10) + 1;
        int assoc = extract_bits(leaf[x86_reg_ebx],22,10) + 1;
        int sets = leaf[x86_reg_ecx] + 1;
        int size = line_size * assoc * sets;
        const char *unit = "B";
        if (size >= 1024*1024) { size >>= 20; unit = "MiB"; }
        else if (size >= 1024) { size >>= 10; unit = "KiB"; }
        char level_str[16], size_str[16];
        snprintf(level_str, sizeof(level_str), "L%d", cache_level);
        snprintf(size_str, sizeof(size_str), "%d%s", size, unit);
        if (cache_type == 0) break;
        printf("%-8s%-8s%8d%8d%8d%8d%8d%8s\n",
            level_str, cache_type_str(cache_type), cache_sharing,
            line_size, partitions, assoc, sets, size_str);
        i++;
    } while (i < 256);
}

static void x86_dump_features()
{
    int leaf[32][3][4] = { 0 };

    for (unsigned i = 0; i < 32; i++) {
        if (i == 0 || i <= (unsigned)leaf[0][0][0]) {
            __x86_cpuidex(leaf[i][0], i, 0);
            __x86_cpuidex(leaf[i][1], i, 1); /* ecx=1 */
        }
        if (i == 0 || i + (1<<31) <= (unsigned)leaf[0][2][0]) {
            __x86_cpuidex(leaf[i][2], i + (1<<31), 0);
        }
    }

    const char *term = getenv("TERM");
    const char *ws = "                    ";
    int color_en = term && strncmp(term, "xterm", 5) == 0;

    for (x86_cpu_feature *f = cpu_feature; f->name; f++)
    {
        int x = f->l >= 0x80000000;
        int l = f->l - (x ? 0x80000000 : 0);
        int c = x ? 2 : f->c;
        int val = !!(leaf[l][c][f->r] & (1 << f->b));
        if (opt_env) {
            printf("%sx86_%s=%d", f != cpu_feature ? ";" : "", f->name, val);
        } else if (opt_color) {
            printf("%sx86_%s=%d%s# %s%s\n", val ? "\x1b[32m" : "\x1b[31m",
                f->name, val, ws+strlen(f->name), f->desc, "\x1b[0m");
        } else {
            printf("x86_%s=%d%s# %s\n",
                f->name, val, ws+strlen(f->name), f->desc);
        }
    }
}

static void x86_dump_cpu_name()
{
    int leaf_0[4], leaf_2[4], leaf_3[4], leaf_4[4];
    char cpu_name[64] = "unknown";

    __x86_cpuidex(leaf_0, 0x80000000, 0);

    if (leaf_0[x86_reg_eax] >= 0x80000004)
    {
        __x86_cpuidex(leaf_2, 0x80000002, 0);
        __x86_cpuidex(leaf_3, 0x80000003, 0);
        __x86_cpuidex(leaf_4, 0x80000004, 0);
        memcpy(cpu_name + 0x00, leaf_2, 0x10);
        memcpy(cpu_name + 0x10, leaf_3, 0x10);
        memcpy(cpu_name + 0x20, leaf_4, 0x10);
    }

    printf("# %s\n", cpu_name);
}
#endif

static void x86_cpuident()
{
#if HAS_X86_CPUID
    x86_dump_cpu_name();

    if (opt_info) {
        x86_dump_cache_info();
    } else {
        x86_dump_features();
    }
#else
    fprintf(stderr, "error: this utility requires an x86 cpu\n");
    exit(1);
#endif
}

/*
 * option processing
 */

static void print_help(int argc, char **argv)
{
    fprintf(stderr,
        "usage: %s [options]\n"
        "\n"
        "Options:\n"
        "  -i, --info                         print cache info\n"
        "  -e, --env                          print as variables\n"
        "  -c, --color                        print in ANSI color\n"
        "  -h, --help                         command line help\n",
        argv[0]
    );
}

static int match_opt(const char *arg, const char *opt, const char *longopt)
{
    return strcmp(arg, opt) == 0 || strcmp(arg, longopt) == 0;
}

static void parse_options(int argc, char **argv)
{
    int i = 1;
    while (i < argc) {
        if (match_opt(argv[i], "-h", "--help")) {
            opt_help++;
            i++;
        } else if (match_opt(argv[i], "-i", "--info")) {
            opt_info++;
            i++;
        } else if (match_opt(argv[i], "-e", "--env")) {
            opt_env++;
            i++;
        } else if (match_opt(argv[i], "-c", "--color")) {
            opt_color++;
            i++;
        } else if (match_opt(argv[i], "-C", "--cache")) {
            opt_color++;
            i++;
        } else {
            fprintf(stderr, "error: unknown option: %s\n", argv[i]);
            opt_help++;
            break;
        }
    }

    if (opt_help) {
        print_help(argc, argv);
        exit(1);
    }
}

int main(int argc, char **argv)
{
    parse_options(argc, argv);
    x86_cpuident();
}