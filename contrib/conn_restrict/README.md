#CONN_RESTRICT EXTENSION

Description:
	Client connect restriction support， for MAC address and time span.

Modification:
Minimum intern changes, most code is in the extension.


Usage:
1.Login as sysdba, create extension conn_restrict;
2.Login as syssso;
3.Call hg_secsch.add_rules to add rules, call hg_secsch.remove_rules to remove rules;
4.Modify postgresql.conf, add shared_preload_libraries = 'conn_restrict', add a new GUC param: conn_restrict.enable = on;
5.Restart the db to check if the rules is applied;



