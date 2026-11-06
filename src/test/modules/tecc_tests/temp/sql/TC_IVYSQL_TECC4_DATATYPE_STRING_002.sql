-- 清理
DROP TABLE IF EXISTS tb_varchar CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
create table tb_varchar(id SERIAL PRIMARY KEY, short_varchar varchar_enc encrypted with (dek = dek_t1), long_varchar varchar_enc encrypted with (dek = dek_t1), empty_varchar varchar_enc encrypted with (dek = dek_t1), null_varchar varchar_enc encrypted with (dek = dek_t1), special_chars_varchar varchar_enc encrypted with (dek = dek_t1), unicode_varchar varchar_enc encrypted with (dek = dek_t1), default_varchar varchar_enc encrypted with (dek = dek_t1) default 'defaultvalue', created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);

INSERT INTO tb_varchar (short_varchar, long_varchar, empty_varchar, special_chars_varchar, unicode_varchar) VALUES
('短文本',
 '这是一个较长的文本内容这是一个较长的文本内容这是一个较长的文本内容这是一个较长的文本内容这是一个较长的文本内容','','特殊字符: !@#$%^&*()_+-=[]{}''|;:,.<>?/~`','Unicode 文本: 🚀 中文 Español Français 日本語 🌍'),('Another short text',REPEAT('This is a long text. ', 100),'','Tab:Newline:' || E'\n' || 'Second line','Emoji: 😊 🎉 💻 中文测试');

select *,pg_typeof(short_varchar),pg_typeof(long_varchar),pg_typeof(empty_varchar),pg_typeof(null_varchar),pg_typeof(special_chars_varchar),pg_typeof(unicode_varchar),pg_typeof(default_varchar) from tb_varchar;

-- 清理
DROP TABLE IF EXISTS tb_varchar CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
