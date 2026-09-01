-- Begin - ReqID:SRS-PLSQL-UTL_RAW
create or replace function sys.utl_raw_bit_and(RAW, RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_bit_and' LANGUAGE C;
create or replace function sys.utl_raw_bit_complement(RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_bit_complement' LANGUAGE C STRICT;
create or replace function sys.utl_raw_bit_or(RAW, RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_bit_or' LANGUAGE C;
create or replace function sys.utl_raw_bit_xor(RAW, RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_bit_xor' LANGUAGE C;
create or replace function sys.utl_raw_cast_from_binary_double(BINARY_DOUBLE, PLS_INTEGER) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_cast_from_binary_double' LANGUAGE C STRICT;
create or replace function sys.utl_raw_cast_from_binary_float(BINARY_FLOAT, PLS_INTEGER) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_cast_from_binary_float' LANGUAGE C STRICT;
create or replace function sys.utl_raw_cast_from_binary_integer(BINARY_INTEGER, PLS_INTEGER) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_cast_from_binary_integer' LANGUAGE C STRICT;
create or replace function sys.utl_raw_cast_from_number(NUMBER) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_cast_from_number' LANGUAGE C STRICT;
create or replace function sys.utl_raw_cast_to_binary_double(RAW, PLS_INTEGER) RETURN BINARY_DOUBLE
	AS 'MODULE_PATHNAME', 'utl_raw_cast_to_binary_double' LANGUAGE C;
create or replace function sys.utl_raw_cast_to_binary_float(RAW, PLS_INTEGER) RETURN BINARY_FLOAT
	AS 'MODULE_PATHNAME', 'utl_raw_cast_to_binary_float' LANGUAGE C;
create or replace function sys.utl_raw_cast_to_binary_integer(RAW, PLS_INTEGER) RETURN BINARY_INTEGER
	AS 'MODULE_PATHNAME', 'utl_raw_cast_to_binary_integer' LANGUAGE C;
create or replace function sys.utl_raw_cast_to_number(RAW) RETURN NUMBER
	AS 'MODULE_PATHNAME', 'utl_raw_cast_to_number' LANGUAGE C STRICT;
create or replace function sys.utl_raw_convert(RAW, VARCHAR2, VARCHAR2) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_convert' LANGUAGE C;
create or replace function sys.utl_raw_overlay(RAW, RAW, BINARY_INTEGER, BINARY_INTEGER, RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_overlay' LANGUAGE C;
create or replace function sys.utl_raw_reverse(RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_reverse' LANGUAGE C;
create or replace function sys.utl_raw_translate(RAW, RAW, RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_translate' LANGUAGE C;
create or replace function sys.utl_raw_transliterate(RAW, RAW, RAW, RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_transliterate' LANGUAGE C;
create or replace function sys.utl_raw_xrange(RAW, RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_raw_xrange' LANGUAGE C;

CREATE OR REPLACE PACKAGE sys.utl_raw AS
	-- Begin - BUG#9092
	--------------
	-- CONSTANTS
	--------------
	big_endian CONSTANT PLS_INTEGER := 1;
	little_endian CONSTANT PLS_INTEGER := 2;
	machine_endian CONSTANT PLS_INTEGER := 3;
	-- End - BUG#9092

	FUNCTION bit_and(r1 RAW, r2 RAW) RETURN RAW;
	FUNCTION bit_complement(r RAW)  RETURN RAW;
	FUNCTION bit_or(r1 RAW, r2 RAW)  RETURN RAW;
	FUNCTION bit_xor(r1 RAW, r2 RAW) RETURN RAW;
	-- cast from
	FUNCTION cast_from_binary_double(n BINARY_DOUBLE, endianess PLS_INTEGER DEFAULT 1) RETURN RAW;
	FUNCTION cast_from_binary_float(n BINARY_FLOAT, endianess PLS_INTEGER DEFAULT 1) RETURN RAW;
	FUNCTION cast_from_binary_integer(n BINARY_INTEGER, endianess PLS_INTEGER DEFAULT 1) RETURN RAW;
	FUNCTION cast_from_number(n NUMBER) RETURN RAW;
	-- cast to
	FUNCTION cast_to_binary_double(r RAW, endianess PLS_INTEGER DEFAULT 1) RETURN BINARY_DOUBLE;
	FUNCTION cast_to_binary_float(r RAW, endianess PLS_INTEGER DEFAULT 1) RETURN BINARY_FLOAT;
	FUNCTION cast_to_binary_integer(r RAW, endianess PLS_INTEGER DEFAULT 1) RETURN BINARY_INTEGER;
	FUNCTION cast_to_number(r RAW) RETURN NUMBER;
	FUNCTION cast_to_nvarchar2(r RAW) RETURN NVARCHAR2;
	FUNCTION cast_to_raw(c VARCHAR2) RETURN RAW;
	FUNCTION cast_to_varchar2(r RAW) RETURN VARCHAR2;
	-- other
	FUNCTION compare(r1 RAW, r2 RAW, pad RAW DEFAULT NULL) RETURN NUMBER;
	FUNCTION concat(r1 RAW DEFAULT NULL, r2 RAW DEFAULT NULL, r3 RAW DEFAULT NULL, r4 RAW DEFAULT NULL,
					r5 RAW DEFAULT NULL, r6 RAW DEFAULT NULL, r7 RAW DEFAULT NULL, r8 RAW DEFAULT NULL,
					r9 RAW DEFAULT NULL, r10 RAW DEFAULT NULL, r11 RAW DEFAULT NULL, r12 RAW DEFAULT NULL) RETURN RAW;
	FUNCTION convert(r RAW, to_charset VARCHAR2, from_charset VARCHAR2) RETURN RAW;
	FUNCTION copies(r RAW, n NUMBER) RETURN RAW;
	FUNCTION length(r RAW) RETURN NUMBER;
	FUNCTION overlay(overlay_str RAW, target RAW, pos BINARY_INTEGER DEFAULT 1, len BINARY_INTEGER DEFAULT NULL, pad RAW DEFAULT NULL) RETURN RAW;
	FUNCTION reverse(r RAW) RETURN RAW;
	FUNCTION substr(r RAW, pos BINARY_INTEGER, len BINARY_INTEGER DEFAULT NULL) RETURN RAW;
	FUNCTION translate(r RAW, from_set RAW, to_set RAW) RETURN RAW;
	FUNCTION transliterate(r RAW, to_set RAW DEFAULT NULL, from_set RAW DEFAULT NULL, pad RAW DEFAULT NULL) RETURN RAW;
	FUNCTION xrange(start_byte RAW DEFAULT NULL, end_byte RAW DEFAULT NULL) RETURN RAW;
END;

CREATE OR REPLACE PACKAGE BODY sys.utl_raw AS
	FUNCTION bit_and(r1 RAW, r2 RAW) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_bit_and(r1, r2);
	END;
	FUNCTION bit_complement(r RAW) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_bit_complement(r);
	END;
	FUNCTION bit_or(r1 RAW, r2 RAW) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_bit_or(r1, r2);
	END;
	FUNCTION bit_xor(r1 RAW, r2 RAW) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_bit_xor(r1, r2);
	END;
	-- cast from 
	FUNCTION cast_from_binary_double(n BINARY_DOUBLE, endianess PLS_INTEGER DEFAULT 1) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_cast_from_binary_double(n, endianess);
	END;
	FUNCTION cast_from_binary_float(n BINARY_FLOAT, endianess PLS_INTEGER DEFAULT 1) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_cast_from_binary_float(n, endianess);
	END;
	FUNCTION cast_from_binary_integer(n BINARY_INTEGER, endianess PLS_INTEGER DEFAULT 1) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_cast_from_binary_integer(n, endianess);
	END;
	FUNCTION cast_from_number(n NUMBER) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_cast_from_number(n);
	END;
	-- cast to
	FUNCTION cast_to_binary_double(r RAW, endianess PLS_INTEGER DEFAULT 1) RETURN BINARY_DOUBLE AS
		result BINARY_DOUBLE;
	BEGIN
		result := sys.utl_raw_cast_to_binary_double(r, endianess);
		if result = -0 THEN
			result := 0;
		END IF;
		RETURN result;
	END;
	FUNCTION cast_to_binary_float(r RAW, endianess PLS_INTEGER DEFAULT 1) RETURN BINARY_FLOAT AS
		result BINARY_FLOAT;
	BEGIN
		result := sys.utl_raw_cast_to_binary_float(r, endianess);
		IF result = -0 THEN
			result := 0;
		END IF;
		RETURN result;
	END;
	FUNCTION cast_to_binary_integer(r RAW, endianess PLS_INTEGER DEFAULT 1) RETURN BINARY_INTEGER AS
	BEGIN
		RETURN sys.utl_raw_cast_to_binary_integer(r, endianess);
	END;
	FUNCTION cast_to_number(r RAW) RETURN NUMBER AS
	BEGIN
		RETURN sys.utl_raw_cast_to_number(r);
	END;
	FUNCTION cast_to_nvarchar2(r RAW) RETURN NVARCHAR2 AS
		server_enc TEXT;
		result NVARCHAR2;
		raw_length INTEGER;
	BEGIN
		IF r is NULL THEN
			RETURN NULL;
		END IF;
		raw_length := PG_CATALOG.LENGTH(r);
		IF raw_length = 0 THEN
			RETURN NULL;
		END IF;
		IF raw_length > 32767 THEN
			-- ora-01489 without value_error
			RAISE EXCEPTION 'result of string concatenation is too long';
		END IF;
		server_enc := PG_CATALOG.CURRENT_SETTING('server_encoding');
		result := PG_CATALOG.CONVERT_FROM(r, server_enc);
		RETURN result;
	END;
	FUNCTION cast_to_raw(c VARCHAR2) RETURN RAW AS
		server_enc TEXT;
		result RAW;
		raw_length INTEGER;
	BEGIN
		IF c is NULL THEN
			RETURN NULL;
		END IF;
		server_enc := PG_CATALOG.CURRENT_SETTING('server_encoding');
		result := PG_CATALOG.CONVERT_TO(c, server_enc);
		raw_length := PG_CATALOG.LENGTH(result);
		IF raw_length = 0 THEN
			RETURN NULL;
		END IF;
		IF raw_length > 32767 THEN
			-- ora-06502 error info not match
			RAISE EXCEPTION 'PL/SQL: numeric or value error: raw variable length too long';
		END IF;
		RETURN result;
	END;
	FUNCTION cast_to_varchar2(r RAW) RETURN VARCHAR2 AS
		server_enc TEXT;
		result VARCHAR2;
		raw_length INTEGER;
	BEGIN
		IF r is NULL THEN
			RETURN NULL;
		END IF;
		raw_length := PG_CATALOG.LENGTH(r);
		IF raw_length = 0 THEN
			RETURN NULL;
		END IF;
		IF raw_length > 32767 THEN
			-- ora-01489 without value_error
			RAISE EXCEPTION 'result of string concatenation is too long';
		END IF;
		server_enc := PG_CATALOG.CURRENT_SETTING('server_encoding');
		result := PG_CATALOG.CONVERT_FROM(r, server_enc);
		RETURN result;
	END;
	FUNCTION compare(r1 IN RAW, r2 IN RAW, pad IN RAW DEFAULT NULL) RETURN NUMBER IS
        param1 BYTEA := r1;
        param2 BYTEA := r2;
        param_pad BYTEA;
        len_param1 INTEGER;
        len_param2 INTEGER;
        max_len INTEGER;
        byte_param1 INTEGER;
        byte_param2 INTEGER;
		byte_pad INTEGER;
    BEGIN
		IF (param1 IS NULL OR PG_CATALOG.LENGTH(param1) = 0) AND (param2 IS NULL OR PG_CATALOG.LENGTH(param2) = 0) THEN
            RETURN 0;
        END IF;
        IF COALESCE(PG_CATALOG.LENGTH(param1), 0) > 32767 OR COALESCE(PG_CATALOG.LENGTH(param2), 0) > 32767 THEN
            -- ora-01489 without value_error
			RAISE EXCEPTION 'result of string concatenation is too long';
        END IF;
        IF pad IS NULL or PG_CATALOG.LENGTH(pad) = 0 THEN
            param_pad := '\x00'::BYTEA;
        ELSE
            param_pad := PG_CATALOG.SUBSTR(pad, 1, 1);
        END IF;
		byte_pad := PG_CATALOG.GET_BYTE(param_pad, 0);
        len_param1 := COALESCE(PG_CATALOG.LENGTH(param1), 0);
        len_param2 := COALESCE(PG_CATALOG.LENGTH(param2), 0);
        max_len := GREATEST(len_param1, len_param2);

        FOR i IN 1..max_len LOOP
            IF param1 IS NOT NULL AND i <= len_param1 THEN
                byte_param1 := PG_CATALOG.GET_BYTE(param1, i - 1);
            ELSE
                byte_param1 := byte_pad;
            END IF;

            IF param2 IS NOT NULL AND i <= len_param2 THEN
                byte_param2 := PG_CATALOG.GET_BYTE(param2, i - 1);
            ELSE
                byte_param2 := byte_pad;
            END IF;

            IF byte_param1 != byte_param2 THEN
                RETURN i;
            END IF;
        END LOOP;
        RETURN 0;
    END;
	FUNCTION concat(r1 RAW DEFAULT NULL, r2 RAW DEFAULT NULL, r3 RAW DEFAULT NULL, r4 RAW DEFAULT NULL,
				r5 RAW DEFAULT NULL, r6 RAW DEFAULT NULL, r7 RAW DEFAULT NULL, r8 RAW DEFAULT NULL,
				r9 RAW DEFAULT NULL, r10 RAW DEFAULT NULL, r11 RAW DEFAULT NULL, r12 RAW DEFAULT NULL) RETURN RAW AS
		result_raw RAW;
		total_len INTEGER := 0;
		current_len INTEGER;
		param_array RAW[] := ARRAY[r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12];
	BEGIN
		FOR i in 1..12 LOOP
			IF param_array[i] IS NOT NULL THEN
				current_len := PG_CATALOG.LENGTH(param_array[i]);
				IF current_len = 0 THEN
					CONTINUE;
				END IF;
				IF current_len > 32767 THEN
					-- ora-01489 without value_error
					RAISE EXCEPTION 'result of string concatenation is too long';
				END IF;
				IF total_len + current_len > 32767 THEN
					-- ora-06502 error info not match
					RAISE EXCEPTION 'PL/SQL: numeric or value error: raw variable length too long';
				END IF;
				total_len := total_len + current_len;
				IF result_raw IS NULL THEN
					result_raw := param_array[i];
				ELSE
					result_raw := result_raw || param_array[i];
				END IF;
			END IF;
		END LOOP;
		RETURN result_raw;
	END;
	FUNCTION convert(r RAW, to_charset VARCHAR2, from_charset VARCHAR2) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_convert(r, to_charset, from_charset);
	END;
	FUNCTION copies(r RAW, n NUMBER) RETURN RAW AS
		raw_len INTEGER;
		copy_count INTEGER;
		result RAW;
	BEGIN
		IF r IS NULL or PG_CATALOG.LENGTH(r) = 0 THEN
			-- ora-06502 error info not match
			RAISE EXCEPTION 'numeric or value error';
		END IF;
		copy_count := PG_CATALOG.ROUND(n);
		IF copy_count < 1 THEN
			-- ora-06502 error info not match
			RAISE EXCEPTION 'numeric or value error';
		END IF;
		raw_len := PG_CATALOG.LENGTH(r);
		IF raw_len >= 32767 THEN
			-- ora-01489 without value_error
			RAISE EXCEPTION 'result of string concatenation is too long';
		END IF;
		IF raw_len * copy_count >= 32767 THEN
			-- ora-06502 error info not match
			RAISE EXCEPTION 'numeric or value error: raw variable length too long';
		END IF;
		result := r;
		FOR i in 2..copy_count LOOP
			result := result || r;
		END LOOP;
		RETURN result;
	END;
	FUNCTION length(r RAW) RETURN NUMBER AS
		raw_len NUMBER;
	BEGIN
		raw_len := PG_CATALOG.OCTET_LENGTH(r);
		IF raw_len >= 32767 THEN
			-- ora-01489 without value_error
			RAISE EXCEPTION 'result of string concatenation is too long';
		END IF;
		RETURN raw_len;
	END;
	FUNCTION overlay(overlay_str RAW, target RAW, pos BINARY_INTEGER DEFAULT 1, len BINARY_INTEGER DEFAULT NULL, pad RAW DEFAULT NULL) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_overlay(overlay_str, target, pos, len, pad);
	END;
	FUNCTION reverse(r RAW) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_reverse(r);
	END;
	FUNCTION substr(r RAW, pos BINARY_INTEGER, len BINARY_INTEGER DEFAULT NULL) RETURN RAW AS
		raw_len INTEGER;
		real_pos INTEGER;
		real_len INTEGER;
	BEGIN
		IF r IS NULL OR PG_CATALOG.LENGTH(r) = 0 THEN
			-- ora-06502 error info not match
			RAISE EXCEPTION 'numeric or value error';
		END IF;
		raw_len := PG_CATALOG.LENGTH(r);
		IF raw_len > 32767 THEN
			-- ora-01489 without value_error
			RAISE EXCEPTION 'result of string concatenation is too long';
		END IF;
		real_pos := pos;
		real_len := len;
		IF (pos > 0 and pos > raw_len) or (pos < 0 and -pos > raw_len) THEN
			-- ora-06502 error info not match
			RAISE EXCEPTION 'numeric or value error';
		END IF;
		IF pos = 0 THEN
			real_pos := 1;
		END IF;
		IF pos < 0 THEN
			real_pos := raw_len + pos + 1;
		END IF;
		IF len IS NULL THEN
			real_len := raw_len - real_pos + 1;
		END IF;
		IF real_len <= 0 THEN
			-- ora-06502 error info not match
			RAISE EXCEPTION 'numeric or value error';
		END IF;
		IF real_len > raw_len - real_pos + 1 THEN
			-- ora-06502 error info not match
			RAISE EXCEPTION 'numeric or value error';
		END IF;
		RETURN PG_CATALOG.SUBSTR(r, real_pos, real_len);
	END;
	FUNCTION translate(r RAW, from_set RAW, to_set RAW) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_translate(r, from_set, to_set);
	END;
	FUNCTION transliterate(r RAW, to_set RAW DEFAULT NULL, from_set RAW DEFAULT NULL, pad RAW DEFAULT NULL) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_transliterate(r, from_set, to_set, pad);
	END;
	FUNCTION xrange(start_byte RAW DEFAULT NULL, end_byte RAW DEFAULT NULL) RETURN RAW AS
	BEGIN
		RETURN sys.utl_raw_xrange(start_byte, end_byte);
	END;
END;

GRANT EXECUTE ON function sys.utl_raw_bit_and TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_bit_complement TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_bit_or TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_bit_xor TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_cast_from_binary_double TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_cast_from_binary_float TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_cast_from_binary_integer TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_cast_from_number TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_cast_to_binary_double TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_cast_to_binary_float TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_cast_to_binary_integer TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_cast_to_number TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_convert TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_overlay TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_translate TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_transliterate TO PUBLIC;
GRANT EXECUTE ON function sys.utl_raw_xrange TO PUBLIC;
GRANT EXECUTE ON package sys.utl_raw TO PUBLIC;
-- End - ReqID:SRS-PLSQL-UTL_RAW
