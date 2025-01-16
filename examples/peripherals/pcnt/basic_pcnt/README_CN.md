# PCNT

## LEVEL_IO 与 EDGE_IO 的区别

首先 EDGE_IO 才是真正的用来监测脉冲的引脚，LEVEL_IO 是控制计数模式的引脚。

```c
ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
```

在这个模式下，我们设置了 EDGE_IO 在 上升沿和下降沿时的计数模式都是递增。同时 LEVEL_IO 用于控制计数模式，在 LEVEL_IO 在高电平的时候，保持当前的计数模式，当 LEVEL_IO 在低电平的时候，翻转计数模式，即向下递减。