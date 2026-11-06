/* 前端解析器节点类型定义 */
#ifndef FE_PARSENODES_H
#define FE_PARSENODES_H

#include "simple_types.h"

/* 基本节点类型 */
typedef struct Node {
    int type;
} Node;

/* 列表类型 */
typedef struct List {
    int length;
    void *head;
    void *tail;
    struct List *next;
} List;

/* 值类型 */
typedef struct Value {
    int type;
    union {
        int ival;
        char *str;
        double dval;
    } val;
} Value;

/* 创建主密钥语句 */
typedef struct CreateMasterKeyStmt {
    int type;
    List *setting_name;
    List *master_key_params;
} CreateMasterKeyStmt;

/* 创建列加密密钥语句 */
typedef struct CreateColumnEncryptionKeyStmt {
    int type;
    List *setting_name;
    List *column_key_params;
} CreateColumnEncryptionKeyStmt;

/* 删除主密钥语句 */
typedef struct DropMasterKeyStmt {
    int type;
    List *setting_name;
} DropMasterKeyStmt;

/* 删除列加密密钥语句 */
typedef struct DropColumnEncryptionKeyStmt {
    int type;
    List *setting_name;
} DropColumnEncryptionKeyStmt;

/* 客户端逻辑全局参数 */
typedef struct ClientLogicGlobalParam {
    int type;
    int key;
    char *value;
    int len;
} ClientLogicGlobalParam;

/* 客户端逻辑列参数 */
typedef struct ClientLogicColumnParam {
    int type;
    int key;
    char *value;
    int len;
} ClientLogicColumnParam;

/* 客户端逻辑全局设置 */
typedef struct CreateClientLogicGlobal {
    int type;
    List *global_key_name;
    List *global_setting_params;
} CreateClientLogicGlobal;

/* 客户端逻辑列设置 */
typedef struct CreateClientLogicColumn {
    int type;
    List *column_key_name;
    List *column_setting_params;
} CreateClientLogicColumn;

/* 枚举类型 */
typedef enum ClientLogicGlobalProperty {
    CLIENT_GLOBAL_FUNCTION = 1,
    CLIENT_GLOBAL_KEY_STORE,
    CLIENT_GLOBAL_KEY_PATH,
    CLIENT_GLOBAL_ALGORITHM
} ClientLogicGlobalProperty;

typedef enum ClientLogicColumnProperty {
    COLUMN_COLUMN_FUNCTION = 1,
    COLUMN_COLUMN_KEK,
    COLUMN_COLUMN_ALGORITHM,
    COLUMN_COLUMN_ENCRYPTED_VALUE
} ClientLogicColumnProperty;

/* 其他语句类型 - 简化定义 */
typedef struct SelectStmt {
    int type;
    /* 简化 */
} SelectStmt;

typedef struct InsertStmt {
    int type;
    /* 简化 */
} InsertStmt;

typedef struct UpdateStmt {
    int type;
    /* 简化 */
} UpdateStmt;

typedef struct DropStmt {
    int type;
    int removeType;
    List *objects;
    int behavior;
    bool missing_ok;
    bool concurrent;
    List *arguments;
} DropStmt;

/* 对象类型枚举 */
typedef enum ObjectType {
    OBJECT_TABLE = 1,
    OBJECT_INDEX,
    OBJECT_SEQUENCE,
    OBJECT_VIEW,
    OBJECT_GLOBAL_SETTING,  /* KEK */
    OBJECT_COLUMN_SETTING,  /* DEK */
    /* 其他类型... */
} ObjectType;

/* 删除行为枚举 */
typedef enum DropBehavior {
    DROP_RESTRICT,
    DROP_CASCADE
} DropBehavior;

#endif /* FE_PARSENODES_H */