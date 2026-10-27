# Submission Report

- Submission generated at 10/27/2026 at 21:32:43

- Machine info: Linux runnervmr9t6g 7.0.0-1017-azure #17-Ubuntu SMP PREEMPT Thu Sep 24 15:54:51 UTC 2026 x86_64 GNU/Linux

## Note to Students

Please read this report carefully before submission.
Ensure that all sections are complete and accurate.
Look for any errors in the build or test outputs.
If you find any issues, correct them before submitting.
Post any questions on the class discussion board for help.


---

## README

# P3 - Subnetting and Forwarding

- Name: Chandler Black
- Email: chandlerblack@u.boisestate.edu
- Class: CS425-001

A command line tool that parses CIDR address blocks, computes their subnet
arithmetic, splits a block into equally sized subnets, and performs the
longest prefix match a router does on every packet.

## Building and Running

```bash
make all        # build debug, release, test, and debug-test targets
make check      # run the unit tests
make report     # run the tests and print a coverage report
make leak       # run the release program under AddressSanitizer
make leak-test  # run the unit tests under AddressSanitizer
```

The program is `./build/release/myapp`:

```
Usage: myapp info <address>/<prefix>
       myapp split <network>/<prefix> <count>
       myapp lookup <table-file> <address>...
```

```bash
./build/release/myapp info 132.178.227.11/25
./build/release/myapp split 192.168.1.0/24 4
./build/release/myapp lookup tables/example.txt 132.178.227.50 8.8.8.8 10.1.2.5 10.1.2.200
```

## Design

Every address is a `uint32_t` in host byte order, so all of the subnet
arithmetic is plain bit manipulation on a 32-bit integer. The only real
subtleties are the boundary prefixes and the lookup.

### The subnet mask and `/0`

Everything flows from `cidr_mask()`. The natural expression
`0xFFFFFFFF << (32 - prefix)` is undefined behavior when `prefix` is 0, because
C says shifting a 32-bit value by 32 bits is undefined, and on x86 the hardware
only uses the low 5 bits of the shift count, so it quietly shifts by 0 and
returns `0xFFFFFFFF` instead of 0. That single bug would make the default route
match only `0.0.0.0` and never fire in practice. `cidr_mask()` handles `prefix
== 0` as an explicit special case that returns `0`, and treats `prefix >= 32`
as `0xFFFFFFFF`, so the undefined shift never happens. Every other function
(`cidr_network`, `cidr_broadcast`, `cidr_contains`, `cidr_is_network`) builds
on that mask, so they inherit the correct `/0` behavior for free.

### `/31` and `/32`

For `/0` through `/30` the network and broadcast addresses are reserved, so the
usable range is `network + 1` to `broadcast - 1` and the host count is
`2^(32 - prefix) - 2`. The two short-link prefixes break that pattern and are
handled as explicit cases:

- A **`/31`** is a point-to-point link (RFC 3021). Both of its two addresses
  are usable, so `cidr_first_host()` returns the network address, `cidr_last_host()`
  returns the broadcast address, and `cidr_host_count()` returns 2.
- A **`/32`** is a single host. First host, last host, network, and broadcast
  are all the same address, and the host count is 1.

The `/0` host count is `2^32 - 2`. The result fits in a `uint32_t`, but the
`2^32` on the way there does not, so `cidr_host_count()` returns a `uint64_t`
and does the shift in 64 bits.

### Splitting

`cidr_subnet()` returns one subnet at a time rather than filling an array,
because splitting `0.0.0.0/0` into `2^31` pieces is a legal request and the
array would be enormous. Equal-sized pieces are only possible when the count is
a power of two, since the split borrows `log2(n)` bits from the host part, so
the function rejects any `n` that is not a power of two, rejects a block that is
not a network, and rejects a split that would push the prefix past `/32`. The
`i`-th subnet starts at `network + i * 2^(32 - new_prefix)`. That offset shift
is guarded against the `new_prefix == 0` case (only reachable when `n == 1` on a
`/0`, where `i` must be 0 and the offset is 0 anyway).

### Longest prefix match

The forwarding table is a growable array of `route_t` (a `realloc`-backed
buffer that doubles when full). `table_add()` refuses a route whose destination
has host bits set and refuses a duplicate block, matching what `ip route add`
does.

`table_lookup()` does a single linear scan over every route. For each route
whose block contains the destination (`cidr_contains`), it keeps the one with
the longest prefix seen so far. Because it compares prefix lengths rather than
returning the first hit, the result does not depend on the order routes were
added, so a table sorted shortest-first gives the same answer as one sorted
longest-first. The default route, `0.0.0.0/0`, is not a special case: it simply
has the shortest possible prefix, so it matches everything and loses to anything
more specific. That is exactly why it is the route of last resort.

A production router holding the full Internet table would walk a binary trie
instead of scanning a list; a linear scan is fine for the table sizes here.

### Parsing

The parsers are written by hand rather than using `inet_aton()` or `sscanf()`,
both of which accept inputs a router must reject. `010.0.0.1` (octal), a
trailing `junk` suffix, a `+` sign, and a prefix with a leading zero are all
rejected. A field is read octet by octet with an explicit range check, a
leading-zero check, and a check that nothing unexpected follows.

## Known Bugs or Issues

None known. All unit tests pass, line coverage of `src/lab.c` is 100%, and both
`make leak` and `make leak-test` run cleanly under AddressSanitizer.

## Experience

Building the address arithmetic from the mask up made the CIDR notation that
used to feel like magic turn into a handful of bit operations. The `/0` shift
bug and the `/31` and `/32` exceptions were the parts that needed the most care,
and writing boundary tests for `/0`, `/31`, `/32`, `0.0.0.0`, and
`255.255.255.255` caught the off-by-one mistakes that casual testing would have
missed. Getting the longest-prefix-match to be order-independent was a good
reminder that a test with a conveniently sorted table can hide a real bug.

---


## Build Output

This section was generated by running `make all` in the project root directory.

```bash
make[1]: Entering directory '/home/runner/work/p3/p3'
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/main.c -o build/debug/main.c.o
mkdir -p build/debug
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address -c src/lab.c -o build/debug/lab.c.o
cc -g -O0 -DDEBUG -fno-omit-frame-pointer -fsanitize=address build/debug/main.c.o build/debug/lab.c.o -o build/debug/myapp_d -fsanitize=address
make[1]: Leaving directory '/home/runner/work/p3/p3'
make[1]: Entering directory '/home/runner/work/p3/p3'
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/main.c -o build/release/main.c.o
mkdir -p build/release
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion -c src/lab.c -o build/release/lab.c.o
cc -Wall -Wextra -O2 -fPIE -MMD -MP -Wformat -Wformat=2 -Wconversion -Wsign-conversion -Wimplicit-fallthrough -fstack-protector-strong -Werror=format-security -Werror=implicit -Werror=incompatible-pointer-types -Werror=int-conversion build/release/main.c.o build/release/lab.c.o -o build/release/myapp 
make[1]: Leaving directory '/home/runner/work/p3/p3'
make[1]: Entering directory '/home/runner/work/p3/p3'
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/main.c -o build/tests/main.c.o
mkdir -p build/tests
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c src/lab.c -o build/tests/lab.c.o
mkdir -p build/tests/harness/
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c tests/harness/unity.c -o build/tests/harness/unity.c.o
mkdir -p build/tests/
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage -c tests/lab-test.c -o build/tests/lab-test.c.o
cc -g -O0 -DTEST -fprofile-arcs -ftest-coverage build/tests/main.c.o build/tests/lab.c.o build/tests/harness/unity.c.o build/tests/lab-test.c.o -o build/tests/myapp_t -fprofile-arcs -ftest-coverage
make[1]: Leaving directory '/home/runner/work/p3/p3'
make[1]: Entering directory '/home/runner/work/p3/p3'
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/main.c -o build/debug-test/main.c.o
mkdir -p build/debug-test
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c src/lab.c -o build/debug-test/lab.c.o
mkdir -p build/debug-test/harness/
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c tests/harness/unity.c -o build/debug-test/harness/unity.c.o
mkdir -p build/debug-test/
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address -c tests/lab-test.c -o build/debug-test/lab-test.c.o
cc -g -O0 -DDEBUG -DTEST -fno-omit-frame-pointer -fsanitize=address build/debug-test/main.c.o build/debug-test/lab.c.o build/debug-test/harness/unity.c.o build/debug-test/lab-test.c.o -o build/debug-test/myapp_td -fsanitize=address
make[1]: Leaving directory '/home/runner/work/p3/p3'
Builds completed. You can run the application with: ./build/release/myapp
You can run the debug build with: ./build/debug/myapp_d
You can run the test build with: ./build/tests/myapp_t
You can run the debug-test build with: ./build/debug-test/myapp_td
```

---

## Coverage Report

This section was generated by running `make report` in the project root directory.

```bash
tests/lab-test.c:734:test_ip_parse_valid:PASS
tests/lab-test.c:735:test_ip_parse_rejects_malformed:PASS
tests/lab-test.c:736:test_ip_format:PASS
tests/lab-test.c:737:test_cidr_parse_valid:PASS
tests/lab-test.c:738:test_cidr_parse_rejects_malformed:PASS
tests/lab-test.c:739:test_cidr_format:PASS
tests/lab-test.c:741:test_cidr_mask:PASS
tests/lab-test.c:742:test_cidr_network_broadcast:PASS
tests/lab-test.c:743:test_cidr_host_range_normal:PASS
tests/lab-test.c:744:test_cidr_host_range_boundaries:PASS
tests/lab-test.c:745:test_cidr_contains:PASS
tests/lab-test.c:746:test_cidr_is_network:PASS
tests/lab-test.c:747:test_cidr_subnet_basic:PASS
tests/lab-test.c:748:test_cidr_subnet_boundaries:PASS
tests/lab-test.c:749:test_cidr_subnet_rejects:PASS
tests/lab-test.c:751:test_route_parse_valid:PASS
tests/lab-test.c:752:test_route_parse_rejects:PASS
tests/lab-test.c:753:test_table_create_destroy_count:PASS
tests/lab-test.c:754:test_table_add:PASS
tests/lab-test.c:755:test_table_load_good:PASS
tests/lab-test.c:756:test_table_load_bad_line_in_middle:PASS
tests/lab-test.c:757:test_table_load_duplicate_line:PASS
tests/lab-test.c:758:test_table_load_comment_only_and_bad_line_number:PASS
tests/lab-test.c:759:test_table_load_null_args:PASS
tests/lab-test.c:760:test_table_lookup_longest_prefix:PASS
tests/lab-test.c:761:test_table_lookup_order_independent:PASS
tests/lab-test.c:762:test_table_lookup_default_route:PASS
tests/lab-test.c:763:test_table_lookup_empty_table:PASS

-----------------------
28 Tests 0 Failures 0 Ignored 
OK
./build/tests/myapp_t
tests/lab-test.c:734:test_ip_parse_valid:PASS
tests/lab-test.c:735:test_ip_parse_rejects_malformed:PASS
tests/lab-test.c:736:test_ip_format:PASS
tests/lab-test.c:737:test_cidr_parse_valid:PASS
tests/lab-test.c:738:test_cidr_parse_rejects_malformed:PASS
tests/lab-test.c:739:test_cidr_format:PASS
tests/lab-test.c:741:test_cidr_mask:PASS
tests/lab-test.c:742:test_cidr_network_broadcast:PASS
tests/lab-test.c:743:test_cidr_host_range_normal:PASS
tests/lab-test.c:744:test_cidr_host_range_boundaries:PASS
tests/lab-test.c:745:test_cidr_contains:PASS
tests/lab-test.c:746:test_cidr_is_network:PASS
tests/lab-test.c:747:test_cidr_subnet_basic:PASS
tests/lab-test.c:748:test_cidr_subnet_boundaries:PASS
tests/lab-test.c:749:test_cidr_subnet_rejects:PASS
tests/lab-test.c:751:test_route_parse_valid:PASS
tests/lab-test.c:752:test_route_parse_rejects:PASS
tests/lab-test.c:753:test_table_create_destroy_count:PASS
tests/lab-test.c:754:test_table_add:PASS
tests/lab-test.c:755:test_table_load_good:PASS
tests/lab-test.c:756:test_table_load_bad_line_in_middle:PASS
tests/lab-test.c:757:test_table_load_duplicate_line:PASS
tests/lab-test.c:758:test_table_load_comment_only_and_bad_line_number:PASS
tests/lab-test.c:759:test_table_load_null_args:PASS
tests/lab-test.c:760:test_table_lookup_longest_prefix:PASS
tests/lab-test.c:761:test_table_lookup_order_independent:PASS
tests/lab-test.c:762:test_table_lookup_default_route:PASS
tests/lab-test.c:763:test_table_lookup_empty_table:PASS

-----------------------
28 Tests 0 Failures 0 Ignored 
OK
mkdir -p ./build/report/html
mkdir -p ./build/report/txt
gcovr -r . --html --html-details --exclude-directories build/tests/harness --exclude '.*main\.c$' --exclude '.*test\.c$' -o ./build/report/html/coverage_report.html
gcovr -r . --txt                 --exclude-directories build/tests/harness --exclude '.*main\.c$' --exclude '.*test\.c$'
------------------------------------------------------------------------------
                           GCC Code Coverage Report
Directory: .
------------------------------------------------------------------------------
File                                         Lines     Exec  Cover   Missing
------------------------------------------------------------------------------
src/lab.c                                      244      244   100%
------------------------------------------------------------------------------
TOTAL                                          244      244   100%
------------------------------------------------------------------------------
```

---

## Address Sanitizer Report

This section was generated by running `make leak-test` in the project root directory.

```bash
tests/lab-test.c:734:test_ip_parse_valid:PASS
tests/lab-test.c:735:test_ip_parse_rejects_malformed:PASS
tests/lab-test.c:736:test_ip_format:PASS
tests/lab-test.c:737:test_cidr_parse_valid:PASS
tests/lab-test.c:738:test_cidr_parse_rejects_malformed:PASS
tests/lab-test.c:739:test_cidr_format:PASS
tests/lab-test.c:741:test_cidr_mask:PASS
tests/lab-test.c:742:test_cidr_network_broadcast:PASS
tests/lab-test.c:743:test_cidr_host_range_normal:PASS
tests/lab-test.c:744:test_cidr_host_range_boundaries:PASS
tests/lab-test.c:745:test_cidr_contains:PASS
tests/lab-test.c:746:test_cidr_is_network:PASS
tests/lab-test.c:747:test_cidr_subnet_basic:PASS
tests/lab-test.c:748:test_cidr_subnet_boundaries:PASS
tests/lab-test.c:749:test_cidr_subnet_rejects:PASS
tests/lab-test.c:751:test_route_parse_valid:PASS
tests/lab-test.c:752:test_route_parse_rejects:PASS
tests/lab-test.c:753:test_table_create_destroy_count:PASS
tests/lab-test.c:754:test_table_add:PASS
tests/lab-test.c:755:test_table_load_good:PASS
tests/lab-test.c:756:test_table_load_bad_line_in_middle:PASS
tests/lab-test.c:757:test_table_load_duplicate_line:PASS
tests/lab-test.c:758:test_table_load_comment_only_and_bad_line_number:PASS
tests/lab-test.c:759:test_table_load_null_args:PASS
tests/lab-test.c:760:test_table_lookup_longest_prefix:PASS
tests/lab-test.c:761:test_table_lookup_order_independent:PASS
tests/lab-test.c:762:test_table_lookup_default_route:PASS
tests/lab-test.c:763:test_table_lookup_empty_table:PASS

-----------------------
28 Tests 0 Failures 0 Ignored 
OK
```

---

## Src Files
### lab.c

```c

/* getline() and ssize_t are POSIX; request them before any header. */
#define _POSIX_C_SOURCE 200809L

#include "lab.h"

#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

/* ------------------------------------------------------------------------
 * Task 1 - Address parsing and formatting
 * --------------------------------------------------------------------- */

/*
 * Parse exactly one decimal octet (0..255) from *p, advancing *p past it.
 * Rejects empty input, non-digits, a leading zero on a multi-digit number,
 * and any value above 255. Returns true and stores the value on success.
 */
static bool parse_octet(const char **p, uint32_t *value) {
  const char *s = *p;

  if (*s < '0' || *s > '9') {
    return false; /* must start with a digit */
  }

  /* A leading zero is only allowed when the whole octet is "0". */
  if (*s == '0' && (s[1] >= '0' && s[1] <= '9')) {
    return false;
  }

  uint32_t v = 0;
  size_t digits = 0;
  while (*s >= '0' && *s <= '9') {
    v = v * 10 + (uint32_t)(*s - '0');
    digits++;
    if (v > 255 || digits > 3) {
      return false;
    }
    s++;
  }

  *value = v;
  *p = s;
  return true;
}

/*
 * Parse four dotted octets starting at *p, advancing *p past the last octet.
 * Does not care what follows; the caller checks that.
 */
static bool parse_dotted_quad(const char **p, uint32_t *addr) {
  uint32_t octets[4];

  for (int i = 0; i < 4; i++) {
    if (!parse_octet(p, &octets[i])) {
      return false;
    }
    if (i < 3) {
      if (**p != '.') {
        return false;
      }
      (*p)++; /* consume the '.' */
    }
  }

  *addr = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
  return true;
}

bool ip_parse(const char *text, uint32_t *addr) {
  if (text == NULL || addr == NULL) {
    return false;
  }

  const char *p = text;
  uint32_t result;
  if (!parse_dotted_quad(&p, &result)) {
    return false;
  }

  /* Nothing may follow the final octet. */
  if (*p != '\0') {
    return false;
  }

  *addr = result;
  return true;
}

void ip_format(uint32_t addr, char out[IP_STR_LEN]) {
  snprintf(out, IP_STR_LEN, "%u.%u.%u.%u", (addr >> 24) & 0xFF,
           (addr >> 16) & 0xFF, (addr >> 8) & 0xFF, addr & 0xFF);
}

bool cidr_parse(const char *text, cidr_t *block) {
  if (text == NULL || block == NULL) {
    return false;
  }

  const char *p = text;
  uint32_t addr;
  if (!parse_dotted_quad(&p, &addr)) {
    return false;
  }

  /* The slash and prefix are required. */
  if (*p != '/') {
    return false;
  }
  p++;

  /* Prefix: decimal 0..32, no leading zero. */
  if (*p < '0' || *p > '9') {
    return false;
  }
  if (*p == '0' && p[1] >= '0' && p[1] <= '9') {
    return false;
  }

  uint32_t prefix = 0;
  size_t digits = 0;
  while (*p >= '0' && *p <= '9') {
    prefix = prefix * 10 + (uint32_t)(*p - '0');
    digits++;
    if (prefix > 32 || digits > 2) {
      return false;
    }
    p++;
  }

  /* Nothing may follow the prefix. */
  if (*p != '\0') {
    return false;
  }

  block->addr = addr;
  block->prefix = (uint8_t)prefix;
  return true;
}

void cidr_format(cidr_t block, char out[CIDR_STR_LEN]) {
  unsigned prefix = block.prefix > 32 ? 32u : block.prefix;
  snprintf(out, CIDR_STR_LEN, "%u.%u.%u.%u/%u", (block.addr >> 24) & 0xFF,
           (block.addr >> 16) & 0xFF, (block.addr >> 8) & 0xFF,
           block.addr & 0xFF, prefix);
}

/* ------------------------------------------------------------------------
 * Task 2 - Subnet arithmetic
 * --------------------------------------------------------------------- */

uint32_t cidr_mask(uint8_t prefix) {
  if (prefix >= 32) {
    return 0xFFFFFFFFu; /* also avoids the undefined 32-bit shift */
  }
  if (prefix == 0) {
    return 0u; /* avoids the undefined shift by 32 */
  }
  return 0xFFFFFFFFu << (32 - prefix);
}

uint32_t cidr_network(cidr_t block) {
  return block.addr & cidr_mask(block.prefix);
}

uint32_t cidr_broadcast(cidr_t block) {
  return block.addr | ~cidr_mask(block.prefix);
}

uint32_t cidr_first_host(cidr_t block) {
  uint8_t prefix = block.prefix > 32 ? 32 : block.prefix;
  if (prefix >= 31) {
    return cidr_network(block); /* /31 and /32: no network address reserved */
  }
  return cidr_network(block) + 1;
}

uint32_t cidr_last_host(cidr_t block) {
  uint8_t prefix = block.prefix > 32 ? 32 : block.prefix;
  if (prefix >= 31) {
    return cidr_broadcast(block); /* /31 and /32: no broadcast reserved */
  }
  return cidr_broadcast(block) - 1;
}

uint64_t cidr_host_count(cidr_t block) {
  uint8_t prefix = block.prefix > 32 ? 32 : block.prefix;
  if (prefix == 32) {
    return 1;
  }
  if (prefix == 31) {
    return 2;
  }
  /* 2^(32 - prefix) - 2, done in 64 bits so /0 does not overflow. */
  return ((uint64_t)1 << (32 - prefix)) - 2;
}

bool cidr_contains(cidr_t block, uint32_t addr) {
  uint32_t mask = cidr_mask(block.prefix);
  return (addr & mask) == (block.addr & mask);
}

bool cidr_is_network(cidr_t block) {
  return (block.addr & ~cidr_mask(block.prefix)) == 0;
}

bool cidr_subnet(cidr_t block, uint32_t n, uint32_t i, cidr_t *out) {
  if (out == NULL) {
    return false;
  }
  if (!cidr_is_network(block)) {
    return false;
  }
  /* n must be a power of two and at least 1. */
  if (n == 0 || (n & (n - 1)) != 0) {
    return false;
  }
  if (i >= n) {
    return false;
  }

  /* Number of borrowed bits is log2(n). */
  uint32_t bits = 0;
  while ((1u << bits) < n) {
    bits++;
  }

  uint8_t base_prefix = block.prefix > 32 ? 32 : block.prefix;
  if ((uint32_t)base_prefix + bits > 32) {
    return false;
  }

  uint8_t new_prefix = (uint8_t)(base_prefix + bits);
  /* Width of each subnet, in host-bit shift terms. */
  uint32_t host_bits = 32u - new_prefix;
  uint32_t network = cidr_network(block);

  /*
   * subnet i starts at network + i * (2^host_bits). Compute the offset with a
   * shift, guarding host_bits == 32 (only when new_prefix == 0, i.e. n == 1
   * and block.prefix == 0, where i must be 0 so the offset is 0 anyway).
   */
  uint32_t offset = (host_bits >= 32) ? 0u : (i << host_bits);

  out->addr = network + offset;
  out->prefix = new_prefix;
  return true;
}

/* ------------------------------------------------------------------------
 * Task 3 - Longest prefix match
 * --------------------------------------------------------------------- */

/* Private forwarding-table layout: a growable array of routes. */
struct fwd_table {
  route_t *routes;
  size_t count;
  size_t capacity;
};

/*
 * Copy the next whitespace-delimited field from *p into buf (size buflen),
 * advancing *p past it. Leading whitespace is skipped. Returns the field
 * length, or (size_t)-1 if the field is empty (end of string) or does not fit.
 */
static size_t next_field(const char **p, char *buf, size_t buflen) {
  const char *s = *p;
  while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') {
    s++;
  }
  if (*s == '\0') {
    return (size_t)-1; /* no field */
  }

  size_t len = 0;
  while (*s != '\0' && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r') {
    if (len + 1 >= buflen) {
      return (size_t)-1; /* field too long for buffer */
    }
    buf[len++] = *s++;
  }
  buf[len] = '\0';
  *p = s;
  return len;
}

bool route_parse(const char *line, route_t *route) {
  if (line == NULL || route == NULL) {
    return false;
  }

  /* Fields are bounded; CIDR and IP strings fit in small buffers. */
  char dest_field[CIDR_STR_LEN + 8];
  char gw_field[IP_STR_LEN + 8];
  char dev_field[DEV_NAME_LEN + 8];

  const char *p = line;

  if (next_field(&p, dest_field, sizeof(dest_field)) == (size_t)-1) {
    return false;
  }
  if (next_field(&p, gw_field, sizeof(gw_field)) == (size_t)-1) {
    return false;
  }

  size_t dev_len = next_field(&p, dev_field, sizeof(dev_field));
  if (dev_len == (size_t)-1) {
    return false;
  }
  /* Device must fit in DEV_NAME_LEN including the terminator. */
  if (dev_len == 0 || dev_len >= DEV_NAME_LEN) {
    return false;
  }

  /* Nothing but trailing whitespace may follow the three fields. */
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
    p++;
  }
  if (*p != '\0') {
    return false;
  }

  /* Destination: "default" or a CIDR network with no host bits. */
  cidr_t dest;
  if (strcmp(dest_field, "default") == 0) {
    dest.addr = 0;
    dest.prefix = 0;
  } else {
    if (!cidr_parse(dest_field, &dest)) {
      return false;
    }
    if (!cidr_is_network(dest)) {
      return false;
    }
  }

  /* Gateway: "direct" (stored as 0) or a dotted quad. */
  uint32_t gateway;
  if (strcmp(gw_field, "direct") == 0) {
    gateway = 0;
  } else {
    if (!ip_parse(gw_field, &gateway)) {
      return false;
    }
  }

  route->dest = dest;
  route->gateway = gateway;
  memcpy(route->dev, dev_field, dev_len + 1);
  return true;
}

fwd_table_t *table_create(void) {
  fwd_table_t *table = malloc(sizeof(*table));
  if (table == NULL) { // GCOVR_EXCL_START
    return NULL;
  } // GCOVR_EXCL_STOP
  table->routes = NULL;
  table->count = 0;
  table->capacity = 0;
  return table;
}

void table_destroy(fwd_table_t *table) {
  if (table == NULL) {
    return;
  }
  free(table->routes);
  free(table);
}

size_t table_count(const fwd_table_t *table) {
  if (table == NULL) {
    return 0;
  }
  return table->count;
}

bool table_add(fwd_table_t *table, const route_t *route) {
  if (table == NULL || route == NULL) {
    return false;
  }
  /* Destination must name a network (no host bits set). */
  if (!cidr_is_network(route->dest)) {
    return false;
  }

  /* Reject a duplicate block (same address and prefix). */
  for (size_t idx = 0; idx < table->count; idx++) {
    if (table->routes[idx].dest.addr == route->dest.addr &&
        table->routes[idx].dest.prefix == route->dest.prefix) {
      return false;
    }
  }

  if (table->count == table->capacity) {
    size_t new_cap = table->capacity == 0 ? 4 : table->capacity * 2;
    route_t *grown = realloc(table->routes, new_cap * sizeof(*grown));
    if (grown == NULL) { // GCOVR_EXCL_START
      return false;
    } // GCOVR_EXCL_STOP
    table->routes = grown;
    table->capacity = new_cap;
  }

  table->routes[table->count] = *route;
  table->count++;
  return true;
}

bool table_load(fwd_table_t *table, FILE *in, size_t *bad_line) {
  if (table == NULL || in == NULL) {
    if (bad_line != NULL) {
      *bad_line = 0;
    }
    return false;
  }

  char *line = NULL;
  size_t cap = 0;
  ssize_t len;
  size_t line_no = 0;

  while ((len = getline(&line, &cap, in)) != -1) {
    line_no++;

    /* Strip a comment: everything from the first '#' onward. */
    char *hash = strchr(line, '#');
    if (hash != NULL) {
      *hash = '\0';
    }

    /* Skip a line that is blank once the comment is removed. */
    bool blank = true;
    for (char *c = line; *c != '\0'; c++) {
      if (*c != ' ' && *c != '\t' && *c != '\n' && *c != '\r') {
        blank = false;
        break;
      }
    }
    if (blank) {
      continue;
    }

    route_t route;
    if (!route_parse(line, &route) || !table_add(table, &route)) {
      free(line);
      if (bad_line != NULL) {
        *bad_line = line_no;
      }
      return false;
    }
  }

  free(line);
  if (bad_line != NULL) {
    *bad_line = 0;
  }
  return true;
}

const route_t *table_lookup(const fwd_table_t *table, uint32_t dest) {
  if (table == NULL) {
    return NULL;
  }

  const route_t *best = NULL;
  for (size_t idx = 0; idx < table->count; idx++) {
    const route_t *r = &table->routes[idx];
    if (cidr_contains(r->dest, dest)) {
      if (best == NULL || r->dest.prefix > best->dest.prefix) {
        best = r;
      }
    }
  }
  return best;
}

```

### lab.h

```c

#ifndef LAB_H
#define LAB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/*
 * Every address in this file is a uint32_t in host byte order, so the first
 * octet of 192.168.1.10 is the most significant byte: 0xC0A8010A.
 */

/** Room for the longest dotted quad, "255.255.255.255", and its '\0'. */
#define IP_STR_LEN 16

/** Room for the longest CIDR string, "255.255.255.255/32", and its '\0'. */
#define CIDR_STR_LEN 19

/** Room for an interface name; IFNAMSIZ on Linux, including the '\0'. */
#define DEV_NAME_LEN 16

/** An address block in CIDR notation: an address and a prefix length. */
typedef struct {
  uint32_t addr;  /**< any address in the block, host byte order */
  uint8_t prefix; /**< 0 to 32 */
} cidr_t;

/** One entry in a forwarding table. */
typedef struct {
  cidr_t dest;            /**< destination block; host bits must be zero */
  uint32_t gateway;       /**< next hop, or 0 when the block is directly connected */
  char dev[DEV_NAME_LEN]; /**< outgoing interface, for example "eth0" */
} route_t;

/** A forwarding table. Its layout is private to lab.c. */
typedef struct fwd_table fwd_table_t;

/* ------------------------------------------------------------------------
 * Task 1 - Address parsing and formatting
 * --------------------------------------------------------------------- */

/**
 * @brief Parse a dotted quad such as "192.168.1.10".
 *
 * Exactly four decimal octets from 0 to 255 separated by single periods, with
 * nothing before, between or after them. An octet with a leading zero ("010")
 * is rejected, because inet_aton() reads it as octal and that is never what a
 * person typing it meant.
 *
 * @param text The string to parse.
 * @param addr Where the address is stored on success; untouched on failure.
 * @return true if text is a valid dotted quad, false otherwise (including NULL).
 */
bool ip_parse(const char *text, uint32_t *addr);

/**
 * @brief Format an address as a dotted quad.
 * @param addr The address, host byte order.
 * @param out A buffer of at least IP_STR_LEN bytes.
 */
void ip_format(uint32_t addr, char out[IP_STR_LEN]);

/**
 * @brief Parse a block in CIDR notation such as "192.168.1.0/24".
 *
 * The address follows the rules of ip_parse() and the prefix is a decimal
 * number from 0 to 32 with no leading zero. The slash and the prefix are
 * required. Host bits may be set ("132.178.227.11/25" is an interface
 * address), so use cidr_is_network() when they must not be.
 *
 * @param text The string to parse.
 * @param block Where the block is stored on success; untouched on failure.
 * @return true if text is valid CIDR notation, false otherwise (including NULL).
 */
bool cidr_parse(const char *text, cidr_t *block);

/**
 * @brief Format a block as "a.b.c.d/p", using the address exactly as stored.
 *
 * A prefix over 32 is written as 32, the way cidr_mask() treats it.
 *
 * @param block The block.
 * @param out A buffer of at least CIDR_STR_LEN bytes.
 */
void cidr_format(cidr_t block, char out[CIDR_STR_LEN]);

/* ------------------------------------------------------------------------
 * Task 2 - Subnet arithmetic
 * --------------------------------------------------------------------- */

/**
 * @brief The subnet mask for a prefix length: /24 is 255.255.255.0.
 *
 * Careful with /0. Shifting a 32 bit value by 32 is undefined behavior in C.
 *
 * @param prefix 0 to 32; anything larger is treated as 32.
 * @return The mask, host byte order.
 */
uint32_t cidr_mask(uint8_t prefix);

/** @brief The lowest address in the block: the address with the host bits cleared. */
uint32_t cidr_network(cidr_t block);

/** @brief The highest address in the block: the address with the host bits set. */
uint32_t cidr_broadcast(cidr_t block);

/**
 * @brief The first address that can be given to a host.
 *
 * For /0 to /30 that is the network address plus one. A /31 is a point to
 * point link (RFC 3021) where both addresses are usable, and a /32 is a single
 * host, so for those two the first host is the network address.
 */
uint32_t cidr_first_host(cidr_t block);

/**
 * @brief The last address that can be given to a host.
 *
 * The broadcast address minus one for /0 to /30, and the broadcast address
 * itself for /31 and /32 (see cidr_first_host()).
 */
uint32_t cidr_last_host(cidr_t block);

/**
 * @brief How many addresses in the block can be given to hosts.
 *
 * 2^(32 - prefix) - 2 for /0 to /30, 2 for /31, and 1 for /32. For a /0 the
 * 2^32 in that formula needs 33 bits, more than a uint32_t holds, which is why
 * this returns a uint64_t.
 */
uint64_t cidr_host_count(cidr_t block);

/** @brief true if addr falls inside the block. */
bool cidr_contains(cidr_t block, uint32_t addr);

/** @brief true if the block's host bits are all zero, so it names a network. */
bool cidr_is_network(cidr_t block);

/**
 * @brief The i-th of n equally sized subnets of a block.
 *
 * Splitting a /24 into 4 gives four /26 blocks, and subnet 0 is the lowest.
 * Equal sizes are only possible when n is a power of two, and the new prefix
 * can not pass 32.
 *
 * @param block The block to split; it must be a network (cidr_is_network()).
 * @param n How many subnets, a power of two from 1 up.
 * @param i Which subnet, 0 to n - 1.
 * @param out Where the subnet is stored on success; untouched on failure.
 * @return false if out is NULL, the block is not a network, n is not a power
 *         of two, the split would need a prefix longer than 32, or i is out of
 *         range.
 */
bool cidr_subnet(cidr_t block, uint32_t n, uint32_t i, cidr_t *out);

/* ------------------------------------------------------------------------
 * Task 3 - Longest prefix match
 * --------------------------------------------------------------------- */

/**
 * @brief Parse one line of a forwarding table file.
 *
 * A line is three fields separated by spaces or tabs:
 *
 *     <destination> <gateway> <device>
 *
 * The destination is a block in CIDR notation with no host bits set, or the
 * word "default" for 0.0.0.0/0. The gateway is a dotted quad, or the word
 * "direct" for a directly connected block (stored as 0). The device is 1 to
 * DEV_NAME_LEN - 1 characters. Leading and trailing whitespace, including a
 * trailing newline, is ignored. Comments are not handled here.
 *
 * @param line The line.
 * @param route Where the route is stored on success; untouched on failure.
 * @return true if the line is a valid route, false otherwise (including NULL).
 */
bool route_parse(const char *line, route_t *route);

/**
 * @brief Create an empty forwarding table.
 * @return The table, which the caller frees with table_destroy(), or NULL if
 *         memory could not be allocated.
 */
fwd_table_t *table_create(void);

/** @brief Free a table and everything in it. NULL is allowed and does nothing. */
void table_destroy(fwd_table_t *table);

/** @brief The number of routes in the table; 0 for NULL. */
size_t table_count(const fwd_table_t *table);

/**
 * @brief Add a copy of a route to the table, growing it as needed.
 * @return false if either argument is NULL, the destination has host bits
 *         set, the table already holds a route to the same block, or memory
 *         could not be allocated. The table is unchanged on failure.
 */
bool table_add(fwd_table_t *table, const route_t *route);

/**
 * @brief Read a forwarding table file and add every route in it.
 *
 * Everything from a '#' to the end of a line is a comment, and a line that is
 * blank once the comment is removed is skipped. Every other line must be a
 * valid route (route_parse()) that table_add() accepts.
 *
 * @param table The table to add to.
 * @param in The open file to read.
 * @param bad_line If not NULL, set to the 1-based number of the first line
 *        that failed, or 0 if the arguments were NULL or the file could not
 *        be read.
 * @return true if every line loaded. On false, routes from the lines before
 *         the bad one stay in the table.
 */
bool table_load(fwd_table_t *table, FILE *in, size_t *bad_line);

/**
 * @brief Longest prefix match: the route whose block contains dest and has the
 *        longest prefix.
 *
 * A default route (0.0.0.0/0) matches everything, so it is chosen only when
 * nothing more specific does. The order the routes were added in does not
 * matter.
 *
 * @return The matching route, which stays owned by the table, or NULL if no
 *         route matches (or table is NULL).
 */
const route_t *table_lookup(const fwd_table_t *table, uint32_t dest);

#endif // LAB_H

```

### main.c

```c

#include "lab.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef TEST
#define main main_exclude
#endif

static const char *USAGE =
    "Usage: myapp info <address>/<prefix>\n"
    "       myapp split <network>/<prefix> <count>\n"
    "       myapp lookup <table-file> <address>...\n";

/* Print the usage message to the given stream. */
static void print_usage(FILE *out) { fputs(USAGE, out); }

/*
 * info <address>/<prefix>
 * Prints the address, prefix, mask, network, broadcast, host range and count.
 * Exit 1 on a malformed argument.
 */
static int cmd_info(const char *arg) {
  cidr_t block;
  if (!cidr_parse(arg, &block)) {
    fprintf(stderr, "myapp: %s: not a valid CIDR block\n", arg);
    return 1;
  }

  char buf[IP_STR_LEN];

  ip_format(block.addr, buf);
  printf("address    %s\n", buf);

  printf("prefix     /%u\n", (unsigned)block.prefix);

  ip_format(cidr_mask(block.prefix), buf);
  printf("netmask    %s\n", buf);

  ip_format(cidr_network(block), buf);
  printf("network    %s\n", buf);

  ip_format(cidr_broadcast(block), buf);
  printf("broadcast  %s\n", buf);

  ip_format(cidr_first_host(block), buf);
  printf("first host %s\n", buf);

  ip_format(cidr_last_host(block), buf);
  printf("last host  %s\n", buf);

  printf("hosts      %llu\n", (unsigned long long)cidr_host_count(block));
  return 0;
}

/*
 * split <network>/<prefix> <count>
 * Prints each of <count> equally sized subnets, lowest first.
 * Exit 1 on a malformed block, a block with host bits set, or a count that is
 * not a power of two (or otherwise invalid).
 */
static int cmd_split(const char *block_arg, const char *count_arg) {
  cidr_t block;
  if (!cidr_parse(block_arg, &block)) {
    fprintf(stderr, "myapp: %s: not a valid CIDR block\n", block_arg);
    return 1;
  }
  if (!cidr_is_network(block)) {
    fprintf(stderr, "myapp: %s: has host bits set, not a network\n", block_arg);
    return 1;
  }

  /* Parse the count: a plain decimal number, no sign, no junk. */
  if (count_arg[0] < '0' || count_arg[0] > '9') {
    fprintf(stderr, "myapp: %s: not a valid count\n", count_arg);
    return 1;
  }
  char *end = NULL;
  errno = 0;
  unsigned long count = strtoul(count_arg, &end, 10);
  if (end == count_arg || *end != '\0' || errno != 0 || count == 0 ||
      count > 0xFFFFFFFFUL) {
    fprintf(stderr, "myapp: %s: not a valid count\n", count_arg);
    return 1;
  }

  uint32_t n = (uint32_t)count;
  /* Verify it is a power of two before splitting, for a clear message. */
  if ((n & (n - 1)) != 0) {
    fprintf(stderr, "myapp: %s: count must be a power of two\n", count_arg);
    return 1;
  }

  char buf[CIDR_STR_LEN];
  for (uint32_t i = 0; i < n; i++) {
    cidr_t sub;
    if (!cidr_subnet(block, n, i, &sub)) {
      fprintf(stderr, "myapp: cannot split %s into %s subnets\n", block_arg,
              count_arg);
      return 1;
    }
    cidr_format(sub, buf);
    printf("%s\n", buf);
  }
  return 0;
}

/*
 * lookup <table-file> <address>...
 * Loads the table, then for each address prints the ip-route-get style line.
 * Exit 2 if the file cannot be opened or has a bad line. Otherwise exit 1 if
 * any address was malformed, else 3 if any address had no route, else 0.
 */
static int cmd_lookup(const char *path, int addrc, char **addrv) {
  FILE *in = fopen(path, "r");
  if (in == NULL) {
    fprintf(stderr, "myapp: %s: cannot open file\n", path);
    return 2;
  }

  fwd_table_t *table = table_create();
  if (table == NULL) { // GCOVR_EXCL_START
    fprintf(stderr, "myapp: out of memory\n");
    fclose(in);
    return 2;
  } // GCOVR_EXCL_STOP

  size_t bad_line = 0;
  if (!table_load(table, in, &bad_line)) {
    fprintf(stderr, "myapp: %s:%zu: not a valid route\n", path, bad_line);
    table_destroy(table);
    fclose(in);
    return 2;
  }
  fclose(in);

  bool any_malformed = false;
  bool any_unreachable = false;

  for (int i = 0; i < addrc; i++) {
    uint32_t dest;
    if (!ip_parse(addrv[i], &dest)) {
      fprintf(stderr, "myapp: %s: not a valid address\n", addrv[i]);
      any_malformed = true;
      continue;
    }

    const route_t *route = table_lookup(table, dest);
    if (route == NULL) {
      printf("%s unreachable\n", addrv[i]);
      any_unreachable = true;
      continue;
    }

    if (route->gateway == 0) {
      printf("%s dev %s\n", addrv[i], route->dev);
    } else {
      char gw[IP_STR_LEN];
      ip_format(route->gateway, gw);
      printf("%s via %s dev %s\n", addrv[i], gw, route->dev);
    }
  }

  table_destroy(table);

  if (any_malformed) {
    return 1;
  }
  if (any_unreachable) {
    return 3;
  }
  return 0;
}

int main(int argc, char **argv) {
  /* No arguments at all: print usage and exit cleanly (make leak runs this). */
  if (argc == 1) {
    print_usage(stdout);
    return 0;
  }

  const char *command = argv[1];

  if (strcmp(command, "info") == 0) {
    if (argc != 3) {
      print_usage(stderr);
      return 1;
    }
    return cmd_info(argv[2]);
  }

  if (strcmp(command, "split") == 0) {
    if (argc != 4) {
      print_usage(stderr);
      return 1;
    }
    return cmd_split(argv[2], argv[3]);
  }

  if (strcmp(command, "lookup") == 0) {
    if (argc < 4) {
      print_usage(stderr);
      return 1;
    }
    return cmd_lookup(argv[2], argc - 3, &argv[3]);
  }

  print_usage(stderr);
  return 1;
}

```

## Tests Files
### lab-test.c

```c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/lab.h"
#include "harness/unity.h"

void setUp(void) {}
void tearDown(void) {}

/* Build a cidr_t without going through the parser. */
static cidr_t make_cidr(uint32_t addr, uint8_t prefix) {
  cidr_t b = {addr, prefix};
  return b;
}

/* ------------------------------------------------------------------------
 * Task 1 - ip_parse
 * --------------------------------------------------------------------- */

void test_ip_parse_valid(void) {
  uint32_t addr = 0;

  TEST_ASSERT_TRUE(ip_parse("192.168.1.10", &addr));
  TEST_ASSERT_EQUAL_HEX32(0xC0A8010A, addr);

  TEST_ASSERT_TRUE(ip_parse("0.0.0.0", &addr));
  TEST_ASSERT_EQUAL_HEX32(0x00000000, addr);

  TEST_ASSERT_TRUE(ip_parse("255.255.255.255", &addr));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, addr);

  TEST_ASSERT_TRUE(ip_parse("132.178.227.11", &addr));
  TEST_ASSERT_EQUAL_HEX32(0x84B2E30B, addr);

  /* A single zero octet is legal. */
  TEST_ASSERT_TRUE(ip_parse("10.0.0.1", &addr));
  TEST_ASSERT_EQUAL_HEX32(0x0A000001, addr);
}

void test_ip_parse_rejects_malformed(void) {
  uint32_t addr = 0xDEADBEEF;

  /* The specific strings the spec lists. */
  TEST_ASSERT_FALSE(ip_parse("1.2.3", &addr));        /* too few octets */
  TEST_ASSERT_FALSE(ip_parse("1.2.3.4.5", &addr));    /* too many octets */
  TEST_ASSERT_FALSE(ip_parse("1..2.3", &addr));       /* empty octet */
  TEST_ASSERT_FALSE(ip_parse("256.1.1.1", &addr));    /* octet > 255 */
  TEST_ASSERT_FALSE(ip_parse("010.0.0.1", &addr));    /* leading zero */
  TEST_ASSERT_FALSE(ip_parse("1.2.3.4x", &addr));     /* trailing junk */
  TEST_ASSERT_FALSE(ip_parse("1.2.3.4/33", &addr));   /* slash is junk here */
  TEST_ASSERT_FALSE(ip_parse("1.2.3/24", &addr));     /* too few, then junk */

  /* More corners. */
  TEST_ASSERT_FALSE(ip_parse("", &addr));             /* empty string */
  TEST_ASSERT_FALSE(ip_parse(".1.2.3", &addr));       /* leading dot */
  TEST_ASSERT_FALSE(ip_parse("1.2.3.", &addr));       /* trailing dot */
  TEST_ASSERT_FALSE(ip_parse("1.2.3.4.", &addr));     /* dot after last */
  TEST_ASSERT_FALSE(ip_parse("1234.1.1.1", &addr));   /* four digits */
  TEST_ASSERT_FALSE(ip_parse("1.2.3.-1", &addr));     /* sign */
  TEST_ASSERT_FALSE(ip_parse("a.b.c.d", &addr));      /* letters */
  TEST_ASSERT_FALSE(ip_parse("1 .2.3.4", &addr));     /* space inside */
  TEST_ASSERT_FALSE(ip_parse("00.0.0.0", &addr));     /* leading zero, two 0s */

  /* NULL inputs. */
  TEST_ASSERT_FALSE(ip_parse(NULL, &addr));
  TEST_ASSERT_FALSE(ip_parse("1.2.3.4", NULL));

  /* addr must be untouched on failure. */
  TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, addr);
}

/* ------------------------------------------------------------------------
 * Task 1 - ip_format
 * --------------------------------------------------------------------- */

void test_ip_format(void) {
  char buf[IP_STR_LEN];

  ip_format(0xC0A8010A, buf);
  TEST_ASSERT_EQUAL_STRING("192.168.1.10", buf);

  ip_format(0x00000000, buf);
  TEST_ASSERT_EQUAL_STRING("0.0.0.0", buf);

  ip_format(0xFFFFFFFF, buf);
  TEST_ASSERT_EQUAL_STRING("255.255.255.255", buf);

  ip_format(0x84B2E300, buf);
  TEST_ASSERT_EQUAL_STRING("132.178.227.0", buf);
}

/* ------------------------------------------------------------------------
 * Task 1 - cidr_parse
 * --------------------------------------------------------------------- */

void test_cidr_parse_valid(void) {
  cidr_t b;

  TEST_ASSERT_TRUE(cidr_parse("192.168.1.0/24", &b));
  TEST_ASSERT_EQUAL_HEX32(0xC0A80100, b.addr);
  TEST_ASSERT_EQUAL_UINT8(24, b.prefix);

  /* Host bits set are allowed by the parser (interface address). */
  TEST_ASSERT_TRUE(cidr_parse("132.178.227.11/25", &b));
  TEST_ASSERT_EQUAL_HEX32(0x84B2E30B, b.addr);
  TEST_ASSERT_EQUAL_UINT8(25, b.prefix);

  /* Boundary prefixes. */
  TEST_ASSERT_TRUE(cidr_parse("0.0.0.0/0", &b));
  TEST_ASSERT_EQUAL_UINT8(0, b.prefix);

  TEST_ASSERT_TRUE(cidr_parse("10.0.0.0/8", &b));
  TEST_ASSERT_EQUAL_UINT8(8, b.prefix);

  TEST_ASSERT_TRUE(cidr_parse("1.2.3.4/31", &b));
  TEST_ASSERT_EQUAL_UINT8(31, b.prefix);

  TEST_ASSERT_TRUE(cidr_parse("1.2.3.4/32", &b));
  TEST_ASSERT_EQUAL_UINT8(32, b.prefix);
}

void test_cidr_parse_rejects_malformed(void) {
  cidr_t b;
  b.addr = 0xDEADBEEF;
  b.prefix = 99;

  TEST_ASSERT_FALSE(cidr_parse("1.2.3.4/33", &b));   /* prefix > 32 */
  TEST_ASSERT_FALSE(cidr_parse("1.2.3.4/024", &b));  /* leading zero prefix */
  TEST_ASSERT_FALSE(cidr_parse("1.2.3.4/", &b));     /* no prefix digits */
  TEST_ASSERT_FALSE(cidr_parse("1.2.3.4", &b));      /* no slash at all */
  TEST_ASSERT_FALSE(cidr_parse("1.2.3/24", &b));     /* bad address */
  TEST_ASSERT_FALSE(cidr_parse("1.2.3.4/24x", &b));  /* trailing junk */
  TEST_ASSERT_FALSE(cidr_parse("1.2.3.4/999", &b));  /* 3-digit prefix */
  TEST_ASSERT_FALSE(cidr_parse("1.2.3.4/-1", &b));   /* sign */
  TEST_ASSERT_FALSE(cidr_parse("1.2.3.4//24", &b));  /* double slash */
  TEST_ASSERT_FALSE(cidr_parse("256.1.1.1/24", &b)); /* octet out of range */
  TEST_ASSERT_FALSE(cidr_parse("1.2.3.4/00", &b));   /* 0 with leading zero */

  TEST_ASSERT_FALSE(cidr_parse(NULL, &b));
  TEST_ASSERT_FALSE(cidr_parse("1.2.3.4/24", NULL));

  /* Untouched on failure. */
  TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, b.addr);
  TEST_ASSERT_EQUAL_UINT8(99, b.prefix);
}

/* ------------------------------------------------------------------------
 * Task 1 - cidr_format
 * --------------------------------------------------------------------- */

void test_cidr_format(void) {
  char buf[CIDR_STR_LEN];

  cidr_format(make_cidr(0xC0A80100, 24), buf);
  TEST_ASSERT_EQUAL_STRING("192.168.1.0/24", buf);

  cidr_format(make_cidr(0x00000000, 0), buf);
  TEST_ASSERT_EQUAL_STRING("0.0.0.0/0", buf);

  /* Address stored exactly as given, even with host bits set. */
  cidr_format(make_cidr(0x84B2E30B, 25), buf);
  TEST_ASSERT_EQUAL_STRING("132.178.227.11/25", buf);

  cidr_format(make_cidr(0xFFFFFFFF, 32), buf);
  TEST_ASSERT_EQUAL_STRING("255.255.255.255/32", buf);

  /* A prefix over 32 is written as 32. */
  cidr_format(make_cidr(0x0A000000, 40), buf);
  TEST_ASSERT_EQUAL_STRING("10.0.0.0/32", buf);
}

/* ------------------------------------------------------------------------
 * Task 2 - cidr_mask
 * --------------------------------------------------------------------- */

void test_cidr_mask(void) {
  TEST_ASSERT_EQUAL_HEX32(0x00000000, cidr_mask(0));   /* /0 must be 0 */
  TEST_ASSERT_EQUAL_HEX32(0x80000000, cidr_mask(1));
  TEST_ASSERT_EQUAL_HEX32(0xFF000000, cidr_mask(8));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFF00, cidr_mask(24));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFF80, cidr_mask(25));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFE, cidr_mask(31));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, cidr_mask(32));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, cidr_mask(40)); /* > 32 treated as 32 */
}

/* ------------------------------------------------------------------------
 * Task 2 - network / broadcast / host range / count
 * --------------------------------------------------------------------- */

void test_cidr_network_broadcast(void) {
  /* 132.178.227.11/25 from the Background section. */
  cidr_t b = make_cidr(0x84B2E30B, 25);
  TEST_ASSERT_EQUAL_HEX32(0x84B2E300, cidr_network(b));   /* .0 */
  TEST_ASSERT_EQUAL_HEX32(0x84B2E37F, cidr_broadcast(b)); /* .127 */

  /* /24 */
  cidr_t c = make_cidr(0xC0A80105, 24);
  TEST_ASSERT_EQUAL_HEX32(0xC0A80100, cidr_network(c));
  TEST_ASSERT_EQUAL_HEX32(0xC0A801FF, cidr_broadcast(c));

  /* /0 spans the whole space. */
  cidr_t z = make_cidr(0x12345678, 0);
  TEST_ASSERT_EQUAL_HEX32(0x00000000, cidr_network(z));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFF, cidr_broadcast(z));

  /* /32 single host. */
  cidr_t h = make_cidr(0x0A0B0C0D, 32);
  TEST_ASSERT_EQUAL_HEX32(0x0A0B0C0D, cidr_network(h));
  TEST_ASSERT_EQUAL_HEX32(0x0A0B0C0D, cidr_broadcast(h));
}

void test_cidr_host_range_normal(void) {
  /* 132.178.227.0/25: first .1, last .126, 126 hosts. */
  cidr_t b = make_cidr(0x84B2E300, 25);
  TEST_ASSERT_EQUAL_HEX32(0x84B2E301, cidr_first_host(b));
  TEST_ASSERT_EQUAL_HEX32(0x84B2E37E, cidr_last_host(b));
  TEST_ASSERT_EQUAL_UINT64(126, cidr_host_count(b));

  /* /24: 254 hosts. */
  cidr_t c = make_cidr(0xC0A80100, 24);
  TEST_ASSERT_EQUAL_HEX32(0xC0A80101, cidr_first_host(c));
  TEST_ASSERT_EQUAL_HEX32(0xC0A801FE, cidr_last_host(c));
  TEST_ASSERT_EQUAL_UINT64(254, cidr_host_count(c));

  /* /30: 2 hosts. */
  cidr_t d = make_cidr(0x0A000000, 30);
  TEST_ASSERT_EQUAL_HEX32(0x0A000001, cidr_first_host(d));
  TEST_ASSERT_EQUAL_HEX32(0x0A000002, cidr_last_host(d));
  TEST_ASSERT_EQUAL_UINT64(2, cidr_host_count(d));
}

void test_cidr_host_range_boundaries(void) {
  /* /0: 2^32 - 2 hosts, first .1, last .254-of-everything. */
  cidr_t z = make_cidr(0x00000000, 0);
  TEST_ASSERT_EQUAL_HEX32(0x00000001, cidr_first_host(z));
  TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFE, cidr_last_host(z));
  TEST_ASSERT_EQUAL_UINT64(4294967294ULL, cidr_host_count(z));

  /* /31: both addresses usable (RFC 3021), 2 hosts. */
  cidr_t p = make_cidr(0x0A000000, 31);
  TEST_ASSERT_EQUAL_HEX32(0x0A000000, cidr_first_host(p));
  TEST_ASSERT_EQUAL_HEX32(0x0A000001, cidr_last_host(p));
  TEST_ASSERT_EQUAL_UINT64(2, cidr_host_count(p));

  /* /32: single host, first == last == the address. */
  cidr_t h = make_cidr(0x0A0B0C0D, 32);
  TEST_ASSERT_EQUAL_HEX32(0x0A0B0C0D, cidr_first_host(h));
  TEST_ASSERT_EQUAL_HEX32(0x0A0B0C0D, cidr_last_host(h));
  TEST_ASSERT_EQUAL_UINT64(1, cidr_host_count(h));

  /* Prefix over 32 treated as 32 everywhere. */
  cidr_t over = make_cidr(0x0A0B0C0D, 40);
  TEST_ASSERT_EQUAL_HEX32(0x0A0B0C0D, cidr_first_host(over));
  TEST_ASSERT_EQUAL_HEX32(0x0A0B0C0D, cidr_last_host(over));
  TEST_ASSERT_EQUAL_UINT64(1, cidr_host_count(over));
}

/* ------------------------------------------------------------------------
 * Task 2 - cidr_contains
 * --------------------------------------------------------------------- */

void test_cidr_contains(void) {
  cidr_t b = make_cidr(0x84B2E300, 25); /* 132.178.227.0/25 */

  TEST_ASSERT_TRUE(cidr_contains(b, 0x84B2E332));  /* .50 yes */
  TEST_ASSERT_FALSE(cidr_contains(b, 0x84B2E3C8)); /* .200 no */
  TEST_ASSERT_TRUE(cidr_contains(b, 0x84B2E300));  /* network itself */
  TEST_ASSERT_TRUE(cidr_contains(b, 0x84B2E37F));  /* broadcast itself */

  /* /0 contains everything. */
  cidr_t z = make_cidr(0, 0);
  TEST_ASSERT_TRUE(cidr_contains(z, 0x00000000));
  TEST_ASSERT_TRUE(cidr_contains(z, 0xFFFFFFFF));

  /* /32 contains only its own address. */
  cidr_t h = make_cidr(0x0A0B0C0D, 32);
  TEST_ASSERT_TRUE(cidr_contains(h, 0x0A0B0C0D));
  TEST_ASSERT_FALSE(cidr_contains(h, 0x0A0B0C0E));
}

/* ------------------------------------------------------------------------
 * Task 2 - cidr_is_network
 * --------------------------------------------------------------------- */

void test_cidr_is_network(void) {
  TEST_ASSERT_TRUE(cidr_is_network(make_cidr(0xC0A80100, 24)));
  TEST_ASSERT_FALSE(cidr_is_network(make_cidr(0xC0A80105, 24)));
  TEST_ASSERT_TRUE(cidr_is_network(make_cidr(0x00000000, 0)));
  TEST_ASSERT_FALSE(cidr_is_network(make_cidr(0x00000001, 0)));
  /* Every address is its own network at /32. */
  TEST_ASSERT_TRUE(cidr_is_network(make_cidr(0x0A0B0C0D, 32)));
  /* 132.178.227.11/25 has host bits set. */
  TEST_ASSERT_FALSE(cidr_is_network(make_cidr(0x84B2E30B, 25)));
}

/* ------------------------------------------------------------------------
 * Task 2 - cidr_subnet
 * --------------------------------------------------------------------- */

void test_cidr_subnet_basic(void) {
  cidr_t b = make_cidr(0xC0A80100, 24); /* 192.168.1.0/24 */
  cidr_t out;

  TEST_ASSERT_TRUE(cidr_subnet(b, 4, 0, &out));
  TEST_ASSERT_EQUAL_HEX32(0xC0A80100, out.addr); /* .0 */
  TEST_ASSERT_EQUAL_UINT8(26, out.prefix);

  TEST_ASSERT_TRUE(cidr_subnet(b, 4, 1, &out));
  TEST_ASSERT_EQUAL_HEX32(0xC0A80140, out.addr); /* .64 */
  TEST_ASSERT_EQUAL_UINT8(26, out.prefix);

  TEST_ASSERT_TRUE(cidr_subnet(b, 4, 2, &out));
  TEST_ASSERT_EQUAL_HEX32(0xC0A80180, out.addr); /* .128 */

  TEST_ASSERT_TRUE(cidr_subnet(b, 4, 3, &out));
  TEST_ASSERT_EQUAL_HEX32(0xC0A801C0, out.addr); /* .192 */

  /* n == 1 returns the block unchanged. */
  TEST_ASSERT_TRUE(cidr_subnet(b, 1, 0, &out));
  TEST_ASSERT_EQUAL_HEX32(0xC0A80100, out.addr);
  TEST_ASSERT_EQUAL_UINT8(24, out.prefix);
}

void test_cidr_subnet_boundaries(void) {
  cidr_t out;

  /* Split /0 into 2: subnet 0 is 0.0.0.0/1, subnet 1 is 128.0.0.0/1. */
  cidr_t z = make_cidr(0, 0);
  TEST_ASSERT_TRUE(cidr_subnet(z, 2, 0, &out));
  TEST_ASSERT_EQUAL_HEX32(0x00000000, out.addr);
  TEST_ASSERT_EQUAL_UINT8(1, out.prefix);
  TEST_ASSERT_TRUE(cidr_subnet(z, 2, 1, &out));
  TEST_ASSERT_EQUAL_HEX32(0x80000000, out.addr);
  TEST_ASSERT_EQUAL_UINT8(1, out.prefix);

  /* n == 1 on /0 (host_bits == 32 path): offset must be 0. */
  TEST_ASSERT_TRUE(cidr_subnet(z, 1, 0, &out));
  TEST_ASSERT_EQUAL_HEX32(0x00000000, out.addr);
  TEST_ASSERT_EQUAL_UINT8(0, out.prefix);

  /* Split a /31 into 2 -> two /32s. */
  cidr_t p = make_cidr(0x0A000000, 31);
  TEST_ASSERT_TRUE(cidr_subnet(p, 2, 0, &out));
  TEST_ASSERT_EQUAL_HEX32(0x0A000000, out.addr);
  TEST_ASSERT_EQUAL_UINT8(32, out.prefix);
  TEST_ASSERT_TRUE(cidr_subnet(p, 2, 1, &out));
  TEST_ASSERT_EQUAL_HEX32(0x0A000001, out.addr);
  TEST_ASSERT_EQUAL_UINT8(32, out.prefix);
}

void test_cidr_subnet_rejects(void) {
  cidr_t b = make_cidr(0xC0A80100, 24);
  cidr_t out;

  /* NULL out. */
  TEST_ASSERT_FALSE(cidr_subnet(b, 4, 0, NULL));

  /* Not a network (host bits set). */
  TEST_ASSERT_FALSE(cidr_subnet(make_cidr(0xC0A80105, 24), 4, 0, &out));

  /* n not a power of two. */
  TEST_ASSERT_FALSE(cidr_subnet(b, 3, 0, &out));
  TEST_ASSERT_FALSE(cidr_subnet(b, 6, 0, &out));

  /* n == 0. */
  TEST_ASSERT_FALSE(cidr_subnet(b, 0, 0, &out));

  /* i out of range. */
  TEST_ASSERT_FALSE(cidr_subnet(b, 4, 4, &out));
  TEST_ASSERT_FALSE(cidr_subnet(b, 4, 99, &out));

  /* Split would exceed /32: /32 into 2 needs a /33. */
  TEST_ASSERT_FALSE(cidr_subnet(make_cidr(0x0A0B0C0D, 32), 2, 0, &out));

  /* /24 into 512 needs 9 borrowed bits -> /33, too long. */
  TEST_ASSERT_FALSE(cidr_subnet(b, 512, 0, &out));
}

/* ------------------------------------------------------------------------
 * Task 3 - route_parse
 * --------------------------------------------------------------------- */

void test_route_parse_valid(void) {
  route_t r;

  TEST_ASSERT_TRUE(route_parse("132.178.227.0/25 direct eno1", &r));
  TEST_ASSERT_EQUAL_HEX32(0x84B2E300, r.dest.addr);
  TEST_ASSERT_EQUAL_UINT8(25, r.dest.prefix);
  TEST_ASSERT_EQUAL_HEX32(0, r.gateway);
  TEST_ASSERT_EQUAL_STRING("eno1", r.dev);

  /* default -> 0.0.0.0/0, with a real gateway. */
  TEST_ASSERT_TRUE(route_parse("default 132.178.227.1 eno1", &r));
  TEST_ASSERT_EQUAL_HEX32(0, r.dest.addr);
  TEST_ASSERT_EQUAL_UINT8(0, r.dest.prefix);
  TEST_ASSERT_EQUAL_HEX32(0x84B2E301, r.gateway);
  TEST_ASSERT_EQUAL_STRING("eno1", r.dev);

  /* Tabs and extra spaces, leading/trailing whitespace, trailing newline. */
  TEST_ASSERT_TRUE(route_parse("\t 10.0.0.0/8\t10.1.0.1   eth1  \n", &r));
  TEST_ASSERT_EQUAL_HEX32(0x0A000000, r.dest.addr);
  TEST_ASSERT_EQUAL_UINT8(8, r.dest.prefix);
  TEST_ASSERT_EQUAL_HEX32(0x0A010001, r.gateway);
  TEST_ASSERT_EQUAL_STRING("eth1", r.dev);

  /* A device of the maximum length (15 chars) is accepted. */
  TEST_ASSERT_TRUE(route_parse("10.0.0.0/8 direct 123456789012345", &r));
  TEST_ASSERT_EQUAL_STRING("123456789012345", r.dev);
}

void test_route_parse_rejects(void) {
  route_t r;
  memset(&r, 0xAB, sizeof(r));

  /* NULL inputs. */
  TEST_ASSERT_FALSE(route_parse(NULL, &r));
  TEST_ASSERT_FALSE(route_parse("default direct eth0", NULL));

  /* Too few fields. */
  TEST_ASSERT_FALSE(route_parse("", &r));
  TEST_ASSERT_FALSE(route_parse("   ", &r));
  TEST_ASSERT_FALSE(route_parse("default", &r));
  TEST_ASSERT_FALSE(route_parse("default direct", &r));

  /* Too many fields (trailing junk after device). */
  TEST_ASSERT_FALSE(route_parse("default direct eth0 extra", &r));

  /* Destination with host bits set is rejected. */
  TEST_ASSERT_FALSE(route_parse("132.178.227.11/25 direct eno1", &r));

  /* Bad destination. */
  TEST_ASSERT_FALSE(route_parse("not-a-block direct eth0", &r));
  TEST_ASSERT_FALSE(route_parse("256.0.0.0/8 direct eth0", &r));

  /* Bad gateway. */
  TEST_ASSERT_FALSE(route_parse("10.0.0.0/8 999.1.1.1 eth0", &r));
  TEST_ASSERT_FALSE(route_parse("10.0.0.0/8 notgw eth0", &r));

  /* Empty / too-long device. The device field holds >= DEV_NAME_LEN chars. */
  TEST_ASSERT_FALSE(route_parse("10.0.0.0/8 direct 1234567890123456", &r));

  /* A field far longer than any internal buffer is rejected cleanly. */
  TEST_ASSERT_FALSE(route_parse(
      "10.0.0.0/8 direct thisdevicenameiswaytoolongtofitinanybuffer123456789",
      &r));
  TEST_ASSERT_FALSE(route_parse(
      "123456789012345678901234567890.0.0.0/8 direct eth0", &r));
}

/* ------------------------------------------------------------------------
 * Task 3 - table create / destroy / count / add
 * --------------------------------------------------------------------- */

void test_table_create_destroy_count(void) {
  fwd_table_t *t = table_create();
  TEST_ASSERT_NOT_NULL(t);
  TEST_ASSERT_EQUAL_UINT(0, table_count(t));
  table_destroy(t);

  /* NULL is allowed. */
  table_destroy(NULL);
  TEST_ASSERT_EQUAL_UINT(0, table_count(NULL));
}

static route_t route_of(const char *line) {
  route_t r;
  memset(&r, 0, sizeof(r));
  TEST_ASSERT_TRUE(route_parse(line, &r));
  return r;
}

void test_table_add(void) {
  fwd_table_t *t = table_create();
  TEST_ASSERT_NOT_NULL(t);

  route_t r = route_of("10.0.0.0/8 direct eth1");
  TEST_ASSERT_TRUE(table_add(t, &r));
  TEST_ASSERT_EQUAL_UINT(1, table_count(t));

  /* Duplicate block (same addr and prefix) is refused. */
  route_t dup = route_of("10.0.0.0/8 10.1.0.1 eth2");
  TEST_ASSERT_FALSE(table_add(t, &dup));
  TEST_ASSERT_EQUAL_UINT(1, table_count(t));

  /* A different prefix on the same address is a different block. */
  route_t r16 = route_of("10.1.0.0/16 direct eth1");
  TEST_ASSERT_TRUE(table_add(t, &r16));
  TEST_ASSERT_EQUAL_UINT(2, table_count(t));

  /* Grow past the initial capacity of 4 to exercise realloc. */
  route_t r2 = route_of("10.2.0.0/16 direct eth1");
  route_t r3 = route_of("10.3.0.0/16 direct eth1");
  route_t r4 = route_of("10.4.0.0/16 direct eth1");
  TEST_ASSERT_TRUE(table_add(t, &r2));
  TEST_ASSERT_TRUE(table_add(t, &r3));
  TEST_ASSERT_TRUE(table_add(t, &r4));
  TEST_ASSERT_EQUAL_UINT(5, table_count(t));

  /* NULL arguments. */
  TEST_ASSERT_FALSE(table_add(NULL, &r));
  TEST_ASSERT_FALSE(table_add(t, NULL));

  /* Destination with host bits set is refused. */
  route_t bad;
  memset(&bad, 0, sizeof(bad));
  bad.dest = make_cidr(0x0A000005, 8); /* 10.0.0.5/8 */
  bad.gateway = 0;
  strcpy(bad.dev, "eth0");
  TEST_ASSERT_FALSE(table_add(t, &bad));
  TEST_ASSERT_EQUAL_UINT(5, table_count(t));

  table_destroy(t);
}

/* ------------------------------------------------------------------------
 * Task 3 - table_load
 * --------------------------------------------------------------------- */

/* Write text to a temp FILE*, rewind, and return it. */
static FILE *file_with(const char *text) {
  FILE *f = tmpfile();
  TEST_ASSERT_NOT_NULL(f);
  fputs(text, f);
  rewind(f);
  return f;
}

void test_table_load_good(void) {
  const char *text =
      "# destination      gateway          device\n"
      "default            132.178.227.1    eno1\n"
      "132.178.227.0/25   direct           eno1\n"
      "\n"                              /* blank line */
      "   \t  \n"                       /* whitespace-only line */
      "10.0.0.0/8         10.1.0.1      eth1\n"
      "10.1.0.0/16        direct        eth1\n"
      "10.1.2.0/24        direct        eth2\n"
      "10.1.2.128/25      10.1.2.1      eth2   # a router behind eth2\n"
      "192.168.0.0/16     10.1.0.1      eth1\n";

  FILE *f = file_with(text);
  fwd_table_t *t = table_create();
  TEST_ASSERT_NOT_NULL(t);

  size_t bad = 12345;
  TEST_ASSERT_TRUE(table_load(t, f, &bad));
  TEST_ASSERT_EQUAL_UINT(0, bad);
  TEST_ASSERT_EQUAL_UINT(7, table_count(t));

  table_destroy(t);
  fclose(f);
}

void test_table_load_bad_line_in_middle(void) {
  /* Line 4 is malformed; the three routes before it must stay. */
  const char *text =
      "default 132.178.227.1 eno1\n"   /* line 1 */
      "10.0.0.0/8 direct eth1\n"       /* line 2 */
      "10.1.0.0/16 direct eth1\n"      /* line 3 */
      "this is not a route\n"          /* line 4 - bad */
      "10.2.0.0/16 direct eth1\n";     /* line 5 - never reached */

  FILE *f = file_with(text);
  fwd_table_t *t = table_create();
  TEST_ASSERT_NOT_NULL(t);

  size_t bad = 0;
  TEST_ASSERT_FALSE(table_load(t, f, &bad));
  TEST_ASSERT_EQUAL_UINT(4, bad);
  TEST_ASSERT_EQUAL_UINT(3, table_count(t)); /* earlier routes kept */

  table_destroy(t);
  fclose(f);
}

void test_table_load_duplicate_line(void) {
  /* A duplicate block makes table_add fail inside table_load. */
  const char *text =
      "10.0.0.0/8 direct eth1\n"
      "10.0.0.0/8 10.1.0.1 eth2\n";

  FILE *f = file_with(text);
  fwd_table_t *t = table_create();

  size_t bad = 0;
  TEST_ASSERT_FALSE(table_load(t, f, &bad));
  TEST_ASSERT_EQUAL_UINT(2, bad);
  TEST_ASSERT_EQUAL_UINT(1, table_count(t));

  table_destroy(t);
  fclose(f);
}

void test_table_load_comment_only_and_bad_line_number(void) {
  const char *text =
      "# just a comment\n"
      "\n"
      "default direct eth0\n"   /* line 3, valid */
      "   # trailing comment only\n";

  FILE *f = file_with(text);
  fwd_table_t *t = table_create();

  size_t bad = 99;
  TEST_ASSERT_TRUE(table_load(t, f, &bad));
  TEST_ASSERT_EQUAL_UINT(0, bad);
  TEST_ASSERT_EQUAL_UINT(1, table_count(t));

  table_destroy(t);
  fclose(f);
}

void test_table_load_null_args(void) {
  fwd_table_t *t = table_create();
  FILE *f = file_with("default direct eth0\n");

  size_t bad = 42;
  TEST_ASSERT_FALSE(table_load(NULL, f, &bad));
  TEST_ASSERT_EQUAL_UINT(0, bad);

  bad = 42;
  TEST_ASSERT_FALSE(table_load(t, NULL, &bad));
  TEST_ASSERT_EQUAL_UINT(0, bad);

  /* bad_line NULL is allowed. */
  TEST_ASSERT_FALSE(table_load(NULL, NULL, NULL));

  table_destroy(t);
  fclose(f);
}

/* ------------------------------------------------------------------------
 * Task 3 - table_lookup (longest prefix match)
 * --------------------------------------------------------------------- */

void test_table_lookup_longest_prefix(void) {
  const char *text =
      "10.0.0.0/8 10.1.0.1 eth1\n"
      "10.1.0.0/16 direct eth1\n"
      "10.1.2.0/24 direct eth2\n"
      "10.1.2.128/25 10.1.2.1 eth2\n";

  FILE *f = file_with(text);
  fwd_table_t *t = table_create();
  size_t bad = 0;
  TEST_ASSERT_TRUE(table_load(t, f, &bad));

  /* 10.1.2.200 is in all four; the /25 wins. */
  const route_t *r = table_lookup(t, 0x0A0102C8); /* 10.1.2.200 */
  TEST_ASSERT_NOT_NULL(r);
  TEST_ASSERT_EQUAL_UINT8(25, r->dest.prefix);
  TEST_ASSERT_EQUAL_HEX32(0x0A010201, r->gateway); /* 10.1.2.1 */

  /* 10.1.2.5 is in /8, /16, /24 but not the /25; the /24 wins. */
  r = table_lookup(t, 0x0A010205); /* 10.1.2.5 */
  TEST_ASSERT_NOT_NULL(r);
  TEST_ASSERT_EQUAL_UINT8(24, r->dest.prefix);

  /* 10.9.0.0 is only in /8. */
  r = table_lookup(t, 0x0A090000);
  TEST_ASSERT_NOT_NULL(r);
  TEST_ASSERT_EQUAL_UINT8(8, r->dest.prefix);

  /* No match, no default route -> NULL. */
  r = table_lookup(t, 0x08080808); /* 8.8.8.8 */
  TEST_ASSERT_NULL(r);

  /* NULL table. */
  TEST_ASSERT_NULL(table_lookup(NULL, 0));

  table_destroy(t);
  fclose(f);
}

void test_table_lookup_order_independent(void) {
  /* Same routes, added shortest-first here vs longest-first elsewhere. */
  const char *text =
      "10.1.2.128/25 10.1.2.1 eth2\n"
      "10.1.2.0/24 direct eth2\n"
      "10.1.0.0/16 direct eth1\n"
      "10.0.0.0/8 10.1.0.1 eth1\n";

  FILE *f = file_with(text);
  fwd_table_t *t = table_create();
  size_t bad = 0;
  TEST_ASSERT_TRUE(table_load(t, f, &bad));

  const route_t *r = table_lookup(t, 0x0A0102C8); /* 10.1.2.200 */
  TEST_ASSERT_NOT_NULL(r);
  TEST_ASSERT_EQUAL_UINT8(25, r->dest.prefix);

  table_destroy(t);
  fclose(f);
}

void test_table_lookup_default_route(void) {
  const char *text =
      "default 132.178.227.1 eno1\n"
      "10.0.0.0/8 direct eth1\n";

  FILE *f = file_with(text);
  fwd_table_t *t = table_create();
  size_t bad = 0;
  TEST_ASSERT_TRUE(table_load(t, f, &bad));

  /* Anything outside 10/8 falls to the default route. */
  const route_t *r = table_lookup(t, 0x08080808); /* 8.8.8.8 */
  TEST_ASSERT_NOT_NULL(r);
  TEST_ASSERT_EQUAL_UINT8(0, r->dest.prefix);
  TEST_ASSERT_EQUAL_HEX32(0x84B2E301, r->gateway);

  /* 10.x prefers the more specific /8. */
  r = table_lookup(t, 0x0A000001);
  TEST_ASSERT_NOT_NULL(r);
  TEST_ASSERT_EQUAL_UINT8(8, r->dest.prefix);

  table_destroy(t);
  fclose(f);
}

void test_table_lookup_empty_table(void) {
  fwd_table_t *t = table_create();
  TEST_ASSERT_NULL(table_lookup(t, 0x0A000001));
  table_destroy(t);
}

/* ------------------------------------------------------------------------ */

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_ip_parse_valid);
  RUN_TEST(test_ip_parse_rejects_malformed);
  RUN_TEST(test_ip_format);
  RUN_TEST(test_cidr_parse_valid);
  RUN_TEST(test_cidr_parse_rejects_malformed);
  RUN_TEST(test_cidr_format);

  RUN_TEST(test_cidr_mask);
  RUN_TEST(test_cidr_network_broadcast);
  RUN_TEST(test_cidr_host_range_normal);
  RUN_TEST(test_cidr_host_range_boundaries);
  RUN_TEST(test_cidr_contains);
  RUN_TEST(test_cidr_is_network);
  RUN_TEST(test_cidr_subnet_basic);
  RUN_TEST(test_cidr_subnet_boundaries);
  RUN_TEST(test_cidr_subnet_rejects);

  RUN_TEST(test_route_parse_valid);
  RUN_TEST(test_route_parse_rejects);
  RUN_TEST(test_table_create_destroy_count);
  RUN_TEST(test_table_add);
  RUN_TEST(test_table_load_good);
  RUN_TEST(test_table_load_bad_line_in_middle);
  RUN_TEST(test_table_load_duplicate_line);
  RUN_TEST(test_table_load_comment_only_and_bad_line_number);
  RUN_TEST(test_table_load_null_args);
  RUN_TEST(test_table_lookup_longest_prefix);
  RUN_TEST(test_table_lookup_order_independent);
  RUN_TEST(test_table_lookup_default_route);
  RUN_TEST(test_table_lookup_empty_table);

  return UNITY_END();
}

```

## Scripts Files
Report generated on 10/27/2026 at 21:32:46


---

## End of Report

SHA-256 Hash of the report: 6715cc423ec12a3e6b04bde1e87c1bdd5ef556c0476817c9878f7cfb3eaf9f35

Do not edit the generated report. Any changes will be reported as academic dishonesty

---
## GitHub Info
- GitHub repo name: ChandlerBlack/p3
- The repository visibility is public.
- The workflow was triggered by ChandlerBlack
