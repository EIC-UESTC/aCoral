/**
 * @file posix_sched.c
 * @author 李杰
 * @brief sched.h 中调度器函数的实现。
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 本文件提供了 POSIX 兼容调度函数的核心实现，作为 POSIX API 与底层 aCoral RTOS内核之间的桥梁。
 * 它处理线程优先级映射、策略管理（主要支持 SCHED_FIFO）以及诸如让出 CPU 等基本调度操作。此文件是 sched.h中声明的 POSIX 调度接口的具体实现。
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建，实现 POSIX 调度相关函数。
 * </table>
 */

/* posix兼容层头文件 */
#include "acoral_posix.h"

/* kernel层头文件 */
#include "lsched.h"
#include "policy.h"


/**
 * @brief 将 POSIX 进程/线程 ID (pid_t) 转换为 aCoral 线程控制块 (TCB) 指针。
 *
 * 此函数作为 POSIX 调度接口层和 aCoral 内核之间的桥梁。
 * 它根据给定的  pid 查找对应的 aCoral 线程。
 * 如果 pid 为 0，则尝试获取当前正在执行的 aCoral 线程。
 * 资源类型信息通过 ACORAL_RES_TYPE() 宏从资源 ID 中提取。
 *
 * @param pid 要查找的 POSIX 进程/线程 ID。
 * 			 - 如果为 0，表示当前线程。
 * 			 - 如果为其他正值，表示特定的线程 ID (映射到 aCoral 的 acoral_id)。
 *
 * @return acoral_thread_t* 如果找到对应的 aCoral 线程，则返回其 TCB 指针。
 * 		        如果未找到线程，或者在不安全的环境（如中断服务程序中尝试获取当前线程）下调用，则返回 NULL。
 * 		        当返回 NULL 时，全局变量 errno会被设置为相应的错误码：
 * 		   - ESRCH: 未找到具有指定 pid 的线程。
 * 		   - EINVAL: (例如) 在中断上下文中尝试获取当前线程 (pid=0)。
 *
 * @note 此函数是 POSIX 调度接口适配层的内部辅助函数。
 * @note pid_t 类型与 aCoral 的 acoral_id 类型兼容。
 * @note 依赖于 aCoral 内核提供的 acoral_cur_thread (获取当前线程)
 *       和 acoral_get_res_by_id (通过 ID 获取资源)
 *       以及 ACORAL_RES_TYPE() 宏来确定资源类型
 */
static acoral_thread_t *posix_get_acoral_thread(pid_t pid)
{
	acoral_thread_t *thread_tcb = NULL;
	acoral_id target_acoral_id;

	if (0 == pid) {	/* posix约定：pid 0 指的是调用者自身(当前线程) */
		/* 检查是否在中断上下文中，在中断中获取 "当前线程" 不安全或无意义。 */
		if (acoral_intr_nesting > 0) {
			//acoral_printerr("posix_get_acoral_thread: Cannot get current thread (pid=0) from ISR context.\n");
			errno = EINVAL;
			return NULL;
		}

		/* 获取 aCoral 内核中表示当前正在运行的线程的指针 */
		if (NULL == acoral_cur_thread)
		{
			//acoral_printerr("posix_get_acoral_thread: acoral_cur_thread is NULL (pid=0).\n");
			errno = ESRCH;
			return NULL;
		}

		/* 获取当前线程的资源id */
		target_acoral_id = acoral_cur_thread->res.id;

		/* 检查当前线程的类型 (通常应该是线程) */
		if (ACORAL_RES_THREAD == ACORAL_RES_TYPE(target_acoral_id))
		{
			thread_tcb = acoral_cur_thread;
		}
		else
		{
			/* 理论上不应该发生，当前线程类型不是线程 */
			errno = ESRCH;
			return NULL;
		}

	}
	else
	{ 	/* 指定的pid */
		target_acoral_id = (acoral_id)pid;

		/* 1.从 ID 获取资源类型 */
		if (ACORAL_RES_THREAD == ACORAL_RES_TYPE(target_acoral_id))
		{
			/* 2.如果类型是线程，再根据 ID 获取 TCB 指针 */
			void *raw_ptr = (void*)acoral_get_res_by_id(target_acoral_id);
			if (NULL != raw_ptr)
			{
				thread_tcb = (acoral_thread_t *)raw_ptr;
			}
			else
			{
				/* 类型是线程，但无法获取资源指针 (ID 可能已失效或池错误) */
				thread_tcb = NULL; /* errno 会在 acoral_get_res_by_id 内部或之后设置 */
			}
		}
		else
		{
			/* ID 代表的资源类型不是线程 */
			thread_tcb = NULL;
			errno = EBADF; /* 表示句柄类型错误 */
		}
	}

	if ((NULL == thread_tcb) && (0 == errno))
	{
		/* 如果之前的步骤失败但没有设置 errno */
		errno = ESRCH;
	}

	return thread_tcb;
}

/**
 * 基于thread.h中优先级的宏定义， POSIX 实时优先级范围可以设置为 [1, 60]。
 * POSIX 优先级 1 (最低) 会映射到 aCoral 用户优先级 60 (最低)。
 * POSIX 优先级 60 (最高) 会映射到 aCoral 用户优先级 1 (最高)。
 */
/**
 * @brief 将 aCoral 用户线程优先级转换为 POSIX 实时优先级。
 * @param acoral_user_prio aCoral 用户线程优先级
 * @param posix_prio_out 指向存储转换后的 POSIX 优先级的指针。如果函数成功，结果会写入这里。
 * @return int 0 表示成功，-1 表示失败 ,并设置 errno。
 */
static int acoral_user_priority_to_posix(acoral_u8 acoral_user_prio, int *posix_prio_out)
{
	if (NULL == posix_prio_out)
	{
		errno = EINVAL;
		return -1;
	}

	if ((acoral_user_prio < ACORAL_MAX_PRIO) || (acoral_user_prio > ACORAL_MINI_PRIO))
	{
		errno = EINVAL;
		return -1;
	}

	*posix_prio_out = POSIX_REALTIME_PRIORITY_MAX  - (acoral_user_prio - ACORAL_MAX_PRIO);

	return 0;
}

/**
 * @brief 将 POSIX 实时优先级转换为 aCoral 用户线程优先级。
 * @param posix_prio POSIX 实时优先级
 * @param acoral_prio_out 指向存储转换后的 aCoral 用户优先级的指针
 * @return int 0 表示成功，-1 表示失败 ，并设置 errno。
 */
static int posix_priority_to_acoral_user(int posix_prio, acoral_u8 *acoral_prio_out)
{
    if (NULL == acoral_prio_out)
    {
        errno = EINVAL;
        return -1;
    }

    if ((posix_prio < POSIX_REALTIME_PRIORITY_MIN) || (posix_prio > POSIX_REALTIME_PRIORITY_MAX))
    {
        errno = EINVAL;
        return -1;
    }

    *acoral_prio_out = (acoral_u8)(ACORAL_MAX_PRIO + (POSIX_REALTIME_PRIORITY_MAX - posix_prio));
    return 0;
}


/**
 * @brief 使当前正在运行的线程主动放弃处理器(运行态→就绪态)
 * POSIX定义：此函数强制正在运行的线程放弃处理器，直到它再次成为其线程列表（对应其调度策略和优先级的就绪队列）的头部。
 *
 * @return int 成功时返回 0。
 * 			        如果无法执行（如 ISR 调用或未找到当前线程/CPU），返回 -1 并设置 errno。
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/sched_yield.html
 */
int sched_yield(void)
{
	acoral_thread_t *current_thread;	/* 指向当前线程的线程控制块 (TCB) 的指针。 */
	acoral_u8 current_prio;				/* 存储当前线程的优先级。 */
	acoral_list_t *thread_ready_node;	/* 指向线程 TCB 内部用于链接到就绪队列的 acoral_list_t 成员的指针(常见的侵入式链表设计) */
	acoral_u32 cpu_id;					/* 存储当前代码执行所在的 CPU 核心的 ID */
	acoral_thread_prio_array_t *p_prio_array_for_this_cpu;	/* 指向当前 CPU 的就绪队列数据结构的指针 */

	/* 1.检查是否在中断上下文 */
	if(acoral_intr_nesting > 0)
	{
		// acoral_printerr("sched_yield() cannot be called from ISR context.\n");
		errno = EPERM; /* 操作不允许 */
		return -1;
	}

	/* 2.获取当前线程的 TCB */
	current_thread = acoral_cur_thread;
	if(current_thread == NULL)
	{
		errno = ESRCH; /* 没有这样的进程/线程 */
		return -1;
	}

	/* 3. 获取当前 CPU ID */
	cpu_id = HAL_GET_CURRENT_CPU();
	if (cpu_id >= CFG_MAX_CPU)
	{
		/* 防御性检查，确保 CPU ID 有效 */
		errno = EINVAL;
		return -1;
	}

	/* 4.获取当前 CPU 对应的 acoral_thread_prio_array_t 指针 */
	p_prio_array_for_this_cpu = &acoral_ready_queues[cpu_id].array;

	/* 5.执行 yield 操作 */
	acoral_enter_critical();	/* 进入临界区，保护就绪队列(这个函数内部也会获取cpu_id) */

	/* 检查线程运行状态的掩码 */
	if ((current_thread->state & ACORAL_THREAD_STATE_RUNNING) != 0)
	{
		/* 获取当前线程的优先级 */
		current_prio = current_thread->prio;

		/* 获取线程TCB中用于链接就绪队列的节点 thread_ready_node */
		thread_ready_node = &current_thread -> ready;

		/* a. 从当前 CPU 的就绪队列中移除当前线程 */
		acoral_thread_prio_queue_del(p_prio_array_for_this_cpu, current_prio, thread_ready_node);

		/* b. 将当前线程重新添加到当前 CPU 的同一优先级就绪队列的尾部 */
		acoral_thread_prio_queue_add(p_prio_array_for_this_cpu, current_prio, thread_ready_node);

		/* c.设置需要调度的标志 */
		acoral_set_need_sched(true);
	}
	acoral_exit_critical(); 	/* 退出临界区 */

	/* 触发调度器 */
	acoral_sched();
	return 0;
}


/**
 * @brief 获取指定 POSIX 调度策略的最小有效优先级
 *
 * 此函数返回由 policy 参数指定的调度策略的最小有效优先级值。
 * 优先级值遵循 POSIX 约定：数值越大，优先级越高。
 *
 * @param policy 一个在 <sched.h> 中定义的调度策略值 (例如 SCHED_OTHER, SCHED_FIFO, SCHED_RR)。
 *
 * @return int 如果成功，返回该策略的最小优先级值。
 *             如果不成功，返回 -1 并设置 errno 为 EINVAL
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/sched_get_priority_min.html
 */
int sched_get_priority_min(int policy)
{
	switch (policy)
	{
		case SCHED_FIFO:	/* 代表 comm_policy */
			return POSIX_REALTIME_PRIORITY_MIN;

		case SCHED_OTHER:	/* 不支持此策略 */
		case SCHED_RR:		/* 不支持此策略 */
			errno = EINVAL;
			return -1;

		default:
			/* policy 参数的值不代表一个已定义的调度策略 */
			errno = EINVAL;
			return -1;
	}
}


/**
 * @brief 获取指定 POSIX 调度策略的最大有效优先级。
 *
 * 此函数返回由 policy 参数指定的调度策略的最大有效优先级值。
 * 优先级值遵循 POSIX 约定：数值越大，优先级越高。
 *
 * @param policy 一个在 <sched.h> 中定义的调度策略值 (例如 SCHED_OTHER, SCHED_FIFO, SCHED_RR)。
 *
 * @return int 如果成功，返回该策略的最大优先级值。
 *             如果不成功，返回 -1 并设置 errno 为 EINVAL。
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/sched_get_priority_min.html
 */
int sched_get_priority_max(int policy)
{
    switch (policy)
    {

        case SCHED_FIFO:	/* 代表 comm_policy */
            return POSIX_REALTIME_PRIORITY_MAX;

        case SCHED_OTHER:	 /* 不支持此策略 */
        case SCHED_RR:		 /* 不支持此策略 */
        	errno = EINVAL;
        	return -1;

        default:
        	/* policy 参数的值不代表一个已定义的调度策略 */
            errno = EINVAL;
            return -1;
    }
}

/**
 * @brief 获取指定线程的调度参数(优先级)
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/sched_getparam.html
 *
 * @param 	pid 目标线程的ID。如果为0，则获取当前线程的参数。
 * @param 	param 指向 struct sched_param 的指针，用于存储获取到的参数。
 * @return 	int 成功返回0，失败返回-1并设置 errno。
 */

int sched_getparam(pid_t pid, struct sched_param *param)
{
	acoral_thread_t *target_thread;
	int current_posix_policy;
	int ret;

	/* 1.检查输出参数 param_ptr 是否为 NULL */
	if(NULL == param)
	{
		errno = EINVAL;
		return -1;
	}

	/* 2. 获取目标线程的 TCB */
	target_thread = posix_get_acoral_thread(pid);
	if (NULL == target_thread)
	{
		//posix_get_acoral_thread内部 设置了errno
		return -1;
	}

	/* 3.检查目标线程当前的调度策略 */
	current_posix_policy = sched_getscheduler(pid);
	if (-1 == current_posix_policy)
	{
		// errno 已经被 sched_getscheduler设置
		return -1;
	}

	if (SCHED_FIFO != current_posix_policy)
	{
		errno = EINVAL;
		return -1;
	}

	/* 4. 将 aCoral 内部线程优先级转换为 POSIX 实时优先级 */
	ret = acoral_user_priority_to_posix(target_thread->prio, &param->sched_priority);
	if (-1 == ret)
	{
		//acoral_user_priority_to_posix 内部设置了 errno
		return -1;
	}

	/* 5. 成功完成(调度参数已填充到 param_ptr 指向的结构体中) */
	return 0;
}

/**
 * @brief 获取指定线程的调度策略(SCHED_FIFO)
 *
 * @param pid 目标线程的ID。
 * 			     如果为 0，则表示获取当前调用线程的策略。
 *
 * @return int 成功时返回 SCHED_FIFO。
 *             失败时返回 -1，并设置全局变量 errno 以指示错误。
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/sched_getscheduler.html
 */
int sched_getscheduler(pid_t pid)
{
	acoral_thread_t *target_thread;

	/* 1. 获取目标线程的 TCB */
	target_thread = posix_get_acoral_thread(pid);

	/* 2. 检查 TCB 获取是否成功 */
	if (NULL == target_thread)
	{
	    // errno 已在 posix_get_acoral_thread 内部被正确设置
		return -1;
	}

	/* 3. 根据 aCoral 内部策略确定 POSIX 策略 */
	switch (target_thread->policy)
	{
		case ACORAL_SCHED_POLICY_COMM:
			// comm_policy 的核心行为是 FIFO
			return SCHED_FIFO;

		default:
			errno = EINVAL;		//策略值本身无效
			return -1;
	}
}


/**
 * @brief 设置指定线程的调度参数(优先级)
 *
 * @param pid 目标线程的ID。如果为0，则设置当前线程的参数。
 * @param param 指向 struct sched_param 的指针，其中包含要设置的参数。
 *
 * @return int 成功返回0，失败返回-1并设置 errno。
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/sched_setparam.html
 */
int sched_setparam(pid_t pid, const struct sched_param *param)
{
	acoral_thread_t *target_thread;
	acoral_u8 new_acoral_prio;
	int current_posix_policy;
	int ret;

	/* 1.参数有效性检查 */
	if (NULL == param)
	{
		errno = EINVAL;
	    return -1;
	}

	/* 2.获取目标线程的 TCB */
	target_thread = posix_get_acoral_thread(pid);
	if (NULL == target_thread)
	{
		// errno 已在 posix_get_acoral_thread 内部设置
		return -1;
	}

	/* 3.检查目标线程当前的调度策略 */
	current_posix_policy = sched_getscheduler(pid);
	if (-1 == current_posix_policy)
	{
		// errno 已经被 sched_getscheduler设置
		return -1;
	}

	if (SCHED_FIFO != current_posix_policy)
	{
		errno = EINVAL;
		return -1;
	}

	/* 4.验证请求的优先级是否在允许的范围内 */
	if ((param->sched_priority < POSIX_REALTIME_PRIORITY_MIN) ||
	    (param->sched_priority > POSIX_REALTIME_PRIORITY_MAX))
	{
		errno = EINVAL; /* 请求的优先级超出范围 */
		return -1;
	}

	/* 5. 将 POSIX 优先级转换为 aCoral 内部优先级 */
	ret = posix_priority_to_acoral_user(param->sched_priority, &new_acoral_prio);
	if (-1 == ret)
	{
		/* 转换失败，posix_priority_to_acoral_user 内部已设置 errno */
	    return -1;
	}

	/*
	 *  6. 实际更改 aCoral 线程的优先级
     *	new_acoral_prio 是已经从 POSIX 优先级转换过来的 aCoral 内部优先级 (acoral_u8)
     *	target_thread 是目标线程的 TCB
	 */
	acoral_thread_change_prio(target_thread, (acoral_u32)new_acoral_prio);

	/* 7. 触发本地调度 */
	if (target_thread->cpu == acoral_current_cpu)
	{
		acoral_set_need_sched(true);
		acoral_sched();
	}

	return 0;
}



/**
 * @brief 设置指定线程的调度策略和参数
 * @details 在当前 aCoral 实现中，仅支持将策略设置为 SCHED_FIFO，并将 aCoral 内部策略相应地设置为 ACORAL_SCHED_POLICY_COMM。
 *
 * @param pid 目标线程的ID。如果为0，则设置当前线程的策略和参数。
 * @param policy 要设置的新的 POSIX 调度策略 (期望为 SCHED_FIFO)。
 * @param param 指向 struct sched_param 的指针，其中包含要设置的参数 (主要是 sched_priority)。
 *
 * @return int 成功时返回目标线程先前的 POSIX 调度策略。
 *             失败时返回 -1 并设置 errno。
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/sched_setscheduler.html
 */
int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param)
{
	acoral_thread_t *target_thread;
	acoral_u8 new_acoral_prio;
	int old_posix_policy;
	acoral_u32 new_acoral_internal_policy;
	int ret;

	/* 1. 参数有效性检查 */
	if (param == NULL)
	{
		errno = EINVAL;
	    return -1;
	}

	/* 2. 验证请求的 POSIX 策略是否受支持 */
	if (SCHED_FIFO != policy)
	{
		errno = EINVAL;
		return -1;
	}
	new_acoral_internal_policy = ACORAL_SCHED_POLICY_COMM;

	/* 3. 获取目标线程的 TCB */
	target_thread = posix_get_acoral_thread(pid);
	if (NULL == target_thread)
	{
		// errno 已在 posix_get_acoral_thread 内部设置
	    return -1;
	}

	/* 4. 获取旧的 POSIX 调度策略 (用于返回值) */
	old_posix_policy = sched_getscheduler(pid);
	if ((-1 == old_posix_policy) && (0 != errno))
	{
		// errno 已经被 sched_getscheduler 设置
		return -1;
	}

	/* 5.验证请求的优先级是否在新策略 (SCHED_FIFO) 的允许范围内 */
	if ((param->sched_priority < POSIX_REALTIME_PRIORITY_MIN) ||
	    (param->sched_priority > POSIX_REALTIME_PRIORITY_MAX))
	{
		errno = EINVAL;
		return -1;
	}

	/* 6.将 POSIX 优先级转换为 aCoral 内部优先级 */
	ret = posix_priority_to_acoral_user(param->sched_priority, &new_acoral_prio);
	if (-1 == ret)
	{
		/* 转换失败，posix_priority_to_acoral_user 内部已设置 errno */
	    return -1;
	}

	/* 7.修改 aCoral 线程状态 */
	/* a.修改策略 */
	acoral_enter_critical();
	target_thread->policy = new_acoral_internal_policy;
	acoral_exit_critical();

	/* b.修改优先级 */
	acoral_thread_change_prio(target_thread, (acoral_u32)new_acoral_prio);

	/* c.触发本地调度 */
	if (target_thread->cpu == acoral_current_cpu)
	{
		acoral_set_need_sched(true);
		acoral_sched();
	}

    return old_posix_policy;	/* 返回旧的 POSIX 策略 */
}


/**
 * @brief 获取 SCHED_RR 的时间片(暂不支持)
 *
 * @param pid 进程ID
 * @param tp  一个指向 timespec 结构体的指针
 *
 * @return 总是返回 -1
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/sched_rr_get_interval.html
 */
int sched_rr_get_interval(pid_t pid, struct timespec *interval)
{
	(void)pid;
	(void)interval;

	errno = EINVAL;
	return -1;
}
