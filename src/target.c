#include "target.h"
#include <stdio.h>
#include <string.h>
#include <sys/personality.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include "arch.h"
#include "utils/align.h"

static bool target_sigtrap(target_t *t, siginfo_t info)
{
    bp_t *bp;
    size_t addr;
    char str[17];

    switch (info.si_code) {
    case TRAP_TRACE:
        /* This is the si_code after single step. Nothing has
         * to be done in such case. */
        return true;
    case TRAP_BRKPT:
    case SI_KERNEL:
        /* When a breakpoint is hit previously, to keep executing instead of
         * hanging on the trap instruction latter, we first rollback pc to the
         * previous instruction and restore the original instruction
         * temporarily. */
        if (!target_get_reg(t, RIP, &addr))
            return false;

        addr -= 1;
        snprintf(str, 17, "%lx", addr);

        if (hashtbl_fetch(&t->tbl, str, (void **) &bp)) {
            t->hit_bp = bp;
            if (!bp_unset(bp))
                return false;
        }
        if (!target_set_reg(t, RIP, addr))
            return false;
        return true;
    default:
        /* FIXME: If we are not here because of single step, then we assume
         * the only left reason for the trap is when hitting software
         * breakpoint, but we may have to consider more different situation. */
        printf("Unknown %d\n", info.si_code);
        return false;
    }
}

static bool target_wait_sig(target_t *t)
{
    int wstatus;
    if (waitpid(t->pid, &wstatus, __WALL) < 0) {
        perror("waitpid");
        return false;
    }

    bool ret = true;

    if (WIFSTOPPED(wstatus) && (WSTOPSIG(wstatus) == SIGTRAP)) {
        siginfo_t info;
        ptrace(PTRACE_GETSIGINFO, t->pid, 0, &info);

        switch (info.si_signo) {
        case SIGTRAP:
            ret = target_sigtrap(t, info);
            break;
        default:
            /* simply ignore these */
            break;
        }
    } else if (WIFEXITED(wstatus)) {
        printf("[Process %d exited]\n", t->pid);
        t->run = false;
    }

    return ret;
}

bool target_lauch(target_t *t, char *cmd)
{
    pid_t pid = fork();
    /* for child process */
    if (pid == 0) {
        // disable address space randomization
        personality(ADDR_NO_RANDOMIZE);
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl(cmd, cmd, NULL);
    }

    /* for parent process */
    if (!target_wait_sig(t))
        return false;

    t->pid = pid;
    t->hit_bp = NULL;
    t->run = true;
    /* we should guarantee the initial value of breakpoint array */
    t->bp_bitmap = 0xffff;
    memset(t->bp, 0, sizeof(bp_t) * MAX_BP);
    hashtbl_create(&t->tbl, MAX_BP);

    int options = PTRACE_O_EXITKILL;
    ptrace(PTRACE_SETOPTIONS, pid, NULL, options);
    printf("PID(%d)\n", t->pid);
    return true;
}

bool target_runnable(target_t *t)
{
    return t->run;
}

static bool target_handle_bp(target_t *t)
{
    if (!t->hit_bp) {
        return true;
    }

    // We have to take the bp first to avoid infinite loop
    bp_t *hit_bp = t->hit_bp;
    t->hit_bp = NULL;

    size_t addr;
    if (!target_get_reg(t, RIP, &addr))
        return false;

    /* If the address isn't at the last breakpoint we hit, it means
     * the user may change pc during this period. We don't execute an extra step
     * of the original instruction in that case. */
    if (addr == hit_bp->addr) {
        if (!target_step(t))
            return false;
    }

    /* restore the trap instruction before we do cont command */
    if (!bp_set(hit_bp))
        return false;

    return true;
}

bool target_step(target_t *t)
{
    if (!t->run) {
        printf("The program is not being run.\n");
        return false;
    }

    if (!target_handle_bp(t))
        return false;

    ptrace(PTRACE_SINGLESTEP, t->pid, NULL, NULL);
    if (!target_wait_sig(t)) {
        printf("bp handle\n");
        return false;
    }
    return true;
}

bool target_conti(target_t *t)
{
    if (!t->run) {
        printf("The program is not being run.\n");
        return false;
    }

    if (!target_handle_bp(t))
        return false;

    ptrace(PTRACE_CONT, t->pid, NULL, NULL);

    if (!target_wait_sig(t))
        return false;

    return true;
}

bool target_set_breakpoint(target_t *t, size_t addr)
{
    int n = __builtin_ffs(t->bp_bitmap);
    if (n == 0) {
        printf("Only at max 16 breakpoints could be set\n");
        return false;
    }

    n -= 1;
    t->bp_bitmap &= ~(1 << n);

    bp_init(&t->bp[n], t->pid, addr);
    if (!bp_set(&t->bp[n]))
        return false;

    if (!hashtbl_add(&t->tbl, t->bp[n].addr_key, &t->bp[n]))
        return false;

    printf("Breakpoint %d at 0x%lx\n", n + 1, addr);
    return true;
}


bool target_set_reg(target_t *t, size_t idx, size_t value)
{
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, t->pid, NULL, &regs);

    *(((size_t *) &regs) + idx) = value;

    ptrace(PTRACE_SETREGS, t->pid, NULL, &regs);
    return true;
}

bool target_get_reg(target_t *t, size_t idx, size_t *value)
{
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, t->pid, NULL, &regs);

    *value = *(((size_t *) &regs) + idx);
    return true;
}

bool target_get_reg_by_name(target_t *t, char *name, size_t *value)
{
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, t->pid, NULL, &regs);

    /* TODO: choose the corresponding register by name */
    bool found = false;
    int idx = 0;
    for (; idx < REGS_CNT; idx++) {
        if (strcmp(name, reg_desc_array[idx].name) == 0) {
            found = true;
            break;
        }
    }

    if (!found)
        return false;

    *value = *(((size_t *) &regs) + idx);
    return true;
}

bool target_read_mem(target_t *t, size_t *buf, size_t len, size_t target_addr)
{
    /* NOTE: maybe we should check the permission first instead of assume
     * all the region could be read. */
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buf;
    local[0].iov_len = len;
    remote[0].iov_base = (void *) target_addr;
    remote[0].iov_len = len;

    if (process_vm_readv(t->pid, local, 1, remote, 1, 0) == -1) {
        perror("process_vm_readv");
        return false;
    }

    return true;
}

bool target_write_mem(target_t *t, size_t *buf, size_t len, size_t target_addr)
{
    /* TODO: let's see if we can modify the tracee's permission to
     * use process_vm_writev on code region. Or we can check the permission
     * first then deciding the approach. */

    uint8_t unit = sizeof(size_t);

    for (; len > unit; len -= unit) {
        int ret = ptrace(PTRACE_POKEDATA, t->pid, (void *) target_addr, *buf);
        if (ret == -1) {
            perror("ptrace_poke");
            return false;
        }

        buf++;
        target_addr += unit;
    }

    /* If a write size is smaller than a word, we should read memory before
     * rewriting it. */
    if (len > 0) {
        size_t value =
            ptrace(PTRACE_PEEKDATA, t->pid, (void *) target_addr, NULL);
        if (value == (size_t) -1) {
            perror("ptrace_peek");
            return false;
        }

        uint8_t *v = (uint8_t *) &value;
        for (size_t i = 0; i < len; i++) {
            v[i] = *((uint8_t *) buf + i);
        }

        int ret = ptrace(PTRACE_POKEDATA, t->pid, (void *) target_addr, value);
        if (ret == -1) {
            perror("ptrace_poke");
            return false;
        }
    }

    return true;
}

void target_close(target_t *t)
{
    hashtbl_destroy(&t->tbl);
}
