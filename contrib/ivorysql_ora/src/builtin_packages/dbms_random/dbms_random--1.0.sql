CREATE OR REPLACE FUNCTION random_mixed_case_string(len INTEGER) RETURNS VARCHAR2 AS $$
DECLARE
    res VARCHAR2 := '';
    chars VARCHAR2 := 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz';
    i INTEGER;
BEGIN
    FOR i IN 1..len LOOP
        res := res || substr(chars, floor(random()*length(chars) + 1), 1);
    END LOOP;
    RETURN res;
END;
$$ LANGUAGE plisql;

CREATE OR REPLACE FUNCTION random_lower_case_string(len INTEGER) RETURNS VARCHAR2 AS $$
DECLARE
    res VARCHAR2 := '';
    chars VARCHAR2 := 'abcdefghijklmnopqrstuvwxyz';
    i INTEGER;
BEGIN
    FOR i IN 1..len LOOP
        res := res || substr(chars, floor(random()*length(chars) + 1), 1);
    END LOOP;
    RETURN res;
END;
$$ LANGUAGE plisql;


CREATE OR REPLACE FUNCTION random_upper_case_string(len INTEGER) RETURNS VARCHAR2 AS $$
DECLARE
    res VARCHAR2 := '';
    chars VARCHAR2 := 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
    i INTEGER;
BEGIN
    FOR i IN 1..len LOOP
        res := res || substr(chars, floor(random()*length(chars) + 1), 1);
    END LOOP;
    RETURN res;
END;
$$ LANGUAGE plisql;


CREATE OR REPLACE FUNCTION random_case_number_string(len INTEGER) RETURNS VARCHAR2 AS $$
DECLARE
    res VARCHAR2 := '';
    chars VARCHAR2 := 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    i INTEGER;
BEGIN
    FOR i IN 1..len LOOP
        res := res || substr(chars, floor(random()*length(chars) + 1), 1);
    END LOOP;
    RETURN res;
END;
$$ LANGUAGE plisql;


CREATE OR REPLACE FUNCTION random_print_case_string(len INTEGER) RETURNS VARCHAR2 AS $$
DECLARE
    res VARCHAR2 := '';
    chars VARCHAR2 := 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 !"#$%&''()*+,-./:;<=>?@[\]^_`{|}~';
    i INTEGER;
BEGIN
    FOR i IN 1..len LOOP
        res := res || substr(chars, floor(random()*length(chars) + 1), 1);
    END LOOP;
    RETURN res;
END;
$$ LANGUAGE plisql;


CREATE OR REPLACE NONEDITIONABLE PACKAGE sys.dbms_random AUTHID current_user IS
    PROCEDURE INITIALIZE (
        val IN INTEGER
    );
    FUNCTION NORMAL return NUMBER;
    FUNCTION RANDOM return INTEGER;
    PROCEDURE SEED (
        val IN INTEGER
    );
    PROCEDURE SEED (
        val IN VARCHAR2(2000)
    );
    FUNCTION STRING (opt CHAR, len NUMBER) return VARCHAR2;
    PROCEDURE TERMINATE;
    FUNCTION VALUE return NUMBER;
    FUNCTION VALUE(low NUMBER, high NUMBER) return NUMBER;
END;

CREATE OR REPLACE PACKAGE BODY sys.dbms_random IS

PROCEDURE INITIALIZE (
    val IN INTEGER
) IS
BEGIN
    select setseed(val);
END INITIALIZE;

FUNCTION NORMAL RETURN NUMBER IS
BEGIN
    RETURN pg_catalog.random();
END;

FUNCTION RANDOM RETURN INTEGER IS
BEGIN
    RETURN floor(pg_catalog.random() * power(2,32) - power(2,31))::bigint;
END;


PROCEDURE SEED (
    val IN INTEGER
) IS
BEGIN
    select setseed(val);
END SEED;

PROCEDURE SEED (
    val IN VARCHAR2(2000)
) IS
BEGIN
    select NULL;
END SEED;


FUNCTION STRING (opt CHAR, len NUMBER) RETURN VARCHAR2 IS
BEGIN
    IF upper(opt) = 'A' THEN
        RETURN random_mixed_case_string(len);
    ELSIF upper(opt) = 'L' THEN
        RETURN random_lower_case_string(len);
    ELSIF upper(opt) = 'X' THEN
        RETURN random_case_number_string(len);
    ELSIF upper(opt) = 'P' THEN
        RETURN random_print_case_string(len);
    ELSE 
        RETURN random_upper_case_string(len);
    END IF;
END;

PROCEDURE TERMINATE IS
BEGIN
    select NULL as TERMINATE;
END TERMINATE;

FUNCTION VALUE RETURN NUMBER IS
BEGIN
    RETURN pg_catalog.random();
END;

FUNCTION VALUE (low NUMBER, high NUMBER) RETURN NUMBER IS
BEGIN
    IF low < high THEN
        RETURN pg_catalog.random()*(high - low) + low;
    ELSE
        RETURN pg_catalog.random()*(low - high) + high;
    END IF;
END;

END;

GRANT EXECUTE ON package sys.dbms_random TO public;