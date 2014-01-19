#include "util.h"
#include <hiredis.h>

//TODO: make abstract
class ExternalKeyValueDB 
{
private:
    redisContext *c;
public:
    ExternalKeyValueDB() {
    }
    bool ConnectTo(const char *hostname, int port) {
        struct timeval timeout = { 1, 500000 }; // 1.5 seconds
        c = redisConnectWithTimeout(hostname, port, timeout);
        if (c == NULL || c->err) {
            if (c) {
                LogPrintf("ExternalKeyValueDB: Connection error: %s\n", c->errstr);
                redisFree(c);
            } else {
                LogPrintf("ExternalKeyValueDB: Connection error: can't allocate redis context\n");
            }
            return false;
        }
        return true;
    }

    bool IsSetup() {
        return c!=NULL;
    }

    redisReply *Command(const char *format, ...) {
        redisReply *ret;
        va_list args;
        va_start(args,format);
        ret = (redisReply *)redisvCommand(c,format,args);
        va_end(args);
        return ret;
    }
};
