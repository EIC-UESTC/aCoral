/**
 * @file posix_mqueue.c
 * @author 李杰
 * @brief Implementation of POSIX message queue functions defined in mqueue.h.
 * @version 1.0
 * @date 2025-06-16
 *
 * @copyright Copyright (c) 2025 EIC-UESTC
 *
 * @par 描述
 * 此文件包含了 POSIX 消息队列（Message Queues）接口的具体实现，提供符合 POSIX 标准的消息队列服务。
 * 它管理消息队列的创建、打开、关闭、发送、接收、属性查询与设置。
 *
 * @par 修订历史
 * <table>
 * <tr><th>版本 <th>作者 <th>日期 <th>修改内容
 * <tr><td>v1.0 <td>李杰 <td>2025-06-16 <td>首次创建，实现 POSIX 消息队列核心功能。
 * </table>
 */

/* C 标准库头文件  */
#include <string.h>

/* posix 兼容层头文件 */
#include "acoral_posix.h"


/**
 * @brief POSIX消息队列的全局对象结构体。
 * @details 对于一个给定的 POSIX 消息队列名称（例如 "/my_queue"），在整个系统中只应该存在一个
 * 		       这样的 posix_mq_object_t实例。不同的进程或线程通过 mq_open打开这个同名队列时，
 * 		       它们最终会共享这同一个核心对象，并通过各自的描述符（posix_mq_desc_t）来引用它。
 * 		       使用自旋锁 (state_lock) 保护内部状态（引用计数和 unlink 标志）。
 */
typedef struct posix_mq_object {
	char				*name;			/* 队列的posix名称(动态分配) */
	acoral_mq_t			*native_mq;		/* 指向kernel消息队列的指针 */
	struct mq_attr		attributes;		/* 存储创建时确定的队列属性(attributes.mq_flags存储O_NONBLOCK状态) */
	int					ref_count_val;	/* 引用计数值 (普通整型) */
	acoral_bool			unlinked;		/* 是否已被mq_unlink调用 */
	acoral_spinlock_t	state_lock; 	/* 保护 ref_count_val 和 unlinked_flag 的自旋锁 */
	acoral_list_t		list_node;		/* 用于将此对象链入全局POSIX消息队列列表 */
} posix_mq_object_t;


/**
 * @brief POSIX消息队列描述符结构体。
 * @details 每个 mqd_t实际上是一个 posix_mq_desc_t类型的指针。
 *          它包含了指向其所关联的共享全局消息队列对象的指针，以及该描述符特有的访问模式。
 */
typedef struct posix_mq_desc {
    posix_mq_object_t   *mq_object;        	/* 指向共享的全局队列对象 */
    int                 access_mode_flags;  /* 此描述符的访问模式 (O_RDONLY, O_WRONLY, O_RDWR) */
    acoral_list_t		desc_list_node;		/* 用于将此描述符链入全局活跃描述符列表 */
} posix_mq_desc_t;

/**
 * @brief POSIX 消息的内部头部结构。
 * @details 用于在消息缓冲区中存储消息的元数据，如实际数据长度和优先级。消息数据紧随此头部结构存储。
 */
typedef struct {
	acoral_u32 actual_len;	/* 实际消息数据长度 (不包含头部本身) */
	acoral_u16 priority;	/* 消息优先级 */
	acoral_char data[];		/* 柔性数组成员：用户消息数据紧随其后 */
} posix_mq_msg_header_t;

/* --- 全局变量定义 --- */
/**
 * @brief 全局活跃消息队列描述符列表头。
 * @details 维护所有当前被 mq_open打开的 posix_mq_desc_t结构体。
 */
static acoral_list_t       	g_posix_mq_active_desc_list_head;

/**
 * @brief 保护 g_posix_mq_active_desc_list_head的自旋锁。
 */
static acoral_spinlock_t   	g_posix_mq_active_desc_list_lock;

/**
 * @brief 全局 POSIX 消息队列对象列表头。
 * @details 维护所有通过 mq_open(带 O_CREAT) 创建的 posix_mq_object_t结构体，即系统中所有命名的消息队列。
 */
static acoral_list_t 		g_posix_mq_list_head;

/**
 * @brief 保护 g_posix_mq_list_head的自旋锁。
 */
static acoral_spinlock_t 	g_posix_mq_list_lock;

/**
 * @brief POSIX 消息队列子系统初始化标志。
 * @details 确保全局资源只被初始化一次。
 */
static acoral_bool 			g_posix_mq_initialized = FALSE;

/* 这个计数器及其相关函数只会在 UNIT_TESTING 被定义时编译 (即在测试构建时) */
#ifdef UNIT_TESTING
static int destroy_call_count = 0; /* 内部变量，仅用于测试 */
#endif

/**
 * @brief 确保POSIX消息队列的全局资源只被初始化一次。
 */
static void posix_mq_initialize_list_once(void)
{
	/* 快速检查 (无锁)，如果已初始化则直接返回，减少锁的争用 */
	if (TRUE == g_posix_mq_initialized)
	{
		return;
	}

	/* 进入一个不允许调度的临界区，禁止中断 */
	acoral_enter_critical();

	/* 双重检查锁定 (Double-Checked Locking) 模式，在临界区内再次检查 */
	if (FALSE == g_posix_mq_initialized)
	{
		/* 初始化全局队列列表头和锁 */
		acoral_list_init(&g_posix_mq_list_head);
		acoral_spin_init(&g_posix_mq_list_lock);

		/* 初始化全局活跃描述符列表头和锁 */
		acoral_list_init(&g_posix_mq_active_desc_list_head);
		acoral_spin_init(&g_posix_mq_active_desc_list_lock);

		g_posix_mq_initialized = TRUE;
	}

	/* 退出临界区，恢复中断 */
	acoral_exit_critical();
}

/**
 * @brief 校验POSIX消息队列名称的有效性。
 *
 * @param name 要校验的消息队列名称字符串。
 * @param name_len_out (输出参数) 如果名称有效，则通过此指针返回名称的实际长度 (不包括'\0')。
 *                     如果名称无效，此参数的内容未定义。
 * @return acoral_bool 如果名称有效，返回 TRUE；否则返回 FALSE，并设置errno。
 */
static acoral_bool posix_mq_validate_name(const char *name, size_t *name_len_out)
{
	size_t len;

	/* 1.检查名称指针是否为空 */
	if ((NULL == name) || ('\0' == name[0]))
	{
		errno = EINVAL;
		return FALSE;
	}

	/* 2.检查是否以单个斜杠 '/' 开头 */
	if ('/' != name[0])
	{
		errno = EINVAL;
		return FALSE;
	}

	/* 3.计算名称长度(不包括末尾的 '\0') */
	len = strlen(name);

	/* 4.检查第一个斜杠后必须有其他字符 (名称不能只是 "/") */
	if (len == 1)
	{
		errno = EINVAL;
		return FALSE;
	}

	/* 5.检查名称长度是否超过最大限制 */
	if (len > NAME_MAX)
	{
		errno = ENAMETOOLONG;
		return FALSE;
	}

	/* 6.检查名称中除前导斜杠外，是否包含其他斜杠 '/' (实现定义的约束) */
	/* strchr(name_str + 1, '/') 会从 name_str 的第二个字符开始查找 '/' */
	if (NULL != strchr(name + 1, '/'))
	{
		errno = EINVAL;
		return FALSE;
	}

	/* 7.如果所有检查都通过，则名称有效 */
	if (NULL == name_len_out)
	{
		/* 通过输出参数返回名称长度 */
		*name_len_out = len;
	}
	return TRUE;
}

/**
 * @brief 在全局POSIX消息队列列表中根据名称查找消息队列对象。
 *
 * @param name 要查找的消息队列的名称字符串。
 * @return posix_mq_object_t* 如果找到匹配的对象，则返回指向该对象的指针；
 *         如果未找到，则返回 NULL。
 */
static posix_mq_object_t *posix_mq_find_in_list(const char *name)
{
	acoral_list_t *current_node;
	posix_mq_object_t *mq_object_iter;

	if (NULL == name)
	{
		return NULL;
	}

	/* 遍历全局消息队列列表 */
	acoral_list_for_each(current_node, &g_posix_mq_list_head)
	{
		/* 从链表节点指针 current_node 获取包含它的 posix_mq_object_t 结构体的指针。 */
		mq_object_iter = list_entry(current_node, posix_mq_object_t, list_node);

		/* 确保对象的名称字段不是NULL，然后比较名称 */
		if ((NULL != mq_object_iter->name) && (0 == strcmp(mq_object_iter->name, name)))
		{
			/* 找到了名称匹配的对象 */
			return mq_object_iter;
		}
	}

	/* 如果遍历完整个列表都没有找到匹配的名称，则返回 NULL */
	return NULL;
}

/**
 *  @brief 销毁一个POSIX消息队列对象及其底层资源。
 *
 *  @param obj 指向要销毁的 posix_mq_object_t 对象的指针。
 *             如果 obj 为 NULL，函数将不执行任何操作。
 *
 */
static void posix_mq_object_destroy(posix_mq_object_t *obj)
{
	if (NULL == obj)
	{
		return;
	}

#ifdef UNIT_TESTING
	/* 每次调用销毁函数时递增计数器(仅用于测试) */
	destroy_call_count++;
#endif

	/* 销毁kernel层消息队列 */
	if (NULL != obj->native_mq)
	{
		acoral_mq_del(obj->native_mq);
		obj->native_mq = NULL; /* 避免悬空指针 */
	}

	/* 释放动态分配的队列名称字符串 */
	if (NULL != obj->name)
	{
		acoral_free(obj->name);
		obj->name = NULL; /* 避免悬空指针 */
	}

	/* 释放 posix_mq_object_t 结构本身占用的内存。 */
	acoral_free(obj);
}

/**
 * @brief 验证描述符的有效性
 *
 * @param mqdes_to_find 要查找的 mqd_t消息队列描述符。
 * @return posix_mq_desc_t* 如果找到有效的描述符，则返回指向该描述符结构体的指针；否则返回 NULL。
 */
static posix_mq_desc_t *posix_mq_find_active_descriptor(mqd_t mqdes_to_find)
{
	posix_mq_desc_t *entry;

	acoral_spin_lock(&g_posix_mq_active_desc_list_lock);

	acoral_list_for_each_entry(entry, posix_mq_desc_t, &g_posix_mq_active_desc_list_head, desc_list_node)
	{
		if ((mqd_t)entry == mqdes_to_find)
		{
			acoral_spin_unlock(&g_posix_mq_active_desc_list_lock);
			return entry;
		}
	}

	acoral_spin_unlock(&g_posix_mq_active_desc_list_lock);
	return NULL;
}

/* 供测试文件调用的函数，用于获取和重置计数器 */
#ifdef UNIT_TESTING
/* 获取销毁计数 */
int get_destroy_call_count()
{
    return destroy_call_count;
}

/* 重置销毁计数 (每次测试开始时调用) */
void reset_destroy_call_count()
{
    destroy_call_count = 0;
}
#endif /* UNIT_TESTING */


/**
 * @brief 将纳秒数转换为 系统时钟节拍数 (ticks)。
 *
 * @param ns 纳秒数。
 * @return 对应的 系统时钟节拍数。如果 ns小于或等于0，返回0。
 */
static acoral_32 acoral_time_ns_to_ticks(long long ns)
{
	acoral_32 ticks;

	if (ns <= 0)
	{
		return 0;
	}

	/* 截断取整，如果结果为0但ns_val>0，则至少返回1个tick，避免精度丢失导致无限等待 */
	ticks = (acoral_32)(ns / (long long)NANOSECONDS_PER_TICK);
	if ((0 == ticks) && (ns > 0))
	{
		ticks = 1;
	}
	return ticks;
}

/**
 * @brief 计算消息队列操作所需的相对超时ticks。
 *
 * 该函数根据队列的非阻塞状态和给定的绝对超时时间，计算出传递给acoral RTOS 内核层消息发送函数所需的超时ticks值。
 *
 * @param queue_obj_flags 消息队列的标志位，用于判断是否为 O_NONBLOCK。
 * @param abstime 指向 timespec 结构的指针，表示绝对超时时间。如果为 NULL，表示无限等待。
 * @param calculated_timeout_ticks 输出参数，用于存储计算出的超时ticks。
 * @param current_ts_kernel 用于在计算超时时获取的当前时间。
 * @return 0表示成功计算超时；-1表示参数无效或 clock_gettime失败，并设置 errno。
 */
static int calculate_relative_timeout_ticks(
    long queue_obj_flags,
    const struct timespec *abstime,
    acoral_32 *calculated_timeout_ticks,
    struct timespec *current_ts_kernel
)
{
	long long ns_difference_kernel;

	if ((NULL == calculated_timeout_ticks) || (NULL == current_ts_kernel))
	{
		errno = EINVAL;
	    return -1;
	}

	if (queue_obj_flags & O_NONBLOCK)
	{
		*calculated_timeout_ticks = 0; /* 非阻塞模式，超时为0 */
	}
	else if (NULL == abstime)
	{
		*calculated_timeout_ticks = ACORAL_WAIT_FOREVER; /* 阻塞模式且无超时时间，无限等待 */
	}
	else
	{
		/* 验证 abstime 的纳秒部分是否合法 */
		if ((abstime->tv_nsec < 0) || (abstime->tv_nsec >= 1000000000L))
		{
			errno = EINVAL;
			return -1;
		}

		/* 获取当前单调时间 */
		if (0 != clock_gettime(CLOCK_MONOTONIC, current_ts_kernel))
		{
			/* clock_gettime 失败会设置 errno */
			return -1;
		}

		/* 比较绝对时间与当前时间 */
		if ((abstime->tv_sec < current_ts_kernel->tv_sec) ||
		   ((abstime->tv_sec == current_ts_kernel->tv_sec) &&
		    (abstime->tv_nsec <= current_ts_kernel->tv_nsec)))
		{
			/* 绝对时间已过或正好是当前时间，立即超时 */
			*calculated_timeout_ticks = 0;
		}
		else
		{
			ns_difference_kernel = (long long)(abstime->tv_sec - current_ts_kernel->tv_sec) * 1000000000LL;
			ns_difference_kernel += (abstime->tv_nsec - current_ts_kernel->tv_nsec);

			if (ns_difference_kernel <= 0)
			{
				/* 虽然绝对时间在未来，但由于浮点或时间戳粒度，差值可能小于或等于0 */
				*calculated_timeout_ticks = 0;
			}
			else
			{
				/* 将纳秒差值转换为RTOS的ticks */
				*calculated_timeout_ticks = acoral_time_ns_to_ticks(ns_difference_kernel);
				/* 确保非零的正纳秒差值至少对应1个tick的超时，避免因精度丢失导致0超时 */
				if ((0 == *calculated_timeout_ticks) && (ns_difference_kernel > 0))
				{
					*calculated_timeout_ticks = 1;
				}
			}
		}
	}
	 return 0;
}


/**
 * @brief 打开或创建一个消息队列
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_open.html
 *
 * @param name 消息队列的名称。
 * @param oflag 打开标志。
 * @param mode POSIX模式参数 (在此实现中被忽略)。
 * @param attr 用户提供的消息队列属性 (如果创建新队列)。
 *
 * @return mqd_t 成功时返回消息队列描述符；失败时返回 (mqd_t)-1 并设置 errno。
 *
 *
 */
mqd_t mq_open(const char *name, int oflags, mode_t mode, struct mq_attr * attr)
{
	size_t name_length = 0;
	posix_mq_object_t *mq_object = NULL; 	/* 指向找到的或新创建的全局对象 */
	posix_mq_object_t *newly_allocated_mq_object_temp = NULL; /* 临时持有乐观创建的对象 */
	posix_mq_desc_t *new_mq_desc = NULL;
	int current_oflag_access_mode;
	acoral_bool acoral_status = FALSE;	/* 标记我们是否已操作了mq_object的引用计数 */

	/* 1.初始化默认属性结构体 */
	struct mq_attr queue_creation_attributes = {
		.mq_flags = 0,		/* mq_flags在创建时通常由oflags_val的O_NONBLOCK决定，或在对象内部设置 */
		.mq_maxmsg = posixconfigMQ_MAX_MESSAGES,
		.mq_msgsize = posixconfigMQ_MAX_SIZE,
		.mq_curmsgs = 0 	/* 新队列当前消息数为0 */
	};

	/* 2.忽略未使用的mode_val参数，避免编译器警告 */
	(void)mode;

	/* 3.确保全局队列列表已初始化(只执行一次) */
	posix_mq_initialize_list_once();

	/* 4.校验队列名称 */
	if (FALSE == posix_mq_validate_name(name, &name_length))
	{
		/* posix_mq_validate_name内部已设置errno */
		return (mqd_t)-1;
	}

	/* 5.校验attr_ptr中的属性值 */
	if ((oflags & O_CREAT) && (NULL != attr))
	{
		if ((attr->mq_maxmsg <= 0) || (attr->mq_msgsize <=0))
		{
			errno = EINVAL;
			return (mqd_t)-1;
		}
		/* 使用用户提供的属性覆盖默认值（仅用于新创建时） */
		queue_creation_attributes.mq_maxmsg = attr->mq_maxmsg;
		queue_creation_attributes.mq_msgsize = attr->mq_msgsize;
	}

	/* 5.1校验访问模式 */
	current_oflag_access_mode = oflags & O_ACCMODE;
	if ((O_RDONLY != current_oflag_access_mode) &&
	   (O_WRONLY != current_oflag_access_mode) &&
	   (O_RDWR != current_oflag_access_mode))
	{
		errno = EINVAL;
		return (mqd_t)-1;
	}

	/* 为了处理TOCTOU竞争，新对象的“大部分”创建工作（内存分配）在全局锁之外进行 */
	if (oflags & O_CREAT)
	{
		/* 只有在可能创建时才预分配最外层结构 */
		newly_allocated_mq_object_temp = (posix_mq_object_t *)acoral_malloc(sizeof(posix_mq_object_t));
		if (NULL == newly_allocated_mq_object_temp)
		{
			errno = ENOMEM;
			return (mqd_t)-1;
		}
		/* 注意：此时不进行memset或更深的初始化，因为可能不会用到它 */
	}

	/* 6.获取保护全局队列列表的自旋锁 */
	acoral_spin_lock(&g_posix_mq_list_lock);

	/* 7.在全局列表中查找是否已存在同名队列 */
	mq_object = posix_mq_find_in_list(name);

	if (NULL != mq_object)
	{
		/* 队列已存在 */
		if (NULL != newly_allocated_mq_object_temp)
		{
			/* 之前乐观分配了，但现在发现已存在，释放掉 */
			acoral_free(newly_allocated_mq_object_temp);
			newly_allocated_mq_object_temp = NULL;
		}

		/* 7.1 处理O_CREAT和O_EXCL同时设置的情况 */
		if ((oflags & O_CREAT) && (oflags & O_EXCL))
		{
			errno = EEXIST;
			goto fail_and_unlock_list;	/* 跳转到释放全局锁并失败的标签 */
		}

		/* 7.2 获取对象自身的状态锁，检查unlinked_flag状态并增加引用计数 */
		acoral_spin_lock(&mq_object->state_lock);	/* 获取对象锁 */
		if (mq_object->unlinked)
		{
			/* 已被unlink的队列不能直接重新打开 */
			acoral_spin_unlock(&mq_object->state_lock);	/* 释放对象锁 */
			errno = ENOENT;
			goto fail_and_unlock_list; /* 跳转到释放全局锁并失败的标签 */
		}
		mq_object->ref_count_val++;					/* 增加引用计数 */
		acoral_status = TRUE;						/* 标记我们操作了引用计数 */
		acoral_spin_unlock(&mq_object->state_lock); /* 释放对象锁 */
	}
	else
	{
		/* 队列不存在 */
		/* 7.3 如果未指定O_CREAT，则返回错误 */
		if (!(oflags & O_CREAT))
		{
			if (NULL != newly_allocated_mq_object_temp)
			{
				acoral_free(newly_allocated_mq_object_temp);
			}
			errno = ENOENT;
			goto fail_and_unlock_list;
		}

		/* 7.4 创建新队列对象 */
		/* 使用之前预分配的 newly_allocated_mq_object_temp */
		if (NULL == newly_allocated_mq_object_temp)
		{
			errno = ENOMEM;
			goto fail_and_unlock_list;
		}
		mq_object = newly_allocated_mq_object_temp;
		newly_allocated_mq_object_temp = NULL;		/* 指针所有权转移给mq_object */

		/* 在仍然持有全局列表锁的情况下，完成新对象的详细初始化 */
		memset(mq_object, 0, sizeof(posix_mq_object_t));
		acoral_spin_init(&mq_object->state_lock);     /* 初始化对象的状态锁 */
		acoral_list_init(&mq_object->list_node);      /* 初始化链表节点 */

		mq_object->name = (char *)acoral_malloc(name_length + 1);
		if (NULL == mq_object->name)
		{
			acoral_free(mq_object);
			mq_object = NULL;
		    errno = ENOMEM;
		    goto fail_and_unlock_list;
		}
		strcpy(mq_object->name, name);

		mq_object->attributes.mq_maxmsg  = queue_creation_attributes.mq_maxmsg;
		mq_object->attributes.mq_msgsize = queue_creation_attributes.mq_msgsize;
		mq_object->attributes.mq_curmsgs = 0;
		if (oflags & O_NONBLOCK)
		{
			mq_object->attributes.mq_flags |= O_NONBLOCK;
		}
		else
		{
			mq_object->attributes.mq_flags &= ~O_NONBLOCK;
		}

		mq_object->native_mq = acoral_mq_create (
		    (acoral_u16)mq_object->attributes.mq_maxmsg,
			(acoral_size)mq_object->attributes.mq_msgsize);
		if (NULL == mq_object->native_mq)
		{
		    acoral_free(mq_object->name);
			acoral_free(mq_object);
			mq_object = NULL;
			errno = ENOSPC;
			goto fail_and_unlock_list;
		}

		acoral_spin_lock(&mq_object->state_lock);
		mq_object->ref_count_val = 1;
		acoral_status = TRUE;
		mq_object->unlinked = FALSE;
		acoral_spin_unlock(&mq_object->state_lock);

		/* 加入全局列表 */
		acoral_list_add_tail(&mq_object->list_node, &g_posix_mq_list_head);
	}

	/* 8.创建并初始化此打开实例的消息队列描述符 (posix_mq_desc_t) */
	new_mq_desc = (posix_mq_desc_t *)acoral_malloc(sizeof(posix_mq_desc_t));
	if (NULL == new_mq_desc)
	{
		errno = ENOMEM;
		if(acoral_status) 	/* 确保我们操作了引用计数（即成功找到了或创建了mq_object） */
		{
			acoral_spin_lock(&mq_object->state_lock);
			mq_object->ref_count_val--;
			/* 如果是新创建的对象，且引用计数回滚到 0，则需要从列表中移除并销毁 */
			/* 这里不再检查 unlinked_flag，因为它从未成功“链接”或“unlink” */
			if ((0 == mq_object->ref_count_val) && (oflags & O_CREAT) && (NULL != mq_object->native_mq))
			{
				/* 检查 mq_object->native_mq 不为 NULL，确保是完整创建的对象 */
				/* 1. 从全局列表移除 (因为在成功创建时已加入) */
				/* 2. 准备好销毁 (设置一个标志，或者直接在这里调用销毁，如果逻辑允许) */
				/* 为了简化，我们直接在这里做清理 */
				acoral_list_del(&mq_object->list_node); 	/* 从全局列表移除 */
				posix_mq_object_destroy(mq_object); 		/* 销毁对象及其内部资源 */
			}
			acoral_spin_unlock(&mq_object->state_lock);
		}
		goto fail_and_unlock_list; /* 跳转到释放全局锁并失败的标签 */
	}

	/* 初始化新的描述符 */
	new_mq_desc->mq_object = mq_object;
	new_mq_desc->access_mode_flags = current_oflag_access_mode;
	acoral_list_init(&new_mq_desc->desc_list_node); /* 初始化描述符的链表节点 */

	/* 将描述符添加到全局活跃描述符列表 */
	/* 这里我们先释放 g_posix_mq_list_lock，再获取 g_posix_mq_active_desc_list_lock */
	acoral_spin_unlock(&g_posix_mq_list_lock);	/* 释放全局队列列表锁 */

	acoral_spin_lock(&g_posix_mq_active_desc_list_lock);
	acoral_list_add_tail(&new_mq_desc->desc_list_node, &g_posix_mq_active_desc_list_head);
	acoral_spin_unlock(&g_posix_mq_active_desc_list_lock);

	/* 9.成功完成，返回描述符，清理乐观创建但未使用的对象 */
	if (NULL != newly_allocated_mq_object_temp)
	{
		acoral_free(newly_allocated_mq_object_temp);
	}
	return (mqd_t)new_mq_desc;	/* 直接返回描述符 */

fail_and_unlock_list: /* 统一的错误出口，仅释放全局队列列表锁 */
	acoral_spin_unlock(&g_posix_mq_list_lock); /* 释放全局列表锁 */

	/* 清理在乐观创建路径中分配但最终未使用的newly_allocated_mq_object_temp */
	if (NULL != newly_allocated_mq_object_temp)
	{
		acoral_free(newly_allocated_mq_object_temp);
	}
	return (mqd_t)-1;
}

/**
 * @brief 关闭一个POSIX消息队列描述符。
 * @detail
 * 此函数移除传入的消息队列描述符(mqdes)与其底层消息队列之间的关联。
 * 它会减少底层消息队列对象的引用计数。如果引用计数变为0并且该队列已被mq_unlink()标记，则此函数将触发底层消息队列及其资源的销毁。
 * 如果此描述符曾用于注册异步通知，该通知注册也将被移除。
 *
 * @param mqdes 要关闭的消息队列描述符。
 *
 * @return int  成功时返回0;失败时返回-1并设置errno。
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_close.html
 *
 */
int mq_close(mqd_t mqdes)
{
	posix_mq_desc_t *desc_to_close; 		/* 指向要关闭的描述符对象 */
	posix_mq_object_t *shared_mq_object;    /* 指向描述符关联的共享全局队列对象 */
	acoral_bool object_should_be_destroyed_after_all_locks = FALSE;  /* 标记共享对象是否需要最终销毁 */
	posix_mq_desc_t     *entry = NULL;
	posix_mq_desc_t     *next_entry = NULL;
	acoral_bool         found_and_removed = FALSE;

	/* 1.校验传入的描述符 mqdes_val 的有效性 */
	if ((NULL == mqdes) || (mqdes == (mqd_t)-1))
	{
		errno = EBADF;
		return -1;
	}

	/* 从全局活跃描述符列表中查找并移除描述符 */
	/* 必须首先获取活跃描述符列表的锁，以保护对列表的访问 */
	acoral_spin_lock(&g_posix_mq_active_desc_list_lock);

	/* 将 mqdes_val 转换为 posix_mq_desc_t* 进行查找。 */
	/* 这里我们遍历链表，找到匹配的描述符并移除。 */
	acoral_list_for_each_entry_safe(entry,next_entry, posix_mq_desc_t, &g_posix_mq_active_desc_list_head,desc_list_node)
	{
		if ((mqd_t)entry == mqdes)
		{
			/* 找到匹配的描述符，从列表中移除 */
			acoral_list_del(&entry->desc_list_node);
			desc_to_close = entry; /* 赋值给 desc_to_close 变量以便后续处理 */
			found_and_removed = TRUE;
			break; /* 找到并移除后即可退出循环 */
		}
	}
	acoral_spin_unlock(&g_posix_mq_active_desc_list_lock); /* 释放活跃描述符列表的锁 */

	/* 如果没有在活跃列表中找到该描述符，说明它已经无效或已关闭 */
	if (FALSE == found_and_removed)
	{
		errno = EBADF;
		return -1;
	}

	/* 此时 desc_to_close 已经是从活跃列表中移除的有效描述符指针 */
	shared_mq_object = desc_to_close->mq_object;
	if (NULL == shared_mq_object)
	{
		errno = EBADF;
		return -1;
	}

	/* 2.处理与此描述符相关的通知请求 (若实现了 mq_notify) */

	/* 3.原子地减少共享消息队列对象的引用计数，并决定是否销毁 */
	acoral_spin_lock(&shared_mq_object->state_lock);
	shared_mq_object->ref_count_val--; 	/* 减少引用计数 */

	/* 防止引用计数变为负数，这通常表示逻辑错误 */
	if (shared_mq_object->ref_count_val < 0)
	{
		shared_mq_object->ref_count_val = 0; /* 强制归零 */
	}

	/* 检查是否满足销毁条件：引用计数为0 且 已被unlink */
	if ((0 == shared_mq_object->ref_count_val) && shared_mq_object->unlinked)
	{
		object_should_be_destroyed_after_all_locks = TRUE;
	}
	acoral_spin_unlock(&shared_mq_object->state_lock);

	/* 4.如果确定需要销毁共享对象，则从全局列表中将其移除 */
	if (object_should_be_destroyed_after_all_locks)
	{
		acoral_spin_lock(&g_posix_mq_list_lock); /* 获取全局列表锁 */
		acoral_list_del(&shared_mq_object->list_node); /* 从全局队列列表中移除 */
		acoral_spin_unlock(&g_posix_mq_list_lock); /* 释放全局列表锁 */
	}

	/* 5.释放消息队列描述符本身的内存 */
	acoral_free(desc_to_close);

	/* 6.如果标记了需要销毁共享对象，则现在执行实际的销毁操作 */
	if (object_should_be_destroyed_after_all_locks)
	{
		posix_mq_object_destroy(shared_mq_object);
	}

	return 0;
}

/**
 * @brief 从系统中移除一个POSIX消息队列的名称
 * @details
 *  此函数将指定名称的消息队列标记为“已解除链接(unlinked)”。
 *  这会阻止后续通过此名称打开该队列（除非使用O_CREAT创建新队列）。
 *  如果调用此函数时，仍有打开的描述符指向该队列，则队列的实际资源销毁
 *  将被推迟到最后一个描述符被关闭之后。函数通常立即返回。
 *
 * @param name 要解除链接的消息队列的名称
 *
 * @return int 成功时返回0；失败时返回-1并设置errno
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_unlink.html
 *
 *
 */
int mq_unlink(const char *name)
{
	posix_mq_object_t *mq_object_to_unlink; /* 指向找到的要unlink的全局队列对象 */
	acoral_bool object_should_be_destroyed = FALSE; /* 标记共享对象是否需要最终销毁 */
	size_t name_len_dummy = 0;

	/* 1.确保全局POSIX MQ子系统已初始化 */
	posix_mq_initialize_list_once();

	/* 2.校验传入的 name参数的有效性 */
	if (FALSE == posix_mq_validate_name(name, &name_len_dummy))
	{
		// errno 已由 posix_mq_validate_name 设置
	    return -1;
	}

	/* 3.获取全局列表锁，以查找并可能操作消息队列对象 */
	acoral_spin_lock(&g_posix_mq_list_lock);

	/* 4.在全局列表中查找具有指定名称的消息队列对象 */
	mq_object_to_unlink = posix_mq_find_in_list(name);

	if (NULL == mq_object_to_unlink)
	{
		/* 对象未找到 */
		acoral_spin_unlock(&g_posix_mq_list_lock); /* 释放全局列表锁 */
		errno = ENOENT;
		return -1;
	}

	/* 对象已找到。现在需要获取对象自身的锁来修改 unlinked 状态和检查 ref_count。 */
	acoral_spin_lock(&mq_object_to_unlink->state_lock); /* 获取对象的状态锁 */

	/* 5.将对象标记为 unlinked */
	mq_object_to_unlink->unlinked = TRUE;	/* 标记为 unlinked */

	if (0 == mq_object_to_unlink->ref_count_val)
	{
		/* 如果引用计数为 0，则立即从全局列表中移除并标记为销毁 */
		acoral_list_del(&mq_object_to_unlink->list_node);
		object_should_be_destroyed = TRUE;	/* 标记需要立即销毁 */
	}
	/* 如果 ref_count_val > 0，则仅标记 unlinked，不从列表中移除，等待 mq_close 销毁。 */
	acoral_spin_unlock(&mq_object_to_unlink->state_lock); 	/* 释放对象锁 */
	acoral_spin_unlock(&g_posix_mq_list_lock);				/* 在所有锁之外执行实际销毁 */

	/* 在释放所有锁之后执行实际的销毁操作，防止死锁 */
	if (object_should_be_destroyed)
	{
		posix_mq_object_destroy(mq_object_to_unlink);
	}
	return 0;
}

/**
 * @brief 获取POSIX消息队列的属性
 * @details
 *  此函数用于查询与指定消息队列描述符关联的消息队列的当前状态和属性。
 *  结果将填充到用户提供的 mq_attr 结构中。
 * @note 需要检查 mqdes 是否仍然指向一个当前活跃且有效的描述符结构体。
 *        实现方式：通过维护一个全局的活跃描述符列表来实现。当一个描述符被创建时，它被添加到这个列表；
 *        当它被关闭时，它从列表中移除。mq_getattr 在使用描述符之前，会先在这个列表中查找它。
 *
 * @param mqdes 一个有效的、已打开的消息队列描述符
 * @param mqstat 一个指向 struct mq_attr 结构的指针，用于接收队列的属性
 *
 * @return int 成功时返回0；失败时返回-1并设置errno
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_getattr.html
 *
 */
int mq_getattr(mqd_t mqdes, struct mq_attr *mqstat)
{
	posix_mq_desc_t *desc = NULL;
	posix_mq_object_t *shared_mq_object = NULL;

	/* 1.校验传入的参数的有效性 */
	if ((NULL == mqdes) || ((mqd_t)-1 == mqdes))
	{
		errno = EBADF;
		return -1;
	}

	/* 严格的描述符有效性检查,在全局活跃描述符列表中查找 mqdes_val */
	desc = posix_mq_find_active_descriptor(mqdes);
	if (NULL == desc)
	{
		// 如果在活跃列表中找不到，则 mqdes 无效或已关闭
		errno = EBADF;
		return -1;
	}

	/* 此时，desc 已经确保是一个指向当前活跃且有效的 posix_mq_desc_t 结构体的指针 */
	/* 后续对 desc 及其成员的访问是安全的 */
	if (NULL == mqstat)
	{
		errno = EINVAL;
		return -1;
	}

	shared_mq_object = desc->mq_object;
	/* 进一步校验内部结构是否有效 */
	if ((NULL == shared_mq_object) ||
	    (NULL == shared_mq_object->native_mq) ||
	    (NULL == shared_mq_object->native_mq->msg_queue_ipc))
	{
	    errno = EBADF;
	    return -1;
	}

	/* 2.从共享对象中获取静态和半静态属性 */
	acoral_spin_lock(&shared_mq_object->state_lock);

	mqstat->mq_flags   = shared_mq_object->attributes.mq_flags;   /* 获取队列的O_NONBLOCK状态 */
	mqstat->mq_maxmsg  = shared_mq_object->attributes.mq_maxmsg;  /* 获取创建时设置的最大消息数 */
	mqstat->mq_msgsize = shared_mq_object->attributes.mq_msgsize; /* 获取创建时设置的最大消息大小 */

	acoral_spin_unlock(&shared_mq_object->state_lock);

	/* 3.获取当前队列中的消息数量 (mq_curmsgs) */
	acoral_spin_lock(&shared_mq_object->native_mq->msg_queue_ipc->lock); /* 获取原生IPC锁 */
	mqstat->mq_curmsgs = shared_mq_object->native_mq->msg_queue_ipc->count; /* 获取当前消息数 */
	acoral_spin_unlock(&shared_mq_object->native_mq->msg_queue_ipc->lock); /* 释放原生IPC锁 */

	return 0;
}


/**
 * @brief 设置POSIX消息队列的属性。
 * @details
 *  此函数用于修改与指定消息队列描述符关联的消息队列的属性。
 *  根据POSIX标准，唯一可修改的属性是mq_flags中的O_NONBLOCK标志。
 *  如果omqstat非NULL，则在修改前会将旧的属性和当前状态存入omqstat。
 *
 * @param mqdes 一个有效的、已打开的消息队列描述符。
 * @param mqstat 指向包含新mq_flags值的mq_attr结构的指针。
 *               此结构中的mq_maxmsg, mq_msgsize, mq_curmsgs将被忽略。
 * @param omqstat (可选) 指向mq_attr结构的指针，用于接收修改前的队列属性和状态。
 *
 * @return int 成功时返回0；失败时返回-1并设置errno。
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_setattr.html
 *
 */
int mq_setattr(
    mqd_t mqdes,
	const struct mq_attr *mqstat,
	struct mq_attr *omqstat
)
{
	posix_mq_desc_t *desc = NULL;
	posix_mq_object_t *shared_mq_object = NULL;

	/* 1.验证 mqdes_val 的有效性 */
	desc = posix_mq_find_active_descriptor(mqdes);
	if (NULL == mqdes)
	{
		errno = EBADF;
		return -1;
	}

	/* 2. 校验 mqstat_ptr (代表新属性) 必须提供(mq_setattr 需要从 mqstat_ptr 中读取要设置的新标志) */
	if (NULL == mqstat)
	{
		errno = EINVAL;
		return -1;
	}

	/* 现在 desc 已经过验证，可以安全地访问其成员 */
	shared_mq_object = desc->mq_object;
	/* 进一步校验内部结构是否有效,这是防御性编程，确保 mq_object 和底层队列存在 */
	if ((NULL == shared_mq_object) ||
		(NULL == shared_mq_object->native_mq))
	{
	    errno = EBADF;
	    return -1;
	}

	/* 3.校验 new_mqstat->mq_flags 的内容 */
	/* POSIX 规定 mq_setattr 只能修改 O_NONBLOCK 标志(mq_flags 必须是 0 或 O_NONBLOCK。) */
	if ((0 != mqstat->mq_flags) && (O_NONBLOCK != mqstat->mq_flags))
	{
		/* 用户试图设置除O_NONBLOCK之外的标志，或者一个无效的组合 */
		errno = EINVAL;
		return -1;
	}

	/* 4.如果 omqstat_ptr (用于存储旧属性) 非 NULL，获取并存储当前的队列属性和状态 */
	acoral_spin_lock(&shared_mq_object->state_lock); /* 获取队列对象的状态锁 */
	if (NULL != omqstat)
	{
		/* 直接从共享队列对象中拷贝旧属性，而不是调用 mq_getattr。 */
		/* 避免可能的死锁或重入问题，并且效率更高。 */
		omqstat->mq_flags   = shared_mq_object->attributes.mq_flags;
		omqstat->mq_maxmsg  = shared_mq_object->attributes.mq_maxmsg;
		omqstat->mq_msgsize = shared_mq_object->attributes.mq_msgsize;
		omqstat->mq_curmsgs = shared_mq_object->attributes.mq_curmsgs;
	}

	/* 5.设置新的队列属性 (只修改 shared_mq_object->attributes.mq_flags 中的 O_NONBLOCK 位) */
	/* 清除当前对象属性中的 O_NONBLOCK 标志位 */
	shared_mq_object->attributes.mq_flags &= ~O_NONBLOCK;

	/* 如果用户传入的新属性中设置了 O_NONBLOCK，则在对象属性中也设置它 */
	if (mqstat->mq_flags & O_NONBLOCK)
	{
		shared_mq_object->attributes.mq_flags |= O_NONBLOCK;
	}

	acoral_spin_unlock(&shared_mq_object->state_lock); /* 释放队列对象的状态锁 */

	return 0;
}


/**
 * @brief 向POSIX消息队列发送一条消息。
 *
 * @param mqdes 一个有效的、为写入而打开的消息队列描述符。
 * @param msg_ptr 指向要发送消息内容的指针。
 * @param msg_len 要发送消息的长度（字节）。必须不大于队列的mq_msgsize。
 * @param msg_prio 消息的优先级。必须小于MQ_PRIO_MAX。
 * @return int 成功时返回0；失败时返回-1并设置errno。
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_send.html
 */
int mq_send(
    mqd_t mqdes,
	const char *msg_ptr,
	size_t msg_len,
	unsigned int msg_prio
)
{
	return mq_timedsend( mqdes, msg_ptr, msg_len, msg_prio, NULL );
}

/**
 * @brief 在指定超时时间内发送消息到消息队列。
 * @detail 尝试将一条消息（带有可选的优先级）发送到一个消息队列，但如果队列已满导致无法立即发送，它会等待一段时间（由绝对超
 *         时时间 abstime 指定），直到队列有空间或超时发生。
 *
 * @param mqdes 消息队列描述符。
 * @param msg_ptr 指向要发送的消息的指针。
 * @param msg_len 要发送消息的长度（字节）。必须不大于队列的mq_msgsize。
 * @param msg_prio 消息优先级。
 * @param abstime 指向 timespec 结构的指针，表示消息发送的绝对超时时间。
 *                如果为 NULL，表示无限等待。
 *
 * @return int 成功时返回0；失败时返回-1并设置errno。
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_send.html
 *
 */
int mq_timedsend(
    mqd_t mqdes,
	const char *msg_ptr,
	size_t msg_len,
	unsigned int msg_prio,
	const struct timespec *abstime
)
{
	posix_mq_desc_t *desc = NULL;
	posix_mq_object_t *share_mq_object = NULL;
	acoral_kernel_error_t acoral_status;
	acoral_32 acoral_timeout_val_ticks_for_native_call;
	long queue_obj_flags;
	size_t user_max_msg_data_size;
	void *send_raw_buffer = NULL; 		/* 用于发送的包含头部和数据的整个缓冲区 */
	acoral_u32 total_packed_msg_len;	/* 整个打包后的消息（头部 + 数据）的总长度 */
	struct timespec current_ts_kernel; 	/* 用于传递给超时计算函数，避免重复调用clock_gettime */
	posix_mq_msg_header_t   *header = NULL;

	/* 1.验证 mqdes 的有效性 */
	desc = posix_mq_find_active_descriptor(mqdes);
	if (NULL == desc)
	{
		errno = EBADF;
		return -1;
	}
	if (NULL == msg_ptr)
	{
		errno = EINVAL;
		return -1;
	}

	/* 现在 desc 已经被验证是活跃的，可以安全访问其成员 */
	share_mq_object = desc->mq_object;
	/* 进一步校验内部结构是否有效，防御性编程 */
	if ((NULL == share_mq_object) || (NULL == share_mq_object->native_mq))
	{
		errno = EBADF;
		return -1;
	}

	/* 2.检查描述符的打开模式是否允许写入 */
	/* 确保描述符是以可写模式打开的 (O_WRONLY 或 O_RDWR) */
	if (!((desc->access_mode_flags & O_ACCMODE) == O_WRONLY ||
		  (desc->access_mode_flags & O_ACCMODE) == O_RDWR))
	{
		errno = EBADF; /* 描述符未以可写模式打开 */
		return -1;
	}

	/* 3.处理消息的优先级 */
	/* POSIX 允许优先级范围为 1 到 POSIX_REALTIME_PRIORITY_MAX。超出范围则返回 EINVAL。 */
#ifdef POSIX_REALTIME_PRIORITY_MAX
	if (msg_prio > POSIX_REALTIME_PRIORITY_MAX)
	{
		errno = EINVAL;
		return -1;
	}
#endif

	/* 4.获取队列属性 (特别是mq_msgsize和O_NONBLOCK状态) */
	/* 使用自旋锁保护对消息队列属性的访问，防止并发修改。 */
	acoral_spin_lock(&share_mq_object->state_lock);
	user_max_msg_data_size  = (size_t)share_mq_object->attributes.mq_msgsize;
	queue_obj_flags = share_mq_object->attributes.mq_flags;
	acoral_spin_unlock(&share_mq_object->state_lock);

	/* 检查用户消息数据长度是否超过队列允许的最大长度 */
	if (msg_len > user_max_msg_data_size)
	{
		errno = EMSGSIZE;
		return -1;
	}

	/* 计算包含头部在内的总消息长度 */
	total_packed_msg_len = (acoral_u32)(msg_len + sizeof(posix_mq_msg_header_t));

	/* 5.调用辅助函数计算所需的超时ticks */
	if (0 != calculate_relative_timeout_ticks(queue_obj_flags,abstime,
										 	 &acoral_timeout_val_ticks_for_native_call,
											 &current_ts_kernel))
	{
		/* calculate_relative_timeout_ticks失败时会设置 errno */
		return -1;
	}

	/* 6.封装消息头部和数据，并调用 kernel 层消息发送函数 */
	/* 分配一个缓冲区来存储包含头部和用户数据的完整消息 */
	send_raw_buffer = acoral_malloc(total_packed_msg_len);
	if (NULL == send_raw_buffer)
	{
		errno = ENOMEM; /* 内存不足 */
		return -1;
	}

	/* 填充消息头部 */
	header = (posix_mq_msg_header_t *)send_raw_buffer;
	header->actual_len = (acoral_size)msg_len; /* 存储实际的用户消息长度 */
	header->priority = msg_prio;               /* 存储优先级 */
	/* 复制用户消息数据到头部后的数据区 */
	memcpy(header->data, msg_ptr, msg_len);

	acoral_status = acoral_mq_send_wait(share_mq_object->native_mq,
										send_raw_buffer,		 	/* 传递包含头部和数据的整个缓冲区 */
										total_packed_msg_len, 		/* 传递整个缓冲区的总长度 */
										acoral_timeout_val_ticks_for_native_call);

	/* 释放发送缓冲区 */
	acoral_free(send_raw_buffer);

	/* 7. 处理kernel层消息发送函数的结果并设置errno */
	if (KR_OK == acoral_status)
	{
		errno = 0;
		return 0;
	}
	else
	{
		/* 根据acoral_status映射到POSIX errno */
		if (KR_IPC_ERR_MQ_FULL == acoral_status)
		{
			if (queue_obj_flags & O_NONBLOCK)
			{
				errno = EAGAIN; /* 非阻塞模式下队列满 */
			}
			else
			{
				/* 阻塞模式下，如果队列满但不是超时或中断，这通常是底层逻辑问题 */
				errno = EIO; /* 理论上不应该出现此错误码 */
			}
		}
		else if ((KR_IPC_ERR_TIMEOUT == acoral_status) || (KR_THREAD_ERR_TIMEOUT == acoral_status))
		{
			errno = ETIMEDOUT; /* 超时错误 */
		}
		else if (KR_IPC_ERR_INTR == acoral_status)
		{
			errno = EINTR; /* 系统调用被中断 */
		}
		else if (KR_IPC_ERR_MSG_SIZE == acoral_status)
		{
			/* 此错误码表示底层队列无法接受此大小的消息 */
			errno = EMSGSIZE;
		}
		else if (KR_IPC_ERR_NULL == acoral_status)
		{
			errno = EFAULT; /* 无效地址（例如空指针参数） */
		}
		else if (KR_IPC_ERR_TYPE == acoral_status)
		{
			errno = EBADF; /* 无效文件描述符或类型不匹配 */
		}
		else
		{
			errno = EIO; /* 通用I/O错误或其他未映射错误 */
		}
		return -1;
	}
}

/**
 * @brief 从POSIX消息队列接收一条消息
 *
 * @param mqdes 一个有效的、为读取而打开的消息队列描述符
 * @param msg_ptr 指向用户提供的缓冲区的指针，用于存储接收到的消息
 * @param msg_len 用户提供缓冲区的大小（字节）。必须不小于队列的mq_msgsize
 * @param msg_prio (可选) 如果非NULL，则用于存储接收到消息的优先级
 *                 如果底层不支持优先级，可能返回固定值或0
 *
 * @return ssize_t 成功时，返回接收到消息的字节长度。失败时，返回-1并设置errno。
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_receive.html
 */
int mq_receive(
    mqd_t mqdes,
	char *msg_ptr,
	size_t msg_len,
	unsigned int *msg_prio
)
{
	return mq_timedreceive( mqdes, msg_ptr, msg_len, msg_prio, NULL);
}

/**
 * @brief 从消息队列接收消息，支持超时等待。
 *
 * @param mqdes 消息队列描述符。
 * @param msg_ptr 指向接收消息的缓冲区的指针。此缓冲区将用于存放接收到的消息。
 * @param msg_len 接收缓冲区的大小。
 * @param msg_prio 指向一个 unsigned int 的指针，用于接收消息的优先级（如果非NULL）。
 * @param abstime 指向 timespec 结构的指针，表示消息接收的绝对超时时间。如果为 NULL，表示无限等待。
 *
 * @return 成功时返回接收到的消息的字节数。失败时返回 -1 并设置 errno。
 *
 * https://pubs.opengroup.org/onlinepubs/9699919799/functions/mq_receive.html
 */
int mq_timedreceive(
    mqd_t mqdes,
	char *msg_ptr,
	size_t msg_len,
	unsigned int *msg_prio,
	const struct timespec *abstime
)
{
	posix_mq_desc_t *desc = NULL;
	posix_mq_object_t *share_mq_object = NULL;
	acoral_kernel_error_t acoral_status;
	acoral_32 acoral_timeout_val_ticks_for_native_call;
	long queue_obj_flags;
	size_t queue_obj_max_msg_size_with_header;	/* 队列存储的最大消息大小，包含自定义头部 */
	struct timespec current_ts_kernel_for_timeout_calc;	/* 用于传递给超时计算辅助函数，避免重复调用 clock_gettime */
	acoral_size  actual_received_data_len = 0;	/* 用于接收从消息头部解析出的实际消息数据长度 */
	void *received_raw_buffer = NULL;	/* 用于接收从 acoral 内核层获取的原始消息（包含头部） */
	posix_mq_msg_header_t   *header = NULL;

	/* 1. 验证 mqdes_val 的有效性 */
	desc = posix_mq_find_active_descriptor(mqdes);
	if (NULL == desc)
	{
		errno = EBADF;
		return -1;
	}

	/* 2. 校验 msg_buf_ptr */
	if (NULL == msg_ptr)
	{
		errno = EINVAL;
		return -1;
	}

	/* 现在 desc 已经过验证，可以安全地访问其成员 */
	share_mq_object = desc->mq_object;
	/* 进一步校验内部结构是否有效，防御性编程 */
	if ((NULL == share_mq_object) || (NULL == share_mq_object->native_mq))
	{
		errno = EBADF;
		return -1;
	}

	/* 3. 检查描述符的打开模式是否允许读取 */
	/* 确保描述符是以可读模式打开的 (O_RDONLY 或 O_RDWR)。 */
	if (!((desc->access_mode_flags & O_ACCMODE) == O_RDONLY ||
		  (desc->access_mode_flags & O_ACCMODE) == O_RDWR))
	{
		errno = EBADF; /* 描述符未以可读模式打开  */
		return -1;
	}

	/* 4. 获取队列属性 (特别是 mq_msgsize 和 O_NONBLOCK 状态) */
	/* 使用自旋锁保护对消息队列属性的访问，防止并发修改。 */
	acoral_spin_lock(&share_mq_object->state_lock);
	size_t user_max_msg_data_size  = (size_t)share_mq_object->attributes.mq_msgsize;
	queue_obj_flags = share_mq_object->attributes.mq_flags;
	acoral_spin_unlock(&share_mq_object->state_lock);

	/* 计算底层队列每个消息槽的实际大小 (包含头部) */
	/* 这个值应该是创建 acoral_mq_t 时设置的 size 参数 */
	queue_obj_max_msg_size_with_header = user_max_msg_data_size + sizeof(posix_mq_msg_header_t);

	/* POSIX 规定如果 msg_buf_len_val 小于 mq_msgsize，应返回 EMSGSIZE */
	if (msg_len < user_max_msg_data_size)
	{
		errno = EMSGSIZE;
		return -1;
	}

	/* 5. 调用辅助函数计算所需的超时 ticks */
	if (0 != calculate_relative_timeout_ticks(queue_obj_flags, abstime,
										 	 &acoral_timeout_val_ticks_for_native_call,
											 &current_ts_kernel_for_timeout_calc))
	{
		// calculate_relative_timeout_ticks 失败时会设置 errno
		return -1;
	}

	/* 6. 调用 acoral 内核层消息接收函数 */
	/* 预先分配一个缓冲区，用于 acoral_mq_recv 存放接收到的消息。 */
	/* 这种模式下，acoral_mq_recv 会将消息复制到此缓冲区。 */
	received_raw_buffer  = acoral_malloc(queue_obj_max_msg_size_with_header); /* 预先分配接收缓冲区 */
	if (NULL == received_raw_buffer)
	{
		errno = ENOMEM;
		return -1;
	}

	acoral_status = acoral_mq_recv(share_mq_object->native_mq,
								   received_raw_buffer, /* 传递分配的缓冲区 */
								   (acoral_size)queue_obj_max_msg_size_with_header, /* 传递整个缓冲区的最大长度 */
								   acoral_timeout_val_ticks_for_native_call);

	/* 7. 处理 acoral 内核层消息接收函数的结果并设置errno */
	if (KR_OK == acoral_status)
	{
		header = (posix_mq_msg_header_t *)received_raw_buffer;
		actual_received_data_len = header->actual_len; /* 获取实际消息数据长度 */

		/* 进一步检查：用户提供的缓冲区是否足够容纳实际接收到的消息数据 */
		if (msg_len < actual_received_data_len)
		{
			acoral_free(received_raw_buffer);
			errno = EMSGSIZE;
			return -1;
		}

		memcpy(msg_ptr, header->data, actual_received_data_len);

		/* 如果用户提供了 msg_prio_ptr 指针，则将优先级写入 */
		/* 这里不需要额外的范围检查，因为该值在发送时已经被校验过。 */
		if (NULL != msg_prio)
		{
			*msg_prio = header->priority;
		}

		acoral_free(received_raw_buffer);
		errno = 0;
		return actual_received_data_len; /* 返回实际接收到的数据字节数 */
	}
	else
	{
		if (NULL != received_raw_buffer)
		{
			acoral_free(received_raw_buffer);
		}

		if ((KR_IPC_ERR_TIMEOUT == acoral_status) || (KR_THREAD_ERR_TIMEOUT == acoral_status))
		{
			errno = ETIMEDOUT;
		}
		else if (KR_IPC_ERR_INTR == acoral_status)
		{
			errno = EINTR;
		}
		else if (KR_IPC_ERR_MQ_EMPTY == acoral_status)
		{
			/* 队列为空 */
			if (queue_obj_flags & O_NONBLOCK)
			{
				errno = EAGAIN; /* 非阻塞模式下队列空，返回 EAGAIN */
			}
			else
			{
				errno = EIO; /* 理论上阻塞模式下不应该出现此错误码 */
			}
		}
		else if (KR_IPC_ERR_MSG_SIZE == acoral_status)
		{
			errno = EMSGSIZE;
		}
		else if (KR_IPC_ERR_NULL == acoral_status)
		{
			errno = EFAULT;
		}
		else if (KR_IPC_ERR_TYPE == acoral_status)
		{
			errno = EBADF;
		}
		else
		{
			errno = EIO;
		}
		return -1;
	}
}




