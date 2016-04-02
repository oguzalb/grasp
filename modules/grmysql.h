#ifndef GR_MYSQL
#define GR_MYSQL
#include "../types/object.h"
#include <my_global.h>
#include <mysql.h>

class MysqlConnection: public Object {
    public:
    MysqlConnection();
    MYSQL *con;
};

void init_grmysql();

#endif
