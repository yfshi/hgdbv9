# Package body replacement with no variables, public globals, and private globals.
# Each DDL step commits before the next step in the explicit permutation.
# The stateless s2_after error is a known compatibility gap in the current baseline.

teardown
{
  DROP PACKAGE iso_pkg_stateless;
  DROP PACKAGE iso_pkg_recompile;
  DROP PACKAGE iso_pkg_private;
}

session s1

# No package variables.
step stateless_create_spec
{
  CREATE PACKAGE iso_pkg_stateless AS
    FUNCTION get_value RETURN integer;
  END;
}
step stateless_create_body
{
  CREATE PACKAGE BODY iso_pkg_stateless AS
    FUNCTION get_value RETURN integer IS
    BEGIN
      RETURN 1;
    END;
  END;
}
step stateless_replace_body
{
  CREATE OR REPLACE PACKAGE BODY iso_pkg_stateless AS
    FUNCTION get_value RETURN integer IS
    BEGIN
      RETURN 10;
    END;
  END;
}
step stateless_s1_before { SELECT iso_pkg_stateless.get_value() AS value FROM dual; }
step stateless_s1_after { SELECT iso_pkg_stateless.get_value() AS value FROM dual; }
step stateless_s1_again { SELECT iso_pkg_stateless.get_value() AS value FROM dual; }

# Public global variable in the specification.
step public_create_spec
{
  CREATE PACKAGE iso_pkg_recompile AS
    counter integer := 0;
    FUNCTION next_value RETURN integer;
  END;
}
step public_create_body
{
  CREATE PACKAGE BODY iso_pkg_recompile AS
    FUNCTION next_value RETURN integer IS
    BEGIN
      counter := counter + 1;
      RETURN counter;
    END;
  END;
}
step public_replace_body
{
  CREATE OR REPLACE PACKAGE BODY iso_pkg_recompile AS
    FUNCTION next_value RETURN integer IS
    BEGIN
      counter := counter + 10;
      RETURN counter;
    END;
  END;
}
step public_s1_before { SELECT iso_pkg_recompile.next_value() AS value FROM dual; }
step public_s1_after { SELECT iso_pkg_recompile.next_value() AS value FROM dual; }
step public_s1_again { SELECT iso_pkg_recompile.next_value() AS value FROM dual; }

# Private global variable in the body.
step private_create_spec
{
  CREATE PACKAGE iso_pkg_private AS
    FUNCTION next_value RETURN integer;
  END;
}
step private_create_body
{
  CREATE PACKAGE BODY iso_pkg_private AS
    counter integer := 0;
    FUNCTION next_value RETURN integer IS
    BEGIN
      counter := counter + 1;
      RETURN counter;
    END;
  END;
}
step private_replace_body
{
  CREATE OR REPLACE PACKAGE BODY iso_pkg_private AS
    counter integer := 0;
    FUNCTION next_value RETURN integer IS
    BEGIN
      counter := counter + 10;
      RETURN counter;
    END;
  END;
}
step private_s1_before { SELECT iso_pkg_private.next_value() AS value FROM dual; }
step private_s1_after { SELECT iso_pkg_private.next_value() AS value FROM dual; }
step private_s1_again { SELECT iso_pkg_private.next_value() AS value FROM dual; }

session s2

# No package variables.
step stateless_s2_before { SELECT iso_pkg_stateless.get_value() AS value FROM dual; }
step stateless_s2_after { SELECT iso_pkg_stateless.get_value() AS value FROM dual; }
step stateless_s2_again { SELECT iso_pkg_stateless.get_value() AS value FROM dual; }

# Public global variable in the specification.
step public_s2_before { SELECT iso_pkg_recompile.next_value() AS value FROM dual; }
step public_s2_after { SELECT iso_pkg_recompile.next_value() AS value FROM dual; }
step public_s2_again { SELECT iso_pkg_recompile.next_value() AS value FROM dual; }

# Private global variable in the body.
step private_s2_before { SELECT iso_pkg_private.next_value() AS value FROM dual; }
step private_s2_after { SELECT iso_pkg_private.next_value() AS value FROM dual; }
step private_s2_again { SELECT iso_pkg_private.next_value() AS value FROM dual; }

permutation
  stateless_create_spec stateless_create_body stateless_s1_before stateless_s2_before stateless_replace_body stateless_s1_after stateless_s2_after stateless_s1_again stateless_s2_again
  public_create_spec public_create_body public_s1_before public_s2_before public_replace_body public_s1_after public_s2_after public_s1_again public_s2_again
  private_create_spec private_create_body private_s1_before private_s2_before private_replace_body private_s1_after private_s2_after private_s1_again private_s2_again
