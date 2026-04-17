
/*****************************************************************
 *
 * Test DBLINK feature
 *
 ****************************************************************/

--add enrol11 user
CREATE USER enrol11 WITH superuser;

set ivorysql.compatible_mode to pg;
show ivorysql.compatible_mode;

create database tdblnk;

set ivorysql.compatible_mode to oracle;
show ivorysql.compatible_mode;

\c tdblnk enrol11
create schema enrol11;

ALTER USER enrol11 with password 'ivy123';

CREATE TABLE it1(c1 int, c2 varchar2(10));
CREATE TABLE it2(b1 int, b2 int);
CREATE TABLE it3(a varchar2(20), b number);
CREATE TABLE it4(x number, y varchar2(10));
CREATE TABLE it5(m varchar2(10), n number(5), e blob);
CREATE TABLE test_null(id integer, name varchar2(256));


INSERT INTO it1 VALUES (11, 'DBLINK'), (1, 'DATATYPE'), (3, 'PACKAGE'), (7, 'SEQUENCE');
INSERT INTO it2 VALUES (1, 1001), (12, 1003), (5, 7), (3, 1005);
INSERT INTO it3 VALUES ('zzz', 26), ('ccc', 3), ('ddd', 4);
INSERT INTO it4 VALUES (100, 'One'), (10, 'Text'), (200, 'Postgres');
INSERT INTO it5(m, n) VALUES ('shared', 99), ('private', 909), ('public', 999);


-- create view
CREATE VIEW vit1 AS SELECT * FROM it1 WHERE c1 < 10;

-- create materialized view
CREATE MATERIALIZED VIEW mivt2 AS SELECT * FROM it2;


\c regression

CREATE TABLE lptb(a int);
INSERT INTO lptb VALUES (1), (100), (1000);

-- create a public DBLINK
show ivorysql.default_dblink_fdw; -- postgres_fdw is default fdw

--
-- Fixed User
--
-- create public dblink
DO $d$
    BEGIN
        EXECUTE $$ create public database link ivyfuplnk
			connect to enrol11 identified by "ivy123"
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;

-- create private dblink
DO $d$
    BEGIN
        EXECUTE $$ create database link ivyfurlnk
			connect to enrol11 identified by "ivy123"
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;


--
-- CURRENT_USER
--
DO $d$
    BEGIN
        EXECUTE $$ create public database link ivycurrent
        	connect to CURRENT_USER
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;

-- must be create a user mapping for current_user DBLINK before using this link
create user mapping for public
 server ivycurrent options (user 'enrol11', password 'ivy123');


--
-- Connected User
--
DO $d$
    BEGIN
        EXECUTE $$ create public database link ivyconned
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;

-- must be create a user mapping for Connected User DBLINK before using this link
create user mapping for public
 server ivyconned options (user 'enrol11', password 'ivy123');


--
-- Shared DBLINK
--
DO $d$
    BEGIN
        EXECUTE $$ create shared public database link spglink
		connect to enrol11 identified by "ivy123"
		AUTHENTICATED BY enrol11 IDENTIFIED BY "ivy123"
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;

/*Begin - BUG#Z1391 */
CREATE OR REPLACE PROCEDURE ins_from_pglnk IS
BEGIN
    INSERT INTO it1@ivyfuplnk SELECT * FROM it1@ivyfuplnk;
END;
/
/*End - BUG#Z1391 */


-- access remote table it1, it2, it3, it4, it5 by DBLINKs
select * from it1@ivyfuplnk;
select * from it2@ivyfurlnk;
select * from it3@ivycurrent;
select * from it4@ivyconned;

select * from it5@spglink; -- error, open_links default is 4

--test synonym
create synonym a for it1@ivyfuplnk;
select * from a;
drop synonym a;


-- access remote view
select * from vit1@ivyfuplnk;

-- access remote materialized view
select * from mivt2@ivyfuplnk;

-- test alter session close command
alter session close database link ivyfuplnk;

-- create local view base on dblink
create view vt1 as select c2 from it1@ivyfuplnk;

-- access local view vt1
select * from vt1;

/* Begin - OA-REQ-1457 */
create or replace view vt_null as
  select a1.* from test_null@ivyfuplnk a1 left join (select * from test_null@ivyfuplnk) b on a1.id = b.id;

\d+ vt_null
/* End - OA-REQ-1457 */

-- where conditions
--select * from it1@ivyfuplnk where it1.c2@ivyfuplnk = 'PACKAGE';
select * from it1@ivyfuplnk where c2 = 'PACKAGE';

-- inner join two remote tables f2 and f3.
select * from it1@ivyfuplnk A inner join it2@ivyfuplnk B on A.c1 = B.b2;


-- update remote table by public dblink
update it1@ivyfuplnk set c2 = 'Itest' where c1 = 1;
select * from it1@ivyfuplnk;

-- delete remote table by public dblink
delete from it1@ivyfuplnk where c1 = 1;
select * from it1@ivyfuplnk;

/*Begin - BUG#Z1391 */
call ins_from_pglnk();
select * from it1@ivyfuplnk;
drop procedure ins_from_pglnk;
/*End - BUG#Z1391 */

-- test alter database link command
select umoptions from pg_user_mappings where srvname = 'ivyfurlnk';

alter database link ivyfurlnk connect to enrol11 identified by ivorysqlpro;

select umoptions from pg_user_mappings where srvname = 'ivyfurlnk';


-- drop link
drop database link ivyfurlnk;
drop public database link ivyfuplnk;
drop public database link ivycurrent;
drop public database link ivyconned;

-- access the shared DBLINK
select * from it5@spglink;

drop public database link spglink;


-- test global_names GUC
show global_names;

alter system set global_names = true;

show global_names;

DO $d$
    BEGIN
        EXECUTE $$ create public database link glolnk
			connect to enrol11 identified by "ivy123"
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;

select * from it1@glolnk; -- error


DO $d$
    BEGIN
        EXECUTE $$ create public database link tdblnk
			connect to enrol11 identified by "ivy123"
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;

select * from it1@tdblnk;

drop public database link glolnk;
drop public database link tdblnk;

show global_names;

alter system set global_names = false;

show global_names;


-- Test Global Name as a Loopback Database Link
select * from lptb@regression;


select current_user;

drop view vt1;
drop view vt_null;
--drop extension postgres_fdw;
drop table lptb;


\c tdblnk enrol11
drop view vit1;
drop MATERIALIZED view mivt2;
drop table it1;
drop table it2;
drop table it3;
drop table it4;
drop table it5;
drop table test_null;
drop schema enrol11;


\c regression

set ivorysql.compatible_mode to pg;
show ivorysql.compatible_mode;
drop database tdblnk;
set ivorysql.compatible_mode to oracle;

--bug#Z1678
create schema enrol11;

CREATE  TABLE enrol11."RES_ACCESSPOINT"("TYPE" VARCHAR2(64) ,"CUTIN_DATE" TIMESTAMP(6) ,
"LONGITUDE" FLOAT ,"LATITUDE" FLOAT ,"CITY_ID" NUMBER ,"STATUS" NUMBER ,"MAINT_DEP" VARCHAR2(64) ,
"COUNTY_ID" NUMBER ,"LOCATION" VARCHAR2(255) ,"RELATED_SITE" NUMBER ,"ZH_LABEL" VARCHAR2(255)   NOT NULL,
"TIME_STAMP" TIMESTAMP(6) ,"STATEFLAG" NUMBER   NOT NULL,"REMARK" VARCHAR2(255) ,
"CREATOR" VARCHAR2(255) ,"INT_ID" NUMBER   NOT NULL,"CREAT_TIME" TIMESTAMP(6) ,"MODIFIER" VARCHAR2(255) ,
"MODIFY_TIME" TIMESTAMP(6) ,"TELEPHONE" VARCHAR2(64) ,"MAINTAIN_PERSON" VARCHAR2(64) ,"MAINTAIN_PHONE" VARCHAR2(64) ,
"IS_WRONG" VARCHAR2(32) ,"WRONG_INFO" VARCHAR2(1024) ,"RESOURCE_POINT_CODE" VARCHAR2(128) ,"PROJECT_ID" VARCHAR2(64) ,
"GPS" VARCHAR2(64) ,"ALIAS" VARCHAR2(128)  DEFAULT '',"REALX" FLOAT ,"REALY" FLOAT ,"EDIT_SESSION" NUMBER ,"IS_RESERVE" NUMBER(10,0) ,
"BUILD_AREA_ID" NUMBER(10,0) ,"CUID" VARCHAR2(255) ,"LOADFLAG" VARCHAR2(48) ,"MANAGE_UNIT" VARCHAR2(128)  DEFAULT '',"IS_DENGGAN" VARCHAR2(24)  DEFAULT '',
"DENGGAN_CODE" VARCHAR2(128)  DEFAULT '',"QRCODE" VARCHAR2(255)  DEFAULT '',"INVENT_ROOM_ID" VARCHAR2(255) ,"ADDRESS_CODE" VARCHAR2(255) ,
"ZICHAN_SITE_CODE1" VARCHAR2(255) ,"ASSETCODE" VARCHAR2(255) ,"RESOURCE_TAG" VARCHAR2(255)  DEFAULT '',
"RELATED_DATA_CENTRE" VARCHAR2(255)  DEFAULT '',"LIFECYCLE_STATUS" VARCHAR2(255)  DEFAULT 2,
"RESOURCE_NUM" VARCHAR2(255)  DEFAULT '',"QUALITOR" VARCHAR2(255)  DEFAULT '',"MOBILE_ADDRESS" VARCHAR2(255)  DEFAULT '',
"RELATED_IRON" VARCHAR2(255)  DEFAULT '',"COLLECT_IRON_NUM" VARCHAR2(255)  DEFAULT '',"COLLECT_IRON_NAME" VARCHAR2(255)  DEFAULT '',
"IS_RELATED_IRON" VARCHAR2(255)  DEFAULT '',"IS_POINT" VARCHAR2(255)  DEFAULT '',"SITE_CODE" VARCHAR2(255)  DEFAULT '',
"SITE_CODE_NAME" VARCHAR2(255)  DEFAULT '',"REALIZE_MODEL" VARCHAR2(255)  DEFAULT '',"MAX_POWER" VARCHAR2(255)  DEFAULT '',
"ASSET_SITE_NUM" VARCHAR2(255)  DEFAULT '',"ASSET_SITE_DESCRIBE" VARCHAR2(255)  DEFAULT '',"TOTAL_POWER" VARCHAR2(255)  DEFAULT '',
"LOCATION_PRIMARY" VARCHAR2(255)  DEFAULT '',"ASSET_TAG" VARCHAR2(255)  DEFAULT '',"ASSETS_NAME" VARCHAR2(255)  DEFAULT '',
"PROVINCE_ID" VARCHAR2(255)  DEFAULT 1289887769,"PROPERTY_UNIT" VARCHAR2(64)  DEFAULT '',"PROPERTY_RIGHT" VARCHAR2(64)  DEFAULT '',
"DATA_QUALITY_PERSON" VARCHAR2(255)  DEFAULT '',"ZH_LABEL_JT" VARCHAR2(255) ,"LOCATION_PRIMARY_Y" VARCHAR2(255) ,
"AIRCONDITIONER_POWER" VARCHAR2(64)  DEFAULT '',"LOCATION_LS" VARCHAR2(255) ,"SHARE_ATTRIBUTE" VARCHAR2(255)  DEFAULT '',
"FIVE_G_FLAG" VARCHAR2(255)  DEFAULT '',"RETIRE_TIME" TIMESTAMP(6) ,"LONGITUDE_WX" VARCHAR2(255) ,"LATITUDE_WX" VARCHAR2(255) ,
"ASSET_TYPE" VARCHAR2(255) ,"SERVICE_SITE_TYPE" VARCHAR2(255) ,"GRID" VARCHAR2(255)  DEFAULT '',"REIMBURSEMENT_NAME" VARCHAR2(255)  DEFAULT '',
"REIMBURSEMENT_CODE" VARCHAR2(255)  DEFAULT '',"STANDARD_ADDRESS" VARCHAR2(255) ,"QR_CODE_NO" VARCHAR2(255)  DEFAULT '',
"PROJECT_CODE" VARCHAR2(255)  DEFAULT '',"PROJECT_NAME" VARCHAR2(255)  DEFAULT '',"ASSET_ADDRESS_CODE" VARCHAR2(255)  DEFAULT '',
"FLOOR_NUM" VARCHAR2(255)  DEFAULT '',"LONGITUDE_JY" NUMBER ,"LATITUDE_JY" NUMBER ,"JY_FROM" VARCHAR2(64) ,"JY_JULI" VARCHAR2(64) ,
"WHETHER_TELECOM_SERVICE" VARCHAR2(255)  DEFAULT '',"TELECOM_SERVICE_PROJECT_CODE" VARCHAR2(255)  DEFAULT '',"TELECOM_SERVICE_PROJECT_NAME" VARCHAR2(255)  DEFAULT '',
"MAINIT_UNIT" VARCHAR2(255) ,"AREA_TYPE" VARCHAR2(255) ,"RELATED_SITE_BAK" NUMBER ,"PMS_DESIGN_CODE" VARCHAR2(255) ,"PMS_DESIGN_NAME" VARCHAR2(255) ,
"IF_VILLAGE_PASS_SERV" VARCHAR2(255) ,"VILLAGE_PASS_SERV_CODE" VARCHAR2(255) ,"VILLAGE_PASS_SERV_NAME" VARCHAR2(255) ,"CHARTER" VARCHAR2(255) ,"CHARTER_PHONE" VARCHAR2(255) );


CREATE  TABLE enrol11."RMS_EQUIPROOM"("INT_ID" NUMBER(38,0)   NOT NULL,
"STATEFLAG" NUMBER   NOT NULL,"TIME_STAMP" TIMESTAMP(6) ,
"CREATOR" VARCHAR2(255) ,"MODIFIER" VARCHAR2(255) ,
"MODIFY_TIME" TIMESTAMP(6) ,"CREAT_TIME" TIMESTAMP(6)  DEFAULT SYSDATE(),"ZH_LABEL" VARCHAR2(256) ,
"ABBREVIATION" VARCHAR2(255) ,"ALIAS" VARCHAR2(255) ,"ROOM_TYPE" VARCHAR2(255) ,"NODEEQUIP_TYPE" VARCHAR2(255) ,
"RELATED_SITE_ID" NUMBER(38,0) ,"STAIRS_IN" NUMBER ,"LENGTH" FLOAT ,"HEIGHT" FLOAT ,"WIDTH" FLOAT ,"RACK_START_ROW" VARCHAR2(255) ,
"RACK_END_ROW" VARCHAR2(255) ,"RACK_START_COL" VARCHAR2(255) ,"RACK_END_COL" VARCHAR2(255) ,"RACK_ROW_DIRECTION" VARCHAR2(255) ,
"RACK_COL_DIRECTION" VARCHAR2(255) ,"IS_COM_BUILD" VARCHAR2(255) ,"COM_BUILD_UNIT" VARCHAR2(255) ,"IS_SHARE" VARCHAR2(255) ,
"SHARE_VENDOR" VARCHAR2(255) ,"REMARK" VARCHAR2(255) ,"CITY_ID" NUMBER(38,0) ,"EQUIPMENTCODE" VARCHAR2(255)  DEFAULT '',
"PROPERTY" VARCHAR2(255)  DEFAULT '',"EQUIPROOM_CODE" VARCHAR2(255)  DEFAULT '',"IS_WRONG" VARCHAR2(255) ,"WRONG_INFO" VARCHAR2(4000) ,
"PROVINCE_ID" NUMBER(38,0)  DEFAULT 1289887769,"COUNTY_ID" NUMBER(38,0)  DEFAULT '',"BUREAU_ID" NUMBER(38,0) ,"EQUIPROOM_ADDRESS" VARCHAR2(255) ,
"PROJECT_ID" NUMBER(38,0) ,"PROJECT_NAME" VARCHAR2(200) ,"DH_SITE_NAME" VARCHAR2(100) ,"CHECK_TIME" TIMESTAMP(6) ,
"HZ_ID" VARCHAR2(100) ,"STATUS" VARCHAR2(40)  DEFAULT 2,"IN_DATE" TIMESTAMP(6) ,"ADMINISTRATION" VARCHAR2(40) ,
"FAIL_ACC_UNIT" VARCHAR2(100) ,"MANAGE_COMPANY" VARCHAR2(100) ,"FADIANJIZU" VARCHAR2(100) ,"FAIL_ACC_UNIT_ID" VARCHAR2(100) ,
"FADIANJIZU_ID" VARCHAR2(100) ,"QRCODE" VARCHAR2(255) ,"TRANS_ZU" VARCHAR2(100) ,"TRANS_ZU_ID" VARCHAR2(100) ,
"SHIFEN_ZU" VARCHAR2(100) ,"SHIFEN_ZU_ID" VARCHAR2(100) ,"TRANS_ROOM_TYPE" VARCHAR2(100) ,"ACREAGE" NUMBER(10,2) ,
"PRO_TASK_ID" NUMBER ,"FILE_PATH" VARCHAR2(1000) ,"LOCATION_INIT_STATUS" NUMBER ,"ROW_NUM" NUMBER ,"COL_NUM" NUMBER ,
"ROW_DIRECTION" NUMBER  DEFAULT 1,"COL_DIRECTION" NUMBER  DEFAULT 3,"SERVICE_LEVEL" VARCHAR2(255) ,"ROW_START_NO" VARCHAR2(56) ,
"COL_START_NO" VARCHAR2(56) ,"TRANS_ID" VARCHAR2(255) ,"SECURITY_LEVEL" VARCHAR2(22) ,
"SURVEY_STATUS" NUMBER  DEFAULT 0,"DONGHUAN_ZU_ID" VARCHAR2(255) ,"DONGHUAN_ZU" VARCHAR2(255) ,
"BACKGROUND_IMAGE" VARCHAR2(512) ,"PRO_DH_ZU_ID" VARCHAR2(255) ,"LONGITUDE" NUMBER(13,7) ,
"LATITUDE" NUMBER(13,7) ,"QUALITOR" VARCHAR2(32) ,"RESFDN" VARCHAR2(255) ,"RELATED_TOWER_CODE" VARCHAR2(255) ,
"PLACE" VARCHAR2(255) ,"CQ_UNIT" VARCHAR2(255) ,"ROOM_LEVEL" NUMBER(38,0) ,"TRANS_ROOM_TYPEID" NUMBER(38,0) ,
"BINDBYHAND" NUMBER ,"SF_DW" VARCHAR2(20) ,"RENT_TYPE" VARCHAR2(255)  DEFAULT '',"WHJB" VARCHAR2(255) ,
"WHLX" VARCHAR2(255) ,"DX" VARCHAR2(255) ,"DH_SITE_TYPE" VARCHAR2(100) ,"JW_DATE" DATE ,"TW_DATE" DATE ,
"AB_FLAG" NUMBER ,"EAM_CODE" VARCHAR2(100) ,"AIR_RATE" VARCHAR2(100) ,"NE_RATE" VARCHAR2(100) ,"TOWER_ROOM_TYPE" VARCHAR2(100) ,
"DH_GXX_NAME" VARCHAR2(100) ,"EAM_ORG" VARCHAR2(100) ,"XZQH" VARCHAR2(100) ,"XHSC" NUMBER ,"ZTSC" NUMBER ,"BJR" VARCHAR2(100) ,
"IS_GDYJ" VARCHAR2(100) ,"POWER_ROOM_TYPE" VARCHAR2(100) ,"AREAROOM" VARCHAR2(255) ,"RELATED_FLOOR_CUID" VARCHAR2(255) ,
"TRANS_ROOM" VARCHAR2(25) ,"IS_TRANSFER" VARCHAR2(32) ,"METERING_MODE" VARCHAR2(32) ,"POWER_TYPE" VARCHAR2(32) ,
"AIR_SUPPLY" VARCHAR2(32) ,"ALREADY_BAY" VARCHAR2(32) ,"CAN_BAY" VARCHAR2(32) ,"ALREADY_AREA" VARCHAR2(32) ,
"CAN_AREA" VARCHAR2(32) ,"IS_DATA" VARCHAR2(32) ,"MAINT_MODE" VARCHAR2(32) ,"KEY_TYPE" VARCHAR2(32) ,
"BUSINESS_BRANCH" VARCHAR2(32) ,"IS_2G_3G" VARCHAR2(32) ,"MAINTANCER" VARCHAR2(32) ,"BUILD_TYPE" VARCHAR2(32) ,
"CAB_COL_DIRECTION" VARCHAR2(32) ,"CAB_ROW_DIRECTION" VARCHAR2(32) ,"CAB_END_COL" VARCHAR2(32) ,
"CAB_START_COL" VARCHAR2(32) ,"CAB_END_ROW" VARCHAR2(32) ,"CAB_START_ROW" VARCHAR2(32) ,
"USEDAREA" VARCHAR2(32) ,"COM_SHARE_UNIT" VARCHAR2(32) ,"CUTIN_DATE" TIMESTAMP(6) ,"STATE" VARCHAR2(32) ,
"ROOM_TYPE1" VARCHAR2(32) ,"OLD_NAME" VARCHAR2(32) ,"MANAGE_UNIT" VARCHAR2(32) ,"INVENT_ROOM_ID" VARCHAR2(255) ,
"DANGER_RANK" VARCHAR2(255) ,"ADDRESS_CODE" VARCHAR2(255) ,"LABEL" VARCHAR2(32) ,"ZICHAN_SITE_CODE1" VARCHAR2(255) ,
"ASSETCODE" VARCHAR2(255) ,"CUID" VARCHAR2(255) ,"OWNERSHIP" VARCHAR2(255)  DEFAULT '',
"SHARED_COMPANY" VARCHAR2(255)  DEFAULT '',"CONSTRUCT_COMPANY" VARCHAR2(255)  DEFAULT '',
"IS_APPROVED" VARCHAR2(255)  DEFAULT '',"STANDARD_NAME" VARCHAR2(255)  DEFAULT '',"COL_CUID" VARCHAR2(255)  DEFAULT '',
"MAIN_DEVICE_MODEL" VARCHAR2(255)  DEFAULT '',"ADDRESS" VARCHAR2(255)  DEFAULT '',"OBJECT_TYPE_CODE" NUMBER  DEFAULT 0,
"ROW_COUNT" NUMBER  DEFAULT 0,"COL_COUNT" NUMBER  DEFAULT 0,"BASE_STA_DEVICE_TYPE" VARCHAR2(255)  DEFAULT '',
"COLUMNCOUNT" NUMBER  DEFAULT 0,"CONTACT_ADDRESS" VARCHAR2(255)  DEFAULT '',"FLOOR_MAXNUM" NUMBER  DEFAULT 0,"LABEL_CN" VARCHAR2(255)  DEFAULT '',
"MAINTAINDEPARTMENT" VARCHAR2(255)  DEFAULT '',"TELEPHONE" VARCHAR2(255)  DEFAULT '',"CONTACTOR" VARCHAR2(255)  DEFAULT '',
"DEVICE_RELATED_PROJECT" VARCHAR2(255)  DEFAULT '',"SEVICE_LEADER_CODE" NUMBER  DEFAULT 0,"OWNER_LINKTEL" VARCHAR2(255)  DEFAULT '',
"OWNER_LINKMAN" VARCHAR2(255)  DEFAULT '',"LEASE" VARCHAR2(255)  DEFAULT '',"RESPONSIBLE_LEAD" VARCHAR2(255)  DEFAULT '',
"RESOURCE_ID" VARCHAR2(255)  DEFAULT '',"ROOM_ITS_TEL" NUMBER  DEFAULT 0,"ERP_CODE" VARCHAR2(255)  DEFAULT '',
"GT_VERSION" NUMBER  DEFAULT 0,"OBJECTID" NUMBER  DEFAULT 0,"RESPONSIBLE_UNIT" VARCHAR2(255)  DEFAULT '',"ROOM_ADMIN_PHONE" VARCHAR2(255)  DEFAULT '',
"ROOM_TYPE_IRMS" NUMBER  DEFAULT 0,"ROOM_ITS" VARCHAR2(255)  DEFAULT '',"ROOM_NO" VARCHAR2(255)  DEFAULT '',
"ROOM_ADMIN" VARCHAR2(255)  DEFAULT '',"LAST_MODIFY_TIME" DATE ,"ROOM_IDENTIFICATION" VARCHAR2(255)  DEFAULT '',
"ROOM_LOCATION" VARCHAR2(255)  DEFAULT '',"ROOM_COVER_REGION" VARCHAR2(255)  DEFAULT '',"TOWER_COMPANY_STATUS" VARCHAR2(255)  DEFAULT '',
"TRANSFER_NOT" VARCHAR2(255)  DEFAULT '',"ROOM_FORM" VARCHAR2(255)  DEFAULT '',"ROOM_STRUCTURE" VARCHAR2(255)  DEFAULT '',
"MOVE_CODE" VARCHAR2(255)  DEFAULT '',"TOWER_COMPANY_ROOM" VARCHAR2(255)  DEFAULT '',"TOWER_COMPANY_SITE" VARCHAR2(255)  DEFAULT '',
"COLLECTION_SITE_CODE" VARCHAR2(255)  DEFAULT '',"COLLECTION_ROOM_CODE" VARCHAR2(255)  DEFAULT '',"COLLECTION_ROOM_NAME" VARCHAR2(255)  DEFAULT '',
"WHETHER_ASSOCIATED_TOWER" VARCHAR2(255)  DEFAULT '',"FLOOR_LOCATION" VARCHAR2(255)  DEFAULT '',"PROPERTY_NATURE" VARCHAR2(255)  DEFAULT '',
"ASSET_NAME" VARCHAR2(255)  DEFAULT '',"IS_MOVING_RING" VARCHAR2(255)  DEFAULT '',
"STANDARD_NOMENCLATURE" VARCHAR2(512)  DEFAULT '',"PROPERTY_UNIT" VARCHAR2(255)  DEFAULT '',"EQUIPMENT_POWER" VARCHAR2(255)  DEFAULT '',
"AIRCONDITIONER_POWER" VARCHAR2(255)  DEFAULT '',"LOCATION_DESCRIPTION" VARCHAR2(255)  DEFAULT '',"LOCATION_CODE" VARCHAR2(255)  DEFAULT '',
"INSTALLABLE_EQUIVALENT_RACK" NUMBER  DEFAULT 0,"ALREADY_EQUIVALENT_RACK" NUMBER  DEFAULT 0,"ROOM_CLASSIFICATION" VARCHAR2(255)  DEFAULT '',
"IS_ADDRESS" VARCHAR2(255)  DEFAULT '',"ROOM_NATURE" VARCHAR2(255)  DEFAULT '',"ADDRESS_NAME" VARCHAR2(255)  DEFAULT '',
"ADDRESS_CODE_WIRELESS" VARCHAR2(255)  DEFAULT '',"MAX_POWER" NUMBER(22,10) ,"POWER_CAPACITY" NUMBER(22,10) ,"RELATED_TIETA_CODE" VARCHAR2(255) ,
"ZX_PROPERTY_UNIT" VARCHAR2(255) ,"ZX_INDEX" VARCHAR2(255) ,"STAIRSIN" NUMBER ,"ZH_LABEL_JT" VARCHAR2(255) ,"PROJECT_NUM" VARCHAR2(255)  DEFAULT '',
"IMPORTANT_ROOM" VARCHAR2(255) ,"RETIRE_TIME" TIMESTAMP(6)  DEFAULT null,"FIVE_G_FLAG" VARCHAR2(255)  DEFAULT '',"ASSET_TYPE" VARCHAR2(255) ,
"SAVEOUTLINE" VARCHAR2(255) ,"GRID" VARCHAR2(255)  DEFAULT '',"REIMBURSEMENT_NAME" VARCHAR2(255)  DEFAULT '',"REIMBURSEMENT_CODE" VARCHAR2(255)  DEFAULT '',
"CORRIGENDUM" VARCHAR2(255) ,"PROJECT_CODE" VARCHAR2(255)  DEFAULT '',"UUID" VARCHAR2(255)  DEFAULT '',"CHINA_TOWER_OPERATIONS_ID" VARCHAR2(255)  DEFAULT '',
"WHETHER_TELECOM_SERVICE" VARCHAR2(255)  DEFAULT '',"TELECOM_SERVICE_PROJECT_CODE" VARCHAR2(255)  DEFAULT '',
"TELECOM_SERVICE_PROJECT_NAME" VARCHAR2(255)  DEFAULT '',"ATTACHMENT" VARCHAR2(2000) ,"PRODUCTION_BUILDING_TYPE" VARCHAR2(64) ,
"POWER_USED" NUMBER(38,0) ,"POWER_TOTAL" NUMBER(38,0) ,"SITE_ID" NUMBER(38,0) ,"OUTLINE" CLOB ,"COMPASS" NUMBER(38,0) ,
"NAME_OFFSET" VARCHAR2(255) ,"ADDRESS_CODE_A" VARCHAR2(255) ,"MAINIT_UNIT" VARCHAR2(255) ,"PMS_DESIGN_CODE" VARCHAR2(255) ,
"PMS_DESIGN_NAME" VARCHAR2(255) ,"HALLWAY_NUM" VARCHAR2(64) ,"HALLWAY_WIDTH" VARCHAR2(64) ,"CHARTER" VARCHAR2(255) ,
"CHARTER_PHONE" VARCHAR2(255) ,"COMPASS_OFFSET" NUMBER(38,0) ,"BUSINESS_UNIT" VARCHAR2(255) ,"PLAN_RACK_NUM" NUMBER(22,0) ,
"LOADABLE_RACK_NUM" NUMBER(22,0) ,"INSTALLED_RACK_NUM" NUMBER(22,0) ,"IS_HEADQUARTERS_USED" VARCHAR2(64) ,"UUID_JITUAN" VARCHAR2(255) ,"
IF_VILLAGE_PASS_SERV" VARCHAR2(255) ,"VILLAGE_PASS_SERV_CODE" VARCHAR2(255) ,"VILLAGE_PASS_SERV_NAME" VARCHAR2(255) );


DO $d$
    BEGIN
        EXECUTE $$ create public database link tdblnk
			connect to enrol11 identified by "ivy123"
			using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = regression)
            ))'$$;
    END;
$d$;

set search_path to enrol11;

CREATE OR REPLACE VIEW enrol11.V_NE_EQUIPROOM
AS SELECT rms_equiproom."INT_ID",
    rms_equiproom."ZH_LABEL",
    rms_equiproom."CITY_ID",
    rms_equiproom."COUNTY_ID",
    rms_equiproom."STATEFLAG",
    rms_equiproom."STATUS" ,
    rms_equiproom."CUID"
   FROM "RMS_EQUIPROOM" rms_equiproom
  WHERE rms_equiproom."STATEFLAG" = 0::number
UNION ALL
 SELECT res_accesspoint."INT_ID",
    res_accesspoint."ZH_LABEL",
    res_accesspoint."CITY_ID",
    res_accesspoint."COUNTY_ID",
    res_accesspoint."STATEFLAG",
    to_char(res_accesspoint."STATUS" ) AS "STATUS" ,
    res_accesspoint."CUID" 
   FROM "RES_ACCESSPOINT" res_accesspoint
  WHERE res_accesspoint."STATEFLAG" = 0::number;
  
reset search_path;

\d enrol11.V_NE_EQUIPROOM

select * from enrol11.V_NE_EQUIPROOM@tdblnk;


CREATE TABLE rms_equiproom(int_id number(38,0),zh_label varchar2(256)
,city_id number(38,0),county_id number(38,0),stateflag number NOT null,status varchar2(40),
cuid varchar2(255));

CREATE TABLE res_accesspoint(int_id number NOT null,
zh_label varchar2(255),city_id number,county_id number,
stateflag number NOT null,status number, cuid varchar2(255));



CREATE OR REPLACE VIEW V_NE_EQUIPROOM0
AS SELECT rms_equiproom.int_id,
    rms_equiproom.zh_label,
    rms_equiproom.city_id,
    rms_equiproom.county_id,
    rms_equiproom.stateflag,
    rms_equiproom.status,
    rms_equiproom.cuid
   FROM rms_equiproom
  WHERE rms_equiproom.stateflag = 0
UNION ALL
 SELECT res_accesspoint.int_id,
    res_accesspoint.zh_label,
    res_accesspoint.city_id,
    res_accesspoint.county_id,
    res_accesspoint.stateflag,
    to_char(res_accesspoint.status) AS status,
    res_accesspoint.cuid
   FROM res_accesspoint
  WHERE res_accesspoint.stateflag = 0;

\d V_NE_EQUIPROOM0


CREATE OR REPLACE VIEW V_NE_EQUIPROOM1
AS SELECT rms_equiproom.int_id,
    rms_equiproom.zh_label,
    rms_equiproom.city_id,
    rms_equiproom.county_id,
    rms_equiproom.stateflag,
    rms_equiproom.status,
    rms_equiproom.cuid
   FROM rms_equiproom
  WHERE rms_equiproom.stateflag = 0;

\d V_NE_EQUIPROOM1

CREATE OR REPLACE VIEW V_NE_EQUIPROOM2
AS
 SELECT res_accesspoint.int_id,
    res_accesspoint.zh_label,
    res_accesspoint.city_id,
    res_accesspoint.county_id,
    res_accesspoint.stateflag,
    to_char(res_accesspoint.status) AS status,
    res_accesspoint.cuid
   FROM res_accesspoint
  WHERE res_accesspoint.stateflag = 0;

\d V_NE_EQUIPROOM2


select * from V_NE_EQUIPROOM0@tdblnk;
select * from V_NE_EQUIPROOM1@tdblnk;
select * from V_NE_EQUIPROOM2@tdblnk;


create table test_id_char(id char(10), ids nvarchar2(256));

create table test_id_char2(id char(5), ids nvarchar2(256));

create view test_view as
  select * from test_id_char
union all
  select * from test_id_char2;

\d test_view

select * from test_view@tdblnk;

create or replace function test_f(id integer) return varchar2 is
 begin
   return 'abc';
 end;
 /

create view test_func_view as select test_f(23) from dual;

\d test_func_view

select * from test_func_view@tdblnk;

--clean data
drop view test_func_view;
drop function test_f;
drop view test_view;
drop table test_id_char;
drop table test_id_char2;
drop view V_NE_EQUIPROOM2;
drop view V_NE_EQUIPROOM1;
drop view V_NE_EQUIPROOM0;
drop table res_accesspoint;
drop table rms_equiproom;
drop view enrol11.V_NE_EQUIPROOM;
drop public database link tdblnk;
drop table enrol11."RMS_EQUIPROOM";
drop table enrol11."RES_ACCESSPOINT";
drop schema enrol11;

--OA#00001554
set ivorysql.enable_ora_default_collation = off;
create schema enrol11;

CREATE TABLE enrol11.RMS_BSSPORT (
	INT_ID NUMBER NOT NULL,
	STATEFLAG NUMBER NOT NULL,
	TIME_STAMP TIMESTAMP(6) NOT NULL,
	CREATOR VARCHAR2(255 char) NULL COLLATE "DEFAULT",
	MODIFIER VARCHAR2(255 char) NULL COLLATE "DEFAULT",
	MODIFY_TIME TIMESTAMP(6) NULL,
	CREAT_TIME TIMESTAMP(6) NULL,
	ZH_LABEL VARCHAR2(255 char) NULL COLLATE "DEFAULT",
	NE_NAME VARCHAR2(255 char) NULL COLLATE "DEFAULT",
	PORT_TYPE VARCHAR2(255 char) NULL COLLATE "DEFAULT",
	RELATED_NE_ID NUMBER NULL,
	STATUS VARCHAR2(255 char) NULL COLLATE "DEFAULT",
	TDDF VARCHAR2(255 char) NULL COLLATE "DEFAULT",
	RATE VARCHAR2(32 char) NULL COLLATE "DEFAULT",
	IP_ADDRESS VARCHAR2(255 char) NULL COLLATE "DEFAULT",
	RELATED_CARD_ID NUMBER NULL,
	CIRCUIT_NAME VARCHAR2(255 char) NULL COLLATE "DEFAULT",
	CITY_ID NUMBER NOT NULL DEFAULT '1'::number,
	IS_WRONG VARCHAR2(255 char) NULL COLLATE "DEFAULT",
	WRONG_INFO VARCHAR2(4000 char) NULL COLLATE "DEFAULT",
	REMARK VARCHAR2(1000 char) NULL DEFAULT NULL::varchar2(4000) COLLATE "DEFAULT",
	REMARK2 VARCHAR2(1000 char) NULL DEFAULT NULL::varchar2(4000) COLLATE "DEFAULT",
	PORT_CLASS VARCHAR2(255 char) NULL COLLATE "DEFAULT",
	RESFAPORT VARCHAR2(255 char) NULL DEFAULT NULL::varchar2(4000) COLLATE "DEFAULT",
	RERATE VARCHAR2(255 char) NULL DEFAULT NULL::varchar2(4000) COLLATE "DEFAULT",
	QRCODE VARCHAR2(255 char) NULL DEFAULT NULL::varchar2(4000) COLLATE "DEFAULT",
	DATE_MANAGE VARCHAR2(255 char) NULL DEFAULT NULL::varchar2(4000) COLLATE "DEFAULT",
	"LOCATION" VARCHAR2(255 char) NULL COLLATE "DEFAULT",
	PORT_NO VARCHAR2(255 char) NULL DEFAULT NULL::varchar2(4000) COLLATE "DEFAULT",
	DATA_MANAGER VARCHAR2(256 char) NULL COLLATE "DEFAULT",
	COUNTY_ID NUMBER NULL,
	RELATED_TYPE VARCHAR2(200 char) NULL DEFAULT NULL::varchar2(4000) COLLATE "DEFAULT",
	CIRCUIT_ID VARCHAR2(255 char) NULL COLLATE "DEFAULT",
	CONSTRAINT RMS_BSSPORT_SYS_C0060416 PRIMARY KEY (int_id, city_id, stateflag)
)
PARTITION BY RANGE (stateflag);
CREATE INDEX rms_bssport_city_bssport ON ONLY enrol11.rms_bssport USING btree (city_id);
CREATE INDEX rms_bssport_rms_bssport_idx1 ON ONLY enrol11.rms_bssport USING btree (stateflag);
CREATE INDEX rms_bssport_rms_bssport_idx2 ON ONLY enrol11.rms_bssport USING btree (port_type);
CREATE INDEX rms_bssport_rms_bssport_idx5 ON ONLY enrol11.rms_bssport USING btree (circuit_name);
CREATE UNIQUE INDEX rms_bssport_rms_bssport_modify ON ONLY enrol11.rms_bssport USING btree (int_id, stateflag, city_id);
CREATE INDEX rms_bssport_rms_bssport_pk_1 ON ONLY enrol11.rms_bssport USING btree (status);
CREATE INDEX rms_bssport_rms_bssport_pk_2 ON ONLY enrol11.rms_bssport USING btree (zh_label);
CREATE INDEX rms_bssport_rms_bssport_pk_3 ON ONLY enrol11.rms_bssport USING btree (related_ne_id);
CREATE INDEX rms_bssport_rms_bssport_pk_4 ON ONLY enrol11.rms_bssport USING btree (zh_label, related_ne_id);
CREATE INDEX rms_bssport_rms_bssport_pk_tddf ON ONLY enrol11.rms_bssport USING btree (tddf);
CREATE INDEX rms_bssport_stateflag_int_id_zh_label_related_ne_id_idx ON ONLY enrol11.rms_bssport USING btree (stateflag, int_id, zh_label, related_ne_id);
CREATE INDEX rms_bssport_w_bssport ON ONLY enrol11.rms_bssport USING btree (is_wrong);


create table enrol11.rms_bsport1 partition of enrol11.RMS_BSSPORT for values from (0) to (6000);
create table enrol11.rms_bsport13 partition of enrol11.RMS_BSSPORT for values from (6000) to (12000);
create table enrol11.rms_bsport14 partition of enrol11.RMS_BSSPORT for values from (12000) to (22000);
create table enrol11.rms_bsport15 partition of enrol11.RMS_BSSPORT for values from (22000) to (35000);
create table enrol11.rms_bsport16 partition of enrol11.RMS_BSSPORT for values from (35000) to (48000);
create table enrol11.rms_bsport17 partition of enrol11.RMS_BSSPORT for values from (48000) to (51000);
create table enrol11.rms_bsport18 partition of enrol11.RMS_BSSPORT for values from  (51000) to (64000);
create table enrol11.rms_bsport19 partition of enrol11.RMS_BSSPORT for values from (64000) to (87000);
create table enrol11.rms_bsport110 partition of enrol11.RMS_BSSPORT for values from (87000) to (100000);
create table enrol11.rms_bsport111 partition of enrol11.RMS_BSSPORT for values from  (100000) to (130000);
create table enrol11.rms_bsport112 partition of enrol11.RMS_BSSPORT for values from (130000) to (150000);
create table enrol11.rms_bsport113 partition of enrol11.RMS_BSSPORT for values from (150000) to (160000);
create table enrol11.rms_bsport114 partition of enrol11.RMS_BSSPORT for values from  (160000) to (200000);
create table enrol11.rms_bsport115 partition of enrol11.RMS_BSSPORT for values from (200000) to (210000);
create table enrol11.rms_bsport116 partition of enrol11.RMS_BSSPORT for values from  (210000) to (300000);
create table enrol11.rms_bsport117 partition of enrol11.RMS_BSSPORT for values from (300000) to (maxvalue);


create table enrol11.test_text(id varchar2(256));

DO $d$
    BEGIN
        EXECUTE $$ create public database link tdblnk
                        connect to enrol11 identified by "ivy123"
                        using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = regression)
            ))'$$;
    END;
$d$;


create or replace package test_pkg is
  function test_f(a integer default 1) return integer;
  function test_f_d(a integer default 1) return integer deterministic;
end;
/



create or replace package body test_pkg is
        function test_f(a integer default 1) return integer is
        begin
                raise notice '1';
                return a;
        end;
        function test_f_d(a integer default 1) return integer is
        begin
                raise notice '1';
                return a;
        end;
end;
/



explain(costs off) select * from enrol11.RMS_BSSPORT@tdblnk where int_id = '100000';
explain(costs off) select * from enrol11.RMS_BSSPORT@tdblnk where test_pkg.test_f(int_id) = '100000';
explain(costs off) select * from enrol11.test_text@tdblnk where id = '100000';

--explain(costs off) select * from enrol11.test_text@tdblnk where standard.test_to_char(id) = '10000';

set ivorysql.enable_compatible_type_shippable = off;

explain(costs off) select * from enrol11.RMS_BSSPORT@tdblnk where int_id = '100000';
explain(costs off) select * from enrol11.RMS_BSSPORT@tdblnk where test_pkg.test_f(int_id) = '100000';
explain(costs off) select * from enrol11.test_text@tdblnk where id = '100000';
--explain(costs off) select * from enrol11.test_text@tdblnk where standard.test_to_char(id) = '10000';


--test bug#5514
set ivorysql.enable_compatible_type_shippable = on;
set ivorysql.first_normal_object_id = 1584;

explain(costs off) select * from enrol11.RMS_BSSPORT@tdblnk where int_id = '100000';
explain(costs off) select * from enrol11.RMS_BSSPORT@tdblnk where test_pkg.test_f(int_id) = '100000';
explain(costs off) select * from enrol11.test_text@tdblnk where id = '100000';

set ivorysql.first_normal_object_id = 36384;

explain(costs off) select * from enrol11.RMS_BSSPORT@tdblnk where int_id = '100000';
explain(costs off) select * from enrol11.RMS_BSSPORT@tdblnk where test_pkg.test_f(int_id) = '100000';
explain(costs off) select * from enrol11.test_text@tdblnk where id = '100000';

--failed
select ora_set_first_normal_objectid();

--clean data
drop package test_pkg;
drop public database link tdblnk;
drop table enrol11.RMS_BSSPORT;
drop table enrol11.test_text;
drop schema enrol11;
