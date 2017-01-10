/*********************************************************************
 *   Copyright 2016, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *********************************************************************/

#include "d4includes.h"
#include <stdarg.h>
#include <assert.h>
#include "ezxml.h"
#include "d4includes.h"
#include "d4odom.h"

/**
This code serves two purposes
1. Preprocess the dap4 serialization wrt endianness, etc.
   (NCD4_processdata)
2. Walk a specified variable instance to convert to netcdf4
   memory representation.
   (NCD4_fillinstance)

*/

/***************************************************/
/* Forwards */
static int fillstring(NCD4meta*, void** offsetp, void* dst, NClist* blobs);
static int fillopfixed(NCD4meta*, d4size_t opaquesize, void** offsetp, void* dst);
static int fillopvar(NCD4meta*, NCD4node* type, void** offsetp, void* dst, NClist* blobs);
static int fillstruct(NCD4meta*, NCD4node* type, d4size_t instancesize, void** offsetp, void* dst, NClist* blobs);
static int fillseq(NCD4meta*, NCD4node* type, d4size_t instancesize, void** offsetp, void* dst, NClist* blobs);

/***************************************************/
/* Macro define procedures */

#ifdef D4DUMPCSUM
static unsigned int debugcrc32(unsigned int crc, const void *buf, size_t size)
{
    int i;
    fprintf(stderr,"crc32: ");
    for(i=0;i<size;i++) {fprintf(stderr,"%02x",((unsigned char*)buf)[i]);}
    fprintf(stderr,"\n");
    return NCD4_crc32(crc,buf,size);
}
#define CRC32 debugcrc32
#else
#define CRC32 NCD4_crc32
#endif

#if 0
#define D4MALLOC(n) d4bytesalloc(d4bytes,(n))
#endif

#define ISTOPLEVEL(var) ((var)->container == NULL || (var)->container->sort == NCD4_GROUP)

/***************************************************/
/* API */

int
NCD4_processdata(NCD4meta* meta)
{
    int ret = NC_NOERR;
    int i;
    NClist* toplevel;
    NCD4node* root = meta->root;
    void* offset;

    /* Recursively walk the tree in prefix order 
       to get the top-level variables; also mark as unvisited */
    toplevel = nclistnew();
    NCD4_getToplevelVars(meta,root,toplevel);

    /* If necessary, byte swap the serialized data */

    /* There are two state wrt checksumming.
       1. the incoming data may have checksums,
          but we are not computing a local checksum.
          Flag for this is meta->checksumming
       2. the incoming data does not have checksums at all.
          Flag for this is meta->nochecksum
    */
    meta->checksumming = (meta->checksummode != NCD4_CSUM_NONE);
    /* However, if the data sent by the server says its does not have checksums,
       then do not bother */
    if(meta->serial.nochecksum)
	meta->checksumming = 0;

    /* Do we need to swap the dap4 data? */
    meta->swap = (meta->serial.hostlittleendian != meta->serial.remotelittleendian);

    /* Compute the  offset and size of the toplevel vars in the raw dap data. */
    offset = meta->serial.dap;
    for(i=0;i<nclistlength(toplevel);i++) {
	NCD4node* var = (NCD4node*)nclistget(toplevel,i);
        if((ret=NCD4_delimit(meta,var,&offset)))
	    FAIL(ret,"delimit failure");
    }

    /* Swap the data for each top level variable,
	including the checksum (if any)
    */
    if(meta->swap) {
        if((ret=NCD4_swapdata(meta,toplevel)))
	    FAIL(ret,"byte swapping failed");
    }

    /* Compute the checksums of the top variables */
    if(meta->checksumming) {
	for(i=0;i<nclistlength(toplevel);i++) {
	    unsigned int csum = 0;
	    NCD4node* var = (NCD4node*)nclistget(toplevel,i);
            csum = CRC32(csum,var->data.dap4data.memory,var->data.dap4data.size);
            var->data.localchecksum = csum;
	}
    }

    /* verify checksums */
    if(meta->checksummode != NCD4_CSUM_NONE) {
        for(i=0;i<nclistlength(toplevel);i++) {
	    NCD4node* var = (NCD4node*)nclistget(toplevel,i);
	    if(var->data.localchecksum != var->data.remotechecksum) {
		fprintf(stderr,"Checksum mismatch: %s\n",var->name);
		abort();
	    }
        }
    }
done:
    return THROW(ret);
}

/*
Build a single instance of a type. The blobs
argument accumulates any malloc'd data so we can
reclaim it in case of an error.

Activity is to walk the variable's data to
produce a copy that is compatible with the
netcdf4 memory format.

Assumes that NCD4_processdata has been called.
*/

int
NCD4_fillinstance(NCD4meta* meta, NCD4node* type, d4size_t instancesize, void** offsetp, void* dst, NClist* blobs)
{
    int ret = NC_NOERR;
    void* offset = *offsetp;

    /* If the type is fixed size, then just copy it */
    if(type->subsort <= NC_UINT64) {
	memcpy(dst,offset,instancesize);
	offset += instancesize;
    } else if(type->subsort == NC_STRING) {/* oob strings */
	if((ret=fillstring(meta,&offset,dst,blobs)))
	    FAIL(ret,"fillinstance");
    } else if(type->subsort == NC_OPAQUE) {
	if(type->opaque.size > 0) {
	    /* We know the size and its the same for all instances */
	    if((ret=fillopfixed(meta,type->opaque.size,&offset,dst)))
	        FAIL(ret,"fillinstance");
	} else {
	    /* Size differs per instance, so we need to convert each opaque to a vlen */
	    if((ret=fillopvar(meta,type,&offset,dst,blobs)))
	        FAIL(ret,"fillinstance");
	}
    } else if(type->subsort == NC_STRUCT) {
	if((ret=fillstruct(meta,type,instancesize,&offset,dst,blobs)))
            FAIL(ret,"fillinstance");
    } else if(type->subsort == NC_SEQ) {
	if((ret=fillseq(meta,type,instancesize,&offset,dst,blobs)))
            FAIL(ret,"fillinstance");
    } else {
	ret = NC_EINVAL;
        FAIL(ret,"fillinstance");
    }
    *offsetp = offset; /* return just past this object in dap data */
done:
    return THROW(ret);
}

static int
fillstruct(NCD4meta* meta, NCD4node* type, d4size_t instancesize, void** offsetp, void* dst, NClist* blobs)
{
    int i,ret = NC_NOERR;
    void* offset = *offsetp;
 
    /* Walk and read each field taking alignments into account */
    for(i=0;i<nclistlength(type->vars);i++) {
	NCD4node* field = nclistget(type->vars,i);
	NCD4node* ftype = field->basetype;
	void* fdst = dst + field->meta.offset;
	d4size_t fieldsize = field->meta.memsize;
	if((ret=NCD4_fillinstance(meta,ftype,fieldsize,&offset,fdst,blobs)))
            FAIL(ret,"fillstruct");
    }
    
done:
    return THROW(ret);
}

static int
fillseq(NCD4meta* meta, NCD4node* type, d4size_t instancesize, void** offsetp, void* dst, NClist* blobs)
{
    int ret = NC_NOERR;
    return ret;
}

/*
Extract and oob a single string instance
*/
static int
fillstring(NCD4meta* meta, void** offsetp, void* dst, NClist* blobs)
{
    int ret = NC_NOERR;
    d4size_t count;    
    void* offset = *offsetp;
    char** stringp;
    char* q;

    /* Get string count (remember, it is already properly swapped) */
    count = GETCOUNTER(offset);
    SKIPCOUNTER(offset);
    /* Transfer out of band */
    q = (char*)malloc(count+1);
    if(q == NULL)
	{FAIL(NC_ENOMEM,"out of space");}
    nclistpush(blobs,q);
    memcpy(q,offset,count);
    q[count] = '\0';
    /* Write the pointer to the string */
    stringp = (char**)dst;
    *stringp = q;
    offset += count;
    *offsetp = offset;
done:
    return THROW(ret);
}

static int
fillopfixed(NCD4meta* meta, d4size_t opaquesize, void** offsetp, void* dst)
{
    int ret = NC_NOERR;
    d4size_t count;
    void* offset = *offsetp;

    /* Get opaque count */
    count = GETCOUNTER(offset);
    SKIPCOUNTER(offset);
    /* verify that it is the correct size */
    if(count != opaquesize)
        FAIL(NC_EVARSIZE,"Expected opaque size to be %lld; found %lld",opaquesize,count);
    /* move */
    memcpy(dst,offset,count);
    offset += count;
    *offsetp = offset;
done:
    return THROW(ret);
}

/*
Move a dap4 variable length opaque out of band.
We treat as if it was (in cdl) ubyte(*).
*/

static int
fillopvar(NCD4meta* meta, NCD4node* type, void** offsetp, void* dst, NClist* blobs)
{
    int ret = NC_NOERR;
    d4size_t count;
    nc_vlen_t* vlen;
    void* offset = *offsetp;
    char* q;

    /* alias dst format */
    vlen = (nc_vlen_t*)dst;

    /* Get opaque count */
    count = GETCOUNTER(offset);
    SKIPCOUNTER(offset);
    /* Transfer out of band */
    q = (char*)malloc(count);
    if(q == NULL) FAIL(NC_ENOMEM,"out of space");
    nclistpush(blobs,q);
    memcpy(q,offset,count);
    vlen->p = q;
    vlen->len = (size_t)count;
    offset += count;
    *offsetp = offset;
done:
    return THROW(ret);
}


/**************************************************/
/* Utilities */
int
NCD4_getToplevelVars(NCD4meta* meta, NCD4node* group, NClist* toplevel)
{
    int ret = NC_NOERR;
    int i;

    if(group == NULL)
	group = meta->root;

    /* Collect vars in this group */
    for(i=0;i<nclistlength(group->vars);i++) {
        NCD4node* node = (NCD4node*)nclistget(group->vars,i);
        nclistpush(toplevel,node);
        node->visited = 0; /* We will set later to indicate written vars */
#ifdef D4DEBUGDATA
fprintf(stderr,"toplevel: var=%s\n",node->name);
#endif
    }
    /* Now, recurse into subgroups; will produce prefix order */
    for(i=0;i<nclistlength(group->groups);i++) {
        NCD4node* g = (NCD4node*)nclistget(group->groups,i);
	if((ret=NCD4_getToplevelVars(meta,g,toplevel))) goto done;
    }
done:
    return THROW(ret);
}
