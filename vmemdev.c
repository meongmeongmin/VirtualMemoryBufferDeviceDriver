#include <linux/moduleparam.h>
#include <linux/version.h> 
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>

#define VMEMDEV_NAME "vmemdev"

static unsigned long buf_size = 1UL << 20;  // 1 MiB
module_param(buf_size, ulong, 0644);    // rw-r--r--
MODULE_PARM_DESC(buf_size, "가상 메모리 버퍼 디바이스 드라이버의 크기");

struct vmemdev
{
    char *buf;
    size_t size;
    struct mutex lock;  // 동기화용 뮤텍스
    struct cdev cdev;   // 캐릭터 디바이스
};

static dev_t vmemdev_devt;  // 디바이스 번호
static struct class *vmemdev_class;
static struct vmemdev vdev; // 디바이스

static char *vmemdev_devnode(const struct device *dev, umode_t *mode)   // 디바이스 권한 설정
{
    if (mode) 
        *mode = 0666;   // rw-rw-rw-
    return NULL;
}

static int vmemdev_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &vdev;
    return 0;
}

static int vmemdev_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t vmemdev_read(struct file *filp, char __user *ubuf, size_t len, loff_t *offset)
{
    return (ssize_t)len;
}

static ssize_t vmemdev_write(struct file *filp, const char __user *ubuf, size_t len, loff_t *offset)
{
    return (ssize_t)len;
}

static loff_t vmemdev_llseek(struct file *filp, loff_t offset, int whence)
{
    return offset;
}

static const struct file_operations vmemdev_fops = {
    .owner   = THIS_MODULE,
    .open    = vmemdev_open,
    .release = vmemdev_release,
    .read    = vmemdev_read,
    .write   = vmemdev_write,
    .llseek  = vmemdev_llseek,
};

static int __init vmemdev_init()
{
    int ret;

    if (buf_size == 0) 
    {
        pr_err(VMEMDEV_NAME ": invalid buf_size = 0\n");
        return -EINVAL;
    }

    vdev.buf = vzalloc((size_t)buf_size);
    if (!vdev.buf) 
    {
        pr_err(VMEMDEV_NAME ": failed to allocate %lu bytes\n", buf_size);
        return -ENOMEM;
    }

    vdev.size = (size_t)buf_size;

    ret = alloc_chrdev_region(&vmemdev_devt, 0, 1, VMEMDEV_NAME);
    if (ret) 
    {
        pr_err(VMEMDEV_NAME ": alloc_chrdev_region failed (%d)\n", ret);
        vfree(vdev.buf);
        return ret;
    }

    cdev_init(&vdev.cdev, &vmemdev_fops);

    ret = cdev_add(&vdev.cdev, vmemdev_devt, 1);
    if (ret) 
    {
        pr_err(VMEMDEV_NAME ": cdev_add failed (%d)\n", ret);
        unregister_chrdev_region(vmemdev_devt, 1);
        vfree(vdev.buf);
        return ret;
    }

    vmemdev_class = class_create(VMEMDEV_NAME);
    if (IS_ERR(vmemdev_class)) 
    {
        ret = PTR_ERR(vmemdev_class);
        pr_err(VMEMDEV_NAME ": class_create failed (%d)\n", ret);
        cdev_del(&vdev.cdev);
        unregister_chrdev_region(vmemdev_devt, 1);
        vfree(vdev.buf);
        return ret;
    }
    
    vmemdev_class->devnode = vmemdev_devnode;   // 0666

    if (IS_ERR(device_create(vmemdev_class, NULL, vmemdev_devt, NULL, VMEMDEV_NAME))) 
    {
        pr_err(VMEMDEV_NAME ": device_create failed\n");
        class_destroy(vmemdev_class);
        cdev_del(&vdev.cdev);
        unregister_chrdev_region(vmemdev_devt, 1);
        vfree(vdev.buf);
        return -ENOMEM;
    }

    pr_info(VMEMDEV_NAME ": initialized (major = %d) size = %zu bytes (/dev/%s)\n", MAJOR(vmemdev_devt), vdev.size, VMEMDEV_NAME);
    return 0;
}

static void __exit vmemdev_exit()
{
    device_destroy(vmemdev_class, vmemdev_devt);
    class_destroy(vmemdev_class);
    cdev_del(&vdev.cdev);
    unregister_chrdev_region(vmemdev_devt, 1);
    vfree(vdev.buf);
    pr_info(VMEMDEV_NAME ": unloaded\n");
}

module_init(vmemdev_init);  // insmod
module_exit(vmemdev_exit);  // rmmod

MODULE_AUTHOR("pr620718");
MODULE_DESCRIPTION("가상 메모리 버퍼 디바이스 드라이버 (/dev/vmemdev)");
MODULE_LICENSE("GPL");