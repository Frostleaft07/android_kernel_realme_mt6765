#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/cacheflush.h>

static DEFINE_PER_CPU(unsigned int, cpu_is_managed);
static DEFINE_MUTEX(cache_clean_mutex);

static int cpufreq_set(struct cpufreq_policy *policy, unsigned int freq)
{
        int ret = -EINVAL;
        unsigned int *setspeed = policy->governor_data;

        pr_debug("cpufreq_set for cpu %u, freq %u kHz\n", policy->cpu, freq);

        mutex_lock(&cache_clean_mutex);

        // Clean cache if RAM is below 800MB
        if (totalram_pages() * PAGE_SIZE < 800 * 1024 * 1024) {
            // Example: Flush the entire cache
            flush_cache_all();
        }

        if (!per_cpu(cpu_is_managed, policy->cpu))
                goto err;

        *setspeed = freq;

        ret = __cpufreq_driver_target(policy, freq, CPUFREQ_RELATION_L);
 err:
        mutex_unlock(&cache_clean_mutex);
        return ret;
}

static ssize_t show_speed(struct cpufreq_policy *policy, char *buf)
{
        return sprintf(buf, "%u\n", policy->cur);
}

static int cpufreq_hatsune_policy_init(struct cpufreq_policy *policy)
{
        unsigned int *setspeed;

        setspeed = kzalloc(sizeof(*setspeed), GFP_KERNEL);
        if (!setspeed)
                return -ENOMEM;

        policy->governor_data = setspeed;
        return 0;
}

static void cpufreq_hatsune_policy_exit(struct cpufreq_policy *policy)
{
        mutex_lock(&cache_clean_mutex);
        kfree(policy->governor_data);
        policy->governor_data = NULL;
        mutex_unlock(&cache_clean_mutex);
}

static int cpufreq_hatsune_policy_start(struct cpufreq_policy *policy)
{
        unsigned int *setspeed = policy->governor_data;

        BUG_ON(!policy->cur);
        pr_debug("started managing cpu %u\n", policy->cpu);

        mutex_lock(&cache_clean_mutex);
        per_cpu(cpu_is_managed, policy->cpu) = 1;
        *setspeed = policy->cur;
        mutex_unlock(&cache_clean_mutex);
        return 0;
}

static void cpufreq_hatsune_policy_stop(struct cpufreq_policy *policy)
{
        unsigned int *setspeed = policy->governor_data;

        pr_debug("managing cpu %u stopped\n", policy->cpu);

        mutex_lock(&cache_clean_mutex);
        per_cpu(cpu_is_managed, policy->cpu) = 0;
        *setspeed = 0;
        mutex_unlock(&cache_clean_mutex);
}

static void cpufreq_hatsune_policy_limits(struct cpufreq_policy *policy)
{
        unsigned int *setspeed = policy->governor_data;

        mutex_lock(&cache_clean_mutex);

        pr_debug("limit event for cpu %u: %u - %u kHz, currently %u kHz, last set to %u kHz\n",
                 policy->cpu, policy->min, policy->max, policy->cur, *setspeed);

        if (policy->max < *setspeed)
                __cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
        else if (policy->min > *setspeed)
                __cpufreq_driver_target(policy, policy->min, CPUFREQ_RELATION_L);
        else
                __cpufreq_driver_target(policy, *setspeed, CPUFREQ_RELATION_L);

        mutex_unlock(&cache_clean_mutex);
}

struct cpufreq_governor cpufreq_gov_hatsune = {
        .name                = "hatsune",
        .init                = cpufreq_hatsune_policy_init,
        .exit                = cpufreq_hatsune_policy_exit,
        .start               = cpufreq_hatsune_policy_start,
        .stop                = cpufreq_hatsune_policy_stop,
        .limits              = cpufreq_hatsune_policy_limits,
        .store_setspeed      = cpufreq_set,
        .show_setspeed       = show_speed,
        .owner               = THIS_MODULE,
};

static int __init cpufreq_gov_hatsune_init(void)
{
        return cpufreq_register_governor(&cpufreq_gov_hatsune);
}

static void __exit cpufreq_gov_hatsune_exit(void)
{
        cpufreq_unregister_governor(&cpufreq_gov_hatsune);
}

MODULE_AUTHOR("Dominik Brodowski <linux@brodo.de>, "
                "Russell King <rmk@arm.linux.org.uk>");
MODULE_DESCRIPTION("CPUfreq policy governor 'hatsune'");
MODULE_LICENSE("GPL");

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_HATSUNE
static
#endif
struct cpufreq_governor *cpufreq_default_governor(void)
{
        return &cpufreq_gov_hatsune;
}

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_HATSUNE
fs_initcall(cpufreq_gov_hatsune_init);
#else
module_init(cpufreq_gov_hatsune_init);
module_exit(cpufreq_gov_hatsune_exit);
#endif