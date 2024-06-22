
#ifdef _WINDOWS
#include "getopt.h"
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <picoquic.h>
#include <picoquic_utils.h>
#include "picomoq/picomoq_test.h"

void picoquic_tls_api_unload();

typedef struct st_picoquic_test_def_t {
    char const* test_name;
    int (*test_fn)();
} picoquic_test_def_t;

typedef enum {
    test_not_run = 0,
    test_excluded,
    test_success,
    test_failed
} test_status_t;

static const picoquic_test_def_t test_table[] = {
    { "format_parse", pmoq_msg_format_test_parse },
    { "format_format", pmoq_msg_format_test_format }
};

static size_t const nb_tests = sizeof(test_table) / sizeof(picoquic_test_def_t);

static int do_one_test(size_t i, FILE* F)
{
    int ret = 0;

    if (i >= nb_tests) {
        fprintf(F, "Invalid test number %zu\n", i);
        ret = -1;
    } else {
        fprintf(F, "Starting test number %zu , %s\n", i, test_table[i].test_name);

        fflush(F);

        ret = test_table[i].test_fn();
        if (ret == 0) {
            fprintf(F, "    Success.\n");
        } else {
            fprintf(F, "    Fails, error: %d.\n", ret);
        }
    }

    fflush(F);

    return ret;
}

int usage(char const * argv0)
{
    fprintf(stderr, "Picomoq test execution\n");
    fprintf(stderr, "Usage: picomoq_t [-x <excluded>] [<list of tests]\n");
    fprintf(stderr, "\nUsage: %s [test1 [test2 ..[testN]]]\n\n", argv0);
    fprintf(stderr, "   Or: %s [-x test]*", argv0);
    fprintf(stderr, "Valid test names are: \n");
    for (size_t x = 0; x < nb_tests; x++) {
        fprintf(stderr, "    ");

        for (int j = 0; j < 4 && x < nb_tests; j++, x++) {
            fprintf(stderr, "%s, ", test_table[x].test_name);
        }
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "Options: \n");
    fprintf(stderr, "  -x test           Do not run the specified test.\n");
    fprintf(stderr, "  -o n1 n2          Only run test numbers in range [n1,n2]");
    fprintf(stderr, "  -n                Disable debug prints.\n");
    fprintf(stderr, "  -h                Print this help message\n");
    fprintf(stderr, "  -S solution_dir   Set the path to the source files to find the default files\n");

    return -1;
}

int get_test_number(char const * test_name)
{
    int test_number = -1;

    for (size_t i = 0; i < nb_tests; i++) {
        if (strcmp(test_name, test_table[i].test_name) == 0) {
            test_number = (int)i;
        }
    }

    return test_number;
}


int main(int argc, char* argv[])
{
    int ret = 0;
    test_status_t * test_status = (test_status_t *) calloc(nb_tests, sizeof(test_status_t));
    int opt;
    size_t first_test = 0;
    size_t last_test = 10000;
    int auto_bypass = 0;
    int disable_debug = 0;
    int retry_failed_test = 0;
    int nb_test_tried = 0;
    int nb_test_failed = 0;

    memset(test_status, 0, nb_tests * sizeof(test_status_t));

    while (ret == 0 && (opt = getopt(argc, argv, "nho:S:x:")) != -1) {
        switch (opt) {
        case 'n':
            disable_debug = 1;
            break;
        case 'r':
            retry_failed_test = 1;
            break;
        case 'x': {
            optind--;
            while (optind < argc) {
                char const* tn = argv[optind];
                if (tn[0] == '-') {
                    break;
                }
                else {
                    int test_number = get_test_number(tn);

                    if (test_number < 0) {
                        fprintf(stderr, "Incorrect test name: %s\n", tn);
                        ret = usage(argv[0]);
                    }
                    else {
                        test_status[test_number] = test_excluded;
                    }
                    optind++;
                }
            }
            break;
        }
        case 'o':
            if (optind + 1 > argc) {
                fprintf(stderr, "option requires more arguments -- o\n");
                ret = usage(argv[0]);
            }
            else {
                int i_first_test = atoi(optarg);
                int i_last_test = atoi(argv[optind++]);
                if (i_first_test < 0 || i_last_test < 0) {
                    fprintf(stderr, "Incorrect first/last: %s %s\n", optarg, argv[optind - 1]);
                    ret = usage(argv[0]);
                }
                else {
                    first_test = (size_t)i_first_test;
                    last_test = (size_t)i_last_test;
                }
            }
            break;
        case 'S':
            picoquic_set_solution_dir(optarg);
            break;
        case 'h':
            usage(argv[0]);
            exit(0);
            break;
        default:
            ret = usage(argv[0]);
            break;
        }
    }

    /* If the argument list ends with a list of selected tests, mark all other tests as excluded */
    if (optind < argc) {
        auto_bypass = 1;
        for (size_t i = 0; i < nb_tests; i++) {
            test_status[i] = test_excluded;
        }
        while (optind < argc) {
            int test_number = get_test_number(argv[optind]);

            if (test_number < 0) {
                fprintf(stderr, "Incorrect test name: %s\n", optarg);
                ret = usage(argv[0]);
            }
            else {
                test_status[test_number] = 0;
            }
            optind++;
        }
    }

    if (disable_debug) {
        debug_printf_suspend();
    }
    else {
        debug_printf_resume();
    }

    /* Execute now all the tests that were not excluded */
    if (ret == 0) {
        for (size_t i = 0; i < nb_tests; i++) {
            if (test_status[i] == test_not_run) {
                nb_test_tried++;
                if (i >= first_test && i <= last_test && do_one_test(i, stdout) != 0) {
                    test_status[i] = test_failed;
                    nb_test_failed++;
                    ret = -1;
                }
                else {
                    test_status[i] = test_success;
                }
            }
            else if (!auto_bypass && test_status[i] == test_excluded) {
                fprintf(stdout, "Test number %d (%s) is bypassed.\n", (int)i, test_table[i].test_name);
            }
        }
    }

    /* Report status, and if specified retry */
    if (nb_test_failed > 0) {
        fprintf(stdout, "Failed test(s): ");
        for (size_t i = 0; i < nb_tests; i++) {
            if (test_status[i] == test_failed) {
                fprintf(stdout, "%s ", test_table[i].test_name);
            }
        }
        fprintf(stdout, "\n");

        if (disable_debug && retry_failed_test) {
            debug_printf_resume();
            ret = 0;
            for (size_t i = 0; i < nb_tests; i++) {
                if (test_status[i] == test_failed) {
                    fprintf(stdout, "Retrying %s:\n", test_table[i].test_name);
                    if (do_one_test(i, stdout) != 0) {
                        test_status[i] = test_failed;
                        ret = -1;
                    }
                    else {
                        /* This was a Heisenbug.. */
                        test_status[i] = test_success;
                    }
                }
            }
            if (ret == 0) {
                fprintf(stdout, "All tests pass after second try.\n");
            }
            else {
                fprintf(stdout, "Still failing: ");
                for (size_t i = 0; i < nb_tests; i++) {
                    if (test_status[i] == test_failed) {
                        fprintf(stdout, "%s ", test_table[i].test_name);
                    }
                }
                fprintf(stdout, "\n");
            }
        }
    }

    if (nb_test_tried > 1) {
        fprintf(stdout, "Tried %d tests, %d fail%s.\n", nb_test_tried,
            nb_test_failed, (nb_test_failed > 1) ? "" : "s");
    }

    /* Free the resource */
    free(test_status);
    picoquic_tls_api_unload();

    return (ret);
}
