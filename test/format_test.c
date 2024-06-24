#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <picoquic.h>
#include <picoquic_utils.h>
#include "picomoq.h"

/* Testing the formats
* Each test case includes:
* - A message type.
* - A message encoding in binary.
* - The value of the key specific type, cast as (void *)
* - Whether the binary is the target encoding (0) , an alternate (1), or an error (2).
*/

typedef struct st_format_test_val_t {
    char* t_name;
    char* t_val;
} format_test_val_t;

#define FVAL(x, y) { #x, #y }

typedef struct st_pmoq_msg_format_test_case_t {
    uint64_t msg_type;
    size_t msg_len;
    uint8_t *msg;
    format_test_val_t* ref_val;
    size_t ref_val_size;
#define pmoq_msg_test_mode_target 0
#define pmoq_msg_test_mode_alternate 1
#define pmoq_msg_test_mode_error 2
    unsigned int mode;
} pmoq_msg_format_test_case_t;

#define FORMAT_TEST_CASE_OK(msg_type, msg, ref) \
    { msg_type, sizeof(msg), msg, ref, sizeof(ref), pmoq_msg_test_mode_target }
#define FORMAT_TEST_CASE_ALT(msg_type, msg, ref) \
    { msg_type, sizeof(msg), msg, ref, sizeof(ref), pmoq_msg_test_mode_alternate }
#define FORMAT_TEST_CASE_ERR(msg_type, msg) \
    { msg_type, sizeof(msg), msg, NULL, 0, pmoq_msg_test_mode_error }

/* Defining a set of message values used for testing */

#define TEST_PATH 'p', 'a', 't', 'h'
#define TEST_PATH_LEN 4
static uint8_t test_path[] = { TEST_PATH };

#define TEST_TRACK_NAME 't', 'r', 'a', 'c', 'k'
#define TEST_TRACK_NAME_LEN 5
static uint8_t test_track_name[] = { TEST_TRACK_NAME };

#define TEST_REASON 't', 'o', 'o', ' ', 'b', 'a', 'd'
#define TEST_REASON_LEN 7
static uint8_t test_reason[] = { TEST_REASON };

#define TEST_AUTH 'a', 'u', 't', 'h'
#define TEST_AUTH_LEN 4
static uint8_t test_auth[] = { TEST_AUTH };

#define test_param_role_p PMOQ_SETUP_PARAMETER_ROLE, 1, pmoq_setup_role_publisher
#define test_param_role_s PMOQ_SETUP_PARAMETER_ROLE, 1, pmoq_setup_role_subscriber
#define test_param_role_ps PMOQ_SETUP_PARAMETER_ROLE, 1, pmoq_setup_role_pubsub
#define test_param_role_bad PMOQ_SETUP_PARAMETER_ROLE, 1, pmoq_setup_role_max + 1
#define test_param_path4 PMOQ_SETUP_PARAMETER_PATH, TEST_PATH_LEN, TEST_PATH
#define test_param_path0 PMOQ_SETUP_PARAMETER_PATH, 0
#define test_param_grease0 0x60, 0x00, 0
#define test_param_grease1 0x60, 0x01, TEST_PATH_LEN, TEST_PATH
#define test_param_grease2 0x60, 0x02, 1, 1
#define test_param_auth PMOQ_PARAMETER_AUTHORIZATION_INFO, TEST_AUTH_LEN, TEST_AUTH

format_test_val_t subscribe[] = {
    FVAL(subscribe_id, 31),
    FVAL(track_alias, 17),
    FVAL(track_namespace, path),
    FVAL(track_name, track),
    FVAL(filter_type, pmoq_msg_filter_latest_group),
    FVAL(auth_info, auth)
};

uint8_t test_msg_subscribe[] = {
    PMOQ_MSG_SUBSCRIBE,
    31,
    17,
    TEST_PATH_LEN*8,
    TEST_PATH,
    TEST_TRACK_NAME_LEN*8,
    TEST_TRACK_NAME,
    pmoq_msg_filter_latest_group,
    1,
    test_param_auth
};

format_test_val_t subscribe_start[] = {
    FVAL(subscribe_id, 31),
    FVAL(track_alias, 17),
    FVAL(track_namespace, path),
    FVAL(track_name, track),
    FVAL(filter_type, pmoq_msg_filter_absolute_start),
    FVAL(start_group, 3),
    FVAL(start_object, 14),
    FVAL(auth_info, auth)
};

uint8_t test_msg_subscribe_start[] = {
    PMOQ_MSG_SUBSCRIBE,
    31,
    17,
    TEST_PATH_LEN*8,
    TEST_PATH,
    TEST_TRACK_NAME_LEN*8,
    TEST_TRACK_NAME,
    pmoq_msg_filter_absolute_start,
    3,
    14,
    1,
    test_param_auth
};

format_test_val_t subscribe_range[] = {
    FVAL(subscribe_id, 31),
    FVAL(track_alias, 17),
    FVAL(track_namespace, path),
    FVAL(track_name, track),
    FVAL(filter_type, pmoq_msg_filter_absolute_range),
    FVAL(end_group, 5),
    FVAL(end_object, 35),
    FVAL(auth_info, auth)
};

uint8_t test_msg_subscribe_range[] = {
    PMOQ_MSG_SUBSCRIBE,
    31,
    17,
    TEST_PATH_LEN*8,
    TEST_PATH,
    TEST_TRACK_NAME_LEN*8,
    TEST_TRACK_NAME,
    pmoq_msg_filter_absolute_range,
    0,
    0,
    5,
    35,
    1,
    test_param_auth
};

#if 0
/* TODO: parser and test cases for subscribe update. */

typedef struct st_pmoq_msg_subscribe_update_t {
    uint64_t subscribe_id;
    uint64_t start_group;
    uint64_t start_object;
    uint64_t end_group;
    uint64_t end_object;
    pmoq_subscribe_parameters_t subscribe_parameters;
} pmoq_msg_subscribe_update_t;
#endif
format_test_val_t subscribe_update[] = {
    FVAL(subscribe_id, 11),
    FVAL(start_group, 0),
    FVAL(start_object, 1),
    FVAL(end_group, 31),
    FVAL(end_object, 27),
    FVAL(auth_info, auth)
};

uint8_t test_msg_subscribe_update[] = {
    PMOQ_MSG_SUBSCRIBE_UPDATE,
    11,
    0,
    1,
    31,
    27,
    1,
    test_param_auth
};


format_test_val_t subscribe_ok[] = {
    FVAL(subscribe_id, 65),
    FVAL(expires, 0),
    FVAL(content_exists, 1),
    FVAL(largest_group_id, 31),
    FVAL(largest_object_id, 27)
};

uint8_t test_msg_subscribe_ok[] = {
    PMOQ_MSG_SUBSCRIBE_OK,
    0x40, 0x41,
    0,
    1,
    31,
    27
};

format_test_val_t subscribe_ok_empty[] = {
    FVAL(subscribe_id, 65),
    FVAL(expires, 0),
    FVAL(content_exists, 0)
};

uint8_t test_msg_subscribe_ok_empty[] = {
    PMOQ_MSG_SUBSCRIBE_OK,
    0x40, 0x41,
    0,
    0
};

format_test_val_t subscribe_error[] = {
    FVAL(subscribe_id, 17),
    FVAL(error_code, PMOQ_SUBSCRIBE_ERROR_RETRY_TRACK_ALIAS),
    FVAL(reason_phrase, reason),
    FVAL(track_alias, 31)
};

uint8_t test_msg_subscribe_error[] = {
    PMOQ_MSG_SUBSCRIBE_ERROR,
    17,
    PMOQ_SUBSCRIBE_ERROR_RETRY_TRACK_ALIAS,
    TEST_REASON_LEN*8,
    TEST_REASON,
    31
};

format_test_val_t announce[] = {
    FVAL(track_namespace, path),
    FVAL(auth_info, auth)
};

uint8_t test_msg_announce_good[] = {
    PMOQ_MSG_ANNOUNCE,
    TEST_PATH_LEN * 8,
    TEST_PATH,
    1,
    test_param_auth
};

uint8_t test_msg_announce_alt[] = {
    PMOQ_MSG_ANNOUNCE,
    TEST_PATH_LEN * 8,
    TEST_PATH,
    3,
    test_param_grease0,
    test_param_auth,
    test_param_grease1
};

uint8_t test_msg_announce_bad[] = {
    PMOQ_MSG_ANNOUNCE,
    TEST_PATH_LEN * 8,
    TEST_PATH,
    2,
    test_param_auth,
    test_param_auth
};

format_test_val_t announce_zero[] = {
    FVAL(track_namespace, path)
};

uint8_t test_msg_announce_zero[] = {
    PMOQ_MSG_ANNOUNCE,
    TEST_PATH_LEN * 8,
    TEST_PATH,
    0
};

format_test_val_t track_namespace[] = {
    FVAL(track_namespace, path)
};

uint8_t test_msg_announce_ok[] = {
    PMOQ_MSG_ANNOUNCE_OK,
    TEST_PATH_LEN * 8,
    TEST_PATH
};

uint8_t test_msg_unannounce[] = {
    PMOQ_MSG_UNANNOUNCE,
    TEST_PATH_LEN * 8,
    TEST_PATH
};

uint8_t test_msg_announce_cancel[] = {
    PMOQ_MSG_ANNOUNCE_CANCEL,
    TEST_PATH_LEN * 8,
    TEST_PATH
};

format_test_val_t announce_error[] = {
    FVAL(track_namespace, path),
    FVAL(error_code, 291),
    FVAL(reason_phrase, reason)
};

uint8_t test_msg_announce_error[] = {
    PMOQ_MSG_ANNOUNCE_ERROR,
    TEST_PATH_LEN * 8,
    TEST_PATH,
    0x41, 0x23,
    TEST_REASON_LEN*8,
    TEST_REASON
};

format_test_val_t unsubscribe[] = {
    FVAL(subscribe_id, 61)
};

uint8_t test_msg_unsubscribe[] = {
    PMOQ_MSG_UNSUBSCRIBE,
    61
};

format_test_val_t subscribe_done[] = {
    FVAL(subscribe_id, 17),
    FVAL(status_code, PMOQ_TRACK_STATUS_IN_PROGRESS),
    FVAL(reason_phrase, reason),
    FVAL(content_exists, 1),
    FVAL(final_group_id, 31),
    FVAL(final_object_id, 17)
};

uint8_t test_msg_subscribe_done[] = {
    PMOQ_MSG_SUBSCRIBE_DONE,
    17,
    PMOQ_TRACK_STATUS_IN_PROGRESS,
    TEST_REASON_LEN*8,
    TEST_REASON,
    1,
    31,
    17
};

format_test_val_t subscribe_done_0[] = {
    FVAL(subscribe_id, 17),
    FVAL(status_code, PMOQ_TRACK_STATUS_IN_PROGRESS),
    FVAL(reason_phrase, reason),
    FVAL(content_exists, 0),
};

uint8_t test_msg_subscribe_done_0[] = {
    PMOQ_MSG_SUBSCRIBE_DONE,
    17,
    PMOQ_TRACK_STATUS_IN_PROGRESS,
    TEST_REASON_LEN*8,
    TEST_REASON,
    0
};

uint8_t test_msg_subscribe_done_bad[] = {
    PMOQ_MSG_SUBSCRIBE_DONE,
    17,
    PMOQ_TRACK_STATUS_IN_PROGRESS,
    TEST_REASON_LEN*8,
    TEST_REASON,
    0x4F, 0xFF,
    31,
    17
};

format_test_val_t track_status_request[] = {
    FVAL(track_namespace, path),
    FVAL(track_name, track)
};

uint8_t test_msg_track_status_request[] = {
    PMOQ_MSG_TRACK_STATUS_REQUEST,
    TEST_PATH_LEN*8,
    TEST_PATH,
    TEST_TRACK_NAME_LEN*8,
    TEST_TRACK_NAME
};

format_test_val_t track_status[] = {
    FVAL(track_namespace, path),
    FVAL(track_name, track),
    FVAL(status_code, PMOQ_TRACK_STATUS_IN_PROGRESS),
    FVAL(last_group_id, 31),
    FVAL(last_object_id, 63)
};

uint8_t test_msg_track_status[] = {
    PMOQ_MSG_TRACK_STATUS,
    TEST_PATH_LEN*8,
    TEST_PATH,
    TEST_TRACK_NAME_LEN*8,
    TEST_TRACK_NAME,
    PMOQ_TRACK_STATUS_IN_PROGRESS,
    31,
    63
};

format_test_val_t track_status_not[] = {
    FVAL(track_namespace, path),
    FVAL(track_name, track),
    FVAL(status_code, PMOQ_TRACK_STATUS_DOES_NOT_EXIST)
};

uint8_t test_msg_track_status_not[] = {
    PMOQ_MSG_TRACK_STATUS,
    TEST_PATH_LEN*8,
    TEST_PATH,
    TEST_TRACK_NAME_LEN*8,
    TEST_TRACK_NAME,
    PMOQ_TRACK_STATUS_DOES_NOT_EXIST
};

uint8_t test_msg_track_status_bad[] = {
    PMOQ_MSG_TRACK_STATUS,
    TEST_PATH_LEN*8,
    TEST_PATH,
    TEST_TRACK_NAME_LEN*8,
    TEST_TRACK_NAME,
    PMOQ_TRACK_STATUS_MAX+1
};

format_test_val_t goaway[] = {
    FVAL(uri, path)
};

uint8_t test_msg_goaway[] = {
    PMOQ_MSG_GOAWAY,
    TEST_PATH_LEN*8,
    TEST_PATH
};

format_test_val_t client_setup_1[] = {
    FVAL(versions, 0),
    FVAL(role, pmoq_setup_role_subscriber),
    FVAL(path, path),
};

uint8_t test_msg_client_setup_1[] = {
    0x40, PMOQ_MSG_CLIENT_SETUP,
    2, 1, 2,
    2,
    test_param_role_s,
    test_param_path4
};

uint8_t test_msg_client_setup_1a[] = {
    0x80, 0, 0, PMOQ_MSG_CLIENT_SETUP,
    2, 1, 2,
    5,
    test_param_grease0,
    test_param_grease1,
    test_param_grease2,
    test_param_path4,
    test_param_role_s
};

uint8_t test_msg_client_setup_err1[] = {
    0x40, PMOQ_MSG_CLIENT_SETUP,
    2, 1, 2,
    3,
    test_param_role_s,
    test_param_path4,
    test_param_role_s
};

format_test_val_t server_setup_1[] = {
    FVAL(selected_version, 19088743),
    FVAL(role,pmoq_setup_role_publisher)
};

uint8_t test_msg_server_setup_1[] = {
    0x40, PMOQ_MSG_SERVER_SETUP,
    0x81, 0x23, 0x45, 0x67,
    1,
    test_param_role_p,
};

format_test_val_t server_setup_2[] = {
    FVAL(selected_version, 1),
    FVAL(role, pmoq_setup_role_pubsub)
};

uint8_t test_msg_server_setup_2[] = {
    0x40, PMOQ_MSG_SERVER_SETUP,
    0x1,
    1,
    test_param_role_ps
};

uint8_t test_msg_server_setup_bad1[] = {
    0x40, PMOQ_MSG_SERVER_SETUP,
    0x1,
    1,
    test_param_role_bad
};

uint8_t test_msg_server_setup_bad2[] = {
    0x40, PMOQ_MSG_SERVER_SETUP,
    0xc1, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    1,
    test_param_role_ps
};

uint8_t test_msg_server_setup_bad3[] = {
    0x40, PMOQ_MSG_SERVER_SETUP,
    1,
    0
};

format_test_val_t stream_header_track[] = {
    FVAL(subscribe_id, 291),
    FVAL(track_alias, 49),
    FVAL(object_send_order, 0)
};

uint8_t test_msg_stream_header_track[] = {
    0x40, PMOQ_MSG_STREAM_HEADER_TRACK,
    0x41, 0x23,
    0x31,
    0
};

format_test_val_t stream_header_group[] = {
    FVAL(subscribe_id, 18),
    FVAL(track_alias, 49),
    FVAL(group_id, 15),
    FVAL(object_send_order, 0)
};

uint8_t test_msg_stream_header_group[] = {
    0x40, PMOQ_MSG_STREAM_HEADER_GROUP,
    0x12,
    0x31,
    0x0f,
    0
};

format_test_val_t object_stream[] = {
    FVAL(subscribe_id, 18),
    FVAL(track_alias, 49),
    FVAL(group_id, 15),
    FVAL(object_send_order, 0),
    FVAL(object_status, PMOQ_OBJECT_STATUS_NORMAL)
};

uint8_t test_msg_object_stream[] = {
    PMOQ_MSG_OBJECT_STREAM,
    0x12,
    0x31,
    0x0f,
    0,
    PMOQ_OBJECT_STATUS_NORMAL
};

uint8_t test_msg_object_stream_bad[] = {
    PMOQ_MSG_OBJECT_STREAM,
    0x12,
    0x31,
    0x0f,
    0,
    PMOQ_OBJECT_STATUS_MAX + 1
};


uint8_t test_msg_object_datagram[] = {
    PMOQ_MSG_OBJECT_DATAGRAM,
    0x12,
    0x31,
    0x0f,
    0,
    PMOQ_OBJECT_STATUS_NORMAL
};


/* Table of test cases */
pmoq_msg_format_test_case_t format_test_cases[] = {
    FORMAT_TEST_CASE_OK(PMOQ_MSG_SUBSCRIBE, test_msg_subscribe, subscribe),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_SUBSCRIBE, test_msg_subscribe_start, subscribe_start),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_SUBSCRIBE, test_msg_subscribe_range, subscribe_range),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_SUBSCRIBE_UPDATE, test_msg_subscribe_update, subscribe_update),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_SUBSCRIBE_OK, test_msg_subscribe_ok, subscribe_ok),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_SUBSCRIBE_OK, test_msg_subscribe_ok_empty, subscribe_ok_empty),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_SUBSCRIBE_ERROR, test_msg_subscribe_error, subscribe_error),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_ANNOUNCE, test_msg_announce_good, announce),
    FORMAT_TEST_CASE_ALT(PMOQ_MSG_ANNOUNCE, test_msg_announce_alt, announce),
    FORMAT_TEST_CASE_ERR(PMOQ_MSG_ANNOUNCE, test_msg_announce_bad),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_ANNOUNCE, test_msg_announce_zero, announce_zero),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_ANNOUNCE_OK, test_msg_announce_ok, track_namespace),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_UNANNOUNCE, test_msg_unannounce, track_namespace),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_ANNOUNCE_CANCEL, test_msg_announce_cancel, track_namespace),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_ANNOUNCE_ERROR, test_msg_announce_error, announce_error),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_UNSUBSCRIBE, test_msg_unsubscribe, unsubscribe),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_SUBSCRIBE_DONE, test_msg_subscribe_done, subscribe_done),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_SUBSCRIBE_DONE, test_msg_subscribe_done_0, subscribe_done_0),
    FORMAT_TEST_CASE_ERR(PMOQ_MSG_SUBSCRIBE_DONE, test_msg_subscribe_done_bad),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_TRACK_STATUS_REQUEST, test_msg_track_status_request, track_status_request),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_TRACK_STATUS, test_msg_track_status, track_status),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_TRACK_STATUS, test_msg_track_status_not, track_status_not),
    FORMAT_TEST_CASE_ERR(PMOQ_MSG_TRACK_STATUS, test_msg_track_status_bad),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_GOAWAY, test_msg_goaway, goaway),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_CLIENT_SETUP, test_msg_client_setup_1, client_setup_1),
    FORMAT_TEST_CASE_ALT(PMOQ_MSG_CLIENT_SETUP, test_msg_client_setup_1a, client_setup_1),
    FORMAT_TEST_CASE_ERR(PMOQ_MSG_CLIENT_SETUP, test_msg_client_setup_err1),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_SERVER_SETUP, test_msg_server_setup_1, server_setup_1),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_SERVER_SETUP, test_msg_server_setup_2, server_setup_2),
    FORMAT_TEST_CASE_ERR(PMOQ_MSG_SERVER_SETUP, test_msg_server_setup_bad1),
    FORMAT_TEST_CASE_ERR(PMOQ_MSG_SERVER_SETUP, test_msg_server_setup_bad2),
    FORMAT_TEST_CASE_ERR(PMOQ_MSG_SERVER_SETUP, test_msg_server_setup_bad3),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_STREAM_HEADER_TRACK, test_msg_stream_header_track, stream_header_track),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_STREAM_HEADER_GROUP, test_msg_stream_header_group, stream_header_group),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_OBJECT_STREAM, test_msg_object_stream, object_stream),
    FORMAT_TEST_CASE_ERR(PMOQ_MSG_OBJECT_STREAM, test_msg_object_stream_bad),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_OBJECT_DATAGRAM, test_msg_object_datagram, object_stream),
};

const size_t format_test_cases_nb = sizeof(format_test_cases) / sizeof(pmoq_msg_format_test_case_t);

/* Message comparator
*/

int pmoq_bits_cmp(const pmoq_bits_t* bs, const pmoq_bits_t* bs_ref)
{
    int ret = 0;

    if (bs->nb_bits != bs_ref->nb_bits) {
        ret = -1;
    }
    else if (bs->bits != NULL && bs_ref->bits != NULL) {
        if (bs->nb_bits != 0) {
            ret = memcmp(bs->bits, bs_ref->bits, (size_t)(bs->nb_bits + 7) / 8);
        }
    }
    else if (bs->bits == NULL && bs_ref->bits == NULL) {
        if (bs->nb_bits != 0) {
            ret = -1;
        }
    }
    else {
        ret = -1;
    }

    return ret;
}

int pmoq_msg_setup_parameters_cmp(const pmoq_setup_parameters_t* param, const pmoq_setup_parameters_t* param_ref)
{
    int ret = 0;

    if (param->role != param_ref->role) {
        ret = -1;
    }
    else if (param->path != NULL && param_ref->path != NULL) {
        if (param->path_length != param_ref->path_length) {
            ret = -1;
        }
        else if (param->path_length != 0) {
            ret = memcmp(param->path, param_ref->path, param->path_length);
        }
    }
    else if (param->path != NULL || param_ref->path != NULL) {
        ret = -1;
    }
    return ret;
}

int pmoq_subscribe_parameters_cmp(const pmoq_subscribe_parameters_t* param, const pmoq_subscribe_parameters_t* param_ref)
{
    int ret = 0;

    if (param->auth_info != NULL && param_ref->auth_info != NULL) {
        if (param->auth_info_len != param_ref->auth_info_len) {
            ret = -1;
        }
        else if (param->auth_info_len != 0) {
            ret = memcmp(param->auth_info, param_ref->auth_info, param->auth_info_len);
        }
    }
    else if (param->auth_info != NULL || param_ref->auth_info != NULL) {
        ret = -1;
    }
    return ret;
}

int mpoq_test_msg_compare(const pmoq_msg_t* msg, const pmoq_msg_t* msg2)
{
    int ret = 0;
    if (msg->msg_type != msg2->msg_type ||
        pmoq_bits_cmp(&msg->track_namespace, &msg2->track_namespace) != 0 ||
        pmoq_bits_cmp(&msg->track_name, &msg2->track_name) != 0 ||
        msg->subscribe_id != msg2->subscribe_id ||
        msg->track_alias != msg2->track_alias ||
        msg->group_id != msg2->group_id ||
        msg->object_send_order != msg2->object_send_order ||
        msg->object_status != msg2->object_status ||
        msg->filter_type != msg2->filter_type ||
        msg->expires != msg2->expires ||
        msg->content_exists != msg2->content_exists ||
        msg->error_code != msg2->error_code ||
        msg->status_code != msg2->status_code ||
        msg->start_group != msg2->start_group ||
        msg->start_object != msg2->start_object ||
        msg->end_group != msg2->end_group ||
        msg->largest_group_id != msg2->largest_group_id ||
        msg->largest_object_id != msg2->largest_object_id ||
        msg->last_group_id != msg2->last_group_id ||
        msg->last_object_id != msg2->last_object_id ||
        msg->final_group_id != msg2->final_group_id ||
        msg->final_object_id != msg2->final_object_id ||
        pmoq_bits_cmp(&msg->reason_phrase, &msg2->reason_phrase) != 0 ||
        pmoq_bits_cmp(&msg->uri, &msg2->uri) != 0 ||
        pmoq_subscribe_parameters_cmp(&msg->subscribe_parameters, &msg2->subscribe_parameters) != 0 ||
        pmoq_msg_setup_parameters_cmp(&msg->setup_parameters, &msg2->setup_parameters) != 0 ||
        msg->selected_version != msg2->selected_version ||
        msg->supported_versions_nb != msg2->supported_versions_nb) {
        ret = -1;
    } else {
        for (uint64_t i = 0; i < msg->supported_versions_nb; i++) {
            if (msg->supported_versions[i] != msg2->supported_versions[i]) {
                ret = -1;
                break;
            }
        }
    }
    return ret;
}

int pmoq_test_set_bits(pmoq_bits_t* v, char* val)
{
    int ret = 0;
    if (strcmp(val, "path") == 0) {
        v->bits = test_path;
        v->nb_bits = TEST_PATH_LEN * 8;
    }
    else if (strcmp(val, "track") == 0) {
        v->bits = test_track_name;
        v->nb_bits = TEST_TRACK_NAME_LEN * 8;
    }
    else if (strcmp(val, "reason") == 0) {
        v->bits = test_reason;
        v->nb_bits = TEST_REASON_LEN * 8;
    }
    else {
        ret = -1;
    }
    return ret;
}

typedef struct st_pmoq_test_named_int_t {
    char* name;
    uint64_t v;
} pmoq_test_named_int_t;

#define TINT(x) { #x, x }

pmoq_test_named_int_t test_named_int[] = {
    TINT(PMOQ_OBJECT_STATUS_NORMAL),
    TINT(PMOQ_OBJECT_STATUS_DOES_NOT_EXIST),
    TINT(PMOQ_OBJECT_STATUS_NO_SUCH_GROUP),
    TINT(PMOQ_OBJECT_STATUS_END_OF_GROUP),
    TINT(PMOQ_OBJECT_STATUS_END_OF_GROUP_AND_TRACK),
    TINT(PMOQ_TRACK_STATUS_IN_PROGRESS),
    TINT(PMOQ_TRACK_STATUS_DOES_NOT_EXIST),
    TINT(PMOQ_TRACK_STATUS_HAS_NOT_BEGUN),
    TINT(PMOQ_TRACK_STATUS_IS_RELAY),
    TINT(pmoq_msg_filter_latest_group),
    TINT(pmoq_msg_filter_latest_object),
    TINT(pmoq_msg_filter_absolute_start),
    TINT(pmoq_msg_filter_absolute_range),
    TINT(pmoq_setup_role_publisher),
    TINT(pmoq_setup_role_subscriber),
    TINT(pmoq_setup_role_pubsub),
    TINT(PMOQ_SUBSCRIBE_ERROR_RETRY_INTERNAL_ERROR),
    TINT(PMOQ_SUBSCRIBE_ERROR_RETRY_INVALID_RANGE),
    TINT(PMOQ_SUBSCRIBE_ERROR_RETRY_TRACK_ALIAS),
};

size_t test_named_int_nb = sizeof(test_named_int) / sizeof(pmoq_test_named_int_t);

int pmoq_test_set_u64(uint64_t * v, char* val)
{
    int ret = 0;

    if (val[0] >= '0' && val[0] <= '9') {
        uint64_t r = val[0] - '0';
        uint8_t* x = (uint8_t*)(val + 1);
        while (*x != 0 && ret == 0) {
            r *= 10;
            r += *x - '0';
            x++;
        }
        if (*x != 0) {
            ret = -1;
        }
        else {
            *v = r;
        }
    }
    else {
        int found = 0;
        for (size_t i = 0; i < test_named_int_nb; i++) {
            if (strcmp(val, test_named_int[i].name) == 0) {
                *v = test_named_int[i].v;
                found = 1;
                break;
            }
        }
        if (!found) {
            ret = -1;
        }
    }
    return ret;
}

int pmoq_test_set_u32(uint32_t* v, char* val)
{
    uint64_t v64;
    int ret = pmoq_test_set_u64(&v64, val);

    if (ret == 0) {
        if (v64 > UINT32_MAX) {
            ret = -1;
        }
        else {
            *v = (uint32_t)v64;
        }
    }
    return ret;
}

int pmoq_test_set_u8(uint8_t * role, char* val)
{
    uint64_t v64;
    int ret = pmoq_test_set_u64(&v64, val);

    if (ret == 0) {
        if (v64 > UINT8_MAX) {
            ret = -1;
        }
        else {
            *role = (uint8_t)v64;
        }
    }
    return ret;
}

int pmoq_test_set_versions(pmoq_msg_t *msg)
{
    msg->supported_versions_nb = 2;
    msg->supported_versions[0] = 1;
    msg->supported_versions[1] = 2;
    return 0;
}

int pmoq_test_set_string(uint8_t** v, size_t * l, char* val)
{
    int ret = 0;
    if (strcmp(val, "path") == 0) {
        *v = test_path;
        *l = TEST_PATH_LEN;
    }
    else if (strcmp(val, "auth") == 0) {
        *v = test_auth;
        *l = TEST_AUTH_LEN;
    }
    else {
        ret = -1;
    }
    return ret;
}

int pmoq_test_set_msg_from_test(pmoq_msg_t *msg, pmoq_msg_format_test_case_t* test)
{
    int ret = 0;
    size_t nb_vals = test->ref_val_size / sizeof(format_test_val_t);

    msg->msg_type = test->msg_type;

    for (size_t i = 0; ret == 0 &&  i < nb_vals; i++) {
        if (strcmp(test->ref_val[i].t_name, "track_namespace") == 0) {
            ret = pmoq_test_set_bits(&msg->track_namespace, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "track_name") == 0) {
            ret = pmoq_test_set_bits(&msg->track_name, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "subscribe_id") == 0) {
            ret = pmoq_test_set_u64(&msg->subscribe_id, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "track_alias") == 0) {
            ret = pmoq_test_set_u64(&msg->track_alias, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "group_id") == 0) {
            ret = pmoq_test_set_u64(&msg->group_id, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "object_send_order") == 0) {
            ret = pmoq_test_set_u64(&msg->object_send_order, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "object_status") == 0) {
            ret = pmoq_test_set_u64(&msg->object_status, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "filter_type") == 0) {
            ret = pmoq_test_set_u64(&msg->filter_type, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "expires") == 0) {
            ret = pmoq_test_set_u64(&msg->expires, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "content_exists") == 0) {
            ret = pmoq_test_set_u8(&msg->content_exists, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "error_code") == 0) {
            ret = pmoq_test_set_u64(&msg->error_code, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "status_code") == 0) {
            ret = pmoq_test_set_u64(&msg->status_code, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "start_group") == 0) {
            ret = pmoq_test_set_u64(&msg->start_group, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "start_object") == 0) {
            ret = pmoq_test_set_u64(&msg->start_object, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "end_group") == 0) {
            ret = pmoq_test_set_u64(&msg->end_group, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "end_object") == 0) {
            ret = pmoq_test_set_u64(&msg->end_object, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "largest_group_id") == 0) {
            ret = pmoq_test_set_u64(&msg->largest_group_id, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "largest_object_id") == 0) {
            ret = pmoq_test_set_u64(&msg->largest_object_id, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "last_group_id") == 0) {
            ret = pmoq_test_set_u64(&msg->last_group_id, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "last_object_id") == 0) {
            ret = pmoq_test_set_u64(&msg->last_object_id, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "final_group_id") == 0) {
            ret = pmoq_test_set_u64(&msg->final_group_id, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "final_object_id") == 0) {
            ret = pmoq_test_set_u64(&msg->final_object_id, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "reason_phrase") == 0) {
            ret = pmoq_test_set_bits(&msg->reason_phrase, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "uri") == 0) {
            ret = pmoq_test_set_bits(&msg->uri, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "auth_info") == 0) {
            ret = pmoq_test_set_string(&msg->subscribe_parameters.auth_info,
                &msg->subscribe_parameters.auth_info_len,
                test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "selected_version") == 0) {
            ret = pmoq_test_set_u32(&msg->selected_version, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "versions") == 0) {
            pmoq_test_set_versions(msg);
        }
        else if (strcmp(test->ref_val[i].t_name, "role") == 0) {
            ret = pmoq_test_set_u8(&msg->setup_parameters.role, test->ref_val[i].t_val);
        }
        else if (strcmp(test->ref_val[i].t_name, "path") == 0) {
            ret = pmoq_test_set_string(&msg->setup_parameters.path,
                &msg->setup_parameters.path_length,
                test->ref_val[i].t_val);
        }
        else {
            ret = -1;
        }
    }

    return ret;
}

int pmoq_msg_format_test_parse_one(pmoq_msg_format_test_case_t* test)
{
    int ret = 0;
    int err = 0;
    const uint8_t* bytes = test->msg;
    const uint8_t* bytes_max = bytes + test->msg_len;
    pmoq_msg_t msg = { 0 };

    bytes = pmoq_msg_parse(bytes, bytes_max, &err, 0, &msg);

    if (test->mode == pmoq_msg_test_mode_error) {
        if (bytes != NULL) {
            ret = -1;
        }
    }
    else if (bytes == NULL) {
        ret = -1;
    }
    else {
        pmoq_msg_t ref = { 0 };

        if (pmoq_test_set_msg_from_test(&ref, test) != 0) {
            ret = -1;
        }
        else if (mpoq_test_msg_compare(&msg, &ref) != 0) {
            ret = -1;
        }
    }

    return ret;   
}

int pmoq_msg_format_test_parse()
{
    int ret = 0;

    for (size_t i = 0; i < format_test_cases_nb; i++) {
        if ((ret = pmoq_msg_format_test_parse_one(&format_test_cases[i])) != 0) {
            printf("Parse test fails: format_test_cases[%zu]\n", i);
            break;
        }
    }

    return ret;
}

int pmoq_msg_format_test_format_one(pmoq_msg_format_test_case_t* test)
{
    int ret = 0;
    pmoq_msg_t msg = { 0 };

    if (pmoq_test_set_msg_from_test(&msg, test) != 0) {
        ret = -1;
    } else {
        uint8_t buf[2048];
        uint8_t* bytes = buf;
        const uint8_t* bytes_max = bytes + sizeof(msg);

        memset(buf, 0xff, sizeof(buf));

        if ((bytes = pmoq_msg_format(bytes, bytes_max, &msg)) == NULL) {
            ret = 0;
        }
        else {
            size_t l = bytes - buf;

            if (l != test->msg_len ||
                memcmp(buf, test->msg, l) != 0) {
                ret = -1;
            }
        }
    }

    return ret;
}

int pmoq_msg_format_test_format()
{
    int ret = 0;
    int nb_formatted = 0;

    for (size_t i = 0; i < format_test_cases_nb; i++) {
        if (format_test_cases[i].mode == pmoq_msg_test_mode_target) {
            nb_formatted++;
            if ((ret = pmoq_msg_format_test_format_one(&format_test_cases[i])) != 0) {
                printf("Format test fails: format_test_cases[%zu]\n", i);
                break;
            }
        }
    }

    return ret;
}

int pmoq_msg_format_test_varlen_one(pmoq_msg_format_test_case_t* test)
{
    int ret = 0;
    const uint8_t* bytes = test->msg;
    const uint8_t* msg_end = bytes + test->msg_len;
    uint64_t mask = 0;

    for (size_t j = 0; ret == 0 && j < test->msg_len - 1; j++) {
        size_t test_len = j;

        if (j < 64 && (mask & (1ull << j)) != 0) {
            continue;
        }

        while (ret == 0) {
            pmoq_msg_t msg = { 0 };
            int err = 0;

            if (test_len < 64) {
                mask |= (1ull << test_len);
            }

            bytes = pmoq_msg_parse(test->msg, test->msg + test_len, &err, 0, &msg);

            if (bytes == NULL) {
                if (err > 0) {
                    test_len += err;
                    if (test_len > test->msg_len) {
                        ret = -1;
                    }
                }
                else if (err == 0) {
                    ret = -1;
                }
                else if (test->mode != pmoq_msg_test_mode_error) {
                    ret = -1;
                }
                else {
                    break;
                }
            }
            else if (bytes != msg_end) {
                ret = -1;
            }
            else if (test->mode == pmoq_msg_test_mode_error) {
                ret = -1;
            }
            else {
                break;
            }
        }
    }

    return ret;   
}

int pmoq_msg_format_test_varlen()
{
    int ret = 0;

    for (size_t i = 0; i < format_test_cases_nb; i++) {
        if ((ret = pmoq_msg_format_test_varlen_one(&format_test_cases[i])) != 0) {
            printf("Format varlen test fails: format_test_cases[%zu]\n", i);
            break;
        }
    }

    return ret;
}