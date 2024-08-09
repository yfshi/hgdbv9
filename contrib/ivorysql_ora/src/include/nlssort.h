#ifndef IVORYSQL_NLSSORT_H
#define IVORYSQL_NLSSORT_H

extern Datum nlssort(PG_FUNCTION_ARGS);
extern void nlssort_init(void);
extern void nlssort_fini(void);

#endif