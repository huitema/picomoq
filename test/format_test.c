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
int mpoq_msg_compare(pmoq_msg_format_test_case_t* test, pmoq_msg_t* msg)
{
    return -1;
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
    else if (mpoq_msg_compare(test, &msg) != 0){
        ret = -1;
    }
    return ret;   
}

int pmoq_msg_format_test()
{
    return -1;
}