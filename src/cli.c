/*
 * blts-vcard-xport-perf - vcard storage performance test suite
 *
 * Copyright (C) 2013 Jolla Ltd.
 * Contact: Martin Kampas <martin.kampas@jollamobile.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <limits.h>
#include <stdlib.h>
#include <time.h>

#include <blts_cli_frontend.h>
#include <blts_log.h>

#include "blts-vcard-xport-perf.h"

enum
{
    MANAGER_NAME_MAX = 64,
};

typedef struct
{
    char manager_name[MANAGER_NAME_MAX];
    char tmp_dir[PATH_MAX];
    int n_contacts;
} test_execution_params;

static void help(const char* help_msg_base)
{
    fprintf(stdout, help_msg_base,
        "-m contact-manager "
        "-d tmp-directory "
        "-n n-contacts"
        ,
        "-m: Contact manager backend to import (export) to (from)\n"
        "-d: Temporary directory path to import (export) from (to)\n"
        "-n: Number of contacts to work with\n"
        );
}

static void *argument_processor(int argc, char **argv)
{
    int i;
    test_execution_params* params = malloc(sizeof(test_execution_params));
    memset(params, 0, sizeof(test_execution_params));

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0) {
            if (++i >= argc) {
                goto error;
            }

            int n_written = snprintf(params->manager_name, MANAGER_NAME_MAX, "%s", argv[i]);
            if (n_written >= MANAGER_NAME_MAX) {
                BLTS_ERROR("%s: MANAGER_NAME_MAX exceeded\n", argv[i]);
                goto error;
            }
        } else if (strcmp(argv[i], "-d") == 0) {
            if (++i >= argc) {
                goto error;
            }

            int n_written = snprintf(params->tmp_dir, PATH_MAX, "%s", argv[i]);
            if (n_written >= PATH_MAX) {
                BLTS_ERROR("%s: PATH_MAX exceeded\n", argv[i]);
                goto error;
            }
        } else if (strcmp(argv[i], "-n") == 0) {
            if (++i >= argc) {
                goto error;
            }

            params->n_contacts = atoi(argv[i]);
            if (params->n_contacts <= 0) {
                BLTS_ERROR("invalid number of contacts: '%s'\n", argv[i]);
                goto error;
            }
        } else {
            goto error;
        }
    }

    if (params->manager_name[0] == '\0') {
        goto error;
    }

    if (params->tmp_dir[0] == '\0') {
        goto error;
    }

    if (params->n_contacts == 0) {
        goto error;
    }

    return params;

error:
    free(params);
    return NULL;
}

static void teardown(void *user_ptr)
{
    if (user_ptr) {
        test_execution_params* params = user_ptr;
        free(params);
    }
}

static int exec_test(void* user_ptr, int test_num);

static blts_cli_testcase test_cases[] =
{
    { "test", exec_test, 90000 },

    BLTS_CLI_END_OF_LIST
};

static int exec_test(void* user_ptr, int test_num)
{
    test_execution_params* params = user_ptr;
    int rc = 0;

    switch(test_num)
    {
    case 1:
        rc = blts_vcard_xport_perf_test(params->manager_name, params->tmp_dir, params->n_contacts);
        break;
    default:
        rc = -EINVAL;
        break;
    }

    return rc;
}

static blts_cli cli =
{
    .test_cases = test_cases,
    .log_file = "blts_vcard_xport_perf.txt",
    .blts_cli_help = help,
    .blts_cli_process_arguments = argument_processor,
    .blts_cli_teardown = teardown
};

int main(int argc, char **argv)
{
    blts_vcard_xport_perf_init(argc, argv);
    return blts_cli_main(&cli, argc, argv);
}
