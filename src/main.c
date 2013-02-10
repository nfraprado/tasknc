/*
 * debug main during restructuring of tasknc
 * by mjheagle
 */

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "configure.h"
#include "sort.h"
#include "task.h"
#include "tasklist.h"

/* macros */
#define clean(t, c) { free_config(c); free_tasks(t); }

/* local functions */
typedef int (*action)(struct task **, struct config *);
int dump_config(struct task ** tasks, struct config * conf);
int print_tasks(struct task ** tasks, struct config * conf);
int version(struct task ** tasks, struct config * conf);
void help();

int main(int argc, char ** argv) {
        /* initialize */
        struct config *conf = default_config();
        struct task ** tasks = NULL;
        action run = NULL;
        bool need_tasks = false;
        bool need_conf = false;

        /* determine which action to take */
        static struct option long_opt[] = {
                {"cfgdump",     no_argument,            0, 'd'},
                {"filter",      required_argument,      0, 'f'},
                {"help",        no_argument,            0, 'h'},
                {"print",       no_argument,            0, 'p'},
                {"sort",        required_argument,      0, 's'},
                {"version",     no_argument,            0, 'v'},
                {0,             0,                      0, 0}
        };
        int opt_index = 0;
        int c;
        while ((c = getopt_long(argc, argv, "df:hps:v", long_opt, &opt_index)) != -1) {
                switch (c) {
                        case 'd':
                                run = dump_config;
                                need_conf = true;
                                break;
                        case 'f':
                                conf_set_filter(conf, optarg);
                                break;
                        case 'p':
                                run = print_tasks;
                                need_tasks = true;
                                break;
                        case 's':
                                conf_set_sort(conf, optarg);
                                break;
                        case 'v':
                                run = version;
                                break;
                        case 'h':
                        case '?':
                        default:
                                help();
                                return 1;
                                break;
                }
        }

        /* set action to tasklist if no action was set */
        if (run == NULL) {
                run = tasklist_window;
                need_tasks = true;
                need_conf = true;
        }

        /* get necessary variables */
        if (need_tasks) {
                tasks = get_tasks(conf_get_filter(conf));
                sort_tasks(tasks, 0, conf_get_sort(conf));
        }

        /* run function */
        if (run)
                return run(tasks, conf);
        else {
                printf("no action to run\n");
                return 1;
        }
}

/* get task version */
int version(struct task ** tasks, struct config * conf) {
        int ret = 0;
        int * version = conf_get_version(conf);
        if (version != NULL)
                printf("task version: %d.%d.%d\n", version[0], version[1], version[2]);
        else
                ret = 1;

        clean(tasks, conf);

        return ret;
}

/* display task list */
int print_tasks(struct task ** tasks, struct config *conf) {
        struct task ** t;
        for (t = tasks; *t != NULL; t++)
                printf("%d:%s\n", task_get_index(*t), task_get_description(*t));

        clean(tasks, conf);

        return 0;
}

/* dump config */
int dump_config(struct task ** tasks, struct config *conf) {
        printf("%s: %d\n", "nc_timeout", conf_get_nc_timeout(conf));
        int * version = conf_get_version(conf);
        if (version)
                printf("%s: %d.%d.%d\n", "version", version[0], version[1], version[2]);
        printf("%s: '%s'\n", "filter", conf_get_filter(conf));
        printf("%s: '%s'\n", "sort", conf_get_sort(conf));

        clean(tasks, conf);

        return 0;
}

/* help function */
void help() {
        fprintf(stderr, "\nUsage: %s [options]\n\n", PROGNAME);
        fprintf(stderr, "  Options:\n"
                        "    -d, --cfgdump      dump the configuration settings\n"
                        "    -f, --filter       set the task list filter\n"
                        "    -h, --help         print this help message\n"
                        "    -p, --print        print task list to stdout\n"
                        "    -s, --sort         set the task list sort mode\n"
                        "    -v, --version      print task version\n"
               );
}
