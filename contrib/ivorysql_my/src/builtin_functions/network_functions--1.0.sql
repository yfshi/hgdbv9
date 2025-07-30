--
-- inet_aton
--
CREATE OR REPLACE FUNCTION ivymy.inet_aton(text)
RETURNS int8
AS 'MODULE_PATHNAME','inetaton'
LANGUAGE C
IMMUTABLE
STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet_aton(bool)
RETURNS int8
AS 'MODULE_PATHNAME','inetaton'
LANGUAGE C
IMMUTABLE STRICT;

--
-- inet_ntoa
--
CREATE OR REPLACE FUNCTION ivymy.inet_ntoa(int8)
RETURNS varchar
AS 'MODULE_PATHNAME','inetntoa'
LANGUAGE C
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet_ntoa(bool)
RETURNS varchar
AS 'MODULE_PATHNAME','inetntoa'
LANGUAGE C
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet_ntoa(bit)
RETURNS varchar
AS 'select ivymy.inet_ntoa(cast($1 as int8))'
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet_ntoa(binary)
RETURNS varchar
AS 'select ivymy.inet_ntoa(cast($1 as int8))'
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet_ntoa(tinyblob)
RETURNS varchar
AS 'select ivymy.inet_ntoa(cast($1 as int8))'
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet_ntoa(blob)
RETURNS varchar
AS 'select ivymy.inet_ntoa(cast($1 as int8))'
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet_ntoa(mediumblob)
RETURNS varchar
AS 'select ivymy.inet_ntoa(cast($1 as int8))'
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet_ntoa(longblob)
RETURNS varchar
AS 'select ivymy.inet_ntoa(cast($1 as int8))'
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet_ntoa(year)
RETURNS varchar
AS 'select ivymy.inet_ntoa(cast($1 as int8))'
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet_ntoa(json)
RETURNS varchar
AS 'select ivymy.inet_ntoa(cast($1 as int8))'
LANGUAGE SQL
IMMUTABLE STRICT;

--
-- inet6_aton
--
CREATE OR REPLACE FUNCTION ivymy.inet6_aton(text)
RETURNS bytea
AS 'MODULE_PATHNAME','inetpton'
LANGUAGE C
IMMUTABLE STRICT;

--
-- inet6_ntoa
--
CREATE OR REPLACE FUNCTION ivymy.inet6_ntoa(text)
RETURNS varchar
AS 'MODULE_PATHNAME','inetntop'
LANGUAGE C
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet6_ntoa(bytea)
RETURNS varchar
AS 'MODULE_PATHNAME','inetntop'
LANGUAGE C
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet6_ntoa(year)
RETURNS varchar
AS 'MODULE_PATHNAME',  'inetntop'
LANGUAGE C
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet6_ntoa(json)
RETURNS varchar
AS 'MODULE_PATHNAME',  'inetntop'
LANGUAGE C
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet6_ntoa(bit)
RETURNS varchar
AS 'select ivymy.inet6_ntoa(cast($1 as text))'
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet6_ntoa(boolean)
RETURNS varchar
AS 'select ivymy.inet6_ntoa(cast($1 as text))'
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet6_ntoa(blob)
RETURNS varchar
AS 'select ivymy.inet6_ntoa(cast($1 as text))'
LANGUAGE SQL
IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION ivymy.inet6_ntoa(anyenum)
RETURNS varchar
AS 'select ivymy.inet6_ntoa(cast($1 as text))'
LANGUAGE SQL
IMMUTABLE STRICT;
