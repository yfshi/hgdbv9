SET plisql.plisql_default_into_strict = false;

-- ============================================================================
-- [definition-time/runtime] basic member/static object behavior
-- ============================================================================
create type ot_person as object (
  name varchar2(30),
  age integer,
  member function get_age return integer deterministic,
  member function describe return varchar2(100),
  member function add_years(delta integer) return ot_person,
  member function calc_age(delta integer, next_age out integer) return integer,
  static function build_person(p_name varchar2(30), p_age integer) return ot_person
);

create type body ot_person as
  member function get_age return integer deterministic is
  begin
    return self.age;
  end;

  member function describe return varchar2(100) is
  begin
    return self.name || ':' || self.age;
  end;

  member function add_years(delta integer) return ot_person is
  begin
    return ot_person(self.name, self.age + delta);
  end;

  member function calc_age(delta integer, next_age out integer) return integer is
  begin
    next_age := self.age + delta;
    return next_age;
  end;

  static function build_person(p_name varchar2(30), p_age integer) return ot_person is
  begin
    return ot_person(p_name, p_age);
  end;
end;
/

-- [runtime] Exercise attribute assignment, member calls, chaining, OUT arguments,
-- explicit constructor calls, and static factory calls.
declare
  p ot_person;
  q ot_person;
  next_age integer;
  ret integer;
begin
  p.name := 'Alice';
  p.age := 19;
  raise notice 'age=%', p.get_age();
  raise notice 'desc=%', p.describe();
  q := p.add_years(2);
  raise notice 'method_desc=%', q.describe();
  raise notice 'chain_desc=%', p.add_years(2).describe();
  ret := p.calc_age(3, next_age);
  raise notice 'ret=%', ret;
  raise notice 'next_age=%', next_age;
  q := ot_person('Bob', 20);
  raise notice 'ctor_desc=%', q.describe();
  q := ot_person.build_person('Bob', 20);
  raise notice 'static_desc=%', q.describe();
end;
/

-- [definition-time/runtime] Store object values in table columns and read them
-- back through SQL and PL/iSQL.
create table ot_person_tab (
  id integer,
  info ot_person
);

insert into ot_person_tab values (1, ot_person('Dora', 28));
insert into ot_person_tab values (2, ot_person.build_person('Evan', 35));

select info.name, info.age from ot_person_tab order by id;
select info.get_age() from ot_person_tab order by id;

declare
  p ot_person;
begin
  select info into p from ot_person_tab where id = 1;
  raise notice 'table_desc=%', p.describe();
  update ot_person_tab
  set info = ot_person(info.name, info.age + 1)
  where id = 1;
  select info into p from ot_person_tab where id = 1;
  raise notice 'table_age=%', p.get_age();
end;
/

drop table ot_person_tab;
-- The object-type catalog rows and body are internal parts of the type, so a
-- standalone object type must not require CASCADE.
drop type ot_person;

-- ============================================================================
-- [definition-time] procedure declarations and body validation
-- ============================================================================
create type ot_counter as object (
  value integer,
  member procedure bump(delta integer),
  static procedure log_value(p_value integer)
);

create type body ot_counter as
  member procedure bump(delta integer) is
  begin
    self.value := self.value + delta;
  end;

  static procedure log_value(p_value integer) is
  begin
    raise notice 'static_proc=%', p_value;
  end;
end;
/

-- [runtime] Verify CALL-form dispatch for static and member procedures.
call ot_counter.log_value(5);

declare
  c ot_counter;
begin
  c := ot_counter(10);
  call c.bump(3);
  raise notice 'member_proc_value=%', c.value;
  call ot_counter.log_value(c.value);
end;
/

drop type ot_counter cascade;

-- ============================================================================
-- [definition-time/runtime] explicit constructor declarations and overloads
-- ============================================================================
create type ot_constructor_person as object (
  name varchar2(30),
  age integer,
  constructor function ot_constructor_person return self as result,
  constructor function ot_constructor_person(p_name varchar2(30)) return self as result,
  constructor function ot_constructor_person(p_name varchar2(30), p_age integer) return self as result,
  member function describe return varchar2(100)
);

create type body ot_constructor_person as
  constructor function ot_constructor_person return self as result is
  begin
    self.name := 'N/A';
    self.age := 0;
    return;
  end;

  constructor function ot_constructor_person(p_name varchar2(30)) return self as result is
  begin
    self.name := p_name;
    self.age := 18;
    return;
  end;

  constructor function ot_constructor_person(p_name varchar2(30), p_age integer) return self as result is
  begin
    self.name := p_name;
    self.age := p_age;
    return;
  end;

  member function describe return varchar2(100) is
  begin
    return self.name || ':' || self.age;
  end;
end;
/

declare
  p ot_constructor_person;
begin
  p := ot_constructor_person();
  raise notice 'ctor0=%', p.describe();
  p := ot_constructor_person('Tom');
  raise notice 'ctor1=%', p.describe();
  p := ot_constructor_person('Tom', 20);
  raise notice 'ctor2=%', p.describe();
  p := ot_constructor_person(p_name => 'Neo', p_age => 21);
  raise notice 'ctor_named=%', p.describe();
  p := ot_constructor_person('Mix', p_age => 22);
  raise notice 'ctor_mixed=%', p.describe();
  raise notice 'ctor_chain=%', ot_constructor_person(p_name => 'Chain', p_age => 23).describe();
end;
/

-- ============================================================================
-- [bind-time/runtime] default constructor fallback and named/mixed arguments
-- ============================================================================
create type ot_default_ctor_person as object (
  name varchar2(30),
  age integer,
  member function describe return varchar2(100)
);

create type body ot_default_ctor_person as
  member function describe return varchar2(100) is
  begin
    return self.name || ':' || self.age;
  end;
end;
/

declare
  p ot_default_ctor_person;
begin
  p := ot_default_ctor_person('Fallback', 99);
  raise notice 'default_ctor=%', p.describe();
  p := ot_default_ctor_person(name => 'Named', age => 100);
  raise notice 'default_ctor_named=%', p.describe();
  p := ot_default_ctor_person('Mixed', age => 101);
  raise notice 'default_ctor_mixed=%', p.describe();
end;
/

-- ============================================================================
-- [definition-time] body/spec consistency checks
-- ============================================================================

-- [definition-time] A body must implement every declared method.
create type ot_missing as object (
  id integer,
  member function get_id return integer,
  static function build_missing(p_id integer) return ot_missing
);

create type body ot_missing as
  member function get_id return integer is
  begin
    return self.id;
  end;
end;
/

drop type ot_missing cascade;

-- [definition-time] A body method signature must match the specification.
create type ot_mismatch as object (
  id integer,
  member function get_id return integer
);

create type body ot_mismatch as
  member function get_id return varchar2(30) is
  begin
    return self.id;
  end;
end;
/

drop type ot_mismatch cascade;

-- ============================================================================
-- [definition-time] Resolve same-name member/static methods successfully
-- ============================================================================
create type ot_dispatch as object (
  value integer,
  member function label return varchar2(30),
  static function label(p_value integer) return varchar2(30)
);

create type body ot_dispatch as
  member function label return varchar2(30) is
  begin
    return 'member=' || self.value;
  end;

  static function label(p_value integer) return varchar2(30) is
  begin
    return 'static=' || p_value;
  end;
end;
/

-- [runtime] Call the member through an object value and the static method
-- through the type name to verify the binder picks the correct receiver model.
declare
  d ot_dispatch;
begin
  d := ot_dispatch(7);
  raise notice 'dispatch_member=%', d.label();
  raise notice 'dispatch_static=%', ot_dispatch.label(8);
end;
/

-- [runtime] Static functions should also bind correctly in plain SQL.
select ot_dispatch.label(9);

drop type ot_dispatch cascade;

-- ============================================================================
-- [definition-time] Object copying semantics, SQL/PL consistency, and table
-- column usage
-- ============================================================================
create type ot_behavior_person as object (
  name varchar2(30),
  age integer,
  member function get_name return varchar2(30),
  member function get_age return integer,
  member function add_years(delta integer) return ot_behavior_person,
  member function age_or_minus1 return integer,
  static function from_values(p_name varchar2(30), p_age integer) return ot_behavior_person,
  static function scalar_tag(p_name varchar2(30)) return varchar2(40)
);

create type body ot_behavior_person as
  member function get_name return varchar2(30) is
  begin
    return self.name;
  end;

  member function get_age return integer is
  begin
    return self.age;
  end;

  member function add_years(delta integer) return ot_behavior_person is
  begin
    return ot_behavior_person(self.name, self.age + delta);
  end;

  member function age_or_minus1 return integer is
  begin
    if self.age is null then
      return -1;
    end if;
    return self.age;
  end;

  static function from_values(p_name varchar2(30), p_age integer) return ot_behavior_person is
  begin
    return ot_behavior_person(p_name, p_age);
  end;

  static function scalar_tag(p_name varchar2(30)) return varchar2(40) is
  begin
    return 'tag=' || p_name;
  end;
end;
/

-- [runtime] Verify copy-by-value behavior, methods returning new objects, NULL
-- attributes, and method dispatch on a NULL object variable.
declare
  a ot_behavior_person;
  b ot_behavior_person;
  c ot_behavior_person;
  n ot_behavior_person;
begin
  a := ot_behavior_person('Ann', 10);
  b := a;
  c := a.add_years(2);
  raise notice 'name=%', a.get_name();
  raise notice 'copy_name=%', b.get_name();
  raise notice 'orig_age=%', a.get_age();
  raise notice 'new_age=%', c.get_age();
  if ot_behavior_person(null, null).get_name() is null then
    raise notice 'null_attr_name_is_null';
  end if;
  raise notice 'null_attr_age=%', ot_behavior_person('Ann', null).age_or_minus1();

  begin
    if n.get_name() is null then
      raise notice 'null_obj_call_is_null';
    end if;
  exception
    when others then
      raise notice 'null_obj_call_error';
  end;
end;
/

-- [runtime] Keep SQL and PL/iSQL method dispatch behavior aligned.
select ot_behavior_person('Sql', 11).get_age();
select ot_behavior_person.from_values('Sql', 11).get_age();
select ot_behavior_person('Sql', 11).add_years(1).get_age();
select ot_behavior_person.from_values('Sql', 11).add_years(2).get_age();
select ot_behavior_person.scalar_tag(ot_behavior_person('Sql', 11).get_name());
select ot_behavior_person.scalar_tag('Sql');

declare
  p ot_behavior_person;
begin
  p := ot_behavior_person.from_values('Sql', 11);
  raise notice 'pl_age=%', p.get_age();
  raise notice 'pl_scalar=%', ot_behavior_person.scalar_tag('Sql');
  raise notice 'pl_nested_scalar=%', ot_behavior_person.scalar_tag(p.get_name());
  raise notice 'pl_chain=%', p.add_years(1).get_age();
  raise notice 'pl_ctor_chain=%', ot_behavior_person('Sql', 11).add_years(1).get_age();
  raise notice 'pl_factory_chain=%', ot_behavior_person.from_values('Sql', 11).add_years(2).get_age();
end;
/

-- [definition-time] Store object values in table columns.
create table ot_behavior_tab (
  id integer,
  info ot_behavior_person
);

-- [runtime] Populate table rows through the default constructor, a static
-- factory, a NULL object value, and partially NULL attributes.
insert into ot_behavior_tab values (1, ot_behavior_person('Tom', 20));
insert into ot_behavior_tab values (2, ot_behavior_person.from_values('Jim', 21));
insert into ot_behavior_tab values (3, null);
insert into ot_behavior_tab values (4, ot_behavior_person(null, 22));

-- [runtime] Read attributes directly and invoke object methods from SQL.
select id, info.name, info.age from ot_behavior_tab order by id;
select id, info.get_age() from ot_behavior_tab where info is not null order by id;
select id, info.add_years(5).get_age() from ot_behavior_tab where info is not null order by id;

-- [runtime] Round-trip object values through SELECT INTO and UPDATE, then
-- verify that later method calls still see the rewritten object value.
declare
  p ot_behavior_person;
begin
  select info into p from ot_behavior_tab where id = 1;
  raise notice 'table_before=%', p.get_age();
  update ot_behavior_tab
  set info = ot_behavior_person(info.name, info.age + 1)
  where id = 1;
  select info into p from ot_behavior_tab where id = 1;
  raise notice 'table_after=%', p.get_age();
end;
/

-- [parse-time] Reject access to a missing attribute.
select info.missing_attr from ot_behavior_tab where id = 1;

-- [bind-time] Reject a missing member method.
select ot_behavior_person('Bad', 1).missing_method();

-- [bind-time] Reject a missing static method.
select ot_behavior_person.missing_static(1);

-- [bind-time] Reject wrong argument counts.
select ot_behavior_person('Bad', 1).add_years();

-- [bind-time] Reject wrong argument types.
select ot_behavior_person('Bad', 1).add_years('oops');

drop table ot_behavior_tab;
drop type ot_behavior_person cascade;

-- ============================================================================
-- [definition-time] Type body, declaration, and constructor error handling
-- ============================================================================

-- [definition-time] A body cannot implement methods that were never declared.
create type ot_extra_body as object (
  id integer,
  member function get_id return integer
);

create type body ot_extra_body as
  member function get_id return integer is
  begin
    return self.id;
  end;

  static function extra return integer is
  begin
    return 1;
  end;
end;
/

drop type ot_extra_body cascade;

-- [definition-time] Duplicate method declarations with the same signature must fail.
create type ot_dup_decl as object (
  id integer,
  member function get_id return integer,
  member function get_id return integer
);

-- [definition-time] Signature equivalence is based on types/modes, not
-- parameter spelling, so these declarations must still conflict.
create type ot_equiv_decl as object (
  id integer,
  member function bump(delta integer) return integer,
  member function bump(step integer) return integer
);

-- [definition-time] Duplicate constructor declarations with equivalent
-- signatures must fail as well.
create type ot_dup_ctor_decl as object (
  id integer,
  constructor function ot_dup_ctor_decl(p_id integer) return self as result,
  constructor function ot_dup_ctor_decl(id_value integer) return self as result
);

-- ============================================================================
-- [bind-time/runtime] Default-constructor and overloaded-constructor edges
-- ============================================================================

-- [runtime] Named and mixed notation should work for the default constructor.
select ot_default_ctor_person(name => 'SQLNamed', age => 200).describe();
select ot_default_ctor_person('SQLMixed', age => 201).describe();

-- [bind-time] Reject too few arguments for the default constructor.
select ot_default_ctor_person('OnlyName');

-- [bind-time] Reject too many arguments for the default constructor.
select ot_default_ctor_person('Too', 1, 2);

-- [bind-time] Reject unknown argument names for the default constructor.
select ot_default_ctor_person(p_name => 'Bad', age => 1);

-- [bind-time] Reject duplicate bindings for the same constructor attribute.
select ot_default_ctor_person('Dup', name => 'Again', age => 1);

-- [parse-time] Keep PostgreSQL's named-argument ordering rule for constructor calls.
select ot_default_ctor_person(name => 'OutOfOrder', 1);

-- [bind-time] Overloaded constructors should honor named and mixed notation.
select ot_constructor_person(p_name => 'SQLNamed', p_age => 30).describe();
select ot_constructor_person('SQLMixed', p_age => 31).describe();

-- [bind-time] If no declared constructor matches, fall back to the default
-- composite-style constructor by attribute name.
select ot_constructor_person(name => 'FallbackCtor', age => 32).describe();

-- [bind-time] Reject too few arguments for overloaded constructors.
select ot_constructor_person(p_age => 33);

-- [bind-time] Reject too many arguments for overloaded constructors.
select ot_constructor_person('Too', 1, 2);

-- [bind-time] Reject unknown argument names for overloaded constructors.
select ot_constructor_person(unknown_arg => 'Bad');

-- [bind-time] Reject duplicate bindings for overloaded constructors.
select ot_constructor_person('Dup', p_name => 'Again');

drop type ot_default_ctor_person cascade;
drop type ot_constructor_person cascade;

-- ============================================================================
-- [future tests] Implicit attribute access without writing self.
-- This is not in the stable regression suite yet; promote it only after the
-- behavior is confirmed to be stable.
-- create type ot_future_implicit as object (
--   age integer,
--   member function get_age_implicit return integer
-- );
-- create type body ot_future_implicit as
--   member function get_age_implicit return integer is
--   begin
--     return age;
--   end;
-- end;
-- /

-- ============================================================================
-- [definition-time/runtime] create / drop / recreate stability
-- ============================================================================
create type ot_recreate as object (
  id integer,
  member function get_id return integer
);

create type body ot_recreate as
  member function get_id return integer is
  begin
    return self.id;
  end;
end;
/

declare
  r ot_recreate;
begin
  r := ot_recreate(1);
  raise notice 'recreate_first=%', r.get_id();
end;
/

drop type ot_recreate cascade;

create type ot_recreate as object (
  id integer,
  member function get_id return integer
);

create type body ot_recreate as
  member function get_id return integer is
  begin
    return self.id;
  end;
end;
/

declare
  r ot_recreate;
begin
  r := ot_recreate(2);
  raise notice 'recreate_second=%', r.get_id();
end;
/

drop type ot_recreate cascade;

-- ============================================================================
-- [definition-time/runtime] UNDER, FINAL / NOT FINAL, and OVERRIDING
-- ============================================================================
create type ot_inherit_base as object (
  name varchar2(30),
  final member function final_name return varchar2(30),
  member function describe return varchar2(60)
) not final;

create type body ot_inherit_base as
  final member function final_name return varchar2(30) is
  begin
    return self.name;
  end;

  member function describe return varchar2(60) is
  begin
    return 'base=' || self.name;
  end;
end;
/

create type ot_inherit_child under ot_inherit_base as object (
  empno integer,
  overriding member function describe return varchar2(60)
) not final;

create type body ot_inherit_child as
  overriding member function describe return varchar2(60) is
  begin
    return 'child=' || self.name || ':' || self.empno;
  end;
end;
/

declare
  c ot_inherit_child;
begin
  c := ot_inherit_child('Alice', 7);
  raise notice 'inherit_override=%', c.describe();
  raise notice 'inherit_final_method=%', c.final_name();
  raise notice 'inherit_generalized=%', (c as ot_inherit_base).describe();
  raise notice 'inherit_generalized_static=%',
    ot_inherit_base.describe((c as ot_inherit_base));
end;
/

select (ot_inherit_child('Bob', 11) as ot_inherit_base).describe();
select ot_inherit_base.describe((ot_inherit_child('Cara', 12) as ot_inherit_base));

create type ot_inherit_grandchild under ot_inherit_child as object (
  deptno integer
);

create type body ot_inherit_grandchild as
end;
/

declare
  g ot_inherit_grandchild;
begin
  g := ot_inherit_grandchild('Eve', 8, 9);
  raise notice 'inherit_chain=%', g.describe();
  raise notice 'inherit_chain_parent=%', g.final_name();
end;
/

drop type ot_inherit_base cascade;

create type ot_final_root as object (
  name varchar2(30)
) final;

create type ot_bad_child under ot_final_root as object (
  empno integer
);

drop type ot_final_root cascade;

create type ot_override_missing as object (
  id integer
) not final;

create type body ot_override_missing as
end;
/

create type ot_override_missing_child under ot_override_missing as object (
  overriding member function describe return varchar2(30)
);

drop type ot_override_missing cascade;

create type ot_override_final_base as object (
  name varchar2(30),
  final member function describe return varchar2(30)
) not final;

create type body ot_override_final_base as
  final member function describe return varchar2(30) is
  begin
    return self.name;
  end;
end;
/

create type ot_override_final_child under ot_override_final_base as object (
  overriding member function describe return varchar2(30)
);

drop type ot_override_final_base cascade;

create type ot_override_required_base as object (
  name varchar2(30),
  member function describe return varchar2(30)
) not final;

create type body ot_override_required_base as
  member function describe return varchar2(30) is
  begin
    return self.name;
  end;
end;
/

create type ot_override_required_child under ot_override_required_base as object (
  member function describe return varchar2(30)
);

drop type ot_override_required_base cascade;

create type ot_kind_conflict_member_base as object (
  member function label return integer
) not final;

create type ot_kind_conflict_static_child under ot_kind_conflict_member_base as object (
  static function label return integer
);

drop type ot_kind_conflict_member_base cascade;

create type ot_kind_conflict_static_base as object (
  static function label return integer
) not final;

create type ot_kind_conflict_member_child under ot_kind_conflict_static_base as object (
  member function label return integer
);

drop type ot_kind_conflict_static_base cascade;

-- ============================================================================
-- [definition-time/runtime] override + overload combination
-- ============================================================================
create type ot_over_combo_base as object (
  name varchar2(30),
  member function pick(x integer) return varchar2(40),
  member function pick(x varchar2(30)) return varchar2(40)
) not final;

create type body ot_over_combo_base as
  member function pick(x integer) return varchar2(40) is
  begin
    return 'base-int=' || self.name || ':' || x;
  end;

  member function pick(x varchar2(30)) return varchar2(40) is
  begin
    return 'base-str=' || self.name || ':' || x;
  end;
end;
/

create type ot_over_combo_child under ot_over_combo_base as object (
  overriding member function pick(x integer) return varchar2(40)
);

create type body ot_over_combo_child as
  overriding member function pick(x integer) return varchar2(40) is
  begin
    return 'child-int=' || self.name || ':' || x;
  end;
end;
/

select ot_over_combo_child('Sql').pick(7);
select ot_over_combo_child('Sql').pick('x');

declare
  c ot_over_combo_child;
begin
  c := ot_over_combo_child('Pl');
  raise notice 'over_combo_int=%', c.pick(8);
  raise notice 'over_combo_str=%', c.pick('y');
end;
/

drop type ot_over_combo_base cascade;

-- ============================================================================
-- [definition-time/runtime] multi-level inheritance stability
-- ============================================================================
create type ot_chain_base as object (
  name varchar2(30),
  member function describe return varchar2(80),
  member function base_only return varchar2(80),
  final member function root_final return varchar2(80)
) not final;

create type body ot_chain_base as
  member function describe return varchar2(80) is
  begin
    return 'base=' || self.name;
  end;

  member function base_only return varchar2(80) is
  begin
    return 'base-only=' || self.name;
  end;

  final member function root_final return varchar2(80) is
  begin
    return 'root-final=' || self.name;
  end;
end;
/

create type ot_chain_child under ot_chain_base as object (
  team varchar2(30),
  overriding member function describe return varchar2(80),
  member function child_only return varchar2(80)
) not final;

create type body ot_chain_child as
  overriding member function describe return varchar2(80) is
  begin
    return 'child=' || self.name || ':' || self.team;
  end;

  member function child_only return varchar2(80) is
  begin
    return 'child-only=' || self.team;
  end;
end;
/

create type ot_chain_grandchild under ot_chain_child as object (
  seqno integer,
  overriding member function describe return varchar2(80),
  member function grand_only return varchar2(80)
);

create type body ot_chain_grandchild as
  overriding member function describe return varchar2(80) is
  begin
    return 'grand=' || self.name || ':' || self.team || ':' || self.seqno;
  end;

  member function grand_only return varchar2(80) is
  begin
    return 'grand-only=' || self.seqno;
  end;
end;
/

select ot_chain_grandchild('Sql', 'ops', 3).describe();
select ot_chain_grandchild('Sql', 'ops', 3).base_only();
select ot_chain_grandchild('Sql', 'ops', 3).child_only();
select ot_chain_base.describe((ot_chain_grandchild('Cast', 'qa', 4) as ot_chain_base));
select ot_chain_child.describe((ot_chain_grandchild('Cast', 'qa', 4) as ot_chain_child));

declare
  g ot_chain_grandchild;
begin
  g := ot_chain_grandchild('Pl', 'dev', 5);
  raise notice 'chain_multi_override=%', g.describe();
  raise notice 'chain_base_visible=%', g.base_only();
  raise notice 'chain_child_visible=%', g.child_only();
  raise notice 'chain_grand_visible=%', g.grand_only();
  raise notice 'chain_root_final=%', g.root_final();
end;
/

create type ot_chain_final_mid under ot_chain_base as object (
  code integer
) final;

create type ot_chain_bad_type under ot_chain_final_mid as object (
  flag integer
);

create type ot_chain_bad_final_grand under ot_chain_child as object (
  overriding member function root_final return varchar2(80)
);

drop type ot_chain_base cascade;

-- ============================================================================
-- [definition-time/runtime] constructor + inheritance
-- ============================================================================
create type ot_ctor_parent as object (
  name varchar2(30),
  code integer,
  member function describe return varchar2(80)
) not final;

create type body ot_ctor_parent as
  member function describe return varchar2(80) is
  begin
    return self.name || ':' || self.code;
  end;
end;
/

create type ot_ctor_child under ot_ctor_parent as object (
  age integer,
  member function child_desc return varchar2(80)
);

create type body ot_ctor_child as
  member function child_desc return varchar2(80) is
  begin
    return self.describe() || ':' || self.age;
  end;
end;
/

select ot_ctor_child('Sql', 7, 10).child_desc();
select ot_ctor_child(name => 'Named', code => 8, age => 11).child_desc();
select ot_ctor_child('Mixed', code => 9, age => 12).describe();

declare
  c ot_ctor_child;
begin
  c := ot_ctor_child('Pl', 13, 14);
  raise notice 'ctor_inherit_default=%', c.child_desc();
  c := ot_ctor_child(name => 'PlNamed', code => 15, age => 16);
  raise notice 'ctor_inherit_named=%', c.child_desc();
end;
/

select ot_ctor_child('TooFew', 1);

create type ot_ctor_decl_child under ot_ctor_parent as object (
  age integer,
  constructor function ot_ctor_decl_child(p_name varchar2(30), p_code integer, p_age integer) return self as result,
  member function child_desc return varchar2(80)
);

create type body ot_ctor_decl_child as
  constructor function ot_ctor_decl_child(p_name varchar2(30), p_code integer, p_age integer) return self as result is
  begin
    self.name := p_name;
    self.code := p_code;
    self.age := p_age;
    return;
  end;

  member function child_desc return varchar2(80) is
  begin
    return self.describe() || ':' || self.age;
  end;
end;
/

select ot_ctor_decl_child('CtorSql', 20, 21).child_desc();
select ot_ctor_decl_child(p_name => 'CtorNamed', p_code => 22, p_age => 23).child_desc();
select ot_ctor_decl_child('CtorMixed', p_code => 24, p_age => 25).child_desc();
select ot_ctor_decl_child(name => 'CtorFallback', code => 26, age => 27).child_desc();

declare
  c ot_ctor_decl_child;
begin
  c := ot_ctor_decl_child(name => 'CtorPlFallback', code => 28, age => 29);
  raise notice 'ctor_inherit_fallback=%', c.child_desc();
end;
/

select ot_ctor_decl_child(name => 'Bad', age => 30);

drop type ot_ctor_parent cascade;

-- ============================================================================
-- [definition-time/runtime] generalized invocation invalid paths
-- ============================================================================
create type ot_gen_base as object (
  name varchar2(30),
  member function describe return varchar2(60),
  constructor function ot_gen_base return self as result
) not final;

create type body ot_gen_base as
  member function describe return varchar2(60) is
  begin
    return 'base=' || self.name;
  end;

  constructor function ot_gen_base return self as result is
  begin
    self.name := 'ctor';
    return;
  end;
end;
/

create type ot_gen_child under ot_gen_base as object (
  age integer
);

create type body ot_gen_child as
end;
/

create type ot_gen_other as object (
  name varchar2(30),
  member function describe return varchar2(60)
);

create type body ot_gen_other as
  member function describe return varchar2(60) is
  begin
    return 'other=' || self.name;
  end;
end;
/

declare
  c ot_gen_child;
begin
  c := ot_gen_child('Pl', 1);
  raise notice 'gen_valid=%', (c as ot_gen_base).describe();
end;
/

select (ot_gen_child('Bad', 2) as ot_gen_other).describe();
select (ot_gen_child('Ctor', 3) as ot_gen_base).ot_gen_base();
select ot_gen_base.ot_gen_base((ot_gen_child('CtorStatic', 4) as ot_gen_base));

drop type ot_gen_base cascade;
drop type ot_gen_other cascade;

-- ============================================================================
-- [definition-time] subtype inherited member naming conflicts
-- ============================================================================
create type ot_name_parent as object (
  id integer,
  member function label return integer
) not final;

create type body ot_name_parent as
  member function label return integer is
  begin
    return self.id;
  end;
end;
/

create type ot_name_dup_attr under ot_name_parent as object (
  id integer
);

create type ot_name_attr_method under ot_name_parent as object (
  label integer
);

create type ot_name_method_attr under ot_name_parent as object (
  member function id return integer
);

drop type ot_name_parent cascade;

-- ============================================================================
-- [definition-time/runtime] static hiding semantics
-- ============================================================================
create type ot_static_hide_parent as object (
  static function label(p_value integer) return varchar2(40)
) not final;

create type body ot_static_hide_parent as
  static function label(p_value integer) return varchar2(40) is
  begin
    return 'parent=' || p_value;
  end;
end;
/

create type ot_static_hide_child under ot_static_hide_parent as object (
  static function label(p_value integer) return varchar2(40)
);

create type body ot_static_hide_child as
  static function label(p_value integer) return varchar2(40) is
  begin
    return 'child=' || p_value;
  end;
end;
/

select ot_static_hide_parent.label(10);
select ot_static_hide_child.label(10);

declare
begin
  raise notice 'static_hide_parent=%', ot_static_hide_parent.label(11);
  raise notice 'static_hide_child=%', ot_static_hide_child.label(11);
end;
/

create type ot_static_hide_bad under ot_static_hide_parent as object (
  overriding static function label(p_value integer) return varchar2(40)
);

drop type ot_static_hide_parent cascade;

-- ============================================================================
-- [definition-time/runtime] object type method default arguments
-- ============================================================================
create type ot_default_arg_object as object (
  name varchar2(30),
  member function greet(prefix varchar2(20) default 'hi', times integer default 1) return varchar2(80),
  static function build_label(base varchar2(30), tag integer default 7) return varchar2(40),
  constructor function ot_default_arg_object(p_name varchar2(30) default 'CtorDefault') return self as result
);

create type body ot_default_arg_object as
  member function greet(prefix varchar2(20) default 'hi', times integer default 1) return varchar2(80) is
  begin
    return prefix || ':' || self.name || ':' || times;
  end;

  static function build_label(base varchar2(30), tag integer default 7) return varchar2(40) is
  begin
    return base || ':' || tag;
  end;

  constructor function ot_default_arg_object(p_name varchar2(30) default 'CtorDefault') return self as result is
  begin
    self.name := p_name;
    return;
  end;
end;
/

select ot_default_arg_object('Sql').greet();
select ot_default_arg_object('Sql').greet('hey');
select ot_default_arg_object('Sql').greet(times => 3);
select ot_default_arg_object.build_label('Tag');
select ot_default_arg_object.build_label(base => 'NamedTag');
select ot_default_arg_object().greet();

declare
  o ot_default_arg_object;
begin
  o := ot_default_arg_object();
  raise notice 'obj_default_ctor=%', o.greet();
  raise notice 'obj_default_member=%', o.greet(times => 4);
  raise notice 'obj_default_static=%', ot_default_arg_object.build_label('Pl');
end;
/

create type ot_default_arg_bad as object (
  member procedure bad_value(p_value out integer default 1)
);

drop type ot_default_arg_object cascade;

-- ============================================================================
-- [bind-time] wrong arity / wrong type diagnostics stay object-type aware
-- ============================================================================
create type ot_diag as object (
  value integer,
  member function add_delta(delta integer) return integer,
  static function label(p_value integer) return varchar2(40)
);

create type body ot_diag as
  member function add_delta(delta integer) return integer is
  begin
    return self.value + delta;
  end;

  static function label(p_value integer) return varchar2(40) is
  begin
    return 'diag=' || p_value;
  end;
end;
/

select ot_diag(1).add_delta();
select ot_diag(1).add_delta(ot_diag(2));
select ot_diag.label();
select ot_diag.label(ot_diag(3));

drop type ot_diag cascade;

-- ============================================================================
-- [definition-time] check_function_bodies validates object type bodies
-- ============================================================================
set check_function_bodies = on;

create type ot_validator_case as object (
  value integer,
  member function broken return integer
);

create type body ot_validator_case as
  member function broken return integer is
  begin
    self.value := 1;
    return self.value;
  end;
end;
/

reset check_function_bodies;

drop type ot_validator_case cascade;

-- ============================================================================
-- [dump/restore] Object type definitions for pg_dump coverage
-- These types cover basic features that must survive dump and restore.
-- ============================================================================

-- Basic object type with mixed member/static methods
create type ot_dump_base as object (
  attr1 varchar2(30),
  attr2 integer,
  member function describe return varchar2(100),
  static function create_default return ot_dump_base
) not final;

create type body ot_dump_base as
  member function describe return varchar2(100) is
  begin
    return self.attr1 || ':' || self.attr2;
  end;
  static function create_default return ot_dump_base is
  begin
    return ot_dump_base('default', 0);
  end;
end;
/

-- Runtime usage before dump
declare
  v ot_dump_base;
begin
  v := ot_dump_base('hello', 42);
  raise notice 'dump_base=%', v.describe();
  v := ot_dump_base.create_default();
  raise notice 'dump_default=%', v.describe();
end;
/

-- Object type with UNDER inheritance and overriding
create type ot_dump_child under ot_dump_base as object (
  extra varchar2(30),
  overriding member function describe return varchar2(100)
);

create type body ot_dump_child as
  overriding member function describe return varchar2(100) is
  begin
    return self.attr1 || ':' || self.attr2 || ':' || self.extra;
  end;
end;
/

-- Runtime usage with inheritance
declare
  v ot_dump_child;
begin
  v := ot_dump_child('child', 99, 'extra-val');
  raise notice 'dump_child=%', v.describe();
end;
/

-- Type marked FINAL (no inheritance allowed)
create type ot_dump_final as object (
  id integer,
  label varchar2(30),
  member function to_string return varchar2(100)
) final;

create type body ot_dump_final as
  member function to_string return varchar2(100) is
  begin
    return '[' || self.id || '] ' || self.label;
  end;
end;
/

-- Verify FINAL type works
declare
  v ot_dump_final;
begin
  v := ot_dump_final(1, 'final_test');
  raise notice 'dump_final=%', v.to_string();
end;
/

-- Chained calls on inherited type
declare
  v ot_dump_base;
  c ot_dump_child;
begin
  v := ot_dump_base('p', 10);
  raise notice 'dump_chain=%', v.attr1;
  c := ot_dump_child('c', 20, 'x');
  raise notice 'dump_child_desc=%', c.describe();
end;
/

drop type ot_dump_child cascade;
drop type ot_dump_base cascade;
drop type ot_dump_final cascade;
