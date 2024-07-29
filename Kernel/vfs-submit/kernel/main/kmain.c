/******************************************************************************/
/* Important Fall 2023 CSCI 402 usage information:                            */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "types.h"
#include "globals.h"
#include "kernel.h"
#include "errno.h"

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadowd.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/disk/ata.h"
#include "drivers/tty/virtterm.h"
#include "drivers/pci.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"
#include "test/s5fs_test.h"

GDB_DEFINE_HOOK(initialized)

void      *bootstrap(int arg1, void *arg2);
void      *idleproc_run(int arg1, void *arg2);
kthread_t *initproc_create(void);
void      *initproc_run(int arg1, void *arg2);
void      *final_shutdown(void);

//ADDED (Kernel 1)
extern void *sunghan_test(int, void*);
extern void *sunghan_deadlock_test(int, void*);
extern void *faber_thread_test(int, void*);
//ADDED (Kernel 2)
extern void *vfstest_main(int, void*);
extern int faber_fs_thread_test(kshell_t *ksh, int argc, char **argv);
extern int faber_directory_test(kshell_t *ksh, int argc, char **argv);

/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
bootstrap(int arg1, void *arg2)
{
        /* If the next line is removed/altered in your submission, 20 points will be deducted. */
        dbgq(DBG_TEST, "SIGNATURE: 53616c7465645f5f02550671acae0e3e16c53b534834a25d31157c5a76044b1d150d784ca52da9dca56ac81020cd3c40\n");
        /* necessary to finalize page table information */
        pt_template_init();

        //NOT_YET_IMPLEMENTED("PROCS: bootstrap");

        
        curproc = proc_create("idle");//idle process
        KASSERT(NULL != curproc);
        KASSERT(PID_IDLE == curproc->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 1.a)\n");
        curthr = kthread_create(curproc, idleproc_run, NULL, NULL);
        KASSERT(NULL != curthr);
        dbg(DBG_PRINT, "(GRADING1A 1.a)\n");
        dbg(DBG_PRINT, "(GRADING1A)\n");
        context_make_active(&curthr->kt_ctx);

        panic("weenix returned to bootstrap()!!! BAD!!!\n");
        return NULL;
}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
idleproc_run(int arg1, void *arg2)
{
        int status;
        pid_t child;

        /* create init proc */
        kthread_t *initthr = initproc_create();
        init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */

#ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes */
        //NOT_YET_IMPLEMENTED("VFS: idleproc_run");
        
        curproc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);//increase ref count
        initthr->kt_proc->p_cwd = vfs_root_vn;
        vref(vfs_root_vn);//increase ref count
        dbg(DBG_PRINT,"(GRADING2A)\n");

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        //NOT_YET_IMPLEMENTED("VFS: idleproc_run");

        do_mkdir("/dev");
        do_mknod("/dev/null", S_IFCHR, MEM_NULL_DEVID);//MEM_NULL_DEVID: major: 1, minor: 0
        do_mknod("/dev/zero", S_IFCHR, MEM_ZERO_DEVID);//MEM_ZERO_DEVID: major: 1, minor: 1
        do_mknod("/dev/tty0", S_IFCHR, MKDEVID(2,0));//tty0: major: 2, minor: 0
        //USE do_stat TO CHECK
        dbg(DBG_PRINT,"(GRADING2A)\n");
#endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);

        return final_shutdown();
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
kthread_t *
initproc_create(void)
{
        //NOT_YET_IMPLEMENTED("PROCS: initproc_create");

        proc_t *p = proc_create("init");//init process
        KASSERT(NULL != p);
        KASSERT(PID_INIT == p->p_pid);
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");
        kthread_t *thr = kthread_create(p, initproc_run, NULL, NULL);
        KASSERT(NULL != thr);
        dbg(DBG_PRINT, "(GRADING1A 1.b)\n");
        dbg(DBG_PRINT, "(GRADING1A)\n");
        return thr;
}


#ifdef __DRIVERS__

int my_faber_thread_test(kshell_t *kshell, int argc, char **argv)
{
        int status = 0;
        proc_t *faberTest = proc_create("faber");
        kthread_t *faberThread = kthread_create(faberTest, faber_thread_test, 0, 0);
        sched_make_runnable(faberThread);
        do_waitpid(faberTest->p_pid , 0, &status);
        return 0;
}
int my_sunghan_deadlock_test(kshell_t *kshell, int argc, char **argv)
{
        int status = 0;
        proc_t *deadlockTest = proc_create("deadlock");
        kthread_t *deadlockThread = kthread_create(deadlockTest, sunghan_deadlock_test, 0, 0);
        sched_make_runnable(deadlockThread);
        do_waitpid(deadlockTest->p_pid , 0, &status);
        return 0;
}
int my_sunghan_test(kshell_t *kshell, int argc, char **argv)
{
        int status = 0;
        proc_t *sunghanTest = proc_create("sunghan");
        kthread_t *sunghanThread = kthread_create(sunghanTest, sunghan_test, 0, 0);
        sched_make_runnable(sunghanThread);
        do_waitpid(sunghanTest->p_pid , 0, &status);
        return 0;
}
int my_vfs_test(kshell_t *kshell, int argc, char **argv)
{
        int status = 0;
        proc_t *vfsTest = proc_create("vfs");
        kthread_t *vfsThread = kthread_create(vfsTest, vfstest_main, 1, NULL);
        sched_make_runnable(vfsThread);
        do_waitpid(vfsTest->p_pid, 0, &status);
        return 0;
}
#endif /* __DRIVERS__ */

/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/sbin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
void *
initproc_run(int arg1, void *arg2)
{
        //NOT_YET_IMPLEMENTED("PROCS: initproc_run");
        
        #ifdef __DRIVERS__
    
        //Kernel 1
        kshell_add_command("sunghan", my_sunghan_test, "Run sunghan_test().");
        kshell_add_command("deadlock", my_sunghan_deadlock_test, "Run sunghan_deadlock_test().");
        kshell_add_command("faber", my_faber_thread_test, "Run faber_thread_test().");
        //Kernel 2
        kshell_add_command("vfs", my_vfs_test, "Run vfstest_main().");
        kshell_add_command("thrtest", faber_fs_thread_test, "Run faber_fs_thread_test().");
        kshell_add_command("dirtest", faber_directory_test, "Run faber_directory_test().");

        kshell_t *kshell = kshell_create(0);
        if (NULL == kshell) panic("init: Couldn't create kernel shell\n");
        while (kshell_execute_next(kshell));
        kshell_destroy(kshell);

        #endif /* __DRIVERS__ */
        return NULL;
}
