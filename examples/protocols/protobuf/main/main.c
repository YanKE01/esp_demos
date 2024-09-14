#include <stdio.h>
#include <pb_encode.h>
#include <pb_decode.h>
#include "simple.pb.h"
#include "string.h"

void app_main(void)
{
    uint8_t buffer[128];
    size_t message_length;
    bool status = false;

    // encode
    SimpleMessage message = SimpleMessage_init_default;
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));

    message.lucky_number = 7;
    status = pb_encode(&stream, SimpleMessage_fields, &message);
    message_length = stream.bytes_written;

    if (!status) {
        printf("Encoding failed: %s\n", PB_GET_ERROR(&stream));
        return;
    }

    // decode
    SimpleMessage message_decode = SimpleMessage_init_default;
    pb_istream_t stream_decode = pb_istream_from_buffer(buffer, message_length);
    status = pb_decode(&stream_decode, SimpleMessage_fields, &message_decode);

    if (!status) {
        printf("Decoding failed: %s\n", PB_GET_ERROR(&stream_decode));
        return;
    }
    printf("Your lucky number was %d!\n", (int)message_decode.lucky_number);
}
