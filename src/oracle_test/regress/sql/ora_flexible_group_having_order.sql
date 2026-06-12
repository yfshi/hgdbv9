/* Begin - OA-REQ-2464 */
DROP TABLE IF EXISTS ora_hgb_orders;
DROP TABLE IF EXISTS ora_hgb_customers;

CREATE TABLE ora_hgb_customers
(
    customer_id NUMBER PRIMARY KEY,
    customer_name VARCHAR2(30),
    region VARCHAR2(20),
    channel VARCHAR2(20)
);

CREATE TABLE ora_hgb_orders
(
    order_id NUMBER PRIMARY KEY,
    customer_id NUMBER,
    dept VARCHAR2(20),
    product VARCHAR2(20),
    order_month VARCHAR2(7),
    amount NUMBER,
    status VARCHAR2(10),
    sales_rep VARCHAR2(20)
);

INSERT INTO ora_hgb_customers VALUES (1, 'North Manufacturing A', 'North', 'Direct');
INSERT INTO ora_hgb_customers VALUES (2, 'North Retail B', 'North', 'Partner');
INSERT INTO ora_hgb_customers VALUES (3, 'East Finance C', 'East', 'Direct');
INSERT INTO ora_hgb_customers VALUES (4, 'East Energy D', 'East', 'Partner');
INSERT INTO ora_hgb_customers VALUES (5, 'South Internet E', 'South', 'Direct');
INSERT INTO ora_hgb_customers VALUES (6, 'West Public Sector F', 'West', 'Partner');

INSERT INTO ora_hgb_orders VALUES (1001, 1, 'Engineering', 'Database',   '2026-01', 120, 'PAID', 'Alice');
INSERT INTO ora_hgb_orders VALUES (1002, 1, 'Engineering', 'Middleware', '2026-01',  80, 'PAID', 'Alice');
INSERT INTO ora_hgb_orders VALUES (1003, 2, 'Engineering', 'Database',   '2026-02', 260, 'PAID', 'Bob');
INSERT INTO ora_hgb_orders VALUES (1004, 2, 'Marketing',   'Service',    '2026-02', 180, 'OPEN', 'Bob');
INSERT INTO ora_hgb_orders VALUES (1005, 3, 'Marketing',   'Database',   '2026-01', 310, 'PAID', 'Carol');
INSERT INTO ora_hgb_orders VALUES (1006, 3, 'Marketing',   'Service',    '2026-03', 140, 'PAID', 'Carol');
INSERT INTO ora_hgb_orders VALUES (1007, 4, 'Finance',     'Database',   '2026-02',  90, 'OPEN', 'David');
INSERT INTO ora_hgb_orders VALUES (1008, 4, 'Finance',     'Middleware', '2026-03', 420, 'PAID', 'David');
INSERT INTO ora_hgb_orders VALUES (1009, 5, 'Engineering', 'Database',   '2026-03', 510, 'PAID', 'Eve');
INSERT INTO ora_hgb_orders VALUES (1010, 5, 'Marketing',   'Service',    '2026-03',  60, 'CANCEL', 'Eve');
INSERT INTO ora_hgb_orders VALUES (1011, 6, 'Engineering', 'Middleware', '2026-01', 220, 'PAID', 'Frank');
INSERT INTO ora_hgb_orders VALUES (1012, 6, 'Finance',     'Database',   '2026-02', 330, 'PAID', 'Frank');
INSERT INTO ora_hgb_orders VALUES (1013, 2, 'Engineering', 'Service',    '2026-03',  75, 'PAID', 'Bob');
INSERT INTO ora_hgb_orders VALUES (1014, 3, 'Marketing',   'Middleware', '2026-02', 155, 'OPEN', 'Carol');
INSERT INTO ora_hgb_orders VALUES (1015, 4, 'Finance',     'Service',    '2026-01', 240, 'PAID', 'David');


-- 'S01 standard: regional paid revenue by product'
SELECT c.region,
       o.product,
       COUNT(*) AS order_cnt,
       SUM(CASE WHEN o.status = 'PAID' THEN 1 ELSE 0 END) AS paid_order_cnt,
       SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) AS paid_amount,
       SUM(o.amount) AS gross_amount
FROM ora_hgb_orders o
JOIN ora_hgb_customers c ON c.customer_id = o.customer_id
WHERE o.status IN ('PAID', 'OPEN')
GROUP BY c.region, o.product
HAVING SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) >= 300
ORDER BY c.region, paid_amount DESC, o.product;

-- 'S01 oracle order: regional paid revenue by product'
SELECT c.region,
       o.product,
       COUNT(*) AS order_cnt,
       SUM(CASE WHEN o.status = 'PAID' THEN 1 ELSE 0 END) AS paid_order_cnt,
       SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) AS paid_amount,
       SUM(o.amount) AS gross_amount
FROM ora_hgb_orders o
JOIN ora_hgb_customers c ON c.customer_id = o.customer_id
WHERE o.status IN ('PAID', 'OPEN')
HAVING SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) >= 300
GROUP BY c.region, o.product
ORDER BY c.region, paid_amount DESC, o.product;

-- 'S02 standard: channel and status order quality'
SELECT c.channel,
       o.status,
       COUNT(*) AS order_cnt,
       COUNT(DISTINCT o.customer_id) AS customer_cnt,
       MIN(o.amount) AS min_amount,
       MAX(o.amount) AS max_amount,
       ROUND(AVG(o.amount), 2) AS avg_amount,
       SUM(o.amount) AS total_amount
FROM ora_hgb_orders o
JOIN ora_hgb_customers c ON c.customer_id = o.customer_id
GROUP BY c.channel, o.status
HAVING COUNT(*) >= 2 OR SUM(o.amount) >= 500
ORDER BY c.channel, total_amount DESC, o.status;

-- 'S02 oracle order: channel and status order quality'
SELECT c.channel,
       o.status,
       COUNT(*) AS order_cnt,
       COUNT(DISTINCT o.customer_id) AS customer_cnt,
       MIN(o.amount) AS min_amount,
       MAX(o.amount) AS max_amount,
       ROUND(AVG(o.amount), 2) AS avg_amount,
       SUM(o.amount) AS total_amount
FROM ora_hgb_orders o
JOIN ora_hgb_customers c ON c.customer_id = o.customer_id
HAVING COUNT(*) >= 2 OR SUM(o.amount) >= 500
GROUP BY c.channel, o.status
ORDER BY c.channel, total_amount DESC, o.status;

-- 'S03 standard: sales representative performance'
SELECT o.sales_rep,
       COUNT(*) AS order_cnt,
       COUNT(DISTINCT o.customer_id) AS customer_cnt,
       SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) AS paid_amount,
       SUM(CASE WHEN o.status = 'OPEN' THEN o.amount ELSE 0 END) AS open_pipeline,
       SUM(CASE WHEN o.status = 'CANCEL' THEN o.amount ELSE 0 END) AS canceled_amount,
       MAX(o.amount) AS largest_order
FROM ora_hgb_orders o
GROUP BY o.sales_rep
HAVING COUNT(*) >= 2
   AND SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) >= 200
ORDER BY paid_amount DESC, o.sales_rep;

-- 'S03 oracle order: sales representative performance'
SELECT o.sales_rep,
       COUNT(*) AS order_cnt,
       COUNT(DISTINCT o.customer_id) AS customer_cnt,
       SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) AS paid_amount,
       SUM(CASE WHEN o.status = 'OPEN' THEN o.amount ELSE 0 END) AS open_pipeline,
       SUM(CASE WHEN o.status = 'CANCEL' THEN o.amount ELSE 0 END) AS canceled_amount,
       MAX(o.amount) AS largest_order
FROM ora_hgb_orders o
HAVING COUNT(*) >= 2
   AND SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) >= 200
GROUP BY o.sales_rep
ORDER BY paid_amount DESC, o.sales_rep;

-- 'S04 standard: monthly product revenue'
SELECT o.order_month,
       o.product,
       COUNT(*) AS order_cnt,
       SUM(o.amount) AS total_amount,
       SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) AS recognized_amount,
       ROUND(SUM(o.amount) / COUNT(*), 2) AS amount_per_order
FROM ora_hgb_orders o
GROUP BY o.order_month, o.product
HAVING SUM(o.amount) >= 250
ORDER BY o.order_month, total_amount DESC, o.product;

-- 'S04 oracle order: monthly product revenue'
SELECT o.order_month,
       o.product,
       COUNT(*) AS order_cnt,
       SUM(o.amount) AS total_amount,
       SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) AS recognized_amount,
       ROUND(SUM(o.amount) / COUNT(*), 2) AS amount_per_order
FROM ora_hgb_orders o
HAVING SUM(o.amount) >= 250
GROUP BY o.order_month, o.product
ORDER BY o.order_month, total_amount DESC, o.product;

-- 'S05 standard: customers with repeat activity'
SELECT c.customer_id,
       c.customer_name,
       c.region,
       c.channel,
       COUNT(*) AS order_cnt,
       COUNT(DISTINCT o.product) AS product_cnt,
       SUM(o.amount) AS total_amount,
       SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) AS paid_amount
FROM ora_hgb_customers c
JOIN ora_hgb_orders o ON o.customer_id = c.customer_id
GROUP BY c.customer_id, c.customer_name, c.region, c.channel
HAVING COUNT(*) >= 2
   AND COUNT(DISTINCT o.product) >= 2
ORDER BY total_amount DESC, c.customer_id;

-- 'S05 oracle order: customers with repeat activity'
SELECT c.customer_id,
       c.customer_name,
       c.region,
       c.channel,
       COUNT(*) AS order_cnt,
       COUNT(DISTINCT o.product) AS product_cnt,
       SUM(o.amount) AS total_amount,
       SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) AS paid_amount
FROM ora_hgb_customers c
JOIN ora_hgb_orders o ON o.customer_id = c.customer_id
HAVING COUNT(*) >= 2
   AND COUNT(DISTINCT o.product) >= 2
GROUP BY c.customer_id, c.customer_name, c.region, c.channel
ORDER BY total_amount DESC, c.customer_id;

-- 'S06 standard: department product mix'
SELECT o.dept,
       o.product,
       COUNT(*) AS order_cnt,
       COUNT(DISTINCT o.sales_rep) AS rep_cnt,
       SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) AS paid_amount,
       SUM(CASE WHEN o.status <> 'PAID' THEN o.amount ELSE 0 END) AS non_paid_amount,
       SUM(o.amount) AS total_amount
FROM ora_hgb_orders o
GROUP BY o.dept, o.product
HAVING COUNT(*) >= 1
   AND SUM(o.amount) >= 200
ORDER BY o.dept, total_amount DESC, o.product;

-- 'S06 oracle order: department product mix'
SELECT o.dept,
       o.product,
       COUNT(*) AS order_cnt,
       COUNT(DISTINCT o.sales_rep) AS rep_cnt,
       SUM(CASE WHEN o.status = 'PAID' THEN o.amount ELSE 0 END) AS paid_amount,
       SUM(CASE WHEN o.status <> 'PAID' THEN o.amount ELSE 0 END) AS non_paid_amount,
       SUM(o.amount) AS total_amount
FROM ora_hgb_orders o
HAVING COUNT(*) >= 1
   AND SUM(o.amount) >= 200
GROUP BY o.dept, o.product
ORDER BY o.dept, total_amount DESC, o.product;

DROP TABLE IF EXISTS emp_2464;
CREATE TABLE emp_2464 (
    emp_id   NUMBER PRIMARY KEY,
    emp_name VARCHAR2(20),
    dept     VARCHAR2(20)
);

INSERT INTO emp_2464 VALUES (1, 'zhangsan', 'IT');
INSERT INTO emp_2464 VALUES (2, 'lisi', 'IT');
INSERT INTO emp_2464 VALUES (3, 'wangwu', 'MAR');
INSERT INTO emp_2464 VALUES (4, 'zhaoliu', 'MAR');
INSERT INTO emp_2464 VALUES (5, 'sunqi', 'MAR');
INSERT INTO emp_2464 VALUES (6, 'zhouba', 'FIN');

--1
SELECT dept AS dep,
       COUNT(*) AS emp_count
FROM emp_2464
GROUP BY dept
HAVING COUNT(*) >= 2
ORDER BY emp_count DESC;

--2
SELECT dept AS dep,
       COUNT(*) AS emp_count
FROM emp_2464
HAVING COUNT(*) >= 2
GROUP BY dept
ORDER BY emp_count DESC;

--cleanup
DROP TABLE ora_hgb_orders;
DROP TABLE ora_hgb_customers;
DROP TABLE emp_2464;
/* End - OA-REQ-2464 */
