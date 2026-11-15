-- perf_ext_multiply.sql
-- %OFFSET% is replaced by the TAP driver. Encrypted business columns are copied
-- as ciphertext with psql -k off; row_id is kept plain and shifted.

INSERT INTO employee_plain
SELECT row_id + %OFFSET%, employee_id, name, salary, address, phone, idcard_no,
       department, job_title, hire_year, age, grade, bonus, city, email, status,
       manager_id, performance_score, remark
FROM stage_employee_plain;

INSERT INTO employee_full_enc
SELECT row_id + %OFFSET%, employee_id, name, salary, address, phone, idcard_no,
       department, job_title, hire_year, age, grade, bonus, city, email, status,
       manager_id, performance_score, remark
FROM stage_employee_full_enc;

INSERT INTO employee_half_enc
SELECT row_id + %OFFSET%, employee_id, name, salary, address, phone, idcard_no,
       department, job_title, hire_year, age, grade, bonus, city, email, status,
       manager_id, performance_score, remark
FROM stage_employee_half_enc;
