/*
 *  libpulp - User-space Livepatching Library
 *
 *  Copyright (C) 2017-2021 SUSE Software Solutions GmbH
 *
 *  This file is part of libpulp.
 *
 *  libpulp is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  libpulp is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libpulp.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _INTROSPECTION_H_
#define _INTROSPECTION_H_

#include <link.h>
#include <stdbool.h>

#include "ptrace.h"
#include "ulp_common.h"

extern int ulp_verbose;
extern int ulp_quiet;

#undef WARN // Defined in ulp_common.h

#define FATAL(format, ...) \
  do { \
    fprintf(stderr, "ulp: " format "\n", ##__VA_ARGS__); \
    fprintf(stderr, "PROGRAM POTENTIALLY LEFT IN INCONSISTENT STATE."); \
  } \
  while (0)

#define WARN(format, ...) \
  do { \
    if (!ulp_quiet) \
      fprintf(stderr, "ulp: " format "\n", ##__VA_ARGS__); \
  } \
  while (0)

#define DEBUG(format, ...) \
  do { \
    if (ulp_verbose) \
      fprintf(stderr, "ulp: " format "\n", ##__VA_ARGS__); \
  } \
  while (0)

struct ulp_process
{
  int pid;
  bool all_threads_hijacked;

  Elf64_Addr load_bias;
  Elf64_Addr dyn_addr;

  struct ulp_thread *threads;
  struct ulp_thread *main_thread;

  struct ulp_dynobj *dynobj_main;
  struct ulp_dynobj *dynobj_libpulp;
  struct ulp_dynobj *dynobj_targets;
  struct ulp_dynobj *dynobj_patches;

  unsigned long global_universe;

  struct ulp_process *next;
};

struct ulp_thread
{
  int tid;
  struct user_regs_struct context;
  int consistent;
  struct ulp_thread *next;
};

struct thread_state
{
  int tid;
  unsigned long universe;

  struct thread_state *next;
};

/** Length of build id, in bytes.  */
#define BUILDID_LEN 20

/** Represents a dynamic object, such as dynamic libraries (.so) or the
 *  executable itself.
 */
struct ulp_dynobj
{
  /** Name of the library. Empty string in case of the executable itself.  */
  char *filename;

  /* Link map, as in _r_debug symbol loaded by the linker.  */
  struct link_map link_map;

  /** Address of dynstr section of current library in the target process.  */
  Elf64_Addr dynstr_addr;

  /** Address of dynsym section of current library in the target process.  */
  Elf64_Addr dynsym_addr;

  /** Number of symbols in the dynsym section of current library in the target
   * process.  */
  int num_symbols;

  /** Build id of current library loaded in the target process.  */
  unsigned char build_id[BUILDID_LEN];

  /* FIXME: only libpulp objects should have those symbols.  */
  Elf64_Addr trigger;
  Elf64_Addr check;
  Elf64_Addr path_buffer;
  Elf64_Addr state;
  Elf64_Addr global;
  Elf64_Addr msg_queue;
  /* end FIXME.  */

  struct thread_state *thread_states;

  /** Next dynobj in the dynobj chain (linked list).  */
  struct ulp_dynobj *next;
};

struct ulp_addresses
{
  Elf64_Addr trigger;
  Elf64_Addr path_buffer;
  Elf64_Addr check;
  Elf64_Addr state;
  Elf64_Addr global;
};

int dig_main_link_map(struct ulp_process *process);

Elf64_Addr get_loaded_symbol_addr_on_disk(struct ulp_dynobj *obj,
                                          const char *sym);

Elf64_Addr get_loaded_symbol_addr(struct ulp_dynobj *obj, int pid,
                                  const char *sym);

int dig_load_bias(struct ulp_process *process);

int parse_main_dynobj(struct ulp_process *process);

int parse_libs_dynobj(struct ulp_process *process);

struct link_map *parse_lib_dynobj(struct ulp_process *process,
                                  struct link_map *link_map_addr);

int initialize_data_structures(struct ulp_process *process);

int hijack_threads(struct ulp_process *process);

int set_id_buffer(struct ulp_process *process, unsigned char *patch_id);

int set_path_buffer(struct ulp_process *process, const char *path);

int patch_applied(struct ulp_process *process, unsigned char *id, int *result);

int apply_patch(struct ulp_process *process, const char *metadata);

int restore_threads(struct ulp_process *process);

int read_global_universe(struct ulp_process *process);

int load_patch_info(const char *livepatch);

int check_patch_sanity();

#if defined ENABLE_STACK_CHECK && ENABLE_STACK_CHECK
int coarse_library_range_check(struct ulp_process *process, char *library);
#endif

#endif
