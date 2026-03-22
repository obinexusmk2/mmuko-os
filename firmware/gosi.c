/*
 * gosi — Gosilang Package Manager & Executable Builder
 * OBINexus Computing | gosilang pkg system v0.1
 * Author: Nnamdi Michael Okpala
 *
 * Pipeline:
 *   riftlang.exe → .so.a → rift.exe → gosilang → .gosi (executable pkg)
 *
 * Usage:
 *   gosi build                  — build from gosi.toml
 *   gosi run <pkg.gosi>         — run a .gosi package
 *   gosi install <pkg.gosi>     — install to system
 *   gosi new <name>             — scaffold new package
 *   gosi clean                  — clean build artifacts
 *   gosi info <pkg.gosi>        — inspect package manifest
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define GOSI_VERSION    "0.1.0"
#define GOSI_MANIFEST   "gosi.toml"
#define GOSI_BUILD_DIR  "build"
#define GOSI_PKG_DIR    "pkg"
#define GOSI_SRC_DIR    "src"

/* ─── Colour codes for terminal output ─── */
#define RED     "\033[0;31m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define BLUE    "\033[0;34m"
#define RESET   "\033[0m"

/* ─── Drift colour states (maps to Drift Theorem) ─── */
typedef enum {
    DRIFT_RED    = 0,   /* diverging  — build error     */
    DRIFT_YELLOW = 1,   /* unresolved — building        */
    DRIFT_GREEN  = 2    /* converging — build success   */
} DriftState;

/* ─── Package manifest (parsed from gosi.toml) ─── */
typedef struct {
    char name[128];
    char version[32];
    char author[128];
    char entry[256];
    char output[256];
    int  hacc;
    int  zero_trust;
    int  humanitarian;
} GosiManifest;

/* ─── Forward declarations ─── */
static void print_banner(void);
static void print_drift(DriftState state, const char* msg);
static int  cmd_build(void);
static int  cmd_run(const char* pkg_path);
static int  cmd_install(const char* pkg_path);
static int  cmd_new(const char* name);
static int  cmd_clean(void);
static int  cmd_info(const char* pkg_path);
static int  parse_manifest(const char* path, GosiManifest* m);
static int  dir_exists(const char* path);
static int  make_dirs(void);

/* ─── Entry point ─── */
int main(int argc, char* argv[]) {
    print_banner();

    if (argc < 2) {
        fprintf(stderr, "Usage: gosi <command> [args]\n\n");
        fprintf(stderr, "Commands:\n");
        fprintf(stderr, "  build              Build package from gosi.toml\n");
        fprintf(stderr, "  run   <pkg.gosi>   Run a .gosi package\n");
        fprintf(stderr, "  install <pkg.gosi> Install package to system\n");
        fprintf(stderr, "  new   <name>       Scaffold new gosilang package\n");
        fprintf(stderr, "  clean              Clean build artifacts\n");
        fprintf(stderr, "  info  <pkg.gosi>   Inspect package\n");
        fprintf(stderr, "  version            Show gosi version\n");
        return 1;
    }

    const char* cmd = argv[1];

    if (strcmp(cmd, "build") == 0) {
        return cmd_build();
    } else if (strcmp(cmd, "run") == 0 && argc >= 3) {
        return cmd_run(argv[2]);
    } else if (strcmp(cmd, "install") == 0 && argc >= 3) {
        return cmd_install(argv[2]);
    } else if (strcmp(cmd, "new") == 0 && argc >= 3) {
        return cmd_new(argv[2]);
    } else if (strcmp(cmd, "clean") == 0) {
        return cmd_clean();
    } else if (strcmp(cmd, "info") == 0 && argc >= 3) {
        return cmd_info(argv[2]);
    } else if (strcmp(cmd, "version") == 0) {
        printf("gosi %s — OBINexus Gosilang Package Manager\n", GOSI_VERSION);
        printf("Toolchain: riftlang.exe → .so.a → rift.exe → gosilang\n");
        return 0;
    } else {
        fprintf(stderr, RED "error:" RESET " unknown command '%s'\n", cmd);
        return 1;
    }
}

/* ─── Banner ─── */
static void print_banner(void) {
    printf(BLUE
        "╔════════════════════════════════════════╗\n"
        "║  gosi — Gosilang Package Manager       ║\n"
        "║  OBINexus Computing  v" GOSI_VERSION "          ║\n"
        "║  #hacc  #noghosting  #sorrynotsorry    ║\n"
        "╚════════════════════════════════════════╝\n"
        RESET "\n");
}

/* ─── Drift-coloured status output ─── */
static void print_drift(DriftState state, const char* msg) {
    switch (state) {
        case DRIFT_RED:
            printf(RED    "  [RED   ✗]" RESET " %s\n", msg);
            break;
        case DRIFT_YELLOW:
            printf(YELLOW " [YELLOW ◎]" RESET " %s\n", msg);
            break;
        case DRIFT_GREEN:
            printf(GREEN  "  [GREEN ✓]" RESET " %s\n", msg);
            break;
    }
}

/* ─── Build command ─── */
static int cmd_build(void) {
    printf("Building package...\n\n");

    /* Check manifest exists */
    if (access(GOSI_MANIFEST, F_OK) != 0) {
        print_drift(DRIFT_RED, "gosi.toml not found — run 'gosi new <name>' first");
        return 1;
    }

    GosiManifest m;
    if (parse_manifest(GOSI_MANIFEST, &m) != 0) {
        print_drift(DRIFT_RED, "Failed to parse gosi.toml");
        return 1;
    }

    printf("  Package : %s v%s\n", m.name, m.version);
    printf("  Author  : %s\n", m.author);
    printf("  Entry   : %s\n", m.entry);
    printf("  Output  : %s\n\n", m.output);

    /* Create build directories */
    make_dirs();

    /* Stage 1: riftlang.exe → .so.a */
    print_drift(DRIFT_YELLOW, "Stage 1: riftlang.exe — parsing .gs sources");
    char stage1[512];
    snprintf(stage1, sizeof(stage1),
        "riftlang.exe %s -o %s/rift/ 2>/dev/null || "
        "echo '[STUB] riftlang stage — .so.a output pending'",
        m.entry, GOSI_BUILD_DIR);
    system(stage1);
    print_drift(DRIFT_YELLOW, "Stage 1 complete → build/rift/*.so.a");

    /* Stage 2: rift.exe → .gs.bin */
    print_drift(DRIFT_YELLOW, "Stage 2: rift.exe — compiling .so.a to bytecode");
    char stage2[512];
    snprintf(stage2, sizeof(stage2),
        "rift.exe %s/rift/ -o %s/bin/ 2>/dev/null || "
        "echo '[STUB] rift.exe stage — .gs.bin output pending'",
        GOSI_BUILD_DIR, GOSI_BUILD_DIR);
    system(stage2);
    print_drift(DRIFT_YELLOW, "Stage 2 complete → build/bin/*.gs.bin");

    /* Stage 3: gosilang → .gosi package */
    print_drift(DRIFT_YELLOW, "Stage 3: gosilang — linking executable package");
    char stage3[512];
    snprintf(stage3, sizeof(stage3),
        "gosilang %s/bin/ -o %s --hacc --no-ghost 2>/dev/null || "
        "echo '[STUB] gosilang link stage — .gosi output pending'",
        GOSI_BUILD_DIR, m.output);
    system(stage3);

    /* Create stub .gosi package for now */
    char stub_path[256];
    snprintf(stub_path, sizeof(stub_path), "%s/%s.gosi", GOSI_PKG_DIR, m.name);
    FILE* stub = fopen(stub_path, "wb");
    if (stub) {
        /* GOSI magic header */
        fprintf(stub, "GOSI\x00\x01");               /* magic + version */
        fprintf(stub, "PKG:%s\n", m.name);
        fprintf(stub, "VER:%s\n", m.version);
        fprintf(stub, "AUT:%s\n", m.author);
        fprintf(stub, "ENT:%s\n", m.entry);
        fprintf(stub, "FLG:%s%s%s\n",
            m.hacc         ? "hacc,"        : "",
            m.zero_trust   ? "zero_trust,"  : "",
            m.humanitarian ? "humanitarian" : "");
        fclose(stub);
        print_drift(DRIFT_GREEN, stub_path);
    }

    printf("\n");
    print_drift(DRIFT_GREEN, "Build complete");
    printf("\n  Run with: " BLUE "gosi run %s\n" RESET, stub_path);
    return 0;
}

/* ─── Run command ─── */
static int cmd_run(const char* pkg_path) {
    printf("Running package: %s\n\n", pkg_path);

    if (access(pkg_path, F_OK) != 0) {
        print_drift(DRIFT_RED, "Package not found — run 'gosi build' first");
        return 1;
    }

    print_drift(DRIFT_YELLOW, "Loading .gosi package into RIFT runtime...");

    /* Read and display package header */
    FILE* f = fopen(pkg_path, "rb");
    if (f) {
        char line[256];
        /* Skip magic bytes */
        fread(line, 1, 6, f);
        printf("\n  Package contents:\n");
        while (fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            printf("    %s\n", line);
        }
        fclose(f);
    }

    printf("\n");
    print_drift(DRIFT_YELLOW, "Executing via gosilang runtime...");

    char run_cmd[512];
    snprintf(run_cmd, sizeof(run_cmd),
        "gosilang --run %s 2>/dev/null || "
        "echo '[STUB] gosilang runtime pending — .gosi execution stub'",
        pkg_path);
    system(run_cmd);

    print_drift(DRIFT_GREEN, "Execution complete");
    return 0;
}

/* ─── Install command ─── */
static int cmd_install(const char* pkg_path) {
    printf("Installing: %s\n\n", pkg_path);

    if (access(pkg_path, F_OK) != 0) {
        print_drift(DRIFT_RED, "Package not found");
        return 1;
    }

    print_drift(DRIFT_YELLOW, "Installing to /usr/local/lib/gosi/...");

    char install_cmd[512];
    snprintf(install_cmd, sizeof(install_cmd),
        "install -d /usr/local/lib/gosi && "
        "install -m 755 %s /usr/local/lib/gosi/ 2>/dev/null || "
        "echo '[INFO] Run with sudo for system install'",
        pkg_path);
    system(install_cmd);

    print_drift(DRIFT_GREEN, "Installed");
    return 0;
}

/* ─── New package scaffold ─── */
static int cmd_new(const char* name) {
    printf("Scaffolding new package: %s\n\n", name);

    /* Create directory structure */
    char path[256];
    snprintf(path, sizeof(path), "%s", name);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/src", name);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/build", name);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/build/rift", name);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/build/bin", name);
    mkdir(path, 0755);
    snprintf(path, sizeof(path), "%s/pkg", name);
    mkdir(path, 0755);

    /* Write gosi.toml */
    char toml_path[256];
    snprintf(toml_path, sizeof(toml_path), "%s/gosi.toml", name);
    FILE* toml = fopen(toml_path, "w");
    if (toml) {
        fprintf(toml,
            "[package]\n"
            "name        = \"%s\"\n"
            "version     = \"0.1.0\"\n"
            "author      = \"\"\n"
            "license     = \"OBINexus-OpenSense\"\n"
            "description = \"\"\n"
            "entry       = \"src/main.gs\"\n\n"
            "[build]\n"
            "toolchain   = \"rift\"\n"
            "target      = \"gosi-bin\"\n"
            "output      = \"pkg/%s.gosi\"\n\n"
            "[build.nlink]\n"
            "tree_shake  = true\n"
            "nomeltdown  = true\n"
            "thread_safe = true\n\n"
            "[features]\n"
            "hacc        = true\n",
            name, name);
        fclose(toml);
        print_drift(DRIFT_GREEN, toml_path);
    }

    /* Write main.gs entry point */
    char gs_path[256];
    snprintf(gs_path, sizeof(gs_path), "%s/src/main.gs", name);
    FILE* gs = fopen(gs_path, "w");
    if (gs) {
        fprintf(gs,
            "// %s — Gosilang Package\n"
            "// OBINexus Computing | #hacc\n\n"
            "@hacc\n"
            "actor Main {\n"
            "    state: isolated;\n\n"
            "    fn run() -> void {\n"
            "        // Entry point\n"
            "        // Your gosilang code here\n"
            "    }\n"
            "}\n\n"
            "GOSSIP main TO Main {\n"
            "    Main.run()\n"
            "}\n",
            name);
        fclose(gs);
        print_drift(DRIFT_GREEN, gs_path);
    }

    /* Write README */
    char readme_path[256];
    snprintf(readme_path, sizeof(readme_path), "%s/README.md", name);
    FILE* readme = fopen(readme_path, "w");
    if (readme) {
        fprintf(readme,
            "# %s\n\n"
            "A Gosilang package.\n\n"
            "## Build\n\n"
            "```\ngosi build\n```\n\n"
            "## Run\n\n"
            "```\ngosi run pkg/%s.gosi\n```\n\n"
            "---\n"
            "*OBINexus Computing — #hacc #noghosting*\n",
            name, name);
        fclose(readme);
        print_drift(DRIFT_GREEN, readme_path);
    }

    printf("\n");
    print_drift(DRIFT_GREEN, "Package scaffolded");
    printf("\n  Next steps:\n");
    printf("    cd %s\n", name);
    printf("    " BLUE "gosi build\n" RESET);
    return 0;
}

/* ─── Clean command ─── */
static int cmd_clean(void) {
    printf("Cleaning build artifacts...\n\n");
    print_drift(DRIFT_YELLOW, "Removing build/");
    system("rm -rf build/rift build/bin 2>/dev/null || true");
    print_drift(DRIFT_GREEN, "Clean complete");
    return 0;
}

/* ─── Info command ─── */
static int cmd_info(const char* pkg_path) {
    printf("Package info: %s\n\n", pkg_path);

    FILE* f = fopen(pkg_path, "rb");
    if (!f) {
        print_drift(DRIFT_RED, "Cannot open package");
        return 1;
    }

    char line[256];
    fread(line, 1, 6, f); /* skip magic */
    printf("  %-12s %s\n", "File:", pkg_path);
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (strncmp(line, "PKG:", 4) == 0)
            printf("  %-12s %s\n", "Name:", line + 4);
        else if (strncmp(line, "VER:", 4) == 0)
            printf("  %-12s %s\n", "Version:", line + 4);
        else if (strncmp(line, "AUT:", 4) == 0)
            printf("  %-12s %s\n", "Author:", line + 4);
        else if (strncmp(line, "FLG:", 4) == 0)
            printf("  %-12s %s\n", "Flags:", line + 4);
    }
    fclose(f);

    printf("\n");
    print_drift(DRIFT_GREEN, "OK");
    return 0;
}

/* ─── Parse gosi.toml (minimal key=value parser) ─── */
static int parse_manifest(const char* path, GosiManifest* m) {
    FILE* f = fopen(path, "r");
    if (!f) return -1;

    memset(m, 0, sizeof(GosiManifest));
    strcpy(m->version, "0.1.0");
    strcpy(m->output,  "pkg/out.gosi");

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        /* Strip quotes from value */
        char key[128], val[256];
        if (sscanf(line, " name = \"%[^\"]\"", val) == 1)
            strncpy(m->name, val, sizeof(m->name)-1);
        else if (sscanf(line, " version = \"%[^\"]\"", val) == 1)
            strncpy(m->version, val, sizeof(m->version)-1);
        else if (sscanf(line, " author = \"%[^\"]\"", val) == 1)
            strncpy(m->author, val, sizeof(m->author)-1);
        else if (sscanf(line, " entry = \"%[^\"]\"", val) == 1)
            strncpy(m->entry, val, sizeof(m->entry)-1);
        else if (sscanf(line, " output = \"%[^\"]\"", val) == 1)
            strncpy(m->output, val, sizeof(m->output)-1);
        else if (strstr(line, "hacc") && strstr(line, "true"))
            m->hacc = 1;
        else if (strstr(line, "zero_trust") && strstr(line, "true"))
            m->zero_trust = 1;
        else if (strstr(line, "humanitarian") && strstr(line, "true"))
            m->humanitarian = 1;
    }

    fclose(f);
    return (m->name[0] && m->entry[0]) ? 0 : -1;
}

/* ─── Utility: check dir exists ─── */
static int dir_exists(const char* path) {
    struct stat s;
    return (stat(path, &s) == 0 && S_ISDIR(s.st_mode));
}

/* ─── Utility: make build dirs ─── */
static int make_dirs(void) {
    mkdir(GOSI_BUILD_DIR,          0755);
    mkdir(GOSI_BUILD_DIR "/rift",  0755);
    mkdir(GOSI_BUILD_DIR "/bin",   0755);
    mkdir(GOSI_PKG_DIR,            0755);
    return 0;
}
