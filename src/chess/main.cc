
#include "movegen.hh"

#ifdef ENABLE_TESTS
#define ANKERL_NANOBENCH_IMPLEMENT
#include "bench/nanobench.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#else
int main(int, char *[]) {

}
#endif // ENABLE_TESTS
