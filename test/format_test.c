#include <stdint.h>
#include "picoquic/picoquic.h"
#include "picoquic/picoquic_utils.h"
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
            ret = memcmp(bs->bits, bs_ref->bits, (bs->nb_bits + 7) / 8);
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
        pmoq_msg_setup_parameters_cmp(&s->setup_parameters, &s_ref->setup_parameters) == 0) {
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
            ret = 0;
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

int pmoq_msg_format_test()
{
    return -1;
}