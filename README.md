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
