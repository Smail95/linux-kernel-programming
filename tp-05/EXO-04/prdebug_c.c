#include <linux/module.h>
#include <linux/timer.h>
#include <linux/kernel_stat.h>

MODULE_DESCRIPTION("A pr_debug kernel module");
MODULE_AUTHOR("Maxime Lorrillere <maxime.lorrillere@lip6.fr>");
MODULE_LICENSE("GPL");

static struct timer_list prdebug_timer; /* the timer in question */
static unsigned long irqs_sum_last;		/* last number of interrupts */

/* the function to call when timer expires */
static void prdebug_timeout(struct timer_list *timer)
{
	unsigned long irqs_now = kstat_cpu(0).irqs_sum;
	/* print the number of interrupts since last interrupts */
	pr_debug("nr irqs %lu\n", irqs_now - irqs_sum_last);
	irqs_sum_last = irqs_now;

	pr_debug("reactivating timer\n");
	mod_timer(timer, jiffies + HZ);
}

static int prdebug_init(void)
{
	/* number of interrupts per cpu (0), since bootup */
	irqs_sum_last = kstat_cpu(0).irqs_sum;
	
	/* prepare a timer for first use - init */
	timer_setup(&prdebug_timer, prdebug_timeout, 0);
	/* modify a timer's timeout */
	mod_timer(&prdebug_timer, jiffies + HZ);

	pr_info("prdebug module loaded\n");
	return 0;
}

static void prdebug_exit(void)
{
	del_timer_sync(&prdebug_timer);
	pr_info("prdebug module unloaded\n");
}

module_init(prdebug_init);
module_exit(prdebug_exit);
