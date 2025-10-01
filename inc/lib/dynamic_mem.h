#ifndef MALLOC_280_H
#define MALLOC_280_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>		/* for size_t */

#ifndef ONLY_MSPACES
#define ONLY_MSPACES 0		/* define to a value */
#elif ONLY_MSPACES != 0
#define ONLY_MSPACES 1
#endif				/* ONLY_MSPACES */
#ifndef NO_MALLINFO
#define NO_MALLINFO 0
#endif				/* NO_MALLINFO */

#ifndef MSPACES
#if ONLY_MSPACES
#define MSPACES 1
#else				/* ONLY_MSPACES */
#define MSPACES 0
#endif				/* ONLY_MSPACES */
#endif				/* MSPACES */

#if !ONLY_MSPACES

#ifndef USE_DL_PREFIX
#define dlcalloc               calloc
#define dlfree                 free
#define dlmalloc               malloc
#define dlmemalign             memalign
#define dlposix_memalign       posix_memalign
#define dlrealloc              realloc
#define dlvalloc               valloc
#define dlpvalloc              pvalloc
#define dlmallinfo             mallinfo
#define dlmallopt              mallopt
#define dlmalloc_trim          malloc_trim
#define dlmalloc_stats         malloc_stats
#define dlmalloc_usable_size   malloc_usable_size
#define dlmalloc_footprint     malloc_footprint
#define dlmalloc_max_footprint malloc_max_footprint
#define dlmalloc_footprint_limit malloc_footprint_limit
#define dlmalloc_set_footprint_limit malloc_set_footprint_limit
#define dlmalloc_inspect_all   malloc_inspect_all
#define dlindependent_calloc   independent_calloc
#define dlindependent_comalloc independent_comalloc
#define dlbulk_free            bulk_free
#endif				/* USE_DL_PREFIX */

#if !NO_MALLINFO
#ifndef HAVE_USR_INCLUDE_MALLOC_H
#ifndef _MALLOC_H
#ifndef MALLINFO_FIELD_TYPE
#define MALLINFO_FIELD_TYPE size_t
#endif				/* MALLINFO_FIELD_TYPE */
#ifndef STRUCT_MALLINFO_DECLARED
#define STRUCT_MALLINFO_DECLARED 1
	struct mallinfo {
		MALLINFO_FIELD_TYPE arena;	/* non-mmapped space allocated from system */
		MALLINFO_FIELD_TYPE ordblks;	/* number of free chunks */
		MALLINFO_FIELD_TYPE smblks;	/* always 0 */
		MALLINFO_FIELD_TYPE hblks;	/* always 0 */
		MALLINFO_FIELD_TYPE hblkhd;	/* space in mmapped regions */
		MALLINFO_FIELD_TYPE usmblks;	/* maximum total allocated space */
		MALLINFO_FIELD_TYPE fsmblks;	/* always 0 */
		MALLINFO_FIELD_TYPE uordblks;	/* total allocated space */
		MALLINFO_FIELD_TYPE fordblks;	/* total free space */
		MALLINFO_FIELD_TYPE keepcost;	/* releasable (via malloc_trim) space */
	};
#endif				/* STRUCT_MALLINFO_DECLARED */
#endif				/* _MALLOC_H */
#endif				/* HAVE_USR_INCLUDE_MALLOC_H */
#endif				/* !NO_MALLINFO */

	void *dlmalloc(size_t);

	void dlfree(void *);


	void *dlcalloc(size_t, size_t);

	void *dlrealloc(void *, size_t);

	void *dlrealloc_in_place(void *, size_t);

	void *dlmemalign(size_t, size_t);


	int dlposix_memalign(void **, size_t, size_t);

	void *dlvalloc(size_t);

	int dlmallopt(int, int);

#define M_TRIM_THRESHOLD     (-1)
#define M_GRANULARITY        (-2)
#define M_MMAP_THRESHOLD     (-3)


	size_t dlmalloc_footprint(void);


	size_t dlmalloc_max_footprint(void);


	size_t dlmalloc_footprint_limit(void);

	size_t dlmalloc_set_footprint_limit(size_t bytes);

	void dlmalloc_inspect_all(void (*handler) (void *, void *, size_t, void *), void *arg);

#if !NO_MALLINFO


	struct mallinfo dlmallinfo(void);
#endif				/* NO_MALLINFO */

	void **dlindependent_calloc(size_t, size_t, void **);


	void **dlindependent_comalloc(size_t, size_t *, void **);


	size_t dlbulk_free(void **, size_t n_elements);


	void *dlpvalloc(size_t);


	int dlmalloc_trim(size_t);


	void dlmalloc_stats(void);

#endif				/* !ONLY_MSPACES */


	size_t dlmalloc_usable_size(const void *);

#if MSPACES

/*
  mspace is an opaque type representing an independent
  region of space that supports mspace_malloc, etc.
*/
	typedef void *mspace;

	mspace create_mspace(size_t capacity, int locked);


	size_t destroy_mspace(mspace msp);

pace (if possible) but not the initial base.
	mspace create_mspace_with_base(void *base, size_t capacity, int locked);


	int mspace_track_large_chunks(mspace msp, int enable);

#if !NO_MALLINFO
/*
  mspace_mallinfo behaves as mallinfo, but reports properties of
  the given space.
*/
	struct mallinfo mspace_mallinfo(mspace msp);
#endif				/* NO_MALLINFO */

/*
  An alias for mallopt.
*/
	int mspace_mallopt(int, int);

/*
  The following operate identically to their malloc counterparts
  but operate only for the given mspace argument
*/
	void *mspace_malloc(mspace msp, size_t bytes);
	void mspace_free(mspace msp, void *mem);
	void *mspace_calloc(mspace msp, size_t n_elements, size_t elem_size);
	void *mspace_realloc(mspace msp, void *mem, size_t newsize);
	void *mspace_realloc_in_place(mspace msp, void *mem, size_t newsize);
	void *mspace_memalign(mspace msp, size_t alignment, size_t bytes);
	void **mspace_independent_calloc(mspace msp, size_t n_elements, size_t elem_size, void *chunks[]);
	void **mspace_independent_comalloc(mspace msp, size_t n_elements, size_t sizes[], void *chunks[]);
	size_t mspace_bulk_free(mspace msp, void **, size_t n_elements);
	size_t mspace_usable_size(const void *mem);
	void mspace_malloc_stats(mspace msp);
	int mspace_trim(mspace msp, size_t pad);
	size_t mspace_footprint(mspace msp);
	size_t mspace_max_footprint(mspace msp);
	size_t mspace_footprint_limit(mspace msp);
	size_t mspace_set_footprint_limit(mspace msp, size_t bytes);
	void mspace_inspect_all(mspace msp, void (*handler) (void *, void *, size_t, void *), void *arg);
#endif				/* MSPACES */

#ifdef __cplusplus
};				/* end of extern "C" */
#endif

#endif /* MALLOC_280_H */
