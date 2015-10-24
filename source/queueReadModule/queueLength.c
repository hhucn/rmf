#include <linux/module.h>    
#include <linux/kernel.h>    
#include <linux/init.h>      
#include <linux/netdevice.h>
#include <net/sch_generic.h>
#include <net/neighbour.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>

#define QUEUELENGTH_DEBUG false

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kary");
MODULE_DESCRIPTION("Get information about queues and statuses, which can force a delay or even a drop of packets in the network kernel.");

// Example module insert for ip address 192.168.0.212 on the device eth0:
// insmod queueLength.ko address=3556812992 deviceName="eth0"
//
// address: uint32 representation of an ipv4 address
// deviiceName: self explanatory


static int address = 0x0;
static char *deviceName = "";

module_param(address, uint, 0);
module_param(deviceName, charp, 0000);

extern struct neigh_table arp_tbl;

static __u8 getArpStatus(char* deviceName, int adress) {
    struct neighbour *n;
    struct net_device *dev = dev_get_by_name(&init_net, deviceName);
    if (dev) {
        n = neigh_lookup(&arp_tbl, &adress, dev);
        if (n) {
            if (dev != NULL) dev_put(dev);
            return n->nud_state;
        } else {
            if (QUEUELENGTH_DEBUG) {
                printk(KERN_INFO "Could not find the right Neighbour to get the current arp status!\n");
            }
            if (dev != NULL) dev_put(dev);
            return NUD_NOARP;
        }
    } else {
        if (QUEUELENGTH_DEBUG) {
            printk(KERN_INFO "Could not find the device.\n");
        }
        if (dev != NULL) dev_put(dev);
        return NUD_NONE;
    }
}

static unsigned int getArpQueueLength(char* deviceName, int adress) {
    struct neighbour *n;
    struct net_device *dev = dev_get_by_name(&init_net, deviceName);
    if (dev) {
        n = neigh_lookup(&arp_tbl, &adress, dev);
        if (n) {
            if (QUEUELENGTH_DEBUG) {
                printk(KERN_INFO "Found cur arp-queuelen in bytes: %d.\n", n->arp_queue_len_bytes);
            }
            if (dev != NULL) dev_put(dev);
            return n->arp_queue_len_bytes;
        } else {
            if (QUEUELENGTH_DEBUG) {
                printk(KERN_INFO "Could not find the right Neighbour to get the current arp length!\n");
            }
            goto errorout;
        }
    } else {
        if (QUEUELENGTH_DEBUG) {
            printk(KERN_INFO "Could not find the device.\n");
        }
        goto errorout;
    }
errorout:
    if (dev != NULL) dev_put(dev);
    return -1;
}

static int getDeviceQueueLength(char* deviceName) {
    struct net_device *dev = dev_get_by_name(&init_net, deviceName);
    if (dev) {
        if (QUEUELENGTH_DEBUG) {
            printk(KERN_INFO "Found max queuelen in frames: %lu.\n", dev->tx_queue_len);
        }
        struct Qdisc *q = dev->qdisc;
        if (q) {
            int curQLength = qdisc_qlen(q);
            if (QUEUELENGTH_DEBUG) {
                printk(KERN_INFO "Found cur queuelen in frames: %d.\n", curQLength);
            }
            if (dev != NULL) dev_put(dev);
            return curQLength;
        }
        if (QUEUELENGTH_DEBUG) {
            printk(KERN_INFO "Could not find the qDisc.\n");
        }
    } else {
        if (QUEUELENGTH_DEBUG) {
            printk(KERN_INFO "Could not find the device.\n");
        }
    }
    if (dev != NULL) dev_put(dev);
    return -1;
}





#define procfs_devicequeue_filename "queueLengthDevice"
//struct proc_dir_entry *procfile_deviceQueue;

static int proc_show_deviceQueue(struct seq_file *buffer, void *v) {
    seq_printf(buffer, "%d\n", getDeviceQueueLength(deviceName));
    return 0;
}

static int proc_open_deviceQueue(struct inode *inode, struct file *file) {
    return single_open(file, proc_show_deviceQueue, NULL);
}
static const struct file_operations fops_deviceQueue = {
    .owner = THIS_MODULE,
    .open = proc_open_deviceQueue,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};


#define procfs_arpqueue_filename "queueLengthArp"

static int proc_show_arpQueue(struct seq_file *buffer, void *v) {
    seq_printf(buffer, "%d\n", getArpQueueLength(deviceName, address));
    return 0;
}

static int proc_open_arpQueue(struct inode *inode, struct file *file) {
    return single_open(file, proc_show_arpQueue, NULL);
}
static const struct file_operations fops_arpQueue = {
    .owner = THIS_MODULE,
    .open = proc_open_arpQueue,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

#define procfs_arpstatus_filename "arpStatus"

static int proc_show_arpStatus(struct seq_file *buffer, void *v) {
    seq_printf(buffer, "%d\n", getArpStatus(deviceName, address));
    return 0;
}

static int proc_open_arpStatus(struct inode *inode, struct file *file) {
    return single_open(file, proc_show_arpStatus, NULL);
}
static const struct file_operations fops_arpStatus = {
    .owner = THIS_MODULE,
    .open = proc_open_arpStatus,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static int __init ql_init(void) {
    printk(KERN_INFO "Starting initialisation.\n");

    printk(KERN_INFO "Addressparameter is: %u\n", address);
    printk(KERN_INFO "Devicename is: %s\n", deviceName);

    proc_create(procfs_devicequeue_filename, 0, NULL, &fops_deviceQueue);
    proc_create(procfs_arpqueue_filename, 0, NULL, &fops_arpQueue);
    proc_create(procfs_arpstatus_filename, 0, NULL, &fops_arpStatus);

    printk(KERN_INFO "Ending initialisation.\n");
    return 0;
}

static void __exit ql_exit(void) {
    remove_proc_entry(procfs_devicequeue_filename, NULL);
    remove_proc_entry(procfs_arpqueue_filename, NULL);
    remove_proc_entry(procfs_arpstatus_filename, NULL);
    printk(KERN_INFO "Cleaning up module.\n");
}

module_init(ql_init);
module_exit(ql_exit);
