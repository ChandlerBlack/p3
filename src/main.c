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
