#ifndef PICOMOQ_H
#define PICOMOQ_H
#ifdef __cplusplus
extern "C" {
#endif
/* Data types for the Pico MoQ messages
* * Notice that there is no code point for Subscribe Update!
*/
#define PMOQ_MSG_OBJECT_STREAM        0x0
#define PMOQ_MSG_OBJECT_DATAGRAM      0x1
#define PMOQ_MSG_SUBSCRIBE            0x3
#define PMOQ_MSG_SUBSCRIBE_OK         0x4
#define PMOQ_MSG_SUBSCRIBE_ERROR      0x5
#define PMOQ_MSG_ANNOUNCE             0x6
#define PMOQ_MSG_ANNOUNCE_OK          0x7
#define PMOQ_MSG_ANNOUNCE_ERROR       0x8
#define PMOQ_MSG_UNANNOUNCE           0x9
#define PMOQ_MSG_UNSUBSCRIBE          0xA
#define PMOQ_MSG_SUBSCRIBE_DONE       0xB
#define PMOQ_MSG_ANNOUNCE_CANCEL      0xC
#define PMOQ_MSG_TRACK_STATUS_REQUEST 0xD
#define PMOQ_MSG_TRACK_STATUS         0xE
#define PMOQ_MSG_GOAWAY               0x10
#define PMOQ_MSG_CLIENT_SETUP         0x40
#define PMOQ_MSG_SERVER_SETUP         0x41
#define PMOQ_MSG_STREAM_HEADER_TRACK  0x50
#define PMOQ_MSG_STREAM_HEADER_GROUP  0x51

#define PMOQ_PARAMETER_VERSION  0x00
#define PMOQ_PARAMETER_PATH  0x01
#define PMOQ_PARAMETER_AUTHORIZATION_INFO 0x02

#define PMOQ_PARAMETERS_NUMBER_MAX 16
#define PMOQ_VERSION_NUMBER_MAX 16

#define PMOQ_OBJECT_STATUS_NORMAL 0x0
#define PMOQ_OBJECT_STATUS_DOES_NOT_EXIST 0x1
#define PMOQ_OBJECT_STATUS_END_NO_SUCH_GROUP 0x2
#define PMOQ_OBJECT_STATUS_END_OF_GROUP 0x3
#define PMOQ_OBJECT_STATUS_END_OF_GROUP_AND_TRACK 0x4
#define PMOQ_OBJECT_STATUS_MAX 0x4

#define PMOQ_BIT_STRING_SIZE_MAX 8192
    
typedef struct st_pmoq_parameter_t {
    uint64_t p_type;
    uint64_t p_length;
    uint64_t p_value;
} pmoq_parameter_t;

typedef enum {
    pmoq_role_publisher = 0x01,
    pmoq_role_subscriber = 0x02,
    pmoq_role_pubsub = 0x03
} pmoq_role_t;

typedef struct st_pmoq_path_t {
    uint64_t path_length;
    uint8_t * path_value;
} pmoq_path_t;

typedef struct st_pmoq_bits_t {
    uint64_t nb_bits;
    uint8_t* bits;
} pmoq_bits_t;

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
#define PMOQ_TRACK_STATUS_DOES_NOT_EXIST 0x01
#define PMOQ_TRACK_STATUS_HAS_NOT_BEGUN 0x02
#define PMOQ_TRACK_STATUS_IS_RELAY 0x03
#define PMOQ_TRACK_STATUS_MAX 0x03
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

typedef struct st_pmoq_setup_parameters_t {
#define PMOQ_SETUP_PARAMETER_ROLE 0x00
#define PMOQ_SETUP_PARAMETER_PATH 0x01
    uint8_t role;
    size_t path_length;
    uint8_t* path;
} pmoq_setup_parameters_t;

typedef struct st_pmoq_msg_client_setup_t {
    uint64_t supported_versions_nb;
    uint32_t supported_versions[PMOQ_VERSION_NUMBER_MAX];
    pmoq_setup_parameters_t setup_parameters;
} pmoq_msg_client_setup_t;

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

typedef struct st_pmoq_msg_t {
    uint64_t msg_type;
    union {
        pmoq_msg_object_stream_t object_stream;
        pmoq_msg_object_stream_t datagram;
        pmoq_msg_subscribe_t subscribe;
        pmoq_msg_subscribe_ok_t subscribe_ok;
        pmoq_msg_subscribe_error_t subscribe_error;
        pmoq_msg_announce_t announce;
        pmoq_msg_track_namespace_t announce_ok;
        pmoq_msg_announce_error_t announce_error;
        pmoq_msg_track_namespace_t unannounce;
        pmoq_msg_unsubscribe_t unsubscribe;
        pmoq_msg_subscribe_done_t subscribe_done;
        pmoq_msg_track_namespace_t announce_cancel;
        pmoq_msg_track_status_request_t track_status_request;
        pmoq_msg_track_status_t track_status;
        pmoq_msg_goaway_t goaway;
        pmoq_msg_client_setup_t client_setup;
        pmoq_msg_server_setup_t server_setup;
        pmoq_msg_stream_header_track_t header_track;
        pmoq_msg_stream_header_group_t header_group;
    } msg_payload;
} pmoq_msg_t;

uint8_t* pmoq_msg_object_stream_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_object_stream_t* object_stream);
const uint8_t* pmoq_msg_object_stream_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_object_stream_t* object_stream);

uint8_t* pmoq_msg_subscribe_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_subscribe_t * subscribe);
const uint8_t* pmoq_msg_subscribe_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_subscribe_t * subscribe);

uint8_t* pmoq_msg_subscribe_ok_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_subscribe_ok_t * subscribe_ok);
const uint8_t* pmoq_msg_subscribe_ok_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_subscribe_ok_t * subscribe_ok);

uint8_t* pmoq_msg_subscribe_error_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_subscribe_error_t * subscribe_error);
const uint8_t* pmoq_msg_subscribe_error_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_subscribe_error_t * subscribe_error);

uint8_t* pmoq_msg_announce_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_announce_t * announce);
const uint8_t* pmoq_msg_announce_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_announce_t * announce);

uint8_t* pmoq_msg_track_namespace_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_track_namespace_t * announce_ok);
const uint8_t* pmoq_msg_track_namespace_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_track_namespace_t * announce_ok);

uint8_t* pmoq_msg_announce_error_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_announce_error_t * announce_error);
const uint8_t* pmoq_msg_announce_error_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_announce_error_t * announce_error);

uint8_t* pmoq_msg_unsubscribe_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_unsubscribe_t * unsubscribe);
const uint8_t* pmoq_msg_unsubscribe_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_unsubscribe_t * unsubscribe);

uint8_t* pmoq_msg_subscribe_done_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_subscribe_done_t * subscribe_done);
const uint8_t* pmoq_msg_subscribe_done_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_subscribe_done_t * subscribe_done);

uint8_t* pmoq_msg_track_status_request_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_track_status_request_t * track_status_request);
const uint8_t* pmoq_msg_track_status_request_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_track_status_request_t * track_status_request);

uint8_t* pmoq_msg_track_status_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_track_status_t * track_status);
const uint8_t* pmoq_msg_track_status_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_track_status_t * track_status);

uint8_t* pmoq_msg_goaway_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_goaway_t * goaway);
const uint8_t* pmoq_msg_goaway_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_goaway_t * goaway);

uint8_t* pmoq_msg_client_setup_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_client_setup_t * client_setup);
const uint8_t* pmoq_msg_client_setup_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_client_setup_t * client_setup);

uint8_t* pmoq_msg_server_setup_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_server_setup_t * server_setup);
const uint8_t* pmoq_msg_server_setup_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_server_setup_t * server_setup);

uint8_t* pmoq_msg_stream_header_track_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_stream_header_track_t * header_track);
const uint8_t* pmoq_msg_stream_header_track_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_stream_header_track_t * header_track);

uint8_t* pmoq_msg_stream_header_group_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_stream_header_group_t * header_group);
const uint8_t* pmoq_msg_stream_header_group_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_stream_header_group_t * header_group);

uint8_t* pmoq_msg_stream_object_header_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_stream_object_header_t* object_header);
const uint8_t* pmoq_msg_stream_object_header_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_stream_object_header_t* object_header);

uint8_t* pmoq_msg_keyed_format(uint8_t* bytes, const uint8_t* bytes_max, uint64_t msg_type, const pmoq_msg_t* msg);
uint8_t* pmoq_msg_format(uint8_t* bytes, const uint8_t* bytes_max, const pmoq_msg_t* msg);
const uint8_t* pmoq_msg_keyed_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, uint64_t msg_type, pmoq_msg_t* msg);
const uint8_t* pmoq_msg_parse(const uint8_t* bytes, const uint8_t* bytes_max, int* err, int needed, pmoq_msg_t* msg);

#ifdef __cplusplus
}
#endif

#endif /* PICOMOQ_H */
