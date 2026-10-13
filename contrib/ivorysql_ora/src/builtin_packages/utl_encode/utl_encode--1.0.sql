-- Begin - ReqID:SRS-PLSQL-UTL_ENCODE
create or replace function sys.utl_encode_base64_decode(RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_encode_base64_decode' LANGUAGE C;
create or replace function sys.utl_encode_base64_encode(RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_encode_base64_encode' LANGUAGE C;
create or replace function sys.utl_encode_mimeheader_decode(VARCHAR2) RETURN VARCHAR2
	AS 'MODULE_PATHNAME', 'utl_encode_mimeheader_decode' LANGUAGE C;
create or replace function sys.utl_encode_mimeheader_encode(VARCHAR2, VARCHAR2, PLS_INTEGER) RETURN VARCHAR2
	AS 'MODULE_PATHNAME', 'utl_encode_mimeheader_encode' LANGUAGE C;
create or replace function sys.utl_encode_quoted_printable_decode(RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_encode_quoted_printable_decode' LANGUAGE C;
create or replace function sys.utl_encode_quoted_printable_encode(RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_encode_quoted_printable_encode' LANGUAGE C;
create or replace function sys.utl_encode_text_decode(VARCHAR2, VARCHAR2, PLS_INTEGER) RETURN VARCHAR2
	AS 'MODULE_PATHNAME', 'utl_encode_text_decode' LANGUAGE C;
create or replace function sys.utl_encode_text_encode(VARCHAR2, VARCHAR2, PLS_INTEGER) RETURN VARCHAR2
	AS 'MODULE_PATHNAME', 'utl_encode_text_encode' LANGUAGE C;
create or replace function sys.utl_encode_uudecode(RAW) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_encode_uudecode' LANGUAGE C;
create or replace function sys.utl_encode_uuencode(RAW, PLS_INTEGER, VARCHAR2, VARCHAR2) RETURN RAW
	AS 'MODULE_PATHNAME', 'utl_encode_uuencode' LANGUAGE C;

CREATE OR REPLACE PACKAGE sys.utl_encode AS
	--------------
	-- CONSTANTS
	--------------
	-- Define constants for use by uuencode's 2nd file type parameter
	complete         CONSTANT PLS_INTEGER := 1; -- includes header and footer
	header_piece     CONSTANT PLS_INTEGER := 2; -- includes header text
	middle_piece     CONSTANT PLS_INTEGER := 3; -- body text only
	end_piece        CONSTANT PLS_INTEGER := 4; -- includes footer text
	-- Define constants for use by text_encode/decode and mimeheader_encode
	-- in the 'encoding' parameter
	base64           CONSTANT PLS_INTEGER := 1;
	quoted_printable CONSTANT PLS_INTEGER := 2;

	-- base64 encode/decode
	FUNCTION base64_decode(r IN RAW) RETURN RAW;
	FUNCTION base64_encode(r IN RAW) RETURN RAW;

	-- mimeheader encode/decode
	FUNCTION mimeheader_decode(buf IN VARCHAR2) RETURN VARCHAR2;
	FUNCTION mimeheader_encode(buf IN VARCHAR2,
							encode_charset IN VARCHAR2 DEFAULT NULL,
							encoding IN PLS_INTEGER DEFAULT NULL) RETURN VARCHAR2;

	-- quoted_printable encode/decode
	FUNCTION quoted_printable_decode(r IN RAW) RETURN RAW;
	FUNCTION quoted_printable_encode(r IN RAW) RETURN RAW;

	-- text encode/decode
	FUNCTION text_decode(buf IN VARCHAR2,
						encode_charset IN VARCHAR2 DEFAULT NULL,
						encoding IN PLS_INTEGER DEFAULT NULL) RETURN VARCHAR2;
	FUNCTION text_encode(buf IN VARCHAR2,
						encode_charset IN VARCHAR2 DEFAULT NULL,
						encoding IN PLS_INTEGER DEFAULT NULL) RETURN VARCHAR2;

	-- uu encode/decode
	FUNCTION uudecode(r IN RAW) RETURN RAW;
	FUNCTION uuencode(r IN RAW,
					type IN PLS_INTEGER DEFAULT complete,
					filename VARCHAR2 DEFAULT 'uuencode.txt',
					permission VARCHAR2 DEFAULT '0') RETURN RAW;
END;

CREATE OR REPLACE PACKAGE BODY sys.utl_encode AS
	-- base64
	FUNCTION base64_decode(r IN RAW) RETURN RAW AS
	BEGIN
		RETURN sys.utl_encode_base64_decode(r);
	END;
	FUNCTION base64_encode(r IN RAW) RETURN RAW AS
	BEGIN
		RETURN sys.utl_encode_base64_encode(r);
	END;

	-- mimeheader
	FUNCTION mimeheader_decode(buf IN VARCHAR2) RETURN VARCHAR2 AS
	BEGIN
		RETURN sys.utl_encode_mimeheader_decode(buf);
	END;
	FUNCTION mimeheader_encode(buf IN VARCHAR2,
							encode_charset IN VARCHAR2 DEFAULT NULL,
							encoding IN PLS_INTEGER DEFAULT NULL) RETURN VARCHAR2 AS
	BEGIN
		RETURN sys.utl_encode_mimeheader_encode(buf, encode_charset, encoding);
	END;

	-- quoted_printable
	FUNCTION quoted_printable_decode(r IN RAW) RETURN RAW AS
	BEGIN
		RETURN sys.utl_encode_quoted_printable_decode(r);
	END;
	FUNCTION quoted_printable_encode(r IN RAW) RETURN RAW AS
	BEGIN
		RETURN sys.utl_encode_quoted_printable_encode(r);
	END;

	-- text
	FUNCTION text_decode(buf IN VARCHAR2,
									encode_charset IN VARCHAR2 DEFAULT NULL,
									encoding IN PLS_INTEGER DEFAULT NULL) RETURN VARCHAR2 AS
	BEGIN
		RETURN sys.utl_encode_text_decode(buf, encode_charset, encoding);
	END;
	FUNCTION text_encode(buf IN VARCHAR2,
									encode_charset IN VARCHAR2 DEFAULT NULL,
									encoding IN PLS_INTEGER DEFAULT NULL) RETURN VARCHAR2 AS
	BEGIN
		RETURN sys.utl_encode_text_encode(buf, encode_charset, encoding);
	END;

	-- uu
	FUNCTION uudecode(r IN RAW) RETURN RAW AS
	BEGIN
		RETURN sys.utl_encode_uudecode(r);
	END;
	FUNCTION uuencode(r IN RAW,
					type IN PLS_INTEGER DEFAULT complete,
					filename VARCHAR2 DEFAULT 'uuencode.txt',
					permission VARCHAR2 DEFAULT '0') RETURN RAW AS
	BEGIN
		RETURN sys.utl_encode_uuencode(r, type, filename, permission);
	END;
END;

GRANT EXECUTE ON function sys.utl_encode_base64_encode TO PUBLIC;
GRANT EXECUTE ON function sys.utl_encode_base64_decode TO PUBLIC;
GRANT EXECUTE ON function sys.utl_encode_mimeheader_decode TO PUBLIC;
GRANT EXECUTE ON function sys.utl_encode_mimeheader_encode TO PUBLIC;
GRANT EXECUTE ON function sys.utl_encode_quoted_printable_decode TO PUBLIC;
GRANT EXECUTE ON function sys.utl_encode_quoted_printable_encode TO PUBLIC;
GRANT EXECUTE ON function sys.utl_encode_text_decode TO PUBLIC;
GRANT EXECUTE ON function sys.utl_encode_text_encode TO PUBLIC;
GRANT EXECUTE ON function sys.utl_encode_uudecode TO PUBLIC;
GRANT EXECUTE ON function sys.utl_encode_uuencode TO PUBLIC;
GRANT EXECUTE ON package sys.utl_encode TO PUBLIC;
-- End - ReqID:SRS-PLSQL-UTL_ENCODE