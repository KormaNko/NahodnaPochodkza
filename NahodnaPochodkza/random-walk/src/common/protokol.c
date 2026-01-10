#define _POSIX_C_SOURCE 200809L

#include "string.h"
#include "protokol.h"
spravy protocol_parse_line(const char *riadok){

if(strcmp(riadok,"HELLO\n") == 0) {
        return PROTO_CMD_HELLO;
        }else if(strcmp(riadok,"QUIT\n") == 0) {
            return PROTO_CMD_QUIT;

            }else if(strcmp(riadok,"STOP\n") == 0) {
            return PROTO_CMD_STOP;

        }else if(strcmp(riadok,"MODE INTERACTIVE\n") == 0)
         {
            return PROTO_CMD_MODE_INTERACTIVE;
         }else if(strcmp(riadok,"MODE SUMMARY\n") == 0)
         {
           return PROTO_CMD_MODE_SUMMARY;
         }else if(strcmp(riadok,"GET STATE\n") == 0)
         {
            return PROTO_CMD_GET_STATE;
            }else{ 
            return PROTO_CMD_UNKNOWN;
         }
}

const char * protocol_cmd_name(spravy cmd) {
    switch (cmd)
    {
    case PROTO_CMD_HELLO:
        return "HELLO\n";
        break;
    case PROTO_CMD_QUIT:
        return "QUIT\n";
        break;   
    case PROTO_CMD_STOP:
        return "STOP\n";
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
