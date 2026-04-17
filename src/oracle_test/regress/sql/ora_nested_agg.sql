/* Begin - OA-REQ-2252 */
DROP TABLE IF EXISTS inp_regist;
CREATE TABLE inp_regist (
    hosnum       VARCHAR2(20 CHAR) NOT NULL,
    inpno        VARCHAR2(20 CHAR) NOT NULL,
    patientid    VARCHAR2(20 CHAR),
    admiss_date  DATE NOT NULL,
    deptcode     VARCHAR2(10 CHAR),
    wardid       VARCHAR2(10 CHAR),
    doctorid     VARCHAR2(20 CHAR),
    yearsold     NUMBER(5),
    inweight     NUMBER(8,2),
    amount       NUMBER(12,2),
    CONSTRAINT pk_inp_regist PRIMARY KEY (inpno, hosnum)
);

INSERT INTO inp_regist VALUES ('1303',   'I0001', 'P001', DATE '2026-04-01', 'D01', 'W01', 'DR01', 35, 60.5, 1200);
INSERT INTO inp_regist VALUES ('1303',   'I0002', 'P002', DATE '2026-04-01', 'D01', 'W01', 'DR01', 42, 70.0, 1800);
INSERT INTO inp_regist VALUES ('130301', 'I0003', 'P003', DATE '2026-04-01', 'D02', 'W02', 'DR02', 28, 55.0, 1500);
INSERT INTO inp_regist VALUES ('130302', 'I0004', 'P004', DATE '2026-04-02', 'D01', 'W01', 'DR01', 51, 68.0, 2100);
INSERT INTO inp_regist VALUES ('130302', 'I0005', 'P005', DATE '2026-04-02', 'D02', 'W03', 'DR03', 31, 49.5, 1300);
INSERT INTO inp_regist VALUES ('1303',   'I0006', 'P006', DATE '2026-04-02', 'D02', 'W03', 'DR03', 63, 80.0, 2600);
INSERT INTO inp_regist VALUES ('0000',   'I0007', 'P007', DATE '2026-04-03', 'D03', 'W04', 'DR04', 40, 66.0, 1000);
INSERT INTO inp_regist VALUES ('130301', 'I0008', 'P008', DATE '2026-04-03', 'D01', 'W02', 'DR02', 24, 52.0, 1700);

-- ! = 
SELECT t.hosnum
 FROM inp_regist t WHERE t.hosnum ! = '!';

-- !=
SELECT t.hosnum
 FROM inp_regist t WHERE t.hosnum != '!';

-- <>
SELECT t.hosnum
 FROM inp_regist t WHERE t.hosnum <> '!';

SELECT
	NVL(MAX(COUNT(t.inpno)),
	0) num1,
	NVL(MIN(TO_CHAR(t.admiss_date, 'yyyy-mm-dd')),
	0) day1
FROM
inp_regist t
WHERE
	t.hosnum <> '0000'
	AND t.hosnum IN('1303', '130301', '130302')
GROUP BY
	TO_CHAR(t.admiss_date, 'yyyy-mm-dd');

SELECT MAX(COUNT(t.inpno)) AS max_day_cnt
FROM inp_regist t
WHERE t.hosnum <> '0000'
  AND t.hosnum IN ('1303', '130301', '130302')
GROUP BY TO_CHAR(t.admiss_date, 'yyyy-mm-dd');

SELECT NVL(MAX(x.cnt), 0) AS num1,
       NVL(MIN(x.day1), '0') AS day1
FROM (
    SELECT TO_CHAR(t.admiss_date, 'yyyy-mm-dd') AS day1,
           COUNT(t.inpno) AS cnt
    FROM inp_regist t
    WHERE t.hosnum <> '0000'
      AND t.hosnum IN ('1303', '130301', '130302')
    GROUP BY TO_CHAR(t.admiss_date, 'yyyy-mm-dd')
) x;

SELECT MAX(COUNT(*)) AS max_cnt
FROM inp_regist
GROUP BY deptcode;

--no group clause
SELECT MAX(COUNT(*)) AS max_cnt FROM inp_regist; --error

SELECT MIN(COUNT(*)) AS min_cnt
FROM inp_regist
GROUP BY deptcode;

SELECT SUM(COUNT(*)) AS total_cnt
FROM inp_regist
GROUP BY deptcode;

SELECT AVG(COUNT(*)) AS avg_cnt
FROM inp_regist
GROUP BY deptcode;

SELECT MAX(SUM(amount)) AS max_sum_amt
FROM inp_regist
GROUP BY deptcode;

SELECT MIN(SUM(amount)) AS min_sum_amt
FROM inp_regist
GROUP BY deptcode;

SELECT AVG(SUM(amount)) AS avg_sum_amt
FROM inp_regist
GROUP BY deptcode;

SELECT SUM(AVG(amount)) AS sum_avg_amt
FROM inp_regist
GROUP BY deptcode;

SELECT MAX(MIN(admiss_date)) AS latest_of_earliest_dates
FROM inp_regist
GROUP BY deptcode;

SELECT MIN(MAX(admiss_date)) AS earliest_of_latest_dates
FROM inp_regist
GROUP BY deptcode;

SELECT AVG(MAX(amount)) AS avg_of_group_max
FROM inp_regist
GROUP BY deptcode;

SELECT AVG(MIN(amount)) AS avg_of_group_min
FROM inp_regist
GROUP BY deptcode;

SELECT MAX(VARIANCE(amount)) AS max_variance_amt
FROM inp_regist
GROUP BY deptcode;

SELECT MAX(CORR(yearsold, amount)) AS max_corr
FROM inp_regist
GROUP BY deptcode;

SELECT AVG(COVAR_POP(yearsold, amount)) AS avg_covar
FROM inp_regist
GROUP BY wardid;

-- agg in having clause
SELECT
	TO_CHAR(t.admiss_date, 'yyyy-mm-dd') AS day1,
	COUNT(t.inpno) AS num1
FROM inp_regist t
  WHERE t.hosnum <> '0000'
	AND t.hosnum IN ('1303', '130301', '130302')
GROUP BY TO_CHAR(t.admiss_date, 'yyyy-mm-dd')
HAVING COUNT(t.inpno) >= 2;

-- agg nested in having clause
SELECT
	TO_CHAR(t.admiss_date, 'yyyy-mm-dd') AS day1,
	COUNT(t.inpno) AS num1
FROM inp_regist t
  WHERE t.hosnum <> '0000'
	AND t.hosnum IN ('1303', '130301', '130302')
GROUP BY TO_CHAR(t.admiss_date, 'yyyy-mm-dd')
HAVING MAX(COUNT(t.inpno)) >= 2;

--3 levels, AVG(MAX(COUNT(...)))
SELECT AVG(x.max_day_cnt_per_dept) AS avg_of_dept_max_day_cnt
FROM (
    SELECT deptcode,
           MAX(day_cnt) AS max_day_cnt_per_dept
    FROM (
        SELECT t.deptcode,
               TO_CHAR(t.admiss_date, 'yyyy-mm-dd') AS day1,
               COUNT(*) AS day_cnt
        FROM inp_regist t
        WHERE t.hosnum <> '0000'
          AND t.hosnum IN ('1303', '130301', '130302')
        GROUP BY t.deptcode, TO_CHAR(t.admiss_date, 'yyyy-mm-dd')
    ) d1
    GROUP BY deptcode
) x;

--4 levels, MAX(AVG(MAX(COUNT(...))))
SELECT MAX(z.avg_max_day_cnt_per_ward) AS final_max_value
FROM (
    SELECT y.wardid,
           AVG(y.max_day_cnt_per_dept) AS avg_max_day_cnt_per_ward
    FROM (
        SELECT d1.wardid,
               d1.deptcode,
               MAX(d1.day_cnt) AS max_day_cnt_per_dept
        FROM (
            SELECT t.wardid,
                   t.deptcode,
                   TO_CHAR(t.admiss_date, 'yyyy-mm-dd') AS day1,
                   COUNT(*) AS day_cnt
            FROM inp_regist t
            WHERE t.hosnum <> '0000'
              AND t.hosnum IN ('1303', '130301', '130302')
            GROUP BY t.wardid,
                     t.deptcode,
                     TO_CHAR(t.admiss_date, 'yyyy-mm-dd')
        ) d1
        GROUP BY d1.wardid, d1.deptcode
    ) y
    GROUP BY y.wardid
) z;

DROP TABLE IF EXISTS t_nested_err;
CREATE TABLE t_nested_err (
    deptcode int,
    inpno    varchar2(5)
);

INSERT INTO t_nested_err VALUES (10, 'a');
INSERT INTO t_nested_err VALUES (10, 'b');
INSERT INTO t_nested_err VALUES (20, 'c');

-- normal column + normal agg + normal nested-agg
SELECT
    deptcode,
    COUNT(inpno),
    MAX(COUNT(inpno))
FROM t_nested_err
GROUP BY deptcode; --error

SELECT MAX(cnt) AS max_cnt
FROM (
    SELECT deptcode, COUNT(inpno) AS cnt
    FROM t_nested_err
    GROUP BY deptcode
);

SELECT
    x.deptcode,
    x.cnt,
    MAX(x.cnt) OVER () AS max_cnt
FROM (
    SELECT deptcode, COUNT(inpno) AS cnt
    FROM t_nested_err
    GROUP BY deptcode
) x;

--clean
DROP TABLE inp_regist PURGE;
DROP TABLE t_nested_err PURGE;
/* End - OA-REQ-2252 */
