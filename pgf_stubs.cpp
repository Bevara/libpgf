/*
 *  Two C++ runtime symbols that libpgf references and no solver exports. A
 *  side module whose imports are not all resolved never instantiates at all -
 *  the filter simply never registers - so both have to be defined here.
 *  libcharls and libape carry the same pair of workarounds.
 */

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <new>

/* libpgf allocates its subband buffers with new(std::nothrow)[] and checks the
 * result, which is exactly the contract implemented here. */
void *operator new[](std::size_t size, const std::nothrow_t &) noexcept
{
	return std::malloc(size);
}

void operator delete[](void *p, const std::nothrow_t &) noexcept
{
	std::free(p);
}

/* div() is not among the libc functions solver_minimal_1 exports, and libpgf
 * reaches it through PGFimage.cpp. */
extern "C" div_t div(int numer, int denom)
{
	div_t r;
	r.quot = numer / denom;
	r.rem = numer % denom;
	return r;
}

extern "C" void __cxa_throw(void *thrown, void *type, void (*destructor)(void *))
{
	(void)thrown;
	(void)type;
	(void)destructor;
	/* Real unwinding would need the exception machinery the solver does not
	 * export, so a throw is fatal to the module rather than recoverable. In
	 * practice libpgf only throws on malformed input, and pgf_decode.cpp
	 * rejects anything that is not a PGF stream before it opens the image, so
	 * the common "wrong file type" case never reaches this point. */
	std::fprintf(stderr, "[PGFDec] libpgf threw an exception - aborting\n");
	std::abort();
}
