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
