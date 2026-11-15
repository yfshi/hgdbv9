/*-------------------------------------------------------------------------
 *
 * hg_rotate_funcs.sql
 *    TECC ALTER KEK 轮换所需的元数据写回函数（SECURITY DEFINER）
 *
 * ReqID:TECC4.0 - Bug 9255
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * 为什么单独一个文件：
 *   本文件用 plpgsql 编写，而 hg_metadata_views.sql 在 initdb 里早于
 *   load_plpgsql() 执行（后者此时语言尚未创建），因此只能放到
 *   load_plpgsql() 之后再跑。
 *
 * 为什么需要这些函数：
 *   ALTER KEK 在前端 libpq 完成 DEK 的"旧 KEK 解包 → 新 KEK 重新封装"，
 *   结果需写回 hg_dek_args.encrypted_value；非对称 KEK 还需写回
 *   hg_kek_args 的 PUBLIC_KEY。此前前端直接在用户会话里发
 *       UPDATE hg_dek_args SET value = convert_to('<hex>', 'UTF8') WHERE ...
 *   但这两张表只有 SELECT 是 PUBLIC，UPDATE 不开放给普通用户，于是即便用户
 *   已被 GRANT USAGE ON KEK，轮换仍以
 *       ERROR: permission denied for table hg_dek_args
 *   失败 —— 与"USAGE 即代表持有该 KEK 的使用权"的权限模型自相矛盾。
 *   这里提供 SECURITY DEFINER 的受控写入口：以定义者（initdb 属主）权限写表，
 *   同时按 session_user 做与 CREATE DEK 一致的 USAGE 校验。
 *
 * 注意：has_kek_privilege(oid,text) 取的是 current_user —— 在 SECURITY DEFINER
 *   下即定义者 —— 因此必须显式传 session_user，否则校验等于没做。
 *   用 session_user（而非 current_user）是安全的：session_user 不受 SET ROLE
 *   影响，而能被 SET ROLE 成的角色必然是调用者的成员（或调用者本身是超级
 *   用户），角色成员关系本来就会被 hg_kek_aclcheck 展开，故不会误放也不会误拦。
 *
 * 注意：initdb 以 `postgres --single -j` 执行本文件，命令以"分号 + 空行"结束
 *   （见 tcop/postgres.c InteractiveBackend）。因此函数体内任何以分号结尾的行
 *   后面都不能跟空行，否则语句会在函数体中间被截断，报
 *   "unterminated dollar-quoted string"。本文件内函数体一律不留空行。
 *
 * 注意：本文件在 load_plpgsql() 之后执行，此时会话 search_path 已被前面几个
 *   initdb 步骤改成 information_schema。必须显式切回 pg_catalog，否则函数会
 *   建到 information_schema 里 —— 用户默认 search_path 看不到它，调用直接报
 *   "function ... does not exist"。hg_kek_info 等视图同样位于 pg_catalog。
 *
 *-------------------------------------------------------------------------
 */

SET search_path = pg_catalog;


/*
 * ALTER KEK 轮换写回 DEK 参数
 *
 * 白名单：只允许轮换会改写的三类参数。该函数对 PUBLIC 开放执行，不能成为
 * 改写 key_algo / dek_config 等语义性元数据的后门。
 */
CREATE FUNCTION hg_kek_rotate_dek_arg(p_dek_oid oid,
                                      p_arg_key text,
                                      p_new_value_hex text)
RETURNS void
LANGUAGE plpgsql
SECURITY DEFINER
SET search_path = pg_catalog
AS $$
DECLARE
    v_kek_id   oid;
    v_kek_name text;
    v_arg_key  text;
BEGIN
    IF p_dek_oid IS NULL OR p_arg_key IS NULL OR p_new_value_hex IS NULL THEN
        RAISE EXCEPTION 'hg_kek_rotate_dek_arg: NULL argument';
    END IF;
    v_arg_key := lower(p_arg_key);
    IF v_arg_key NOT IN ('encrypted_value', 'he_ec_privkey', 'he_eg_privkey') THEN
        RAISE EXCEPTION 'hg_kek_rotate_dek_arg: arg "%" is not rotatable', p_arg_key;
    END IF;
    SELECT d.kek_id INTO v_kek_id FROM hg_dek d WHERE d.oid = p_dek_oid;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'data encryption key with OID % does not exist', p_dek_oid;
    END IF;
    /* 校验对象必须是发起调用的会话用户（session_user），不能是 SECURITY DEFINER
     * 的定义者 —— has_kek_privilege(oid,text) 取 current_user，在定义者上下文里
     * 就等于没校验。详见文件头注释。 */
    IF NOT has_kek_privilege(session_user, v_kek_id, 'USAGE') THEN
        SELECT k.kek_name INTO v_kek_name FROM hg_kek k WHERE k.oid = v_kek_id;
        RAISE EXCEPTION 'permission denied for key encryption key %', v_kek_name;
    END IF;
    UPDATE hg_dek_args SET value = convert_to(p_new_value_hex, 'UTF8')
     WHERE dek_id = p_dek_oid AND lower(key::text) = v_arg_key;
END;
$$;

/* 函数默认即对 PUBLIC 授予 EXECUTE；此处显式写出以便审计。
 * 真正的权限判定由函数体内的 has_kek_privilege(session_user, ...) 完成。 */
GRANT EXECUTE ON FUNCTION hg_kek_rotate_dek_arg(oid, text, text) TO PUBLIC;

/*
 * ALTER KEK 非对称轮换写回 KEK 自身参数
 *
 * 非对称（sm2）KEK 轮换后须把 hg_kek_args 里的 PUBLIC_KEY 换成新公钥，否则
 * 之后新建 HE DEK 时前端仍按旧公钥做信封加密、运行时用新私钥解密而失败。
 * 存储格式与 hg_dek_args.encrypted_value 不同：这里是 byteain 解码后的二进制值
 * （65 字节解压点），调用方直接传 bytea。
 */
CREATE FUNCTION hg_kek_rotate_kek_arg(p_kek_oid oid,
                                      p_arg_key text,
                                      p_new_value bytea)
RETURNS void
LANGUAGE plpgsql
SECURITY DEFINER
SET search_path = pg_catalog
AS $$
DECLARE
    v_kek_name text;
    v_arg_key  text;
BEGIN
    IF p_kek_oid IS NULL OR p_arg_key IS NULL OR p_new_value IS NULL THEN
        RAISE EXCEPTION 'hg_kek_rotate_kek_arg: NULL argument';
    END IF;
    /* 白名单：轮换只改写 KEK 的公钥参数。 */
    v_arg_key := lower(p_arg_key);
    IF v_arg_key <> 'public_key' THEN
        RAISE EXCEPTION 'hg_kek_rotate_kek_arg: arg "%" is not rotatable', p_arg_key;
    END IF;
    IF NOT EXISTS (SELECT 1 FROM hg_kek k WHERE k.oid = p_kek_oid) THEN
        RAISE EXCEPTION 'key encryption key with OID % does not exist', p_kek_oid;
    END IF;
    /* 同上：校验调用者，而非 SECURITY DEFINER 的定义者 */
    IF NOT has_kek_privilege(session_user, p_kek_oid, 'USAGE') THEN
        SELECT k.kek_name INTO v_kek_name FROM hg_kek k WHERE k.oid = p_kek_oid;
        RAISE EXCEPTION 'permission denied for key encryption key %', v_kek_name;
    END IF;
    UPDATE hg_kek_args SET value = p_new_value
     WHERE kek_id = p_kek_oid
       AND function_name = 'kek_config'
       AND lower(key::text) = v_arg_key;
END;
$$;

GRANT EXECUTE ON FUNCTION hg_kek_rotate_kek_arg(oid, text, bytea) TO PUBLIC;
