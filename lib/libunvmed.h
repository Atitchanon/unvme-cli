/* SPDX-License-Identifier: LGPL-2.1-or-later OR MIT */

#ifndef LIBUNVMED_H
#define LIBUNVMED_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <sys/time.h>

/*
 * `libunvmed`-specific structures
 */
struct unvme;
struct unvme_cmd;

/*
 * NVMe spec-based data structures defined in `libvfn`
 */
union nvme_cmd;
struct nvme_cqe;

/*
 * `libvfn`-specific structures
 */
struct nvme_sq;
struct nvme_cq;

int unvmed_pci_bind(const char *bdf);
int unvmed_pci_unbind(const char *bdf);

struct unvme *unvmed_get(const char *bdf);

struct nvme_sq *unvmed_get_sq(struct unvme *u, uint32_t qid);
struct nvme_cq *unvmed_get_cq(struct unvme *u, uint32_t qid);

struct unvme *unvmed_init_ctrl(const char *bdf, uint32_t max_nr_ioqs);
void unvmed_free_ctrl(struct unvme *u);
void unvmed_free_ctrl_all(void);
void unvmed_reset_ctrl(struct unvme *u);
int unvmed_create_adminq(struct unvme *u);
int unvmed_enable_ctrl(struct unvme *u, uint8_t iosqes, uint8_t iocqes, uint8_t mps, uint8_t css);
int unvmed_create_cq(struct unvme *u, uint32_t qid, uint32_t qsize, uint32_t vector);
int unvmed_create_sq(struct unvme *u, uint32_t qid, uint32_t qsize, uint32_t cqid);
int unvmed_map_vaddr(struct unvme *u, void *buf, size_t len, uint64_t *iova, unsigned long flags);
int unvmed_unmap_vaddr(struct unvme *u, void *buf);
struct unvme_cmd *unvmed_alloc_cmd(struct unvme *u, int sqid, size_t data_len);
void unvmed_cmd_free(void *p);
int unvmed_free_cmds(struct unvme *u, uint32_t sqid);
void unvmed_cmd_post(struct unvme_cmd *cmd, union nvme_cmd *sqe, unsigned long flags);
struct nvme_cqe *unvmed_cmd_cmpl(struct unvme_cmd *cmd);
int unvmed_sq_update_tail_and_wait(struct unvme *u, uint32_t sqid, struct nvme_cqe **cqes);
int unvmed_map_prp(struct unvme_cmd *cmd);

enum unvmed_cmd_flags {
	/* No doorbell update after posting one or more commands */
	UNVMED_CMD_F_NODB	= 1 << 0,
};

int unvmed_id_ns(struct unvme *u, uint32_t nsid, void *buf, unsigned long flags);
int unvmed_read(struct unvme *u, uint32_t sqid, uint32_t nsid, uint64_t slba,
		uint16_t nlb, void *buf, size_t size, unsigned long flags);
int unvmed_write(struct unvme *u, uint32_t sqid, uint32_t nsid, uint64_t slba,
		 uint16_t nlb, void *buf, size_t size, unsigned long flags);
int unvmed_passthru(struct unvme *u, uint32_t sqid, void *buf, size_t size,
		    union nvme_cmd *sqe, bool read, unsigned long flags);

/*
 * `struct unvme` starts with `struct nvme_ctrl`, so convert easily.
 */
#define __unvmed_ctrl(u)		((struct nvme_ctrl *)(u))

/*
 * `struct nvme_ctrl` of `libvfn` access helpers.
 */
#define unvmed_bdf(u)		(__unvmed_ctrl(u)->pci.bdf)
#define unvmed_reg(u)		(__unvmed_ctrl(u)->regs)

#define unvmed_cqe_status(cqe)	(le16_to_cpu((cqe)->sfp) >> 1)

#define unvmed_read32(u, offset)	\
	le32_to_cpu(mmio_read32(unvmed_reg(u) + (offset)))
#define unvmed_read64(u, offset)	\
	le64_to_cpu(mmio_read64(unvmed_reg(u) + (offset)))
#define unvmed_write32(u, offset, value) \
	mmio_write32(unvmed_reg(u) + (offset), cpu_to_le32((value)))
#define unvmed_write64(u, offset, value) \
	mmio_write64(unvmed_reg(u) + (offset), cpu_to_le64((value)))

#define __unvmed_free_cmd __attribute__((cleanup(unvmed_cmd_free)))

#endif
