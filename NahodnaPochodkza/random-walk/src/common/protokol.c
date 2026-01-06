#define _POSIX_C_SOURCE 200809L

#include "string.h"
#include "protokol.h"


const char * protocol_cmd_name(spravy cmd) {
    switch (cmd)
    {
    case PROTO_CMD_HELLO:
        return "HELLO\n";
        break;
    case PROTO_CMD_QUIT:
        return "QUIT\n";
        break;    
    case PROTO_CMD_MODE_INTERACTIVE:
        return "MODE INTERACTIVE\n";
        break;
    case PROTO_CMD_MODE_SUMMARY:
        return "MODE SUMMARY\n";
        break;
    case PROTO_CMD_GET_STATE:
        return "GET STATE\n";
        break;        
    default:
        return "UNKNOWN\n";
        break;
    }
}

spravy protocol_parse_line(const char *line)
{
    if (strcmp(line, "HELLO\n") == 0)
        return PROTO_CMD_HELLO;

    if (strcmp(line, "QUIT\n") == 0)
        return PROTO_CMD_QUIT;

    if (strcmp(line, "MODE INTERACTIVE\n") == 0)
        return PROTO_CMD_MODE_INTERACTIVE;

    if (strcmp(line, "MODE SUMMARY\n") == 0)
        return PROTO_CMD_MODE_SUMMARY;

    if (strcmp(line, "GET STATE\n") == 0)
        return PROTO_CMD_GET_STATE;

   if (strncmp(line, "MODE OBSTACLES ", 15) == 0)
    return PROTO_CMD_MODE_OBSTACLES;


    return PROTO_CMD_UNKNOWN;
}
