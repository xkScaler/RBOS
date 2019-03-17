/*
 * A LSM modules which implements role based access control.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/security.h>
#include <linux/xattr.h>
#include <linux/capability.h>
#include <linux/unistd.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/swap.h>

#include <linux/spinlock.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <linux/ext2_fs.h>
#include <linux/proc_fs.h>
#include <linux/kd.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/tty.h>
#include <net/icmp.h>
#include <net/ip.h>		/* for sysctl_local_port_range[] */
#include <net/tcp.h>		/* struct or_callable used in sock_rcv_skb */
#include <asm/uaccess.h>
#include <asm/ioctls.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>	/* for network interface checks */
#include <linux/netlink.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/quota.h>
#include <linux/un.h>		/* for Unix socket types */
#include <net/af_unix.h>	/* for Unix socket types */
#include <linux/parser.h>
#include <linux/nfs_mount.h>
#include <net/ipv6.h>
#include <linux/hugetlb.h>
#include <linux/personality.h>
#include <linux/sysctl.h>
#include <linux/audit.h>
#include <linux/string.h>
#include <linux/unistd.h>
#include <linux/lsm_hooks.h>

//定义将要hook的系统调用
#define SYSCALL_CONNECT		0x01
#define SYSCALL_SOCKET		0x02
#define SYSCALL_MKDIR		0x04
#define SYSCALL_RMDIR		0x08
#define SYSCALL_TASK_CREATE 0x10

//定义几个配置文件的路径,此处是硬编码的
#define ROLE_CONFIG		"/etc/rbos/role_config"
#define USER_CONFIG		"/etc/rbos/user_config"

//最多支持16种角色,128个用户的分配
#define SAMPLE_MAX_ROLE 	16
#define SAMPLE_MAX_USER		128

#define SAMPLE_MAX_BUF		1024


//角色结构
typedef struct __role__
{
	unsigned int roleid;
	unsigned int right;
}_ROLE_STRUCT;
_ROLE_STRUCT all_roles[SAMPLE_MAX_ROLE]={{0,0},};
unsigned int all_roles_cnt=0;

//用户结构
typedef struct __user__
{
	unsigned int userid;
	unsigned int right;
}_USER_STRUCT;
_USER_STRUCT all_users[SAMPLE_MAX_USER]={{0,SYSCALL_CONNECT | SYSCALL_SOCKET | SYSCALL_MKDIR |SYSCALL_RMDIR | SYSCALL_TASK_CREATE},};
unsigned int all_users_cnt=1;
typedef union {
	struct _socket_info{
		int domain;
		int type;
		int protocol;
	};
	struct _connect_info {
		struct socket *sock;
		struct sockaddr *address;
		int addrlen;
	}connect_info;
	
	struct _mkdir_info {
		struct inode *dir;
		struct dentry *dentry;
		int mask;
	}mkdir_info;
	
	struct _rmdir_info {
		struct inode *dir;
		struct dentry *dentry;
	}rmdir_info;
	struct _taskcreate_info{
		unsigned long clone_flags;
	};
}perm_info_t;

extern struct security_operations *security_ops;
unsigned int sample_hash_str(char *str,int len)
{
	unsigned int rst =1,__PBASE__=19950817,__MODBASE__=1e9+7;
	for(int i =0;i<len;i++)
	{
		rst = (rst * __PBASE__ +str[i] )%__MODBASE__;
	}
	return rst;
}
int check_connect_perm(perm_info_t *info)
{
	printk(KERN_WARNING "___Check connect permission___:: %s\n", __FUNCTION__);	
	return 0;
}

int check_link_perm(perm_info_t *info)
{
	printk(KERN_WARNING "___Check link permission___:: %s\n", __FUNCTION__);
	printk(KERN_WARNING "link file: %s\n", info->link_info.old_dentry->d_iname);
	printk(KERN_WARNING "______________________________________\n");
	return 0;
}

int check_unlink_perm(perm_info_t *info)
{
	printk(KERN_WARNING "___Check unlink permission___:: %s\n", __FUNCTION__);
	printk(KERN_WARNING "unlink file: %s\n", info->unlink_info.dentry->d_iname);
	printk(KERN_WARNING "______________________________________\n");
	return 0;
}

int check_symlink_perm(perm_info_t *info)
{
	printk(KERN_WARNING "___Check symlink permission___:: %s\n", __FUNCTION__);
	printk(KERN_WARNING "symlink file: %s\n", info->symlink_info.name);
	printk(KERN_WARNING "______________________________________\n");
	return 0;
}

int check_mkdir_perm(perm_info_t *info)
{
	printk(KERN_WARNING "___Check mkdir permission___:: %s\n", __FUNCTION__);
	printk(KERN_WARNING "mkdir: %s\n", info->mkdir_info.dentry->d_iname);
	printk(KERN_WARNING "______________________________________\n");
	return 0;
}

int check_rmdir_perm(perm_info_t *info)
{
	printk(KERN_WARNING "___Check rmdir permission___:: %s\n", __FUNCTION__);
	printk(KERN_WARNING "rmdir: %s\n", info->rmdir_info.dentry->d_iname);
	printk(KERN_WARNING "______________________________________\n");
	return 0;
}


static int check_perm(int syscall_type, perm_info_t *perm_info)
{
	int ret=0;
	printk(KERN_WARNING "____Check Permission___::%s\n", __FUNCTION__);
	
	switch (syscall_type) {
	case SYSCALL_CONNECT:
		ret = check_connect_perm(perm_info);
		break;
		
	case SYSCALL_LINK:
		ret = check_link_perm(perm_info);
		break;

	case SYSCALL_UNLINK:
		ret = check_unlink_perm(perm_info);
		break;

	case SYSCALL_SYMLINK:
		ret = check_symlink_perm(perm_info);
		break;

	case SYSCALL_MKDIR:
		ret = check_mkdir_perm(perm_info);
		break;

	case SYSCALL_RMDIR:
		ret = check_rmdir_perm(perm_info);
		break;
	}
	
	return ret;  
}

static int sample_socket_connect(struct socket *sock, struct sockaddr *address, int addrlen)
{
	perm_info_t perm_info;
	perm_info.connect_info.sock = sock;
	perm_info.connect_info.address = address;
	perm_info.connect_info.addrlen = addrlen;
	
	return check_perm(SYSCALL_CONNECT, &perm_info);
}


static int sample_inode_link(struct dentry *old_dentry, struct inode *dir, struct dentry *new_dentry)
{
	perm_info_t perm_info; 

	perm_info.link_info.old_dentry = old_dentry;
	perm_info.link_info.dir = dir;
	perm_info.link_info.new_dentry = new_dentry;
	
	return check_perm(SYSCALL_LINK, &perm_info);

}

static int sample_inode_unlink(struct inode *dir, struct dentry *dentry)
{
	perm_info_t perm_info; 

	perm_info.unlink_info.dir = dir;
	perm_info.unlink_info.dentry = dentry;
	
	return check_perm(SYSCALL_UNLINK, &perm_info);	
}

static int sample_inode_symlink(struct inode *dir, struct dentry *dentry, const char *name)
{
	perm_info_t perm_info; 

	perm_info.symlink_info.dir = dir;
	perm_info.symlink_info.dentry = dentry;
	perm_info.symlink_info.name = name;
	
	return check_perm(SYSCALL_SYMLINK, &perm_info);

}

static int sample_inode_mkdir(struct inode *dir, struct dentry *dentry, int mask)
{
	perm_info_t perm_info; 

	perm_info.mkdir_info.dir = dir;
	perm_info.mkdir_info.dentry = dentry;
	perm_info.mkdir_info.mask = mask;
	
	return check_perm(SYSCALL_MKDIR, &perm_info);
	
}

static int sample_inode_rmdir(struct inode *dir, struct dentry *dentry)
{
	perm_info_t perm_info; 

	perm_info.rmdir_info.dir = dir;
	perm_info.rmdir_info.dentry = dentry;
	
	return check_perm(SYSCALL_RMDIR, &perm_info);
}


static void get_role_config(void)
{
	char buf[SAMPLE_MAX_BUF]={0};
	struct file * f=NULL;
	const char * filename =ROLE_CONFIG;
	char * p;
	int i;
	char * line_start;
	printk(KERN_INFO "get role config from %s.\n",filename);
	mm_segment_t oldfs;
	oldfs =get_fs();
	set_fs(KERNEL_DS);
	f =file_open(filename,O_RDONLY,0);
	if( IS_ERR(F) || (f==NULL))
	{
		return -1;
	}
	p=buf;
	
	while(vfs_read(f,buf+i,1,&f->f_ops)==1)
	{
		if(i==SAMPLE_MAX_BUF)
			//读满缓存区
		{
			break;
		}
		if(buf[i]==':')
			//读到用户了
		{
			unsigned int hs=sample_hash_str(line_start,buf+i-line_start);
			all_roles[all_roles_cnt].roleid =hs;
		}
		//TODO
		if(buf[i]=='\n')
			//遇到换行符了
		{
			
		}
		i++;
	}
	set_fs(oldfs);
}
static struct security_hook_list demo_hooks[]=
{
	LSM_HOOK_INIT(socket_connect,sample_socket_connect),
	LSM_HOOK_INIT(inode_link,sample_inode_link),
	LSM_HOOK_INIT(inode_unlink,sample_inode_unlink),
	LSM_HOOK_INIT(inode_symlink,sample_inode_symlink),
	LSM_HOOK_INIT(inode_mkdir,sample_inode_mkdir),
	LSM_HOOK_INIT(inode_rmdir,sample_inode_rmdir)
};


static  int sample_init(void)
{

	printk(KERN_INFO "ADD LSM SAMPLE.\n");
	security_add_hooks(demo_hooks,ARRAY_SIZE(demo_hooks));

	printk(KERN_INFO "Sample:  Initializing.\n");

	return 0;
}

static  void sample_exit(void)
{
	printk(KERN_INFO "Sample: Exiting.\n");	
}



module_init(sample_init);


module_exit(sample_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("JMH");
MODULE_DESCRIPTION("A LSM Driver implements RBAC");
