/*-------------------------------------------------------------------------
 *
 * user.h
 *	  Commands for manipulating roles (formerly called users).
 *
 *
 * src/include/commands/user.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef USER_H
#define USER_H

#include "catalog/objectaddress.h"
#include "libpq/crypt.h"
#include "nodes/parsenodes.h"
#include "parser/parse_node.h"

/* GUC. Is actually of type PasswordType. */
extern int	Password_encryption;

/* Hook to check passwords in CreateRole() and AlterRole() */
typedef void (*check_password_hook_type) (const char *username, const char *shadow_pass, PasswordType password_type, Datum validuntil_time, bool validuntil_null);

extern PGDLLIMPORT check_password_hook_type check_password_hook;

extern Oid	CreateRole(ParseState *pstate, CreateRoleStmt *stmt);
extern Oid	AlterRole(AlterRoleStmt *stmt);
extern Oid	AlterRoleSet(AlterRoleSetStmt *stmt);
extern void DropRole(DropRoleStmt *stmt);
extern void GrantRole(GrantRoleStmt *stmt);
extern ObjectAddress RenameRole(const char *oldname, const char *newname);
extern void DropOwnedObjects(DropOwnedStmt *stmt);
extern void ReassignOwnedObjects(ReassignOwnedStmt *stmt);
extern List *roleSpecsToIds(List *memberNames);
/* Begin - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
extern void createDBDefaultResGroup(Oid dboid);
extern void dropDBDefaultResGroup(Oid dboid);
/* End - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
/* Begin - ReqID:SRS-MISC-USER */
extern void createsysrole(const char *srolname, Oid dboid, Oid templateid);
extern bool isinstancelevel(void);
extern Oid FindUserLevelbyoid(Oid useroid);
extern Oid CurrentUserOwnDboid(void);
extern bool get_role_canlogin(const char *srolname);
extern bool get_role_canlogin_byoid(Oid userid);
extern char *CurrentUsername(void);
/* Begin - ReqID:SRS-MISC-USER */

/* Begin - Req: SRS-MYSQL-PROTOCOL */
typedef char * (*mysql_encryt_password_hook_type) (PasswordType target_type, const char *rolename, const char *password);
extern PGDLLIMPORT mysql_encryt_password_hook_type mysql_encrypt_password_hook;
/* End - Req: SRS-MYSQL-PROTOCOL */

#endif							/* USER_H */
