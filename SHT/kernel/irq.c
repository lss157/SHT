/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2006-02-24     Bernard      first version
 * 2006-05-03     Bernard      add IRQ_DEBUG
 * 2016-08-09     ArdaFu       add interrupt enter and leave hook.
 * 2018-11-22     Jesven       interrupt_get_nest function add disable irq
 * 2021-08-15     Supperthomas fix the comment
 * 2022-01-07     Gabriel      Moving __on_xxxxx_hook to irq.c
 * 2022-07-04     Yunjie       fix DEBUG_LOG
 */

#include <sht.h>
//#include <rtthread.h>

#ifndef __on_interrupt_enter_hook
    #define __on_interrupt_enter_hook()          __ON_HOOK_ARGS(interrupt_enter_hook, ())
#endif
#ifndef __on_interrupt_leave_hook
    #define __on_interrupt_leave_hook()          __ON_HOOK_ARGS(interrupt_leave_hook, ())
#endif

#if defined(USING_HOOK) && defined(HOOK_USING_FUNC_PTR)

static void (*interrupt_enter_hook)(void);
static void (*interrupt_leave_hook)(void);

/**
 * @ingroup Hook
 *
 * @brief This function set a hook function when the system enter a interrupt
 *
 * @note The hook function must be simple and never be blocked or suspend.
 *
 * @param hook the function point to be called
 */
void interrupt_enter_sethook(void (*hook)(void))
{
    interrupt_enter_hook = hook;
}

/**
 * @ingroup Hook
 *
 * @brief This function set a hook function when the system exit a interrupt.
 *
 * @note The hook function must be simple and never be blocked or suspend.
 *
 * @param hook the function point to be called
 */
void interrupt_leave_sethook(void (*hook)(void))
{
    interrupt_leave_hook = hook;
}
#endif /* USING_HOOK */

/**
 * @addtogroup Kernel
 */

/**@{*/

#ifdef USING_SMP
#define interrupt_nest cpu_self()->irq_nest
#else
volatile uint8_t interrupt_nest = 0;
#endif /* USING_SMP */


/**
 * @brief This function will be invoked by BSP, when enter interrupt service routine
 *
 * @note Please don't invoke this routine in application
 *
 * @see interrupt_leave
 */
void interrupt_enter(void)
{
    long level;

    level = hw_interrupt_disable();
    interrupt_nest ++;
    hw_interrupt_enable(level);
}

/**
 * @brief This function will be invoked by BSP, when leave interrupt service routine
 *
 * @note Please don't invoke this routine in application
 *
 * @see interrupt_enter
 */
void interrupt_leave(void)
{
    long level;

    level = hw_interrupt_disable();
    interrupt_nest --;
    hw_interrupt_enable(level);
}


/**
 * @brief This function will return the nest of interrupt.
 *
 * User application can invoke this function to get whether current
 * context is interrupt context.
 *
 * @return the number of nested interrupts.
 */
__weak uint8_t interrupt_get_nest(void)
{
    uint8_t ret;
    long level;

    level = hw_interrupt_disable();
    ret = interrupt_nest;
    hw_interrupt_enable(level);
    return ret;
}

/**@}*/

