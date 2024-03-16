/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-10-21     Bernard      the first version.
 * 2011-10-27     aozima       update for cortex-M4 FPU.
 * 2011-12-31     aozima       fixed stack align issues.
 * 2012-01-01     aozima       support context switch load/store FPU register.
 * 2012-12-11     lgnq         fixed the coding style.
 * 2012-12-23     aozima       stack addr align to 8byte.
 * 2012-12-29     Bernard      Add exception hook.
 * 2013-06-23     aozima       support lazy stack optimized.
 * 2018-07-24     aozima       enhancement hard fault exception handler.
 * 2019-07-03     yangjie      add __ffs() for armclang.
 * 2022-06-12     jonas        fixed __ffs() for armclang.
 */

//#include <rtthread.h>
#include "stm32wlxx_hal.h"
#include "print.h"
#include "sht.h"
#include "sht_def.h"

#if               /* ARMCC */ (  (defined ( __CC_ARM ) && defined ( __TARGET_FPU_VFP ))    \
                  /* Clang */ || (defined ( __clang__ ) && defined ( __VFP_FP__ ) && !defined(__SOFTFP__)) \
                  /* IAR */   || (defined ( __ICCARM__ ) && defined ( __ARMVFP__ ))        \
                  /* GNU */   || (defined ( __GNUC__ ) && defined ( __VFP_FP__ ) && !defined(__SOFTFP__)) )
#define USE_FPU   1
#else
#define USE_FPU   0
#endif

/* exception and interrupt handler table */
uint32_t interrupt_from_thread;
uint32_t interrupt_to_thread;
uint32_t thread_switch_interrupt_flag;
/* exception hook */
static long (*exception_hook)(void *context) = NULL;

struct exception_stack_frame
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
};

struct stack_frame
{
#if USE_FPU
    uint32_t flag;
#endif /* USE_FPU */

    /* r4 ~ r11 register */
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;

    struct exception_stack_frame exception_stack_frame;
};

struct exception_stack_frame_fpu
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;

#if USE_FPU
    /* FPU register */
    uint32_t S0;
    uint32_t S1;
    uint32_t S2;
    uint32_t S3;
    uint32_t S4;
    uint32_t S5;
    uint32_t S6;
    uint32_t S7;
    uint32_t S8;
    uint32_t S9;
    uint32_t S10;
    uint32_t S11;
    uint32_t S12;
    uint32_t S13;
    uint32_t S14;
    uint32_t S15;
    uint32_t FPSCR;
    uint32_t NO_NAME;
#endif
};

struct stack_frame_fpu
{
    uint32_t flag;

    /* r4 ~ r11 register */
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;

#if USE_FPU
    /* FPU register s16 ~ s31 */
    uint32_t s16;
    uint32_t s17;
    uint32_t s18;
    uint32_t s19;
    uint32_t s20;
    uint32_t s21;
    uint32_t s22;
    uint32_t s23;
    uint32_t s24;
    uint32_t s25;
    uint32_t s26;
    uint32_t s27;
    uint32_t s28;
    uint32_t s29;
    uint32_t s30;
    uint32_t s31;
#endif

    struct exception_stack_frame_fpu exception_stack_frame;
};

uint8_t *hw_stack_init(void       *tentry,
                             void       *parameter,
                             uint8_t *stack_addr,
                             void       *texit)
{
    struct stack_frame *stack_frame;
    uint8_t         *stk;
    unsigned long       i;

    stk  = stack_addr + sizeof(uint32_t);
    stk  = (uint8_t *)ALIGN_DOWN((uint32_t)stk, 8);
    stk -= sizeof(struct stack_frame);

    stack_frame = (struct stack_frame *)stk;

    /* init all register */
    for (i = 0; i < sizeof(struct stack_frame) / sizeof(uint32_t); i ++)
    {
        ((uint32_t *)stack_frame)[i] = 0xdeadbeef;
    }

    stack_frame->exception_stack_frame.r0  = (unsigned long)parameter; /* r0 : argument */
    stack_frame->exception_stack_frame.r1  = 0;                        /* r1 */
    stack_frame->exception_stack_frame.r2  = 0;                        /* r2 */
    stack_frame->exception_stack_frame.r3  = 0;                        /* r3 */
    stack_frame->exception_stack_frame.r12 = 0;                        /* r12 */
    stack_frame->exception_stack_frame.lr  = (unsigned long)texit;     /* lr */
    stack_frame->exception_stack_frame.pc  = (unsigned long)tentry;    /* entry point, pc */
    stack_frame->exception_stack_frame.psr = 0x01000000L;              /* PSR */

#if USE_FPU
    stack_frame->flag = 0;
#endif /* USE_FPU */

    /* return task's current stack address */
    return stk;
}

/**
 * This function set the hook, which is invoked on fault exception handling.
 *
 * @param exception_handle the exception handling hook function.
 */
void hw_exception_install(long (*exception_handle)(void *context))
{
    exception_hook = exception_handle;
}

#define SCB_CFSR        (*(volatile const unsigned *)0xE000ED28) /* Configurable Fault Status Register */
#define SCB_HFSR        (*(volatile const unsigned *)0xE000ED2C) /* HardFault Status Register */
#define SCB_MMAR        (*(volatile const unsigned *)0xE000ED34) /* MemManage Fault Address register */
#define SCB_BFAR        (*(volatile const unsigned *)0xE000ED38) /* Bus Fault Address Register */
#define SCB_AIRCR       (*(volatile unsigned long *)0xE000ED0C)  /* Reset control Address Register */
#define SCB_RESET_VALUE 0x05FA0004                               /* Reset value, write to SCB_AIRCR can reset cpu */

#define SCB_CFSR_MFSR   (*(volatile const unsigned char*)0xE000ED28)  /* Memory-management Fault Status Register */
#define SCB_CFSR_BFSR   (*(volatile const unsigned char*)0xE000ED29)  /* Bus Fault Status Register */
#define SCB_CFSR_UFSR   (*(volatile const unsigned short*)0xE000ED2A) /* Usage Fault Status Register */

#ifdef USING_FINSH
static void usage_fault_track(void)
{
    sht_print("usage fault:\n");
    sht_print("SCB_CFSR_UFSR:0x%02X ", SCB_CFSR_UFSR);

    if(SCB_CFSR_UFSR & (1<<0))
    {
        /* [0]:UNDEFINSTR */
        sht_print("UNDEFINSTR ");
    }

    if(SCB_CFSR_UFSR & (1<<1))
    {
        /* [1]:INVSTATE */
        sht_print("INVSTATE ");
    }

    if(SCB_CFSR_UFSR & (1<<2))
    {
        /* [2]:INVPC */
        sht_print("INVPC ");
    }

    if(SCB_CFSR_UFSR & (1<<3))
    {
        /* [3]:NOCP */
        sht_print("NOCP ");
    }

    if(SCB_CFSR_UFSR & (1<<8))
    {
        /* [8]:UNALIGNED */
        sht_print("UNALIGNED ");
    }

    if(SCB_CFSR_UFSR & (1<<9))
    {
        /* [9]:DIVBYZERO */
        sht_print("DIVBYZERO ");
    }

    sht_print("\n");
}

static void bus_fault_track(void)
{
    sht_print("bus fault:\n");
    sht_print("SCB_CFSR_BFSR:0x%02X ", SCB_CFSR_BFSR);

    if(SCB_CFSR_BFSR & (1<<0))
    {
        /* [0]:IBUSERR */
        sht_print("IBUSERR ");
    }

    if(SCB_CFSR_BFSR & (1<<1))
    {
        /* [1]:PRECISERR */
        sht_print("PRECISERR ");
    }

    if(SCB_CFSR_BFSR & (1<<2))
    {
        /* [2]:IMPRECISERR */
        sht_print("IMPRECISERR ");
    }

    if(SCB_CFSR_BFSR & (1<<3))
    {
        /* [3]:UNSTKERR */
        sht_print("UNSTKERR ");
    }

    if(SCB_CFSR_BFSR & (1<<4))
    {
        /* [4]:STKERR */
        sht_print("STKERR ");
    }

    if(SCB_CFSR_BFSR & (1<<7))
    {
        sht_print("SCB->BFAR:%08X\n", SCB_BFAR);
    }
    else
    {
        sht_print("\n");
    }
}

static void mem_manage_fault_track(void)
{
    sht_print("mem manage fault:\n");
    sht_print("SCB_CFSR_MFSR:0x%02X ", SCB_CFSR_MFSR);

    if(SCB_CFSR_MFSR & (1<<0))
    {
        /* [0]:IACCVIOL */
        sht_print("IACCVIOL ");
    }

    if(SCB_CFSR_MFSR & (1<<1))
    {
        /* [1]:DACCVIOL */
        sht_print("DACCVIOL ");
    }

    if(SCB_CFSR_MFSR & (1<<3))
    {
        /* [3]:MUNSTKERR */
        sht_print("MUNSTKERR ");
    }

    if(SCB_CFSR_MFSR & (1<<4))
    {
        /* [4]:MSTKERR */
        sht_print("MSTKERR ");
    }

    if(SCB_CFSR_MFSR & (1<<7))
    {
        /* [7]:MMARVALID */
        sht_print("SCB->MMAR:%08X\n", SCB_MMAR);
    }
    else
    {
        sht_print("\n");
    }
}

static void hard_fault_track(void)
{
    if(SCB_HFSR & (1UL<<1))
    {
        /* [1]:VECTBL, Indicates hard fault is caused by failed vector fetch. */
        sht_print("failed vector fetch\n");
    }

    if(SCB_HFSR & (1UL<<30))
    {
        /* [30]:FORCED, Indicates hard fault is taken because of bus fault,
                        memory management fault, or usage fault. */
        if(SCB_CFSR_BFSR)
        {
            bus_fault_track();
        }

        if(SCB_CFSR_MFSR)
        {
            mem_manage_fault_track();
        }

        if(SCB_CFSR_UFSR)
        {
            usage_fault_track();
        }
    }

    if(SCB_HFSR & (1UL<<31))
    {
        /* [31]:DEBUGEVT, Indicates hard fault is triggered by debug event. */
        sht_print("debug event\n");
    }
}
#endif /* USING_FINSH */

struct exception_info
{
    uint32_t exc_return;
    struct stack_frame stack_frame;
};

void hw_hard_fault_exception(struct exception_info *exception_info)
{
#if defined(USING_FINSH) && defined(MSH_USING_BUILT_IN_COMMANDS)
    extern long list_thread(void);
#endif
    struct exception_stack_frame *exception_stack = &exception_info->stack_frame.exception_stack_frame;
    struct stack_frame *context = &exception_info->stack_frame;

    if (exception_hook != NULL)
    {
        long result;

        result = exception_hook(exception_stack);
        if (result == 0) return;
    }

    sht_print("psr: 0x%08x\r\n", context->exception_stack_frame.psr);

    sht_print("r0: 0x%08x\r\n", context->exception_stack_frame.r0);
    sht_print("r1: 0x%08x\r\n", context->exception_stack_frame.r1);
    sht_print("r2: 0x%08x\r\n", context->exception_stack_frame.r2);
    sht_print("r3: 0x%08x\r\n", context->exception_stack_frame.r3);
    sht_print("r4: 0x%08x\r\n", context->r4);
    sht_print("r5: 0x%08x\r\n", context->r5);
    sht_print("r6: 0x%08x\r\n", context->r6);
    sht_print("r7: 0x%08x\r\n", context->r7);
    sht_print("r8: 0x%08x\r\n", context->r8);
    sht_print("r9: 0x%08x\r\n", context->r9);
    sht_print("r10: 0x%08x\r\n", context->r10);
    sht_print("r11: 0x%08x\r\n", context->r11);
    sht_print("r12: 0x%08x\r\n", context->exception_stack_frame.r12);
    sht_print("lr: 0x%08x\r\n", context->exception_stack_frame.lr);
    sht_print("pc: 0x%08x\r\n", context->exception_stack_frame.pc);

    if (exception_info->exc_return & (1 << 2))
    {
        sht_print("hard fault on thread: %s\r\n\r\n", sht_cur_thread->name);

#if defined(USING_FINSH) && defined(MSH_USING_BUILT_IN_COMMANDS)
        list_thread();
#endif
    }
    else
    {
        sht_print("hard fault on handler\r\n\r\n");
    }

    if ( (exception_info->exc_return & 0x10) == 0)
    {
        sht_print("FPU active!\r\n");
    }

#ifdef USING_FINSH
    hard_fault_track();
#endif /* USING_FINSH */

    while (1);
}

/**
 * shutdown CPU
 */
__attribute__((weak)) void hw_cpu_shutdown(void)
{
    sht_print("shutdown...\n");
}

/**
 * reset CPU
 */
__attribute__((weak)) void hw_cpu_reset(void)
{
    SCB_AIRCR = SCB_RESET_VALUE;
}

#ifdef USING_CPU_FFS
/**
 * This function finds the first bit set (beginning with the least significant bit)
 * in value and return the index of that bit.
 *
 * Bits are numbered starting at 1 (the least significant bit).  A return value of
 * zero from any of these functions means that the argument was zero.
 *
 * @return return the index of the first bit set. If value is 0, then this function
 * shall return 0.
 */
#if defined(__CC_ARM)
__asm int __ffs(int value)
{
    CMP     r0, #0x00
    BEQ     exit

    RBIT    r0, r0
    CLZ     r0, r0
    ADDS    r0, r0, #0x01

exit
    BX      lr
}
#elif defined(__clang__)
int __ffs(int value)
{
    __asm volatile(
        "CMP     %1, #0x00            \n"
        "BEQ     1f                   \n"

        "RBIT    %1, %1               \n"
        "CLZ     %0, %1               \n"
        "ADDS    %0, %0, #0x01        \n"

        "1:                           \n"

        : "=r"(value)
        : "r"(value)
    );
    return value;
}
#elif defined(__IAR_SYSTEMS_ICC__)
int __ffs(int value)
{
    if (value == 0) return value;

    asm("RBIT %0, %1" : "=r"(value) : "r"(value));
    asm("CLZ  %0, %1" : "=r"(value) : "r"(value));
    asm("ADDS %0, %1, #0x01" : "=r"(value) : "r"(value));

    return value;
}
#elif defined(__GNUC__)
int __ffs(int value)
{
    return __builtin_ffs(value);
}
#endif

#endif
