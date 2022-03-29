#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <limits.h>

#define ENTRY_BUF_SIZE 128

struct entry {
    char* command;
    int trigger;
    unsigned long color;

    time_t next;
    char buf[ENTRY_BUF_SIZE];
};

struct entry* load(int* num, sigset_t* sigset) {

    char* buf = NULL;
    {
        int bufsize = 1, pos = 0, numread;
        for (;buf = realloc(buf, (bufsize+=128)), numread = fread(buf+pos, 1, 128, stdin);
              pos+=numread);
         buf[pos] = '\0';
    }

    struct entry* entries = NULL;
    *num = 0;
    sigemptyset(sigset);

    for (char* c = buf; *c; c++) {
        for (; isspace(*c); c++); 
        if (*c == '\0') break;

        if (*c == '#') {
            c = strchr(c+1, '#')+1;
            continue;
        }

        entries = realloc(entries, (++*num) * sizeof(struct entry));
        struct entry* entry = &entries[*num-1];

        entry->command = strdup(strsep(&c, ";"));

        char* end;
        entry->trigger = strtoul(strsep(&c, ";"), &end, 10);
        switch (*end) {
            case 'm': entry->trigger *= 60;             break;
            case 'h': entry->trigger *= 3600;           break;
            case '!': 
                sigaddset(sigset, SIGRTMIN+entry->trigger);
                entry->trigger = -entry->trigger; 
            break;
        }

        if (*c != ';') {
            entry->color = strtoul(c, NULL, 16);
            strsep(&c, ";");
        } else
            entry->color = 0xFFFFFF;
    }

    free(buf);
    return entries;
}

static void fetch(struct entry* entry) {
    entry->buf[0] = '\0';

    FILE* out = popen(entry->command, "r");
    if (!out) goto fail;
    fgets(entry->buf, ENTRY_BUF_SIZE, out);
    if (pclose(out) != 0) goto fail;

    strtok(entry->buf, "\n");
    return;
fail:
    strncpy(entry->buf, "Command failed", ENTRY_BUF_SIZE);
}

static void flush(struct entry* entries, int num_entries) {
    putchar('[');
    for (int i = 0; i < num_entries; i++) {
        putchar('{');

        printf("\"full_text\":\"%s\"", entries[i].buf);
        printf(",\"color\":\"#%lx\"", entries[i].color);

        putchar('}');
        if (i < num_entries-1)
            putchar(',');
    }
    puts("],");
    fflush(stdout);
}

int main(int argc, char *argv[]) {

    int num_entries;
    sigset_t rtsigset;
    struct entry* entries = load(&num_entries, &rtsigset);
    sigprocmask(SIG_BLOCK, &rtsigset, NULL);

    puts("{\"version\": 1 }");
    putchar('[');

    for (int i = 0; i < num_entries; i++) {
        fetch(&entries[i]);    
        entries[i].next = time(NULL) + entries[i].trigger;
    }
    flush(entries, num_entries);

    while (1) {
        time_t timest = time(NULL);

        time_t wait = INT_MAX;
        for (int i = 0; i < num_entries; i++) {
            if (entries[i].trigger <= 0) continue;

            if (timest >= entries[i].next) wait = 0;
            else if (entries[i].next - timest < wait)
                wait = entries[i].next - timest;
        }

        struct timespec wait_ts = {.tv_sec = wait};
        int sig = sigtimedwait(&rtsigset, NULL, &wait_ts);
        if (sig > 0)
            for (int i = 0; i < num_entries; i++)
                if (sig-SIGRTMIN == -entries[i].trigger)
                    fetch(&entries[i]);

        for (int i = 0; i < num_entries; i++) {
            if (entries[i].trigger <= 0) continue;

            timest = time(NULL);
            if (timest >= entries[i].next) {
                fetch(&entries[i]);
                entries[i].next = timest + entries[i].trigger;
            }
        }

        flush(entries, num_entries);
    }
}
