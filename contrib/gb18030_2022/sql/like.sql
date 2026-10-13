-- LIKE/ILIKE tests for the GB18030 server encoding.
--
-- GB18030 is special for LIKE: the trailing byte of a multibyte character can
-- be an ASCII wildcard byte (0x5f '_' or 0x5c '\'), so matching must compare
-- and advance whole characters.  ILIKE must also fold case on a whole-character
-- basis: only single-byte ASCII is folded, never an ASCII-looking trailing byte
-- of a multibyte character.
--
-- Test strings are built with convert_from(..., 'GB18030') to keep this file
-- pure ASCII.
create database gb18030_like encoding='gb18030' LC_COLLATE='C' LC_CTYPE='C' TEMPLATE=template0;
\c gb18030_like
load 'gb18030_2022';
show server_encoding;
set client_encoding = 'UTF8';
show client_encoding;

-- Raw bytes and character lengths of the test data.
select length(convert_from('\x8140','GB18030')) as nchar_8140,
       length(convert_from('\x815f','GB18030')) as nchar_815f,
       length(convert_from('\x815c','GB18030')) as nchar_815c,
       length(convert_from('\x81308130','GB18030')) as nchar_4byte;
select encode(convert_to(convert_from('\x8140','GB18030'),'GB18030'),'hex') as h_8140,
       encode(convert_to(convert_from('\x815f','GB18030'),'GB18030'),'hex') as h_815f,
       encode(convert_to(convert_from('\x815c','GB18030'),'GB18030'),'hex') as h_815c;

-- LIKE: a trailing '_' byte inside a multibyte character is not a wildcard.
select convert_from('\x8140','GB18030') LIKE convert_from('\x815f','GB18030') as bad_wildcard_false;
select convert_from('\x8140','GB18030') NOT LIKE convert_from('\x815f','GB18030') as bad_wildcard_not_true;

-- LIKE: a trailing '\' byte inside a multibyte character is not an escape.
select convert_from('\x8140','GB18030') LIKE convert_from('\x815c','GB18030') as bad_escape_false;
select convert_from('\x8140','GB18030') NOT LIKE convert_from('\x815c','GB18030') as bad_escape_not_true;

-- LIKE: identical multibyte characters match, different ones don't.
select convert_from('\x8140','GB18030') LIKE convert_from('\x8140','GB18030') as same_true;
select convert_from('\x81308130','GB18030') LIKE convert_from('\x81308130','GB18030') as same4_true;
select convert_from('\x81308130','GB18030') LIKE convert_from('\x81308131','GB18030') as diff4_false;

-- LIKE: a real '_' after a multibyte character is a wildcard.
select convert_from('\x814041','GB18030') LIKE convert_from('\x81405f','GB18030') as real_us_true;
select convert_from('\x81404142','GB18030') LIKE convert_from('\x81405f','GB18030') as real_us_false;

-- LIKE: '%' scanning before a multibyte character with a trailing '_'.
select convert_from('\x8140','GB18030') LIKE convert_from('\x25815f','GB18030') as pct_bad_wildcard_false;
select convert_from('\x41815f','GB18030') LIKE convert_from('\x25815f','GB18030') as pct_match_true;

-- LIKE: escaped multibyte character with a trailing '\'.
select convert_from('\x815c','GB18030') LIKE convert_from('\x5c815c','GB18030') as esc_char_true;

-- ILIKE: ASCII case folding works...
select 'A' ILIKE 'a' as ascii_true;
select convert_from('\x814041','GB18030') ILIKE convert_from('\x814061','GB18030') as ascii_after_mb_true;
-- ...but the trailing byte of a multibyte character is not folded.
select convert_from('\x8141','GB18030') ILIKE convert_from('\x8161','GB18030') as no_fold_false;
select convert_from('\x8141','GB18030') NOT ILIKE convert_from('\x8161','GB18030') as no_fold_not_true;
select convert_from('\x8141','GB18030') ILIKE convert_from('\x8141','GB18030') as same_ilike_true;

-- ILIKE: wildcards with multibyte characters.
select convert_from('\x814041','GB18030') ILIKE convert_from('\x81405f','GB18030') as ilike_us_true;
select convert_from('\x8141','GB18030') ILIKE convert_from('\x258161','GB18030') as ilike_pct_no_fold_false;

-- LIKE with an explicit ESCAPE clause: the trailing bytes of multibyte
-- characters must not be treated as wildcards or as the escape character.
select convert_from('\x8140','GB18030') LIKE convert_from('\x815f','GB18030') ESCAPE '#' as esc_clause_bad_wildcard_false;
select convert_from('\x8140','GB18030') LIKE convert_from('\x815c','GB18030') ESCAPE '#' as esc_clause_bad_escape_false;

-- ESCAPE '#' makes a following '_' literal.
select '_' LIKE '#_' ESCAPE '#' as esc_literal_us_true;
select 'A' LIKE '#_' ESCAPE '#' as esc_literal_us_false;

-- ESCAPE '#' before a multibyte character escapes that whole character.
select convert_from('\x815f','GB18030') LIKE convert_from('\x23815f','GB18030') ESCAPE '#' as esc_mb_true;
select convert_from('\x8140','GB18030') LIKE convert_from('\x23815f','GB18030') ESCAPE '#' as esc_mb_false;

-- ESCAPE '' (no escape character): a '\' byte inside a multibyte character is
-- part of the character and must be left intact.
select convert_from('\x815c','GB18030') LIKE convert_from('\x815c','GB18030') ESCAPE '' as esc_none_true;
select convert_from('\x815c','GB18030') LIKE convert_from('\x5c815c','GB18030') ESCAPE '' as esc_none_false;

-- A multibyte ESCAPE character.
select convert_from('\x815f','GB18030') LIKE convert_from('\x8140815f','GB18030') ESCAPE convert_from('\x8140','GB18030') as esc_mb_char_true;
select convert_from('\x8140','GB18030') LIKE convert_from('\x8140815f','GB18030') ESCAPE convert_from('\x8140','GB18030') as esc_mb_char_false;

-- ILIKE with an explicit ESCAPE clause.
select convert_from('\x8141','GB18030') ILIKE convert_from('\x238161','GB18030') ESCAPE '#' as iesc_no_fold_false;
select convert_from('\x8141','GB18030') ILIKE convert_from('\x238141','GB18030') ESCAPE '#' as iesc_same_true;
select convert_from('\x814041','GB18030') ILIKE convert_from('\x23814041','GB18030') ESCAPE '#' as iesc_mb_true;

-- Real characters whose GB18030-2022 encodings end in a special byte:
--   乗 (U+4E57) = 0x81 0x5C   (trailing '\', the escape character)
--   乢 (U+4E62) = 0x81 0x5F   (trailing '_', the single-char wildcard)
--   丄 (U+4E04) = 0x81 0x41   (trailing 'A')
--   乤 (U+4E64) = 0x81 0x61   (trailing 'a')
select encode(convert_to('乗','GB18030'),'hex') as cheng_hex,
       encode(convert_to('乢','GB18030'),'hex') as gui_hex,
       encode(convert_to('丄','GB18030'),'hex') as shang_hex,
       encode(convert_to('乤','GB18030'),'hex') as xia_hex;
select '乗' = convert_from('\x815c','GB18030') as cheng_is_815c,
       '乢' = convert_from('\x815f','GB18030') as gui_is_815f,
       '丄' = convert_from('\x8141','GB18030') as shang_is_8141,
       '乤' = convert_from('\x8161','GB18030') as xia_is_8161;

-- LIKE with 乗: the trailing 0x5C must not act as an escape character.
select '乗' LIKE '乗' as cheng_like_true;
select '乗' LIKE '_' as cheng_us_true;
select '乗' LIKE '\乗' as cheng_esc_true;
select '乗' NOT LIKE '\乗' as cheng_esc_not_false;
select '乗' LIKE convert_from('\x8140','GB18030') as cheng_other_false;
select '乗' LIKE '%乗' as cheng_pct_cheng_true;
select '乗' LIKE '%乢' as cheng_pct_gui_false;

-- LIKE with 乢: the trailing 0x5F must not act as a wildcard.
select '乢' LIKE '乢' as gui_like_true;
select '乢' LIKE '_' as gui_us_true;
select '乢' LIKE convert_from('\x8140','GB18030') as gui_other_false;
select '乢' NOT LIKE convert_from('\x8140','GB18030') as gui_other_not_true;
select '乢' LIKE '%乗' as gui_pct_cheng_false;
select '乢' LIKE '%乢' as gui_pct_gui_true;

-- LIKE with 乗 and an explicit ESCAPE clause.
select '乗' LIKE '#乗' ESCAPE '#' as cheng_esc_clause_true;
select '乗' LIKE convert_from('\x23815c','GB18030') ESCAPE '#' as cheng_esc_clause_mb_true;
select '乗' LIKE convert_from('\x23815f','GB18030') ESCAPE '#' as cheng_esc_clause_wrong_false;

-- ILIKE with 乗 and 乢: they have no case, and their trailing bytes must not
-- be folded.
select '乗' ILIKE '乗' as cheng_ilike_true;
select '乗' ILIKE '\乗' as cheng_ilike_esc_true;
select '乗' ILIKE '#乗' ESCAPE '#' as cheng_ilike_esc_clause_true;
select '乢' ILIKE '乢' as gui_ilike_true;
select '乢' ILIKE convert_from('\x8140','GB18030') as gui_ilike_other_false;
-- 丄 (0x81 0x41) vs 乤 (0x81 0x61): the trailing 'A'/'a' must NOT be folded.
select '丄' ILIKE '乤' as shang_xia_no_fold_false;
select '丄' NOT ILIKE '乤' as shang_xia_no_fold_not_true;
-- But ASCII letters next to a multibyte character still fold under ILIKE.
select '乗A' ILIKE '乗a' as cheng_ascii_fold_true;
select '丄A' ILIKE '丄a' as shang_ascii_fold_true;

\c postgres
drop database gb18030_like;