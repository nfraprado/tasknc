/*
 * parse options for tasknc
 * by mjheagle
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "configure.h"
#include "common.h"
#include "task.h"

/* error codes */
#define CONFIG_SUCCESS 0
#define CONFIG_ERROR_ARGC 2
#define CONFIG_CMD_UNHANDLED 3
#define CONFIG_VAR_UNHANDLED 4

/* configuration struct */
struct config {
        bool debug;
        char *filter;
        char *sort;
        char *task_format;
        FILE *logfd;
        int nc_timeout;
        int *version;
};

/* allocate a config struct with default options */
struct config *default_config() {
        struct config *conf = calloc(1, sizeof(struct config));

        conf->debug = false;
        conf->nc_timeout = 1000;
        conf->filter = strdup("status:pending");
        conf->sort = strdup("n");
        conf->task_format = strdup("%3n (%-10p) %d");

        return conf;
}

/* free allocated fields in a config and the struct itself */
void free_config(struct config *conf) {
        check_free(conf->version);
        check_fclose(conf->logfd);
        check_free(conf->filter);
        check_free(conf->sort);
        check_free(conf->task_format);

        free(conf);
}

/* function to parse an integer from a string */
int config_parse_int(char *str) {
        int ret = 0;
        sscanf(str, "%d", &ret);

        return ret;
}

/* function to set a config variable */
int config_set(struct config *conf, const int argc, char **argv) {
        if (argc != 2)
                return CONFIG_ERROR_ARGC;

        int ret = CONFIG_SUCCESS;

        if (strcmp(argv[0], "nc_timeout") == 0)
                conf->nc_timeout = config_parse_int(argv[1]);
        else if (strcmp(argv[0], "logpath") == 0) {
                check_fclose(conf->logfd);
                conf->logfd = fopen(argv[1], "a");
        }
        else if (strcmp(argv[0], "filter") == 0)
                conf_set_filter(conf, argv[1]);
        else if (strcmp(argv[0], "sort") == 0)
                conf_set_sort(conf, argv[1]);
        else if (strcmp(argv[0], "task_format") == 0)
                conf_set_task_format(conf, argv[1]);
        else
                ret = CONFIG_VAR_UNHANDLED;

        return ret;
}

/* parse a config string and evaluate its instructions */
int config_parse_string(struct config *conf, char *str) {
        /* debug */
        char *p = strchr(str, '\n');
        if (p != NULL)
                *p = 0;

        int ret = CONFIG_SUCCESS;

        /* split into individual args */
        int nargs = 4;
        int argc = 0;
        char **argv = calloc(nargs, sizeof(char *));
        while (*str != 0) {
                /* check for comment */
                if (*str == '#')
                        break;

                /* check for whitespace */
                if (*str == ' ' || *str == '\t' || *str == '\n') {
                        str++;
                        continue;
                }

                /* resize argv if necessary */
                if (argc>=nargs-1) {
                        nargs *= 2;
                        argv = realloc(argv, nargs*sizeof(char *));
                }

                /* get this arg */
                char *end = NULL;
                if (*str == '"') {
                        end = strchrnul(str+1, '"');
                        str++;
                }
                else
                        end = strchrnul(str, ' ');
                const int len = end-str;
                if (len == 0)
                        continue;
                argv[argc] = calloc(len, sizeof(char));
                strncpy(argv[argc], str, len);

                /* move to next field */
                argc++;
                if (*str == '"')
                        end++;
                str = end;
        }

        /* shrink argv to only necessary size */
        argv = realloc(argv, (1+argc)*sizeof(char *));

        /* pass arguments to next function */
        if (argc == 0)
                ret = 1;
        else if (strcmp(argv[0], "set") == 0)
                ret = config_set(conf, argc-1, argv+1);
        else
                ret = 1;

        /* free allocated memory */
        int n;
        for (n = 0; n < argc; n++)
                free(argv[n]);
        free(argv);

        return ret;
}

/* parse a config file descriptor by handling each of its strings */
int config_parse_file(struct config *conf, FILE *file) {
        int linelen = 256;

        char *line = calloc(linelen, sizeof(char));
        while (fgets(line, linelen-1, file) != NULL || !feof(file)) {
                /* check for longer lines */
                while (strchr(line, '\n') == NULL) {
                        char *tmp = calloc(linelen, sizeof(char));
                        if (fgets(tmp, linelen-1, file) == NULL)
                                break;
                        line = realloc(line, 2*linelen*sizeof(char));
                        strncat(line, tmp, linelen-1);
                        free(tmp);
                        linelen *= 2;
                }

                /* parse line */
                config_parse_string(conf, line);
                memset(line, 0, linelen);
        }
        free(line);

        return CONFIG_SUCCESS;
}

/* get version */
int *conf_get_version(struct config *conf) {
        if (conf->version == NULL)
                conf->version = task_version();

        return conf->version;
}

/* get nc_timeput from configuration struct */
int conf_get_nc_timeout(struct config *conf) {
        return conf->nc_timeout;
}

/* get log file descriptor from configuration struct */
FILE *conf_get_logfd(struct config *conf) {
        return conf->logfd;
}

/* get task filter */
char *conf_get_filter(struct config *conf) {
        return conf->filter;
}

/* set task filter */
void conf_set_filter(struct config *conf, char *filter) {
        check_free(conf->filter);
        conf->filter = strdup(filter);
}

/* get task sort */
char *conf_get_sort(struct config *conf) {
        return conf->sort;
}

/* set task sort */
void conf_set_sort(struct config *conf, char *sort) {
        check_free(conf->sort);
        conf->sort = strdup(sort);
}

/* get task format */
char *conf_get_task_format(struct config *conf) {
        return conf->task_format;
}

/* set task format */
void conf_set_task_format(struct config *conf, char *task_format) {
        check_free(conf->task_format);
        conf->task_format = strdup(task_format);
}

/* get debug */
bool conf_get_debug(struct config *conf) {
        return conf->debug;
}

/* set debug */
void conf_set_debug(struct config *conf, bool debug) {
        conf->debug = debug;
}

/* dump config to file*/
void dump_config_file(FILE *out, struct config *conf) {
        fprintf(out, "%s:\t\t%d\n", "debug", conf_get_debug(conf));
        fprintf(out, "%s:\t%d\n", "nc_timeout", conf_get_nc_timeout(conf));
        int * version = conf_get_version(conf);
        if (version)
                printf("%s:\t%d.%d.%d\n", "version", version[0], version[1], version[2]);
        fprintf(out, "%s:\t\t'%s'\n", "filter", conf_get_filter(conf));
        fprintf(out, "%s:\t\t'%s'\n", "sort", conf_get_sort(conf));
        fprintf(out, "%s:\t'%s'\n", "task_format", conf_get_task_format(conf));
}
