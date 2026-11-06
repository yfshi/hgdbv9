-- tecc_std_regress.sql
-- 标准 TECC 回归测试入口
-- 用法: psql -f tecc_std_regress.sql
-- Windows 需先设置: set TECC_KEK_PATH=D:\path\to\kek_dir
--
-- 注意：本文件仅用于手动执行单一回归测试。
-- 如果通过 vcregress.pl 运行，请使用 parallel_schedule 中的条目。

-- 当前测试用例
\ir tecc_bug8000.sql
