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
