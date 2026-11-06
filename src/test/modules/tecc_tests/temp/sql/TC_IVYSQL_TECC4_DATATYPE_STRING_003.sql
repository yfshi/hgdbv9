-- 清理
DROP TABLE IF EXISTS tb_bpchar CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
create table tb_bpchar(id SERIAL PRIMARY KEY, short_bpchar bpchar_enc encrypted with (dek = dek_t1), long_bpchar bpchar_enc encrypted with (dek = dek_t1), empty_bpchar bpchar_enc encrypted with (dek = dek_t1), null_bpchar bpchar_enc encrypted with (dek = dek_t1), special_chars_bpchar bpchar_enc encrypted with (dek = dek_t1), unicode_bpchar bpchar_enc encrypted with (dek = dek_t1), default_bpchar bpchar_enc encrypted with (dek = dek_t1) default 'defaultvalue', created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);

INSERT INTO tb_bpchar (short_bpchar, long_bpchar, empty_bpchar, special_chars_bpchar, unicode_bpchar) VALUES
('短文本',
 '这是一个较长的文本内容这是一个较长的文本内容这是一个较长的文本内容这是一个较长的文本内容这是一个较长的文本内容','','特殊字符: !@#$%^&*()_+-=[]{}''|;:,.<>?/~`','Unicode 文本: 🚀 中文 Español Français 日本語 🌍'),('Another short text',REPEAT('This is a long text. ', 100),'','Tab:Newline:' || E'\n' || 'Second line','Emoji: 😊 🎉 💻 中文测试');

select *,pg_typeof(short_bpchar),pg_typeof(long_bpchar),pg_typeof(empty_bpchar),pg_typeof(null_bpchar),pg_typeof(special_chars_bpchar),pg_typeof(unicode_bpchar),pg_typeof(default_bpchar) from tb_bpchar;

-- 清理
DROP TABLE IF EXISTS tb_bpchar CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
