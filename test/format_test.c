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

typedef struct st_pmoq_msg_format_test_case_t {
    uint64_t msg_type;
    size_t msg_len;
    uint8_t *msg;
    void* payload_ref;
#define pmoq_msg_test_mode_target 0
#define pmoq_msg_test_mode_alternate 1
#define pmoq_msg_test_mode_error 2
    unsigned int mode;
} pmoq_msg_format_test_case_t;

#define FORMAT_TEST_CASE_OK(msg_type, msg, ref) \
    { msg_type, sizeof(msg), msg, (void*)&ref, pmoq_msg_test_mode_target }
#define FORMAT_TEST_CASE_ALT(msg_type, msg, ref) \
    { msg_type, sizeof(msg), msg, (void*)&ref, pmoq_msg_test_mode_alternate }
#define FORMAT_TEST_CASE_ERR(msg_type, msg) \
    { msg_type, sizeof(msg), msg, NULL, pmoq_msg_test_mode_error }

/* Defining a set of message values used for testing */
#if 0
typedef struct st_pmoq_msg_object_stream_t {
    uint64_t subscribe_id;
    uint64_t track_alias;
    uint64_t group_id;
    uint64_t object_send_order;
    uint64_t object_status;
    /* Object Payload */
} pmoq_msg_object_stream_t;

typedef struct st_pmoq_subscribe_parameters_t {
#define pmoq_para_auth_info_key 0x02
    size_t auth_info_len;
    uint8_t * auth_info;
} pmoq_subscribe_parameters_t;

typedef struct st_pmoq_msg_subscribe_t {
    uint64_t subscribe_id;
    uint64_t track_alias;
    pmoq_bits_t track_namespace;
    pmoq_bits_t track_name;
#define pmoq_msg_filter_latest_group 0x1
#define pmoq_msg_filter_latest_object 0x2
#define pmoq_msg_filter_absolute_start 0x3
#define pmoq_msg_filter_absolute_range 0x4
#define pmoq_msg_filter_max 0x4
    uint64_t filter_type;
    unsigned int has_start : 1;
    unsigned int has_end : 1;
    uint64_t start_group;
    uint64_t start_object;
    uint64_t end_group;
    uint64_t end_object;
    pmoq_subscribe_parameters_t subscribe_parameters;
} pmoq_msg_subscribe_t;

typedef struct st_pmoq_msg_subscribe_update_t {
    uint64_t subscribe_id;
    uint64_t start_group;
    uint64_t start_object;
    uint64_t end_group;
    uint64_t end_object;
    pmoq_subscribe_parameters_t subscribe_parameters;
} pmoq_msg_subscribe_update_t;

typedef struct st_pmoq_msg_subscribe_ok_t {
    uint64_t subscribe_id;
    uint64_t expires;
    uint8_t content_exists; /* value: 0 or 1 */
    uint64_t largest_group_id; /* Only present if content_exists == 1 */
    uint64_t largest_object_id; /* Only present if content_exists == 1 */
} pmoq_msg_subscribe_ok_t;

typedef struct st_pmoq_msg_subscribe_error_t {
    uint64_t subscribe_id;
    uint64_t error_code;
    pmoq_bits_t reason_phrase;
    uint64_t track_alias;
} pmoq_msg_subscribe_error_t;

typedef struct st_pmoq_msg_announce_t {
    pmoq_bits_t track_namespace;
    pmoq_subscribe_parameters_t announce_parameters;
} pmoq_msg_announce_t;

typedef struct st_pmoq_msg_track_namespace_t {
    pmoq_bits_t track_namespace;
} pmoq_msg_track_namespace_t;

typedef struct st_pmoq_msg_announce_error_t {
    pmoq_bits_t track_namespace;
    uint64_t error_code;
    pmoq_bits_t reason_phrase;
} pmoq_msg_announce_error_t;

typedef struct st_pmoq_msg_unsubscribe_t {
    uint64_t subscribe_id;
} pmoq_msg_unsubscribe_t;

typedef struct st_pmoq_msg_subscribe_done_t {
    uint64_t subscribe_id;
    uint64_t status_code;
    pmoq_bits_t reason_phrase;
    uint8_t content_exists; /* value: 0 or 1 */
    uint64_t final_group_id; /* Only present if content_exists == 1 */
    uint64_t final_object_id; /* Only present if content_exists == 1 */
} pmoq_msg_subscribe_done_t;

typedef struct st_pmoq_msg_track_status_request_t {
    pmoq_bits_t track_namespace;
    pmoq_bits_t track_name;
} pmoq_msg_track_status_request_t;

typedef struct st_pmoq_msg_track_status_t {
    pmoq_bits_t track_namespace;
    pmoq_bits_t track_name;
    uint64_t status_code;
#define PMOQ_TRACK_STATUS_IN_PROGRESS 0x00
#define PMOQ_TRACK_STATUS_DOES_NOT_EXISTS 0x01
#define PMOQ_TRACK_STATUS_HAS_NOT_BEGUN 0x02
#define PMOQ_TRACK_STATUS_IS_RELAY 0x03
    uint64_t last_group_id; /* Only present if status code requires it */
    uint64_t last_object_id; /* Only present if status code requires it */
} pmoq_msg_track_status_t;

typedef struct st_pmoq_msg_goaway_t {
    pmoq_bits_t uri;
} pmoq_msg_goaway_t;

#define     pmoq_setup_role_undef 0
#define     pmoq_setup_role_publisher 1
#define     pmoq_setup_role_subscriber 2
#define     pmoq_setup_role_pubsub 3
#define     pmoq_setup_role_max 3
#endif

#define TEST_PATH 'p', 'a', 't', 'h'
#define TEST_PATH_LEN 4
static uint8_t test_path[] = { TEST_PATH };

#define test_param_role_p PMOQ_SETUP_PARAMETER_ROLE, 1, pmoq_setup_role_publisher
#define test_param_role_s PMOQ_SETUP_PARAMETER_ROLE, 1, pmoq_setup_role_subscriber
#define test_param_role_ps PMOQ_SETUP_PARAMETER_ROLE, 1, pmoq_setup_role_pubsub
#define test_param_role_bad PMOQ_SETUP_PARAMETER_ROLE, 1, pmoq_setup_role_max + 1
#define test_param_path4 PMOQ_SETUP_PARAMETER_PATH, TEST_PATH_LEN, TEST_PATH
#define test_param_path0 PMOQ_SETUP_PARAMETER_PATH, 0
#define test_param_grease0 0x60, 0x00, 0
#define test_param_grease1 0x60, 0x01, TEST_PATH_LEN, TEST_PATH
#define test_param_grease2 0x60, 0x02, 1, 1


pmoq_msg_client_setup_t client_setup_1 = {
    2,
    { 1, 2 },
    {
        pmoq_setup_role_subscriber,
        TEST_PATH_LEN,
        test_path
    }
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

pmoq_msg_server_setup_t server_setup_1 = {
    0x1234567,
    {
        pmoq_setup_role_publisher,
        0,
        NULL
    }
};

uint8_t test_msg_server_setup_1[] = {
    0x40, PMOQ_MSG_SERVER_SETUP,
    0x81, 0x23, 0x45, 0x67,
    1,
    test_param_role_p,
};

pmoq_msg_server_setup_t server_setup_2 = {
    1,
    {
        pmoq_setup_role_pubsub,
        0,
        NULL
    }
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

/* Table of test cases */
pmoq_msg_format_test_case_t format_test_cases[] = {
    FORMAT_TEST_CASE_OK(PMOQ_MSG_CLIENT_SETUP, test_msg_client_setup_1, client_setup_1),
    FORMAT_TEST_CASE_ALT(PMOQ_MSG_CLIENT_SETUP, test_msg_client_setup_1a, client_setup_1),
    FORMAT_TEST_CASE_ERR(PMOQ_MSG_CLIENT_SETUP, test_msg_client_setup_err1),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_SERVER_SETUP, test_msg_server_setup_1, server_setup_1),
    FORMAT_TEST_CASE_OK(PMOQ_MSG_SERVER_SETUP, test_msg_server_setup_2, server_setup_2),
    FORMAT_TEST_CASE_ERR(PMOQ_MSG_SERVER_SETUP, test_msg_server_setup_bad1),
    FORMAT_TEST_CASE_ERR(PMOQ_MSG_SERVER_SETUP, test_msg_server_setup_bad2),
    FORMAT_TEST_CASE_ERR(PMOQ_MSG_SERVER_SETUP, test_msg_server_setup_bad3),
};

const size_t format_test_cases_nb = sizeof(format_test_cases) / sizeof(pmoq_msg_format_test_case_t);

#if 0
typedef struct st_pmoq_msg_server_setup_t {
    uint32_t selected_version;
    pmoq_setup_parameters_t setup_parameters;
} pmoq_msg_server_setup_t;

typedef struct st_pmoq_msg_stream_header_track_t {
    uint64_t subscribe_id;
    uint64_t track_alias;
    uint64_t object_send_order;
} pmoq_msg_stream_header_track_t;

typedef struct st_pmoq_msg_stream_header_group_t {
    uint64_t subscribe_id;
    uint64_t track_alias;
    uint64_t group_id;
    uint64_t object_send_order;
} pmoq_msg_stream_header_group_t;

typedef struct st_pmoq_msg_stream_object_header_t {
    uint64_t object_id;
    uint64_t payload_length;
    uint64_t object_status; /* Only present if object payload length = 0 */
                            /* Object Payload */
} pmoq_msg_stream_object_header_t;
#endif

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

int pmoq_msg_object_stream_cmp(const pmoq_msg_object_stream_t* os, const pmoq_msg_object_stream_t* os_ref)
{
    int ret = 0;
    if (os->subscribe_id != os_ref->subscribe_id ||
        os->track_alias != os_ref->track_alias ||
        os->group_id != os_ref->group_id ||
        os->object_send_order != os_ref->object_send_order ||
        os->object_status != os_ref->object_status) {
        ret = -1;
    }
    return ret;
}

int pmoq_msg_subscribe_cmp(const pmoq_msg_subscribe_t * s, const pmoq_msg_subscribe_t * s_ref )
{
    int ret = 0;

    if (s->subscribe_id != s_ref->subscribe_id || 
        s->track_alias != s_ref->track_alias ||
        pmoq_bits_cmp(&s->track_namespace, &s_ref->track_namespace) == 0 ||
        pmoq_bits_cmp(&s->track_name, &s_ref->track_name) == 0 ||
        s->filter_type != s_ref->filter_type ||
        pmoq_subscribe_parameters_cmp(&s->subscribe_parameters, &s_ref->subscribe_parameters) != 0) {
        ret = -1;
    }
    else if (s->filter_type == pmoq_msg_filter_absolute_start ||
        s->filter_type == pmoq_msg_filter_absolute_range) {
        if (s->start_group != s_ref->start_group ||
            s->start_object != s_ref->start_object) {
            ret = -1;
        }
        else if (s->filter_type == pmoq_msg_filter_absolute_range) {
            if (s->start_group != s_ref->start_group ||
                s->start_object != s_ref->start_object) {
                ret = -1;
            }
        }
    }
    return ret;
}

int pmoq_msg_subscribe_ok_cmp(const pmoq_msg_subscribe_ok_t * s, const pmoq_msg_subscribe_ok_t * s_ref)
{
    int ret = 0;
    if (s->subscribe_id != s_ref->subscribe_id ||
        s->expires != s_ref->expires ||
        s->content_exists != s_ref->content_exists) {
        ret = -1;
    }
    else if (s->content_exists > 0) {
        if (s->largest_group_id != s_ref->largest_group_id ||
            s->largest_object_id != s_ref->largest_object_id) {
            ret = -1;
        }
    }

    return ret;
}

int pmoq_msg_subscribe_error_cmp(const pmoq_msg_subscribe_error_t * s, const pmoq_msg_subscribe_error_t * s_ref)
{
    int ret = 0;

    if (s->subscribe_id != s_ref->subscribe_id ||
        s->error_code != s_ref->error_code ||
        pmoq_bits_cmp(&s->reason_phrase, &s_ref->reason_phrase) != 0 ||
        s->track_alias != s_ref->track_alias) {
        ret = -1;
    }
    return ret;
}

int pmoq_msg_announce_cmp(const pmoq_msg_announce_t * a, const pmoq_msg_announce_t * a_ref)
{
    int ret = 0;

    if (pmoq_bits_cmp(&a->track_namespace, &a_ref->track_namespace) != 0 ||
        pmoq_subscribe_parameters_cmp(&a->announce_parameters, &a_ref->announce_parameters) != 0 ) {
        ret = -1;
    }

    return ret;
}

int pmoq_msg_track_namespace_cmp(const pmoq_msg_track_namespace_t * tns, const pmoq_msg_track_namespace_t * tns_ref)
{
    /* is there a size limit ? */
    return pmoq_bits_cmp(&tns->track_namespace, &tns_ref->track_namespace);
}

int pmoq_msg_announce_error_cmp(const pmoq_msg_announce_error_t * a, const pmoq_msg_announce_error_t * a_ref)
{
    int ret = 0;

    if (pmoq_bits_cmp(&a->track_namespace, &a_ref->track_namespace) != 0 ||
        a->error_code != a_ref->error_code ||
        pmoq_bits_cmp(&a->reason_phrase, &a_ref->reason_phrase) != 0) {
        ret = -1;
    }
    return ret;
}

int pmoq_msg_unsubscribe_cmp(const pmoq_msg_unsubscribe_t* u, const pmoq_msg_unsubscribe_t* u_ref)
{
    int ret = 0;

    if (u->subscribe_id != u_ref->subscribe_id) {
        ret = -1;
    }

    return ret;
}

int pmoq_msg_subscribe_done_cmp(const pmoq_msg_subscribe_done_t * s, const pmoq_msg_subscribe_done_t * s_ref)
{
    int ret = 0;

    if (s->subscribe_id != s_ref->subscribe_id ||
        s->status_code != s_ref->status_code ||
        pmoq_bits_cmp(&s->reason_phrase, &s_ref->reason_phrase) != 0 ||
        s->content_exists != s_ref->content_exists) {
        ret = -1;
    } else if (s->content_exists > 0) {
        if (s->subscribe_id != s_ref->final_group_id ||
            s->content_exists != s_ref->final_object_id) {
            ret = -1;
        }
    }
    return ret;
}

int pmoq_msg_track_status_request_cmp(const pmoq_msg_track_status_request_t * ts, const pmoq_msg_track_status_request_t * ts_ref)
{
    int ret = 0;

    if (pmoq_bits_cmp(&ts->track_namespace, &ts_ref->track_namespace) != 0 ||
        pmoq_bits_cmp(&ts->track_name, &ts_ref->track_name) != 0) {
        ret = -1;
    }
    return ret; 
}

int pmoq_msg_track_status_cmp(const pmoq_msg_track_status_t * ts, const pmoq_msg_track_status_t * ts_ref)
{
    int ret = 0;

    if (pmoq_bits_cmp(&ts->track_namespace, &ts_ref->track_namespace) != 0 ||
        pmoq_bits_cmp(&ts->track_name, &ts_ref->track_name) != 0 ||
        ts->status_code != ts_ref->status_code) {
        ret = -1;
    }
    else if (ts->status_code == PMOQ_TRACK_STATUS_IN_PROGRESS) {
        if (ts->last_group_id != ts_ref->last_group_id ||
            ts->last_object_id != ts_ref->last_object_id) {
            ret = -1;
        }
    }
    return ret;
}

int pmoq_msg_goaway_cmp(const pmoq_msg_goaway_t * g, const pmoq_msg_goaway_t * g_ref)
{
    return pmoq_bits_cmp(&g->uri, &g_ref->uri);
}

int pmoq_msg_client_setup_cmp(const pmoq_msg_client_setup_t * cs, const pmoq_msg_client_setup_t * cs_ref)
{
    int ret = 0;

    if (cs->supported_versions_nb != cs_ref->supported_versions_nb ||
        pmoq_msg_setup_parameters_cmp(&cs->setup_parameters, &cs_ref->setup_parameters) != 0) {
        ret = -1;
    }
    else {
        for (uint64_t i = 0; i < cs->supported_versions_nb; i++) {
            if (cs->supported_versions[i] != cs_ref->supported_versions[i]) {
                ret = -1;
                break;
            }
        }
    }

    return ret;
}

int pmoq_msg_server_setup_cmp(const pmoq_msg_server_setup_t* s, const pmoq_msg_server_setup_t* s_ref)
{
    int ret = 0;

    if (s->selected_version != s_ref->selected_version ||
        pmoq_msg_setup_parameters_cmp(&s->setup_parameters, &s_ref->setup_parameters) != 0) {
        ret = -1;
    }
    return ret;
}

int pmoq_msg_stream_header_track_cmp(const pmoq_msg_stream_header_track_t * h, const pmoq_msg_stream_header_track_t * h_ref)
{
    int ret = 0;

    if (h->subscribe_id != h_ref->subscribe_id ||
        h->track_alias != h_ref->track_alias ||
        h->object_send_order != h_ref->object_send_order){
        ret = -1;
    }
    return ret;
}

int pmoq_msg_stream_header_group_cmp(const pmoq_msg_stream_header_group_t * h, const pmoq_msg_stream_header_group_t * h_ref)
{
    int ret = 0;

    if (h->subscribe_id != h_ref->subscribe_id ||
        h->track_alias != h_ref->track_alias ||
        h->group_id != h_ref->group_id ||
        h->object_send_order != h_ref->object_send_order){
        ret = -1;
    }
    return ret;
}

int pmoq_msg_stream_object_header_cmp(const pmoq_msg_stream_object_header_t * o, const pmoq_msg_stream_object_header_t * o_ref)
{ 
    int ret = 0;

    if (o->object_id != o_ref->object_id ||
        o->payload_length != o_ref->payload_length) {
        ret = -1;
    }
    else if (o->payload_length == 0) {
        if (o->object_status != o_ref->object_status) {
            ret = -1;
        }
    }
    return ret;
}

int mpoq_test_msg_compare(pmoq_msg_format_test_case_t* test, const pmoq_msg_t* msg)
{
    int ret = 0;

    if (msg->msg_type != test->msg_type) {
        ret = -1;
    }
    else {
        switch (test->msg_type) {
        case PMOQ_MSG_OBJECT_STREAM:
            ret = pmoq_msg_object_stream_cmp(&msg->msg_payload.object_stream,
                (pmoq_msg_object_stream_t*)test->payload_ref);
            break;
        case PMOQ_MSG_OBJECT_DATAGRAM:
            ret = pmoq_msg_object_stream_cmp(&msg->msg_payload.datagram,
                (pmoq_msg_object_stream_t*)test->payload_ref);
            break;
        case PMOQ_MSG_SUBSCRIBE:
            ret = pmoq_msg_subscribe_cmp(&msg->msg_payload.subscribe,
                (pmoq_msg_subscribe_t*)test->payload_ref);
            break;
        case PMOQ_MSG_SUBSCRIBE_OK:
            ret = pmoq_msg_subscribe_ok_cmp(&msg->msg_payload.subscribe_ok,
                (pmoq_msg_subscribe_ok_t*)test->payload_ref);
            break;
        case PMOQ_MSG_SUBSCRIBE_ERROR:
            ret = pmoq_msg_subscribe_error_cmp(&msg->msg_payload.subscribe_error,
                (pmoq_msg_subscribe_error_t*)test->payload_ref);
            break;
        case PMOQ_MSG_ANNOUNCE:
            ret = pmoq_msg_announce_cmp(&msg->msg_payload.announce,
                (pmoq_msg_announce_t*)test->payload_ref);
            break;
        case PMOQ_MSG_ANNOUNCE_OK:
            ret = pmoq_msg_track_namespace_cmp(&msg->msg_payload.announce_ok,
                (pmoq_msg_track_namespace_t*)test->payload_ref);
            break;
        case PMOQ_MSG_ANNOUNCE_ERROR:
            ret = pmoq_msg_announce_error_cmp(&msg->msg_payload.announce_error,
                (pmoq_msg_announce_error_t*)test->payload_ref);
            break;
        case PMOQ_MSG_UNANNOUNCE:
            ret = pmoq_msg_track_namespace_cmp(&msg->msg_payload.unannounce,
                (pmoq_msg_track_namespace_t*)test->payload_ref);
            break;
        case PMOQ_MSG_UNSUBSCRIBE:
            ret = pmoq_msg_unsubscribe_cmp(&msg->msg_payload.unsubscribe,
                (pmoq_msg_unsubscribe_t*)test->payload_ref);
            break;
        case PMOQ_MSG_SUBSCRIBE_DONE:
            ret = pmoq_msg_subscribe_done_cmp(&msg->msg_payload.subscribe_done,
                (pmoq_msg_subscribe_done_t*)test->payload_ref);
            break;
        case PMOQ_MSG_ANNOUNCE_CANCEL:
            ret = pmoq_msg_track_namespace_cmp(&msg->msg_payload.announce_cancel,
                (pmoq_msg_track_namespace_t*)test->payload_ref);
            break;
        case PMOQ_MSG_TRACK_STATUS_REQUEST:
            ret = pmoq_msg_track_status_request_cmp(&msg->msg_payload.track_status_request,
                (pmoq_msg_track_status_request_t*)test->payload_ref);
            break;
        case PMOQ_MSG_TRACK_STATUS:
            ret = pmoq_msg_track_status_cmp(&msg->msg_payload.track_status,
                (pmoq_msg_track_status_t*)test->payload_ref);
            break;
        case PMOQ_MSG_GOAWAY:
            ret = pmoq_msg_goaway_cmp(&msg->msg_payload.goaway,
                (pmoq_msg_goaway_t*)test->payload_ref);
            break;
        case PMOQ_MSG_CLIENT_SETUP:
            ret = pmoq_msg_client_setup_cmp(&msg->msg_payload.client_setup,
                (pmoq_msg_client_setup_t*)test->payload_ref);
            break;
        case PMOQ_MSG_SERVER_SETUP:
            ret = pmoq_msg_server_setup_cmp(&msg->msg_payload.server_setup,
                (pmoq_msg_server_setup_t*)test->payload_ref);
            break;
        case PMOQ_MSG_STREAM_HEADER_TRACK:
            ret = pmoq_msg_stream_header_track_cmp(&msg->msg_payload.header_track,
                (pmoq_msg_stream_header_track_t*)test->payload_ref);
            break;
        case PMOQ_MSG_STREAM_HEADER_GROUP:
            ret = pmoq_msg_stream_header_group_cmp(&msg->msg_payload.header_group,
                (pmoq_msg_stream_header_group_t*)test->payload_ref);
            break;
        default:
            /* Unexpected */
            ret = -1;
            break;
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
    else if (mpoq_test_msg_compare(test, &msg) != 0){
        ret = -1;
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

    msg.msg_type = test->msg_type;
    switch (test->msg_type) {
    case PMOQ_MSG_OBJECT_STREAM:
        memcpy(&msg.msg_payload.object_stream,
            test->payload_ref, sizeof(pmoq_msg_object_stream_t));
        break;
    case PMOQ_MSG_OBJECT_DATAGRAM:
        memcpy(&msg.msg_payload.datagram,
            test->payload_ref, sizeof(pmoq_msg_object_stream_t));
        break;
    case PMOQ_MSG_SUBSCRIBE:
        memcpy(&msg.msg_payload.subscribe,
            test->payload_ref, sizeof(pmoq_msg_subscribe_t));
        break;
    case PMOQ_MSG_SUBSCRIBE_OK:
        memcpy(&msg.msg_payload.subscribe_ok,
            test->payload_ref, sizeof(pmoq_msg_subscribe_ok_t));
        break;
    case PMOQ_MSG_SUBSCRIBE_ERROR:
        memcpy(&msg.msg_payload.subscribe_error,
            test->payload_ref, sizeof(pmoq_msg_subscribe_error_t));
        break;
    case PMOQ_MSG_ANNOUNCE:
        memcpy(&msg.msg_payload.announce,
            test->payload_ref, sizeof(pmoq_msg_announce_t));
        break;
    case PMOQ_MSG_ANNOUNCE_OK:
        memcpy(&msg.msg_payload.announce_ok,
            test->payload_ref, sizeof(pmoq_msg_track_namespace_t));
        break;
    case PMOQ_MSG_ANNOUNCE_ERROR:
        memcpy(&msg.msg_payload.announce_error,
            test->payload_ref, sizeof(pmoq_msg_announce_error_t));
        break;
    case PMOQ_MSG_UNANNOUNCE:
        memcpy(&msg.msg_payload.unannounce,
            test->payload_ref, sizeof(pmoq_msg_track_namespace_t));
        break;
    case PMOQ_MSG_UNSUBSCRIBE:
        memcpy(&msg.msg_payload.unsubscribe,
            test->payload_ref, sizeof(pmoq_msg_unsubscribe_t));
        break;
    case PMOQ_MSG_SUBSCRIBE_DONE:
        memcpy(&msg.msg_payload.subscribe_done,
            test->payload_ref, sizeof(pmoq_msg_subscribe_done_t));
        break;
    case PMOQ_MSG_ANNOUNCE_CANCEL:
        memcpy(&msg.msg_payload.announce_cancel,
            test->payload_ref, sizeof(pmoq_msg_track_namespace_t));
        break;
    case PMOQ_MSG_TRACK_STATUS_REQUEST:
        memcpy(&msg.msg_payload.track_status_request,
            test->payload_ref, sizeof(pmoq_msg_track_status_request_t));
        break;
    case PMOQ_MSG_TRACK_STATUS:
        memcpy(&msg.msg_payload.track_status,
            test->payload_ref, sizeof(pmoq_msg_track_status_t));
        break;
    case PMOQ_MSG_GOAWAY:
        memcpy(&msg.msg_payload.goaway,
            test->payload_ref, sizeof(pmoq_msg_goaway_t));
        break;
    case PMOQ_MSG_CLIENT_SETUP:
        memcpy(&msg.msg_payload.client_setup,
            test->payload_ref, sizeof(pmoq_msg_client_setup_t));
        break;
    case PMOQ_MSG_SERVER_SETUP:
        memcpy(&msg.msg_payload.server_setup,
            test->payload_ref, sizeof(pmoq_msg_server_setup_t));
        break;
    case PMOQ_MSG_STREAM_HEADER_TRACK:
        memcpy(&msg.msg_payload.header_track,
            test->payload_ref, sizeof(pmoq_msg_stream_header_track_t));
        break;
    case PMOQ_MSG_STREAM_HEADER_GROUP:
        memcpy(&msg.msg_payload.header_group,
            test->payload_ref, sizeof(pmoq_msg_stream_header_group_t));
        break;
    default:
        /* Unexpected */
        ret = -1;
        break;
    }

    if (ret == 0) {
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