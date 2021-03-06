#include "grmysql.h"
#include "../vm.h"

extern Class *str_type;
extern Object *none;
extern Class *class_type;
Class *mysql_connection_type;
extern Class *exception_type;
extern std::unordered_map<string, Object*> *builtins;
extern Object *none;

MysqlConnection::MysqlConnection() {
    this->type = mysql_connection_type;
}

void grmysql_open() {
    // TODO need to decide what to do with type check failures, in an easier way
    String *password = (String *)POP_TYPE(str_type);
    if (password == NULL)
        {POP();POP();POP();POP();POP();PUSH(none);return;}
    String *charset = (String *)POP_TYPE(str_type);
    if (charset == NULL)
        {POP();POP();POP();POP();PUSH(none);return;}
    String *db_name = (String *)POP_TYPE(str_type);
    if (db_name == NULL)
        {POP();POP();POP();PUSH(none);return;}
    String *user = (String *)POP_TYPE(str_type);
    if (user == NULL)
        {POP();POP();PUSH(none);return;}
    String *host = (String *)POP_TYPE(str_type);
    if (host == NULL)
        {POP();PUSH(none);return;}
    MysqlConnection *con = (MysqlConnection *)POP_TYPE(mysql_connection_type);
    if (con == NULL)
        return;
    if (mysql_real_connect(con->con, host->sval.c_str(), user->sval.c_str(), password->sval.c_str(), 
            db_name->sval.c_str(), 0, NULL, 0) == NULL) {
        fprintf(stderr, "%s\n", mysql_error(con->con));
        mysql_close(con->con);
        newerror_internal("Couldn't connect to mysql", exception_type);
        return;
    }

    if (mysql_set_character_set(con->con, charset->sval.c_str())) {
        fprintf(stderr, "%s\n", mysql_error(con->con));
        mysql_close(con->con);
        newerror_internal("New client character couldn't be set\n",
               exception_type);
    }
    PUSH(none);
}

void grmysql_query() {
    String *query = (String *)POP_TYPE(str_type);
    if (query == NULL)
        {POP();return;}
    MysqlConnection *con = (MysqlConnection *)POP_TYPE(mysql_connection_type);
    if (con == NULL)
        return;
    if (mysql_query(con->con, query->sval.c_str())) {
        fprintf(stderr, "%s\n", mysql_error(con->con));
        newerror_internal("Query error", exception_type);
        mysql_close(con->con);
        return;
    }
    MYSQL_RES *result = mysql_store_result(con->con);
  
    if (result == NULL) {
        fprintf(stderr, "%s\n", mysql_error(con->con));
        newerror_internal("Result couldn't be fetched", exception_type);
        mysql_close(con->con);
        return;
    }

    int num_fields = mysql_num_fields(result);

    MYSQL_ROW row;
    List *results_obj = new List();
    while ((row = mysql_fetch_row(result))) {
        List *row_obj = new List();
        results_obj->list->push_back(row_obj);
        for(int i = 0; i < num_fields; i++) { 
            row_obj->list->push_back(row[i] ? new String(row[i]) : none);
        } 
    }
  
    mysql_free_result(result);
    PUSH(results_obj);
}

void grmysql_new() {
    if (POP_TYPE(class_type) == NULL)
        return;
    PUSH(new MysqlConnection());
}

void grmysql_init() {
    MysqlConnection *con = (MysqlConnection *)POP_TYPE(mysql_connection_type);
    if (con == NULL)
        return;
    con->con = mysql_init(NULL);

    if (con->con == NULL) {
        fprintf(stderr, "%s\n", mysql_error(con->con));
        newerror_internal("Couldn't init mysql", exception_type);
        return;
    }
    PUSH(none);
}

void grmysql_close() {
    MysqlConnection *con = (MysqlConnection *)POP_TYPE(mysql_connection_type);
    if (con == NULL)
        return;
    mysql_close(con->con);
    PUSH(none);
}

void init_grmysql() {
    Module *grmysql = new Module(NULL);
    mysql_connection_type = new Class("MysqlConnection", grmysql_new, 1);
    mysql_connection_type->setmethod("__init__", grmysql_init, 1);
    mysql_connection_type->setmethod("close", grmysql_close, 1);
    mysql_connection_type->setmethod("open", grmysql_open, 6);
    mysql_connection_type->setmethod("query", grmysql_query, 2);
    grmysql->setfield("MysqlConnection", mysql_connection_type);
    (*builtins)["mysql"] = grmysql;
}
