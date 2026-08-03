--
-- BOOLEAN
--

--
-- sanity check - if this fails go insane!
--
SELECT 1 AS one;


-- ******************testing built-in type bool********************

-- check bool input syntax

SELECT true AS true;

SELECT false AS false;

SELECT CAST('t' AS bool) AS true;

SELECT CAST('   f           ' AS bool) AS false;

SELECT CAST('true' AS bool) AS true;

SELECT CAST('test' AS bool) AS error;

SELECT CAST('false' AS bool) AS false;

SELECT CAST('foo' AS bool) AS error;

SELECT CAST('y' AS bool) AS true;

SELECT CAST('yes' AS bool) AS true;

SELECT CAST('yeah' AS bool) AS error;

SELECT CAST('n' AS bool) AS false;

SELECT CAST('no' AS bool) AS false;

SELECT CAST('nay' AS bool) AS error;

SELECT CAST('on' AS bool) AS true;

SELECT CAST('off' AS bool) AS false;

SELECT CAST('of' AS bool) AS false;

SELECT CAST('o' AS bool) AS error;

SELECT CAST('on_' AS bool) AS error;

SELECT CAST('off_' AS bool) AS error;

SELECT CAST('1' AS bool) AS true;

SELECT CAST('11' AS bool) AS error;

SELECT CAST('0' AS bool) AS false;

SELECT CAST('000' AS bool) AS error;

SELECT CAST('' AS bool) AS error;

-- and, or, not in qualifications

SELECT CAST('t' AS bool) or CAST('f' AS bool) AS true;

SELECT CAST('t' AS bool) and CAST('f' AS bool) AS false;

SELECT not CAST('f' AS bool) AS true;

SELECT CAST('t' AS bool) = CAST('f' AS bool) AS false;

SELECT CAST('t' AS bool) <> CAST('f' AS bool) AS true;

SELECT CAST('t' AS bool) > CAST('f' AS bool) AS true;

SELECT CAST('t' AS bool) >= CAST('f' AS bool) AS true;

SELECT CAST('f' AS bool) < CAST('t' AS bool) AS true;

SELECT CAST('f' AS bool) <= CAST('t' AS bool) AS true;

-- explicit casts to/from text
SELECT 'TrUe'::text::boolean AS true, 'fAlse'::text::boolean AS false;
SELECT '    true   '::text::boolean AS true,
       '     FALSE'::text::boolean AS false;
SELECT true::boolean::text AS true, false::boolean::text AS false;

SELECT '  tru e '::text::boolean AS invalid;    -- error
SELECT ''::text::boolean AS invalid;            -- error

CREATE TABLE BOOLTBL1 (f1 bool);

INSERT INTO BOOLTBL1 (f1) VALUES (CAST('t' AS bool));

INSERT INTO BOOLTBL1 (f1) VALUES (CAST('True' AS bool));

INSERT INTO BOOLTBL1 (f1) VALUES (CAST('true' AS bool));


-- BOOLTBL1 should be full of true's at this point
SELECT BOOLTBL1.* FROM BOOLTBL1;


SELECT BOOLTBL1.*
   FROM BOOLTBL1
   WHERE f1 = CAST('true' AS bool);


SELECT BOOLTBL1.*
   FROM BOOLTBL1
   WHERE f1 <> CAST('false' AS bool);

SELECT BOOLTBL1.*
   FROM BOOLTBL1
   WHERE booleq(CAST('false' AS bool), f1);

INSERT INTO BOOLTBL1 (f1) VALUES (CAST('f' AS bool));

SELECT BOOLTBL1.*
   FROM BOOLTBL1
   WHERE f1 = CAST('false' AS bool);


CREATE TABLE BOOLTBL2 (f1 bool);

INSERT INTO BOOLTBL2 (f1) VALUES (CAST('f' AS bool));

INSERT INTO BOOLTBL2 (f1) VALUES (CAST('false' AS bool));

INSERT INTO BOOLTBL2 (f1) VALUES (CAST('False' AS bool));

INSERT INTO BOOLTBL2 (f1) VALUES (CAST('FALSE' AS bool));

-- This is now an invalid expression
-- For pre-v6.3 this evaluated to false - thomas 1997-10-23
INSERT INTO BOOLTBL2 (f1)
   VALUES (CAST('XXX' AS bool));

-- BOOLTBL2 should be full of false's at this point
SELECT BOOLTBL2.* FROM BOOLTBL2;


SELECT BOOLTBL1.*, BOOLTBL2.*
   FROM BOOLTBL1, BOOLTBL2
   WHERE BOOLTBL2.f1 <> BOOLTBL1.f1;


SELECT BOOLTBL1.*, BOOLTBL2.*
   FROM BOOLTBL1, BOOLTBL2
   WHERE boolne(BOOLTBL2.f1,BOOLTBL1.f1);


SELECT BOOLTBL1.*, BOOLTBL2.*
   FROM BOOLTBL1, BOOLTBL2
   WHERE BOOLTBL2.f1 = BOOLTBL1.f1 and BOOLTBL1.f1 = CAST('false' AS bool);


SELECT BOOLTBL1.*, BOOLTBL2.*
   FROM BOOLTBL1, BOOLTBL2
   WHERE BOOLTBL2.f1 = BOOLTBL1.f1 or BOOLTBL1.f1 = CAST('true' AS bool)
   ORDER BY BOOLTBL1.f1, BOOLTBL2.f1;

--
-- SQL syntax
-- Try all combinations to ensure that we get nothing when we expect nothing
-- - thomas 2000-01-04
--

SELECT f1
   FROM BOOLTBL1
   WHERE f1 IS TRUE;

SELECT f1
   FROM BOOLTBL1
   WHERE f1 IS NOT FALSE;

SELECT f1
   FROM BOOLTBL1
   WHERE f1 IS FALSE;

SELECT f1
   FROM BOOLTBL1
   WHERE f1 IS NOT TRUE;

SELECT f1
   FROM BOOLTBL2
   WHERE f1 IS TRUE;

SELECT f1
   FROM BOOLTBL2
   WHERE f1 IS NOT FALSE;

SELECT f1
   FROM BOOLTBL2
   WHERE f1 IS FALSE;

SELECT f1
   FROM BOOLTBL2
   WHERE f1 IS NOT TRUE;

--
-- Tests for BooleanTest
--
CREATE TABLE BOOLTBL3 (d text, b bool, o int);
INSERT INTO BOOLTBL3 (d, b, o) VALUES ('true', true, 1);
INSERT INTO BOOLTBL3 (d, b, o) VALUES ('false', false, 2);
INSERT INTO BOOLTBL3 (d, b, o) VALUES ('null', null, 3);

SELECT
    d,
    b IS TRUE AS istrue,
    b IS NOT TRUE AS isnottrue,
    b IS FALSE AS isfalse,
    b IS NOT FALSE AS isnotfalse,
    b IS UNKNOWN AS isunknown,
    b IS NOT UNKNOWN AS isnotunknown
FROM booltbl3 ORDER BY o;


-- Test to make sure short-circuiting and NULL handling is
-- correct. Use a table as source to prevent constant simplification
-- to interfer.
CREATE TABLE booltbl4(isfalse bool, istrue bool, isnul bool);
INSERT INTO booltbl4 VALUES (false, true, null);
\pset null '(null)'

-- AND expression need to return null if there's any nulls and not all
-- of the value are true
SELECT istrue AND isnul AND istrue FROM booltbl4;
SELECT istrue AND istrue AND isnul FROM booltbl4;
SELECT isnul AND istrue AND istrue FROM booltbl4;
SELECT isfalse AND isnul AND istrue FROM booltbl4;
SELECT istrue AND isfalse AND isnul FROM booltbl4;
SELECT isnul AND istrue AND isfalse FROM booltbl4;

-- OR expression need to return null if there's any nulls and none
-- of the value is true
SELECT isfalse OR isnul OR isfalse FROM booltbl4;
SELECT isfalse OR isfalse OR isnul FROM booltbl4;
SELECT isnul OR isfalse OR isfalse FROM booltbl4;
SELECT isfalse OR isnul OR istrue FROM booltbl4;
SELECT istrue OR isfalse OR isnul FROM booltbl4;
SELECT isnul OR istrue OR isfalse FROM booltbl4;


--
-- Clean up
-- Many tables are retained by the regression test, but these do not seem
--  particularly useful so just get rid of them for now.
--  - thomas 1997-11-30
--

DROP TABLE  BOOLTBL1;

DROP TABLE  BOOLTBL2;

DROP TABLE  BOOLTBL3;

DROP TABLE  BOOLTBL4;
