#include "parser/parse_type.h"

extern Oid SynnameGetRelname_recursive(const char *synname, Oid synnsp, char **objschema, char **objname, char **objdblnksrv, List **synoids);
extern bool SynnameGetFuncInfo(const char *synname,const char *synnsp, char **funcname,char **funcnsp, List **synoids);
extern Type LookupTypeNameFromSynonym(ParseState *pstate, const TypeName *typeName, int32 *typmod_p, List **synoids, bool missing_ok);
extern Oid SynnameGetPkgoid_recursive(const char *synname, Oid nspkg, List **synoids);