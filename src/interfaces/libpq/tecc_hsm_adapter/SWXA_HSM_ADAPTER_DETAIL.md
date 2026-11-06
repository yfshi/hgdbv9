# 三未信安 (SWXA) HSM 适配器详细设计

## 1. 适配器实现

### 1.1 init() — 初始化和配置文件校验

```c
static HsmErrorCode
swxa_init(HsmDevice **device, const char *config_file)
{
    HsmDevice *dev;
    const char *pgdata;
    char ini_path[MAXPGPATH];
    struct stat st;

    if (!device)
        return HSM_ERROR_INVALID_PARAM;

    /* config_file 传入的是 KEY_PATH（如 "hsm:id=1"），非 ini 路径 */
    /* ini 文件路径自动从 $PGDATA/swsds.ini 解析 */
    (void)config_file;

    pgdata = getenv("PGDATA");
    if (pgdata)
        snprintf(ini_path, sizeof(ini_path), "%s/swsds.ini", pgdata);
    else
        snprintf(ini_path, sizeof(ini_path), "swsds.ini");

    /* 校验配置文件是否存在 */
    if (stat(ini_path, &st) != 0)
    {
        fprintf(stderr, "ERROR: swsds.ini not found at %s\n", ini_path);
        fprintf(stderr, "HINT: Please copy swsds.ini to $PGDATA/ directory\n");
        return HSM_ERROR_INIT_FAILED;
    }

    /* 确保是普通文件 */
    if (!S_ISREG(st.st_mode))
    {
        fprintf(stderr, "ERROR: %s is not a regular file\n", ini_path);
        return HSM_ERROR_INIT_FAILED;
    }

    dev = (HsmDevice *) calloc(1, sizeof(HsmDevice));
    if (!dev)
        return HSM_ERROR_INTERNAL;

    dev->config_file = strdup(ini_path);
    dev->device_handle = NULL;
    dev->session_handle = NULL;
    dev->connected = false;

    *device = dev;
    return HSM_SUCCESS;
}
```

**校验说明**：
- `stat()` 检查 `swsds.ini` 是否存在
- `S_ISREG()` 确保不是目录或特殊文件
- ini 文件内容的有效性（IP/端口/密码）由 `SDF_OpenDeviceWithPathAndName` 在 connect 时验证
- `config_file` 参数（即 KEY_PATH 值）在 init 阶段不使用，由调用方在 init 之前通过 `parse_hsm_key_path` 解析出 key_index

### 1.2 connect() — 设备连接与会话打开

```c
static HsmErrorCode
swxa_connect(HsmDevice *device)
{
    int ret;
    void *hDevice = NULL;
    void *hSession = NULL;

    if (!device || !device->config_file)
        return HSM_ERROR_INVALID_PARAM;

    if (device->connected)
        return HSM_SUCCESS;

    /* 通过 ini 配置文件打开设备 */
    ret = SDF_OpenDeviceWithPathAndName(device->config_file, &hDevice);
    if (ret != SDR_OK)
    {
        fprintf(stderr, "ERROR: Failed to open password machine: 0x%08x\n", ret);
        fprintf(stderr, "HINT: Check network connectivity and swsds.ini configuration\n");
        return sdf_error_to_hsm_error(ret);
    }

    /* 打开会话 */
    ret = SDF_OpenSession(hDevice, &hSession);
    if (ret != SDR_OK)
    {
        fprintf(stderr, "ERROR: Failed to open session: 0x%08x\n", ret);
        SDF_CloseDevice(hDevice);
        return sdf_error_to_hsm_error(ret);
    }

    device->device_handle = hDevice;
    device->session_handle = hSession;
    device->connected = true;

    return HSM_SUCCESS;
}
```

**注意**：`SDF_OpenDeviceWithPathAndName` 声明为：
```c
SGD_RV SDF_OpenDeviceWithPathAndName(char *pcCfgfile, void **phDeviceHandle);
```
参数类型为 `char*` 和 `void**`，不是 `SGD_HANDLE*`。传给 `SDF_OpenSession` 时 `hDevice` 可直接作为 `SGD_HANDLE` 使用。

### 1.3 disconnect() — 会话关闭和设备断开

```c
static HsmErrorCode
swxa_disconnect(HsmDevice *device)
{
    if (!device)
        return HSM_ERROR_INVALID_PARAM;

    if (!device->connected)
        return HSM_SUCCESS;

    if (device->session_handle)
    {
        SDF_CloseSession(device->session_handle);
        device->session_handle = NULL;
    }

    if (device->device_handle)
    {
        SDF_CloseDevice(device->device_handle);
        device->device_handle = NULL;
    }

    device->connected = false;
    return HSM_SUCCESS;
}
```

### 1.4 cleanup() — 资源释放

```c
static void
swxa_cleanup(HsmDevice *device)
{
    if (!device)
        return;

    if (device->connected)
        swxa_disconnect(device);

    if (device->config_file)
        free(device->config_file);

    free(device);
}
```

### 1.5 check_key_exists() — 密钥存在性检查

```c
static HsmErrorCode
swxa_check_key_exists(HsmDevice *device, unsigned int key_index)
{
    int ret;
    unsigned int status = 0;

    if (!device || !device->connected)
        return HSM_ERROR_CONNECT_FAILED;

    /* 使用 GetKeyStatus 扩展接口，支持按索引查询 */
    ret = GetKeyStatus(device->session_handle,
                       SGD_SMS4,       /* 对称密钥类型 */
                       key_index,      /* 密钥索引 */
                       &status);

    if (ret != SDR_OK)
    {
        fprintf(stderr, "ERROR: GetKeyStatus failed for index %u: 0x%08x\n",
                key_index, ret);
        return sdf_error_to_hsm_error(ret);
    }

    if (status != 1)
    {
        fprintf(stderr, "ERROR: Key at index %u does not exist in password machine\n",
                key_index);
        fprintf(stderr, "HINT: Please inject the key into the password machine first\n");
        return HSM_ERROR_KEY_NOT_FOUND;
    }

    return HSM_SUCCESS;
}
```

**备选方案**：使用 `SDF_GetSymmKeyHandle` 替代 `GetKeyStatus`：

```c
/* 备选：SDF_GetSymmKeyHandle 返回成功即表示密钥存在 */
SGD_HANDLE hKeyHandle = 0;
ret = SDF_GetSymmKeyHandle(hSession, key_index, &hKeyHandle);
if (ret == SDR_OK)
{
    SDF_DestroyKey(hSession, hKeyHandle);
    return HSM_SUCCESS;
}
return HSM_ERROR_KEY_NOT_FOUND;
```

## 2. fe-swxa-adapter.h 接口定义

```c
#ifndef FE_SWXA_ADAPTER_H
#define FE_SWXA_ADAPTER_H

#include "fe-hsm-adapter.h"

/* 获取三未信安 SWXA 适配器实例 */
extern HsmAdapterInterface *get_swxa_adapter(void);

#endif /* FE_SWXA_ADAPTER_H */
```

## 3. 适配器注册

### 3.1 fe-hsm-adapter.h 新增声明

```c
/* 三未信安 SWXA 适配器 */
extern HsmAdapterInterface *get_swxa_adapter(void);
```

### 3.2 fe-hsm-adapter.c 新增适配器实例

```c
/* Static adapter instances */
static HsmAdapterInterface tzhsm_adapter = {0};
static HsmAdapterInterface mock_hsm_adapter = {0};
static HsmAdapterInterface swxa_adapter = {0};  /* 新增 */

/* 已有的 get_tzhsm_adapter / get_mock_hsm_adapter ... */

HsmAdapterInterface *
get_swxa_adapter(void)
{
    if (swxa_adapter.init == NULL)
    {
        swxa_adapter.init = swxa_init;
        swxa_adapter.connect = swxa_connect;
        swxa_adapter.disconnect = swxa_disconnect;
        swxa_adapter.cleanup = swxa_cleanup;
        swxa_adapter.check_key_exists = swxa_check_key_exists;
        swxa_adapter.error_string = swxa_error_string;
        swxa_adapter.is_connected = swxa_is_connected;
        /* 阶段二/三再实现 */
        swxa_adapter.sm4_encrypt = NULL;
        swxa_adapter.sm4_decrypt = NULL;
        swxa_adapter.encrypt = NULL;
        swxa_adapter.decrypt = NULL;
    }
    return &swxa_adapter;
}
```

### 3.3 适配器查找函数（统一入口）

```c
HsmAdapterInterface *get_hsm_adapter(const char *kms_type)
{
    if (strcmp(kms_type, "hsmkms") == 0)
        return get_tzhsm_adapter();
    else if (strcmp(kms_type, "swxakms") == 0)
        return get_swxa_adapter();
    else
        return NULL;
}
```

## 4. CREATE KEK 处理器（swxakms 分支）

### 4.1 validate_create_kek_options 扩展

`create_kek_processor.c` 中的 `validate_create_kek_options` 函数当前支持 `localkms` 和 `hsmkms` 两种 KEY_STORE，需要允许 `swxakms` 通过校验：

```c
/* 在选项校验函数中，扩展 KEY_STORE 允许值 */
if (strcmp(str_value, "localkms") != 0 &&
    strcmp(str_value, "hsmkms") != 0 &&
    strcmp(str_value, "swxakms") != 0)   /* 新增 */
{
    fprintf(stderr, "ERROR: Invalid KEY_STORE type: %s\n", str_value);
    fprintf(stderr, "Supported types: 'localkms', 'hsmkms', 'swxakms'\n");
    return -1;
}
```

### 4.2 tecc_create_kek_processor_run 新增分支

```c
/* 在 tecc_create_kek_processor_run 中 */
if (strcmp(key_store, "hsmkms") == 0)
{
    result = process_hsm_kek(create_kek_stmt->kek_name, key_path, kms_args);
    ...
}
else if (strcmp(key_store, "swxakms") == 0)  /* 新增分支 */
{
    result = process_swxa_kek(create_kek_stmt->kek_name, key_path, kms_args);
    ...
}
else if (strcmp(key_store, "localkms") == 0)
{
    ...
}
```

### 4.3 process_swxa_kek 实现

`swxakms` 与 `hsmkms` 处理流程相似，都是将 key_path 传给适配器检查密钥存在性。区别在于使用 `get_swxa_adapter()` 而非 `get_tzhsm_adapter()`。

可直接复用现有的 `process_hsm_kek()` 函数，因为该函数调用 `setup_hsm_kek()`，我们需要让 `setup_hsm_kek()` 接收 adapter 类型参数，或新增 `process_swxa_kek`。

推荐方案：新增 `process_swxa_kek()` 函数，与 `process_hsm_kek()` 结构相同，但使用 `get_swxa_adapter()`。

## 5. 错误处理

### 5.1 配置文件错误

| 场景 | 检测位置 | 错误信息 |
|---|---|---|
| `$PGDATA` 未设置且当前目录无 swsds.ini | init() | `swsds.ini not found at <path>. Please copy swsds.ini to $PGDATA/ directory` |
| `$PGDATA/swsds.ini` 不存在 | init() | 同上 |
| swsds.ini 不是普通文件 | init() | `<path> is not a regular file` |
| 密码机网络不可达 | connect() | `Failed to open password machine: 0x<err>. Check network connectivity and swsds.ini configuration` |
| 密码机认证失败 | connect() | 同上（SDF 返回相应错误码） |

### 5.2 密钥操作错误

| 场景 | 检测位置 | 错误信息 |
|---|---|---|
| KEY_PATH 格式非法 | validate 阶段 | `Invalid KEY_PATH format: <val>. Expected format: hsm:id=<index>` |
| 密钥索引超出范围 | check_key_exists() | `GetKeyStatus failed for index <N>: 0x<err>` |
| 密钥不存在 | check_key_exists() | `Key at index <N> does not exist. Please inject the key into the password machine first` |

### 5.3 SDF 错误码映射

```c
static HsmErrorCode
sdf_error_to_hsm_error(int sdf_error)
{
    switch (sdf_error)
    {
        case SDR_OK:            return HSM_SUCCESS;
        case SDR_OPENDEVICE:    return HSM_ERROR_CONNECT_FAILED;
        case SDR_OPENSESSION:   return HSM_ERROR_CONNECT_FAILED;
        case SDR_COMMFAIL:      return HSM_ERROR_CONNECT_FAILED;
        case SDR_HARDFAIL:      return HSM_ERROR_INIT_FAILED;
        case SDR_NOTSUPPORT:    return HSM_ERROR_NOT_SUPPORTED;
        case SDR_KEYNOTEXIST:   return HSM_ERROR_KEY_NOT_FOUND;
        case SDR_PARDENY:       return HSM_ERROR_KEY_ACCESS_DENIED;
        default:                return HSM_ERROR_OPERATION_FAILED;
    }
}
```

注意 `GetKeyStatus` 的返回值：返回 `SDR_OK` 表示查询成功（不代表密钥存在），密钥是否存在由 `status` 输出参数判断。

## 6. KEY_PATH 解析

复用 `fe-hsm-adapter.c` 中的 `parse_hsm_key_path()` 函数，无需修改：

```c
bool
parse_hsm_key_path(const char *key_path, unsigned int *key_index)
{
    const char *prefix = "hsm:id=";
    size_t prefix_len = strlen(prefix);
    char *endptr;
    long index;

    if (!key_path || !key_index)
        return false;

    if (strncmp(key_path, prefix, prefix_len) != 0)
        return false;

    errno = 0;
    index = strtol(key_path + prefix_len, &endptr, 10);
    if (errno != 0 || endptr == key_path + prefix_len || *endptr != '\0')
        return false;
    if (index < 1 || index > 65535)
        return false;

    *key_index = (unsigned int)index;
    return true;
}
```

## 7. 注意事项

1. **密钥预置**：KEK 需要在密码机中预先创建，适配器只检查存在性，不生成 KEK
2. **ini 内容不校验**：`swsds.ini` 的 IP/端口/密码有效性由 `SDF_OpenDeviceWithPathAndName` 在连接时验证，适配器只检查文件存在性
3. **并发控制**：每个操作独立打开/关闭设备和会话，后续可优化连接池
4. **线程安全**：SDF API 在多线程环境下需注意 `SessionHandleManage=0` 配置
5. **头文件路径**：`swsds.h` 放在 `src/include/hsm/`，无需新增编译参数
6. **阶段实现**：阶段一只实现 init/connect/disconnect/cleanup/check_key_exists，encrypt/decrypt 在阶段二/三实现
