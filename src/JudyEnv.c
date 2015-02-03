#include <stdlib.h>
#include "Rinternals.h"
#include "Rdefines.h"
#include "R_ext/Callbacks.h"
#include "R_ext/Rdynload.h"
#include "R_ext/Print.h"
#include <assert.h>
#include <Judy.h>

#define JUDY_PJERR_TEST(x) do { \
    if (x == PJERR) \
	errorcall(R_NilValue, "judy array memory exhausted (PJERR)"); \
} while (0)
#define JUDY_JERR_TEST(x) do { \
    if (x == JERR) \
	errorcall(R_NilValue, "judy array memory exhausted (JERR)"); \
} while (0)

#define EXTRACT_JUDY_TABLE(table,x) do { \
    table = (judy_table *) x->privateData; \
} while (0)

typedef struct judy_table_t {
    SEXP prot;
    Pvoid_t table;
    int count;
} judy_table;

static Rboolean udb_exists(const char * const symstr, Rboolean *canCache, R_ObjectTable *udb){
    judy_table *jtab;
    uint8_t *jindex;
    Word_t *jvalue;

    if(udb->active == FALSE) return FALSE;

    EXTRACT_JUDY_TABLE(jtab,udb);

    jindex = (uint8_t *)symstr;
    JSLG(jvalue, jtab->table, jindex);
    JUDY_PJERR_TEST(jvalue);

    return (jvalue == NULL)? FALSE : TRUE;
}

static SEXP udb_get(const char * const name, Rboolean *canCache, R_ObjectTable *udb){
    SEXP extptr;
    judy_table *jtab;
    uint8_t *jindex;
    Word_t *jvalue;

    EXTRACT_JUDY_TABLE(jtab,udb);

    jindex = (uint8_t *)name;
    JSLG(jvalue, jtab->table, jindex);
    JUDY_PJERR_TEST(jvalue);

    if (jvalue == NULL) return R_UnboundValue;

    extptr = (SEXP) *jvalue;
    return R_ExternalPtrTag(extptr);
}

static int udb_remove(const char * const name, R_ObjectTable *udb){
    SEXP extptr, back_extptr, cdr_extptr;
    judy_table *jtab;
    uint8_t *jindex;
    Word_t *jvalue;
    int deleted;

    EXTRACT_JUDY_TABLE(jtab,udb);

    jindex = (uint8_t *)name;
    JSLG(jvalue, jtab->table, jindex);
    JUDY_PJERR_TEST(jvalue);

    if (jvalue){
	extptr = (SEXP) *jvalue;
	JSLD(deleted,jtab->table,jindex);
	if (deleted) {
	    (jtab->count)--;

	    back_extptr = (SEXP) R_ExternalPtrAddr(extptr);
	    cdr_extptr = R_ExternalPtrProtected(extptr);

	    if (back_extptr)
		R_SetExternalPtrProtected(back_extptr, cdr_extptr);
	    else
		R_SetExternalPtrProtected(jtab->prot, cdr_extptr);
	}
	return 1;
    }
    return 0;
}

static SEXP udb_assign(const char * const name, SEXP val, R_ObjectTable *udb){
    SEXP extptr, cdr_extptr;
    judy_table *jtab;
    uint8_t *jindex;
    Word_t *jvalue;

    if(udb->active == FALSE) return R_UnboundValue;

    EXTRACT_JUDY_TABLE(jtab,udb);

    jindex = (uint8_t *)name;
    JSLG(jvalue, jtab->table, jindex);
    JUDY_PJERR_TEST(jvalue);

    if (jvalue != NULL){
	extptr = (SEXP) *jvalue;
	R_SetExternalPtrTag(extptr,val);
	return val;
    }

    /* New Symbol <-> Value assignment */
    (jtab->count)++;
    JSLI(jvalue, jtab->table, jindex);
    JUDY_PJERR_TEST(jvalue);

    /* current list of external pointers */
    cdr_extptr = R_ExternalPtrProtected(jtab->prot);
    
    /* our new external pointer value, addr: NULL, tag: val, prot: cdr_extptr */
    PROTECT(extptr = R_MakeExternalPtr(NULL,val,cdr_extptr));

    /* Assignment here places the external pointer in the judy table */
    *jvalue = (Word_t) extptr;

    /* Replace the new list */
    R_SetExternalPtrProtected(jtab->prot, extptr);

    /* Fill in back pointer if not null */
    if (cdr_extptr != R_NilValue)
	R_SetExternalPtrAddr(cdr_extptr,(void *)cdr_extptr);

    UNPROTECT(1);

    return val;

}

static SEXP udb_objects(R_ObjectTable *udb){
    SEXP ans;
    judy_table *jtab;
    uint8_t jindex[1000000];
    Word_t *jvalue;
    int i=0;

    EXTRACT_JUDY_TABLE(jtab,udb);

    PROTECT(ans = allocVector(STRSXP, jtab->count));

    jindex[0] = '\0';
    JSLF(jvalue, jtab->table, jindex);
    while (jvalue != NULL){
	SET_STRING_ELT(ans,i++,mkChar((const char *)jindex));
	JSLN(jvalue, jtab->table, jindex);
    }

    UNPROTECT(1);
    return ans;
}

void DestroyJudyEnv(SEXP x){
    R_ObjectTable *udb;
    judy_table *jtab;
    Word_t bytes;

    if (TYPEOF(x) != EXTPTRSXP)
	errorcall(R_NilValue,"not an external pointer (DestroyJudyEnv)");
    udb = R_ExternalPtrAddr(x);
    jtab = (judy_table *) udb->privateData;
    if (jtab) JSLFA(bytes,jtab->table);
    free(udb->privateData);
    free(udb);
}


SEXP NewJudyEnv(SEXP e){
    R_ObjectTable *udb;
    judy_table *jtab;
    SEXP hashtab;
    
    udb = (R_ObjectTable *) calloc(1,sizeof(R_ObjectTable));
    udb->active = TRUE;
    jtab = calloc(1,sizeof(judy_table));
    udb->privateData = jtab;

    udb->exists = udb_exists;
    udb->get = udb_get;
    udb->remove = udb_remove;
    udb->assign = udb_assign;
    udb->objects = udb_objects;

    PROTECT(hashtab = R_MakeExternalPtr(udb, R_NilValue, R_NilValue));
    R_RegisterCFinalizer(hashtab,DestroyJudyEnv);

    SET_HASHTAB(e,hashtab);

    /* We'll protect values in the hash table by using a doubly linked list
       of external pointers: EXTPTR_PTR() is the back pointer, EXTPTR_TAG is the value,
       and EXTPTR_PROT is the rest of the list */
    jtab->prot = hashtab;

    UNPROTECT(1);

    return e;
}

static R_CallMethodDef callMethods[]  = {
	{"NewJudyEnv", (DL_FUNC)&NewJudyEnv, 1},
	{NULL, NULL, 0}
};

void R_init_JudyEnv(DllInfo *info){
	R_registerRoutines(info, NULL, callMethods, NULL, NULL);
}
