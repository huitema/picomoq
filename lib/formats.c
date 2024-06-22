/* Code to encode, decode, skip, etc.
* The _format() functions take a message structure as input
* and produce a string of bytes; the _parse() functions do
* the reverse.
* 
* TODO: we are going to receive data from the network as segments
* of message. There are no message boundaries, so we don't know
* whether we have received enough data to parse the message.
* The plan is try the parse function. If it succeeds, then
* we can parse. If not, we either need more bytes or need to
* complaint that the message is not formated properly -- for
* example if it refers to an unsupported message type. Thus,
* we need to return an error code in "parse", set to 0 if we
* merely need more bytes.
* 
* Question: do names have size limits?
*/
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <picoquic.h>
#include <picoquic_utils.h>
#include "picomoq.h"

const uint8_t* pmoq_varint_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, uint64_t * v)
{
    const uint8_t* r_bytes;
    if ((r_bytes = picoquic_frames_varint_decode(bytes, bytes_max, v)) == NULL) {
        if (bytes < bytes_max) {
            *err = needed + VARINT_LEN_T(bytes, int) - (int)(bytes_max - bytes);
        }
        else {
            *err = needed + 1;
        }
    }
    return r_bytes;
}

const uint8_t* pmoq_uint8_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, uint8_t * v)
{
    const uint8_t* r_bytes;
    if ((r_bytes = picoquic_frames_uint8_decode(bytes, bytes_max, v)) == NULL) {
        *err = needed + 1;
    }
    return r_bytes;
}

uint8_t* pmoq_bits_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_bits_t* bits_string)
{
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, bits_string->nb_bits)) != NULL) {
        uint64_t nb_bytes = (bits_string->nb_bits + 7) >> 3;
        if (bytes + nb_bytes > bytes_max) {
            bytes = NULL;
        }
        else {
            memcpy(bytes, bits_string->bits, (size_t)nb_bytes);
            bytes += nb_bytes;
        }
    }
    return bytes;
}

const uint8_t* pmoq_bits_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_bits_t* bits_string)
{
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &bits_string->nb_bits)) != NULL) {
        uint64_t nb_bytes = (bits_string->nb_bits + 7) >> 3;
        if (bytes + nb_bytes > bytes_max) {
            if (bits_string->nb_bits > PMOQ_BIT_STRING_SIZE_MAX) {
                *err = -1;
            }
            else {
                *err = needed + (int)nb_bytes - (int)(bytes_max - bytes);
            }
            bytes = NULL;
        }
        else {
            bits_string->bits = (uint8_t *)bytes;
            bytes += nb_bytes;
        }
    }
    return bytes;
}

uint8_t* pmoq_msg_string_parameter_format(uint8_t* bytes, const uint8_t* bytes_max, uint64_t key, size_t l, const uint8_t* v)
{
    /* Encode key, length, value */
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, key)) != NULL) {
        bytes = picoquic_frames_length_data_encode(bytes, bytes_max, l, v);
    }
    return bytes;
}
uint8_t* pmoq_msg_varint_parameter_format(uint8_t* bytes, const uint8_t* bytes_max, uint64_t key, uint64_t v)
{
    /* Encode key, length, value */
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, key)) != NULL) {
        if (bytes + 2 > bytes_max) {
            bytes = NULL;
        }
        else {
            uint8_t* byte_l = bytes++;
            *byte_l = 0;
            if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, v)) != NULL) {
                *byte_l = (uint8_t)(bytes - byte_l) - 1;
            }
        }
    }
    return bytes;
}
const uint8_t* pmoq_msg_parameter_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, uint64_t *key, uint64_t *l, uint8_t**v)
{
    /* Decode key, length, value */
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 1, key)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, l)) != NULL) {
        if (*l > PMOQ_BIT_STRING_SIZE_MAX) {
            *err = -1;
            bytes = NULL;
        } else if (bytes + (int)*l > bytes_max) {
            *err = needed + (int)*l - (int)(bytes_max - bytes);
            bytes = NULL;
        }
        else {
            *v = (uint8_t*)bytes;
            bytes += (int)*l;
        }
    }
    return bytes;
}
uint8_t* pmoq_msg_setup_parameters_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_setup_parameters_t* param)
{
    uint64_t nb_params = (param->path == NULL) ? 1: 2;
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, nb_params)) != NULL &&
        (bytes = pmoq_msg_varint_parameter_format(bytes, bytes_max, PMOQ_SETUP_PARAMETER_ROLE, (uint64_t)param->role)) != NULL){
        if (param->path != NULL) {
            bytes = pmoq_msg_string_parameter_format(bytes, bytes_max, PMOQ_SETUP_PARAMETER_PATH, param->path_length, param->path);
        }
    }
    return bytes;
}
const uint8_t* pmoq_msg_setup_parameters_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_setup_parameters_t * param) {
    uint64_t nb_params = 0;
    memset(param, 0, sizeof(pmoq_setup_parameters_t));
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &nb_params)) != NULL) {
        if (nb_params > PMOQ_PARAMETERS_NUMBER_MAX) {
            *err = -1;
            bytes = NULL;
        }
        else if (nb_params == 0) {
            /* There must be at least one parameter, the role. */
            *err = -1;
            bytes = NULL;
        }
        else {
            for (int i = 0; i < (int)nb_params; i++) {
                uint64_t key;
                uint64_t l;
                uint8_t* v;

                if ((bytes = pmoq_msg_parameter_parse(bytes, bytes_max, err,
                    needed + 2 * ((int)nb_params - i - 1), &key, &l, &v)) == NULL) {
                    break;
                }
                else {
                    switch (key) {
                    case PMOQ_SETUP_PARAMETER_ROLE:
                        if (l != 1 || *v == pmoq_setup_role_undef || *v > pmoq_setup_role_max) {
                            /* malformed! */
                            bytes = NULL;
                            *err = -1;
                        }
                        else if (param->role != pmoq_setup_role_undef) {
                            /* Double definition! */
                            bytes = NULL;
                            *err = -1;
                        }
                        else {
                            param->role = (uint8_t)*v;
                        }
                        break;
                    case PMOQ_SETUP_PARAMETER_PATH:
                        if (param->path != NULL) {
                            /* Double definition! */
                            bytes = NULL;
                            *err = -1;
                        }
                        else {
                            param->path = v;
                            param->path_length = (size_t)l;
                        }
                        break;
                    default:
                        /* By default, ignore unused parameters */
                        break;
                    }
                }
            }
        }
    }

    return bytes;
}

uint8_t* pmoq_subscribe_parameters_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_subscribe_parameters_t* param)
{
    uint64_t nb_params = (param->auth_info == NULL) ? 0 : 1;
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, nb_params)) != NULL){
        if (param->auth_info != NULL) {
            bytes = pmoq_msg_string_parameter_format(bytes, bytes_max, PMOQ_SETUP_PARAMETER_PATH, param->auth_info_len, param->auth_info);
        }
    }
    return bytes;
}
const uint8_t* pmoq_subscribe_parameters_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_subscribe_parameters_t * param) {
    uint64_t nb_params = 0;
    memset(param, 0, sizeof(pmoq_setup_parameters_t));
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &nb_params)) != NULL) {
        if (nb_params > PMOQ_PARAMETERS_NUMBER_MAX) {
            *err = -1;
            bytes = NULL;
        }
        else {
            for (int i = 0; i < (int)nb_params; i++) {
                uint64_t key;
                uint64_t l;
                uint8_t* v;

                if ((bytes = pmoq_msg_parameter_parse(bytes, bytes_max, err,
                    needed + 2*((int)nb_params-i-1), &key, &l, &v)) == NULL) {
                    break;
                }
                else {
                    switch (key) {
                    case pmoq_para_auth_info_key:
                        if (param->auth_info != NULL) {
                            /* Double definition! */
                            bytes = NULL;
                            *err = -1;
                        }
                        else {
                            param->auth_info = v;
                            param->auth_info_len = (size_t)l;
                        }
                        break;
                    default:
                        /* By default, ignore unused parameters */
                        break;
                    }
                }
            }
        }
    }

    return bytes;
}

uint8_t* pmoq_msg_object_stream_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_object_stream_t* object_stream) {
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, object_stream->subscribe_id)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, object_stream->track_alias)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, object_stream->group_id)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, object_stream->object_send_order)) != NULL) {
        bytes = picoquic_frames_varint_encode(bytes, bytes_max, object_stream->object_status);
    }
    return bytes;
}
const uint8_t* pmoq_msg_object_stream_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_object_stream_t* object_stream)
{
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 4, &object_stream->subscribe_id)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 3, &object_stream->track_alias)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 2, &object_stream->group_id)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 1, &object_stream->object_send_order)) != NULL) {
        bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &object_stream->object_status);
    }
    return bytes;
}

uint8_t* pmoq_msg_subscribe_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_subscribe_t * subscribe)
{
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe->subscribe_id)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe->track_alias)) != NULL &&
        (bytes = pmoq_bits_format(bytes, bytes_max, &subscribe->track_namespace)) != NULL &&
        (bytes = pmoq_bits_format(bytes, bytes_max, &subscribe->track_name)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe->filter_type)) != NULL) {
        if (subscribe->filter_type == pmoq_msg_filter_absolute_start ||
            subscribe->filter_type == pmoq_msg_filter_absolute_range) {
            if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe->start_group)) != NULL &&
                (bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe->start_object)) != NULL &&
                subscribe->filter_type == pmoq_msg_filter_absolute_range) {
                if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe->end_group)) != NULL)
                    bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe->end_object);
            }
        }
        if (bytes != NULL) {
            bytes = pmoq_subscribe_parameters_format(bytes, bytes_max, &subscribe->subscribe_parameters);
        }
    }
    return bytes;
}
const uint8_t* pmoq_msg_subscribe_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_subscribe_t* subscribe)
{
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 4, &subscribe->subscribe_id)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 3, &subscribe->track_alias)) != NULL &&
        (bytes = pmoq_bits_parse(bytes, bytes_max, err, needed + 2, &subscribe->track_namespace)) != NULL &&
        (bytes = pmoq_bits_parse(bytes, bytes_max, err, needed + 1, &subscribe->track_name)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &subscribe->filter_type)) != NULL) {
        if (subscribe->filter_type == pmoq_msg_filter_absolute_start ||
            subscribe->filter_type == pmoq_msg_filter_absolute_range) {
            if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &subscribe->start_group)) != NULL &&
                (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &subscribe->start_object)) != NULL &&
                subscribe->filter_type == pmoq_msg_filter_absolute_range) {
                if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &subscribe->end_group)) != NULL)
                    bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &subscribe->end_object);
            }
        }
        else if (subscribe->filter_type == 0 ||
            subscribe->filter_type > pmoq_msg_filter_max) {
            bytes = NULL;
        }

        if (bytes != NULL) {
            bytes = pmoq_subscribe_parameters_parse(bytes, bytes_max, err, needed, &subscribe->subscribe_parameters);
        }
    }
    return bytes;
}

uint8_t* pmoq_msg_subscribe_ok_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_subscribe_ok_t * subscribe_ok)
{
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe_ok->subscribe_id)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe_ok->expires)) != NULL &&
        (bytes = picoquic_frames_uint8_encode(bytes, bytes_max, subscribe_ok->content_exists)) != NULL) {
        if (subscribe_ok->content_exists > 0) {
            if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe_ok->largest_group_id)) != NULL) {
                bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe_ok->largest_object_id);
            }
        }
    }
    return bytes;
}
const uint8_t* pmoq_msg_subscribe_ok_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_subscribe_ok_t* subscribe_ok)
{
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 2, &subscribe_ok->subscribe_id)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 1, &subscribe_ok->expires)) != NULL &&
        (bytes = pmoq_uint8_parse(bytes, bytes_max, err, needed, &subscribe_ok->content_exists)) != NULL) {
        if (subscribe_ok->content_exists == 1) {
            if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 1, &subscribe_ok->largest_group_id)) != NULL) {
                bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &subscribe_ok->largest_object_id);
            }
        }
        else if (subscribe_ok->content_exists > 1) {
            *err = -1;
            bytes = NULL;
        }
    }
    return bytes;
}

uint8_t* pmoq_msg_subscribe_error_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_subscribe_error_t * subscribe_error)
{
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe_error->subscribe_id)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe_error->error_code)) != NULL &&
        (bytes = pmoq_bits_format(bytes, bytes_max, &subscribe_error->reason_phrase)) != NULL) {
        bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe_error->error_code);
    }
    return bytes; 
}
const uint8_t* pmoq_msg_subscribe_error_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_subscribe_error_t* subscribe_error)
{
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 3, &subscribe_error->subscribe_id)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 2, &subscribe_error->error_code)) != NULL &&
        (bytes = pmoq_bits_parse(bytes, bytes_max, err, needed + 1, &subscribe_error->reason_phrase)) != NULL) {
        bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &subscribe_error->error_code);
    }
    return bytes;
}

uint8_t* pmoq_msg_announce_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_announce_t * announce)
{
    if ((bytes = pmoq_bits_format(bytes, bytes_max, &announce->track_namespace)) != NULL) {
        bytes = pmoq_subscribe_parameters_format(bytes, bytes_max, &announce->announce_parameters);
    }
 
    return bytes;
}
const uint8_t* pmoq_msg_announce_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_announce_t * announce)
{
    if ((bytes = pmoq_bits_parse(bytes, bytes_max, err, needed+1, &announce->track_namespace)) != NULL) {
        bytes = pmoq_subscribe_parameters_parse(bytes, bytes_max, err, needed, &announce->announce_parameters);
    }

    return bytes;
}

uint8_t* pmoq_msg_track_namespace_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_track_namespace_t * tns)
{
    /* is there a size limit ? */
    return pmoq_bits_format(bytes, bytes_max, &tns->track_namespace);
}
const uint8_t* pmoq_msg_track_namespace_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_track_namespace_t* tns)
{
    return pmoq_bits_parse(bytes, bytes_max, err, needed, &tns->track_namespace);
}

uint8_t* pmoq_msg_announce_error_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_announce_error_t * announce_error)
{
    if ((bytes = pmoq_bits_format(bytes, bytes_max, &announce_error->track_namespace)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, announce_error->error_code)) != NULL) {
        bytes = pmoq_bits_format(bytes, bytes_max, &announce_error->reason_phrase);
    }
    return bytes;
}
const uint8_t* pmoq_msg_announce_error_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_announce_error_t* announce_error)
{
    if ((bytes = pmoq_bits_parse(bytes, bytes_max, err, needed + 1, &announce_error->track_namespace)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 1, &announce_error->error_code)) != NULL) {
        bytes = pmoq_bits_parse(bytes, bytes_max, err, needed, &announce_error->reason_phrase);
    }
    return bytes;
}

uint8_t* pmoq_msg_unsubscribe_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_unsubscribe_t * unsubscribe)
{
    return bytes = picoquic_frames_varint_encode(bytes, bytes_max, unsubscribe->subscribe_id);
}
const uint8_t* pmoq_msg_unsubscribe_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_unsubscribe_t * unsubscribe)
{
    return bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &unsubscribe->subscribe_id);
}

uint8_t* pmoq_msg_subscribe_done_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_subscribe_done_t * subscribe_done)
{
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe_done->subscribe_id)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe_done->status_code)) != NULL &&
        (bytes = pmoq_bits_format(bytes, bytes_max, &subscribe_done->reason_phrase)) != NULL &&
        (bytes = picoquic_frames_uint8_encode(bytes, bytes_max, subscribe_done->content_exists)) != NULL) {
        if (subscribe_done->content_exists > 0) {
            if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe_done->final_group_id)) != NULL) {
                bytes = picoquic_frames_varint_encode(bytes, bytes_max, subscribe_done->final_object_id);
            }
        }
    }
    return bytes;
}
const uint8_t* pmoq_msg_subscribe_done_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_subscribe_done_t* subscribe_done)
{
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 3, &subscribe_done->subscribe_id)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 2, &subscribe_done->status_code)) != NULL &&
        (bytes = pmoq_bits_parse(bytes, bytes_max, err, needed + 1, &subscribe_done->reason_phrase)) != NULL &&
        (bytes = pmoq_uint8_parse(bytes, bytes_max, err, needed, &subscribe_done->content_exists)) != NULL) {
        if (subscribe_done->content_exists == 1) {
            if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 1, &subscribe_done->final_group_id)) != NULL) {
                bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &subscribe_done->final_object_id);
            }
            else if (subscribe_done->content_exists > 1) {
                *err = -1;
                bytes = NULL;
            }
        }
    }
    return bytes;
}

uint8_t* pmoq_msg_track_status_request_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_track_status_request_t * track_status_request)
{
    if ((bytes = pmoq_bits_format(bytes, bytes_max, &track_status_request->track_namespace)) != NULL) {
        bytes = pmoq_bits_format(bytes, bytes_max, &track_status_request->track_name);
    }
    return bytes; 
}
const uint8_t* pmoq_msg_track_status_request_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_track_status_request_t* track_status_request)
{
    if ((bytes = pmoq_bits_parse(bytes, bytes_max, err, needed + 1, &track_status_request->track_namespace)) != NULL) {
        bytes = pmoq_bits_parse(bytes, bytes_max, err, needed, &track_status_request->track_name);
    }
    return bytes;
}

uint8_t* pmoq_msg_track_status_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_track_status_t * track_status)
{
    if ((bytes = pmoq_bits_format(bytes, bytes_max, &track_status->track_namespace)) != NULL &&
        (bytes = pmoq_bits_format(bytes, bytes_max, &track_status->track_name)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, track_status->status_code)) != NULL) {
        if (track_status->status_code == PMOQ_TRACK_STATUS_IN_PROGRESS) {
            if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, track_status->last_group_id)) != NULL) {
                bytes = picoquic_frames_varint_encode(bytes, bytes_max, track_status->last_object_id);
            }
        }
    }
    return bytes; 
}
const uint8_t* pmoq_msg_track_status_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_track_status_t* track_status)
{
    if ((bytes = pmoq_bits_parse(bytes, bytes_max, err, needed + 2, &track_status->track_namespace)) != NULL &&
        (bytes = pmoq_bits_parse(bytes, bytes_max, err, needed + 1, &track_status->track_name)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &track_status->status_code)) != NULL) {
        if (track_status->status_code == PMOQ_TRACK_STATUS_IN_PROGRESS) {
            if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 1, &track_status->last_group_id)) != NULL) {
                bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &track_status->last_object_id);
            }
            else if (track_status->status_code > PMOQ_TRACK_STATUS_IS_RELAY) {
                *err = -1;
                bytes = NULL;
            }
        }
    }
    return bytes;
}

uint8_t* pmoq_msg_goaway_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_goaway_t * goaway)
{
    return pmoq_bits_format(bytes, bytes_max, &goaway->uri);
}
const uint8_t* pmoq_msg_goaway_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_goaway_t * goaway)
{
    return pmoq_bits_parse(bytes, bytes_max, err, needed, &goaway->uri);
}

uint8_t* pmoq_msg_client_setup_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_client_setup_t * client_setup)
{
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, client_setup->supported_versions_nb)) != NULL) {
        if (client_setup->supported_versions_nb > PMOQ_VERSION_NUMBER_MAX) {
            bytes = NULL;
        }
        else {
            for (uint64_t i = 0; i < client_setup->supported_versions_nb; i++) {
                if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, client_setup->supported_versions[i])) == NULL) {
                    break;
                }
            }
            if (bytes != NULL) {
                /* Encode parameters */
                bytes = pmoq_msg_setup_parameters_format(bytes, bytes_max, &client_setup->setup_parameters);
            }
        }
    }

    return bytes;
}
const uint8_t* pmoq_msg_client_setup_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_client_setup_t * client_setup)
{
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 2, &client_setup->supported_versions_nb)) != NULL) {
        if (client_setup->supported_versions_nb > PMOQ_VERSION_NUMBER_MAX) {
            bytes = NULL;
        }
        else {
            for (int i = 0; i < (int)client_setup->supported_versions_nb; i++) {
                uint64_t v;
                if ((bytes = pmoq_varint_parse(bytes, bytes_max, err,
                    needed + (int)client_setup->supported_versions_nb - i, &v)) == NULL ||
                    v > UINT32_MAX) {
                    bytes = NULL;
                    break;
                }
                client_setup->supported_versions[i] = (uint32_t)v;
            }
            if (bytes != NULL) {
                /* Decode parameters */
                bytes = pmoq_msg_setup_parameters_parse(bytes, bytes_max, err, needed, &client_setup->setup_parameters);
            }
        }
    }
    
    return bytes;
}

uint8_t* pmoq_msg_server_setup_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_server_setup_t* server_setup)
{
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, server_setup->selected_version)) != NULL) {
        bytes = pmoq_msg_setup_parameters_format(bytes, bytes_max, &server_setup->setup_parameters);
    }
    return bytes;
}
const uint8_t* pmoq_msg_server_setup_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_server_setup_t* server_setup)
{
    uint64_t v;
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed+1, &v)) != NULL) {
        if (v > UINT32_MAX) {
            bytes = NULL;
            *err = -1;
        }
        else {
            server_setup->selected_version = (uint32_t)v;
            bytes = pmoq_msg_setup_parameters_parse(bytes, bytes_max, err, needed, &server_setup->setup_parameters);
        }
    }
    return bytes;
}

uint8_t* pmoq_msg_stream_header_track_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_stream_header_track_t * header_track)
{
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, header_track->subscribe_id)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, header_track->track_alias)) != NULL)
    {
        bytes = picoquic_frames_varint_encode(bytes, bytes_max, header_track->object_send_order);
    }
    return bytes;
}
const uint8_t* pmoq_msg_stream_header_track_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_stream_header_track_t* header_track)
{
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 2, &header_track->subscribe_id)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 1, &header_track->track_alias)) != NULL)
    {
        bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &header_track->object_send_order);
    }
    return bytes;
}

uint8_t* pmoq_msg_stream_header_group_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_stream_header_group_t * header_group)
{ 
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, header_group->subscribe_id)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, header_group->track_alias)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, header_group->group_id)) != NULL)
    {
        bytes = picoquic_frames_varint_encode(bytes, bytes_max, header_group->object_send_order);
    }
    return bytes;
}
const uint8_t* pmoq_msg_stream_header_group_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_stream_header_group_t* header_group)
{
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 3, &header_group->subscribe_id)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 2, &header_group->track_alias)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed + 1, &header_group->group_id)) != NULL) {
        bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &header_group->object_send_order);
    }
    return bytes;
}

uint8_t* pmoq_msg_stream_object_header_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_stream_object_header_t * object_header)
{ 
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, object_header->object_id)) != NULL &&
        (bytes = picoquic_frames_varint_encode(bytes, bytes_max, object_header->payload_length)) != NULL) {
        if (object_header->payload_length == 0) {
            bytes = picoquic_frames_varint_encode(bytes, bytes_max, object_header->object_status);
        }
    }
    return bytes;
}
const uint8_t* pmoq_msg_stream_object_header_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_stream_object_header_t * object_header)
{ 
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed+1, &object_header->object_id)) != NULL &&
        (bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &object_header->payload_length)) != NULL) {
        if (object_header->payload_length == 0) {
            bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &object_header->object_status);
            if (object_header->object_status > PMOQ_OBJECT_STATUS_END_OF_GROUP_AND_TRACK) {
                *err = -1;
                bytes = NULL;
            }
        }
    }
    return bytes;
}

uint8_t * pmoq_msg_keyed_format(uint8_t* bytes, const uint8_t* bytes_max, uint64_t msg_type, const pmoq_msg_t* msg)
{
    switch (msg_type) {
    case PMOQ_MSG_OBJECT_STREAM:
        bytes = pmoq_msg_object_stream_format(bytes, bytes_max, &msg->msg_payload.object_stream);
        break;
    case PMOQ_MSG_OBJECT_DATAGRAM:
        bytes = pmoq_msg_object_stream_format(bytes, bytes_max, &msg->msg_payload.datagram);
        break;
    case PMOQ_MSG_SUBSCRIBE:
        bytes = pmoq_msg_subscribe_format(bytes, bytes_max, &msg->msg_payload.subscribe);
        break;
    case PMOQ_MSG_SUBSCRIBE_OK:
        bytes = pmoq_msg_subscribe_ok_format(bytes, bytes_max, &msg->msg_payload.subscribe_ok);
        break;
    case PMOQ_MSG_SUBSCRIBE_ERROR:
        bytes = pmoq_msg_subscribe_error_format(bytes, bytes_max, &msg->msg_payload.subscribe_error);
        break;
    case PMOQ_MSG_ANNOUNCE:
        bytes = pmoq_msg_announce_format(bytes, bytes_max, &msg->msg_payload.announce);
        break;
    case PMOQ_MSG_ANNOUNCE_OK:
        bytes = pmoq_msg_track_namespace_format(bytes, bytes_max, &msg->msg_payload.announce_ok);
        break;
    case PMOQ_MSG_ANNOUNCE_ERROR:
        bytes = pmoq_msg_announce_error_format(bytes, bytes_max, &msg->msg_payload.announce_error);
        break;
    case PMOQ_MSG_UNANNOUNCE:
        bytes = pmoq_msg_track_namespace_format(bytes, bytes_max, &msg->msg_payload.unannounce);
        break;
    case PMOQ_MSG_UNSUBSCRIBE:
        bytes = pmoq_msg_unsubscribe_format(bytes, bytes_max, &msg->msg_payload.unsubscribe);
        break;
    case PMOQ_MSG_SUBSCRIBE_DONE:
        bytes = pmoq_msg_subscribe_done_format(bytes, bytes_max, &msg->msg_payload.subscribe_done);
        break;
    case PMOQ_MSG_ANNOUNCE_CANCEL:
        bytes = pmoq_msg_track_namespace_format(bytes, bytes_max, &msg->msg_payload.announce_cancel);
        break;
    case PMOQ_MSG_TRACK_STATUS_REQUEST:
        bytes = pmoq_msg_track_status_request_format(bytes, bytes_max, &msg->msg_payload.track_status_request);
        break;
    case PMOQ_MSG_TRACK_STATUS:
        bytes = pmoq_msg_track_status_format(bytes, bytes_max, &msg->msg_payload.track_status);
        break;
    case PMOQ_MSG_GOAWAY:
        bytes = pmoq_msg_goaway_format(bytes, bytes_max, &msg->msg_payload.goaway);
        break;
    case PMOQ_MSG_CLIENT_SETUP:
        bytes = pmoq_msg_client_setup_format(bytes, bytes_max, &msg->msg_payload.client_setup);
        break;
    case PMOQ_MSG_SERVER_SETUP:
        bytes = pmoq_msg_server_setup_format(bytes, bytes_max, &msg->msg_payload.server_setup);
        break;
    case PMOQ_MSG_STREAM_HEADER_TRACK:
        bytes = pmoq_msg_stream_header_track_format(bytes, bytes_max, &msg->msg_payload.header_track);
        break;
    case PMOQ_MSG_STREAM_HEADER_GROUP:
        bytes = pmoq_msg_stream_header_group_format(bytes, bytes_max, &msg->msg_payload.header_group);
        break;
    default:
        /* Unexpected */
        bytes = NULL;
        break;
    }
    return bytes;
}
uint8_t * pmoq_msg_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_t* msg)
{
    if ((bytes = picoquic_frames_varint_encode(bytes, bytes_max, msg->msg_type)) != NULL) {
        bytes = pmoq_msg_keyed_format(bytes, bytes_max, msg->msg_type, msg);
    }
    return bytes;
}

const uint8_t * pmoq_msg_keyed_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, uint64_t msg_type, pmoq_msg_t* msg)
{
    switch (msg_type) {
    case PMOQ_MSG_OBJECT_STREAM:
        bytes = pmoq_msg_object_stream_parse(bytes, bytes_max, err, needed, &msg->msg_payload.object_stream);
        break;
    case PMOQ_MSG_OBJECT_DATAGRAM:
        bytes = pmoq_msg_object_stream_parse(bytes, bytes_max, err, needed, &msg->msg_payload.datagram);
        break;
    case PMOQ_MSG_SUBSCRIBE: 
        bytes = pmoq_msg_subscribe_parse(bytes, bytes_max, err, needed, &msg->msg_payload.subscribe);
        break;
    case PMOQ_MSG_SUBSCRIBE_OK:
        bytes = pmoq_msg_subscribe_ok_parse(bytes, bytes_max, err, needed, &msg->msg_payload.subscribe_ok);
        break;
    case PMOQ_MSG_SUBSCRIBE_ERROR:
        bytes = pmoq_msg_subscribe_error_parse(bytes, bytes_max, err, needed, &msg->msg_payload.subscribe_error);
        break;
    case PMOQ_MSG_ANNOUNCE:
        bytes = pmoq_msg_announce_parse(bytes, bytes_max, err, needed, &msg->msg_payload.announce);
        break;
    case PMOQ_MSG_ANNOUNCE_OK:
        bytes = pmoq_msg_track_namespace_parse(bytes, bytes_max, err, needed, &msg->msg_payload.announce_ok);
        break;
    case PMOQ_MSG_ANNOUNCE_ERROR:
        bytes = pmoq_msg_announce_error_parse(bytes, bytes_max, err, needed, &msg->msg_payload.announce_error);
        break;
    case PMOQ_MSG_UNANNOUNCE:
        bytes = pmoq_msg_track_namespace_parse(bytes, bytes_max, err, needed, &msg->msg_payload.unannounce);
        break;
    case PMOQ_MSG_UNSUBSCRIBE:
        bytes = pmoq_msg_unsubscribe_parse(bytes, bytes_max, err, needed, &msg->msg_payload.unsubscribe);
        break;
    case PMOQ_MSG_SUBSCRIBE_DONE:
        bytes = pmoq_msg_subscribe_done_parse(bytes, bytes_max, err, needed, &msg->msg_payload.subscribe_done);
        break;
    case PMOQ_MSG_ANNOUNCE_CANCEL:
        bytes = pmoq_msg_track_namespace_parse(bytes, bytes_max, err, needed, &msg->msg_payload.announce_cancel);
        break;
    case PMOQ_MSG_TRACK_STATUS_REQUEST:
        bytes = pmoq_msg_track_status_request_parse(bytes, bytes_max, err, needed, &msg->msg_payload.track_status_request);
        break;
    case PMOQ_MSG_TRACK_STATUS:
        bytes = pmoq_msg_track_status_parse(bytes, bytes_max, err, needed, &msg->msg_payload.track_status);
        break;
    case PMOQ_MSG_GOAWAY:
        bytes = pmoq_msg_goaway_parse(bytes, bytes_max, err, needed, &msg->msg_payload.goaway);
        break;
    case PMOQ_MSG_CLIENT_SETUP:
        bytes = pmoq_msg_client_setup_parse(bytes, bytes_max, err, needed, &msg->msg_payload.client_setup);
        break;
    case PMOQ_MSG_SERVER_SETUP:
        bytes = pmoq_msg_server_setup_parse(bytes, bytes_max, err, needed, &msg->msg_payload.server_setup);
        break;
    case PMOQ_MSG_STREAM_HEADER_TRACK:
        bytes = pmoq_msg_stream_header_track_parse(bytes, bytes_max, err, needed, &msg->msg_payload.header_track);
        break;
    case PMOQ_MSG_STREAM_HEADER_GROUP:
        bytes = pmoq_msg_stream_header_group_parse(bytes, bytes_max, err, needed, &msg->msg_payload.header_group);
        break;
    default:
        /* Unexpected */
        *err = -1;
        bytes = NULL;
        break;
    }
    return bytes;
}
const uint8_t * pmoq_msg_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_t* msg)
{
    if ((bytes = pmoq_varint_parse(bytes, bytes_max, err, needed, &msg->msg_type)) != NULL) {
        bytes = pmoq_msg_keyed_parse(bytes, bytes_max, err, needed, msg->msg_type, msg);
    }
    return bytes;
} 
