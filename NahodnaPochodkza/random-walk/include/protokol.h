#ifndef PROTOCOL_H
#define PROTOCOL_H
typedef enum {
PROTO_CMD_HELLO,
PROTO_CMD_QUIT,
PROTO_CMD_STOP,
PROTO_CMD_MODE_INTERACTIVE,
PROTO_CMD_MODE_SUMMARY,
PROTO_CMD_GET_STATE,
PROTO_CMD_UNKNOWN
}spravy;

spravy protocol_parse_line(const char *riadok);

const char * protocol_cmd_name(spravy cmd);

#endif
