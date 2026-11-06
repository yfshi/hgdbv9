-- 清理
DROP TABLE IF EXISTS tb_text CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;

-- 前置条件
CREATE KEK kek_t1 WITH (KEY_STORE=localkms, KEY_PATH='/tmp/kek_t1.key');
CREATE DEK dek_t1 WITH VALUES (KEK=kek_t1);

-- 测试步骤
create table tb_text(id SERIAL PRIMARY KEY, short_text text_enc encrypted with (dek = dek_t1), long_text text_enc encrypted with (dek = dek_t1), empty_text text_enc encrypted with (dek = dek_t1), null_text text_enc encrypted with (dek = dek_t1), special_chars_text text_enc encrypted with (dek = dek_t1), unicode_text text_enc encrypted with (dek = dek_t1), default_text text_enc encrypted with (dek = dek_t1) default 'defaultvalue', created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);

INSERT INTO tb_text (short_text, long_text, empty_text, special_chars_text, unicode_text) VALUES
('短文本',
 '这是一个较长的文本内容这是一个较长的文本内容这是一个较长的文本内容这是一个较长的文本内容这是一个较长的文本内容','','特殊字符: !@#$%^&*()_+-=[]{}''|;:,.<>?/~`','Unicode 文本: 🚀 中文 Español Français 日本語 🌍'),('Another short text',REPEAT('This is a long text. ', 100),'','Tab:Newline:' || E'\n' || 'Second line','Emoji: 😊 🎉 💻 中文测试');

select *,pg_typeof(short_text),pg_typeof(long_text),pg_typeof(empty_text),pg_typeof(null_text),pg_typeof(special_chars_text),pg_typeof(unicode_text),pg_typeof(default_text) from tb_text;

-- 清理
DROP TABLE IF EXISTS tb_text CASCADE;
DROP DEK IF EXISTS dek_t1 CASCADE;
DROP KEK IF EXISTS kek_t1 CASCADE;
