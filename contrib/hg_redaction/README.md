### This mudule is the data dynamic masking module,and its main function is to implement data dynamic masking.

# Data dynamic masking
	During dynamic data masking, the data is dynamically masked,
	the masked data hides the real information,
	and the data displayed to users is the masked  version.

# Create the extension of hg_redaction
	1、Modify the configuration file postgres.conf
		 Append hg_redaction shared_preload_libraries.
	2、start or restart database.
	3、create data masking extension.
		 create extension hg_redaction;
	 
# Create a dynamic data masking use case
	1、Turn on the data masking policy switch.
		 set hg_redaction.enable to on;
	2、create a table
		create table test_redaction (
			id int,
			name varchar(22),
			salary money,
			hiredate timestamp,
			telephone text
			);
	3、create a redaction policy(user uperuser or syssso can create)
		 create redaction policy p1 on test_redaction for(user = 'markbo') add column salary using hg_secsch.redact_cash, add column telephone using hg_secsch.redact_phone;
	4、You can view the create redaction policy through the table
	   hg_secsch.hg_redaction_policy，and you can alse check which tables
	   and columns have redaction policy through the table
	   hg_secsch.hg_redaction_column.
