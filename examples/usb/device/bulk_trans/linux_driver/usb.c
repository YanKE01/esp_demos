// SPDX-License-Identifier: GPL
#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <linux/timekeeping.h>

#define TX_BUFFER_SIZE     1728
#define TEST_ROUND_COUNT   1000
int count =0;

static const struct usb_device_id id_table[] = {
    {USB_DEVICE(0xcafe, 0x4001)},
    {}};
MODULE_DEVICE_TABLE(usb, id_table);

struct async_context {
    struct usb_device *udev;
    int out_pipe;
    int in_pipe;

    struct completion out_done;
    struct completion in_done;

    atomic_t out_completed;
    atomic_t in_completed;

    ktime_t start;
};

static void bulk_out_urb_complete(struct urb *urb)
{
    struct async_context *ctx = urb->context;
    if (urb->status)
        pr_err("URB OUT error: %d\n", urb->status);

    if (atomic_inc_return(&ctx->out_completed) == TEST_ROUND_COUNT)
        complete(&ctx->out_done);

    usb_free_urb(urb);
    kfree(urb->transfer_buffer);
}

static void bulk_in_urb_complete(struct urb *urb)
{
    struct async_context *ctx = urb->context;
    if (urb->status){
        pr_err("URB IN error: %d\n", urb->status);
    } else {
        count++;
        pr_info("Received %d bytes from device, count: %d\n", urb->actual_length, count);
        // print_hex_dump(KERN_INFO, "USB IN: ", DUMP_PREFIX_OFFSET, 16, 1,
        //                urb->transfer_buffer, urb->actual_length, false);
    }

    if (atomic_inc_return(&ctx->in_completed) == TEST_ROUND_COUNT)
        complete(&ctx->in_done);

    usb_free_urb(urb);
    kfree(urb->transfer_buffer);
}

static int esp_usb_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    struct usb_device *udev = interface_to_usbdev(interface);
    struct usb_host_interface *iface_desc = interface->cur_altsetting;
    struct usb_endpoint_descriptor *endpoint;
    struct async_context *ctx;
    uint8_t *buf;
    int i, retval = 0;
    int out_pipe = 0, in_pipe = 0;

    pr_info("USB device detected.\n");

    for (i = 0; i < iface_desc->desc.bNumEndpoints; i++) {
        endpoint = &iface_desc->endpoint[i].desc;

        if (usb_endpoint_is_bulk_out(endpoint)) {
            out_pipe = usb_sndbulkpipe(udev, endpoint->bEndpointAddress);
            pr_info("Found bulk OUT endpoint: 0x%02x\n", endpoint->bEndpointAddress);
        } else if (usb_endpoint_is_bulk_in(endpoint)) {
            in_pipe = usb_rcvbulkpipe(udev, endpoint->bEndpointAddress);
            pr_info("Found bulk IN endpoint: 0x%02x\n", endpoint->bEndpointAddress);
        }
    }

    if (!out_pipe || !in_pipe) {
        pr_err("Missing required bulk endpoints\n");
        return -ENODEV;
    }

    ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;

    ctx->udev = usb_get_dev(udev);
    ctx->out_pipe = out_pipe;
    ctx->in_pipe = in_pipe;
    init_completion(&ctx->out_done);
    init_completion(&ctx->in_done);
    atomic_set(&ctx->out_completed, 0);
    atomic_set(&ctx->in_completed, 0);

    pr_info("Submitting %d async OUT and IN URBs...\n", TEST_ROUND_COUNT);
    ctx->start = ktime_get();

    for (i = 0; i < TEST_ROUND_COUNT; i++) {
        struct urb *urb = usb_alloc_urb(0, GFP_KERNEL);
        if (!urb) {
            retval = -ENOMEM;
            break;
        }
        buf = kmalloc(TX_BUFFER_SIZE, GFP_KERNEL);
        if (!buf) {
            usb_free_urb(urb);
            retval = -ENOMEM;
            break;
        }
        memset(buf, 0xA5, TX_BUFFER_SIZE);
        usb_fill_bulk_urb(urb, ctx->udev, ctx->out_pipe,
                          buf, TX_BUFFER_SIZE,
                          bulk_out_urb_complete, ctx);
        retval = usb_submit_urb(urb, GFP_KERNEL);
        if (retval) {
            pr_err("Submit OUT failed: %d\n", retval);
            usb_free_urb(urb);
            kfree(buf);
            break;
        }
    }

    // for (i = 0; i < TEST_ROUND_COUNT; i++) {
    //     struct urb *urb = usb_alloc_urb(0, GFP_KERNEL);
    //     if (!urb) {
    //         retval = -ENOMEM;
    //         break;
    //     }
    //     buf = kmalloc(TX_BUFFER_SIZE, GFP_KERNEL);
    //     if (!buf) {
    //         usb_free_urb(urb);
    //         retval = -ENOMEM;
    //         break;
    //     }
    //     usb_fill_bulk_urb(urb, ctx->udev, ctx->in_pipe,
    //                       buf, TX_BUFFER_SIZE,
    //                       bulk_in_urb_complete, ctx);
    //     retval = usb_submit_urb(urb, GFP_KERNEL);
    //     if (retval) {
    //         pr_err("Submit IN failed: %d\n", retval);
    //         usb_free_urb(urb);
    //         kfree(buf);
    //         break;
    //     }
    // }

    wait_for_completion(&ctx->out_done);
    // wait_for_completion(&ctx->in_done);

    ktime_t end = ktime_get();
    s64 duration_ns = ktime_to_ns(ktime_sub(end, ctx->start));
    u64 total_bytes = (u64)TX_BUFFER_SIZE * TEST_ROUND_COUNT * 1;
    u64 speed_kbps = total_bytes * 8 * 1000000 / duration_ns;

    pr_info("Async Bulk IN+OUT: %llu bytes in %lld ns → ~%llu kbps (~%llu KB/s)\n",
            total_bytes, duration_ns, speed_kbps, speed_kbps / 8);

    usb_put_dev(ctx->udev);
    kfree(ctx);
    return retval;
}

static void esp_usb_disconnect(struct usb_interface *interface)
{
    pr_info("USB device disconnected.\n");
}

static struct usb_driver esp_usb_driver = {
    .name = "esp_usb_async_driver",
    .probe = esp_usb_probe,
    .disconnect = esp_usb_disconnect,
    .id_table = id_table,
};

static int __init esp_usb_init(void)
{
    pr_info("ESP_USB async module loaded.\n");
    return usb_register(&esp_usb_driver);
}

static void __exit esp_usb_exit(void)
{
    pr_info("ESP_USB async module unloaded.\n");
    usb_deregister(&esp_usb_driver);
}

module_init(esp_usb_init);
module_exit(esp_usb_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yanke2@espressif.com + ChatGPT");
MODULE_DESCRIPTION("Async USB Bulk IN/OUT Test Driver");
MODULE_VERSION("1.1");
