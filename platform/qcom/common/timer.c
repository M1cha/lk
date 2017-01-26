/*
 * Copyright (c) 2015 Stefan Kristiansson
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <trace.h>
#include <err.h>
#include <kernel/thread.h>
#include <platform/timer.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <sys/types.h>
#include <lib/fixed_point.h>
#include <platform/qcom-msm8960.h>

#define LOCAL_TRACE 0

#define GPT_MATCH_VAL        (0x0000)
#define GPT_COUNT_VAL        (0x0004)
#define GPT_ENABLE           (0x0008)
#define GPT_CLEAR            (0x000C)

#define DGT_MATCH_VAL        (0x0000)
#define DGT_COUNT_VAL        (0x0004)
#define DGT_ENABLE           (0x0008)
#define DGT_CLEAR            (0x000C)
#define DGT_CLK_CTL          (0x0010)

#define GPT_TIMER_OFFSET     (0x0004)
#define DGT_TIMER_OFFSET     (0x0024)

#define DGTREG(reg)    (*REG32(control_base + DGT_TIMER_OFFSET + (reg)))
#define GPTREG(reg)    (*REG32(control_base + GPT_TIMER_OFFSET + (reg)))

#define GPT_ENABLE_CLR_ON_MATCH_EN        2
#define GPT_ENABLE_EN                     1
#define DGT_ENABLE_CLR_ON_MATCH_EN        2
#define DGT_ENABLE_EN                     1

#define SPSS_TIMER_STATUS_DGT_EN    (1 << 0)

static platform_timer_callback t_callback;
static addr_t control_base;
static spin_lock_t lock = SPIN_LOCK_INITIAL_VALUE;

static lk_time_t periodic_interval;
static uint32_t timer_freq;
static struct fp_32_64 timer_freq_msec_conversion;
static struct fp_32_64 ms_per_gptcnt;
static struct fp_32_64 us_per_gptcnt;

static volatile uint64_t ticks = 0;

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval)
{
    LTRACEF("callback %p, arg %p, interval %u\n", callback, arg, interval);

    uint64_t ticks = u64_mul_u64_fp32_64(interval, timer_freq_msec_conversion);
    if (unlikely(ticks == 0))
        ticks = 1;
    if (unlikely(ticks > 0xffffffff))
        ticks = 0xffffffff;

    spin_lock_saved_state_t state;
    spin_lock_irqsave(&lock, state);

    t_callback = callback;

    periodic_interval = interval;

    DGTREG(DGT_MATCH_VAL) = ticks;
    DGTREG(DGT_CLEAR) = 0;
    DGTREG(DGT_ENABLE) = DGT_ENABLE_EN | DGT_ENABLE_CLR_ON_MATCH_EN;

    spin_unlock_irqrestore(&lock, state);

    return NO_ERROR;
}

static uint64_t read_gptcnt(void)
{
    return (uint64_t)GPTREG(GPT_COUNT_VAL);
}

static lk_time_t gptcnt_to_lk_time(uint64_t gptcnt)
{
    return u32_mul_u64_fp32_64(gptcnt, ms_per_gptcnt);
}

static lk_bigtime_t gptcnt_to_lk_bigtime(uint64_t gptcnt)
{
    return u64_mul_u64_fp32_64(gptcnt, us_per_gptcnt);
}

lk_bigtime_t current_time_hires(void)
{
    return gptcnt_to_lk_bigtime(read_gptcnt());
}

lk_time_t current_time(void)
{
    return gptcnt_to_lk_time(read_gptcnt());
}

static enum handler_return platform_tick(void *arg)
{
    LTRACE;

    ticks += periodic_interval;

    if (t_callback) {
        return t_callback(arg, ticks);
    } else {
        return INT_NO_RESCHEDULE;
    }
}

void qcom_timer_early_init(addr_t _control_base)
{
    control_base = _control_base;

    // enable gpt
    GPTREG(GPT_ENABLE) = GPT_ENABLE_EN;

    uint32_t gpt_freq = 33000;
    fp_32_64_div_32_32(&ms_per_gptcnt, 1000, gpt_freq);
    fp_32_64_div_32_32(&us_per_gptcnt, 1000 * 1000, gpt_freq);
}

void qcom_timer_init(uint32_t freq)
{
    // save the timer frequency for later calculations
    timer_freq = freq;

    // precompute the conversion factor for global time to real time
    fp_32_64_div_32_32(&timer_freq_msec_conversion, timer_freq, 1000);

    // disable timer
    DGTREG(DGT_ENABLE) = 0;

    // DGT uses LPXO source which is 27MHz. Set clock divider to 4.
    DGTREG(DGT_CLK_CTL) = 3;

    // register the platform tick
    register_int_handler(INT_DEBUG_TIMER_EXP, &platform_tick, NULL);
    unmask_interrupt(INT_DEBUG_TIMER_EXP);
}
