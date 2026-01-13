#pragma once

#define TASK_NO_PARAM                                       NULL
#define TASK_NO_HANDLER                                     NULL

#define STACK_SIZE_2KB                                      (2 * 1024)
#define STACK_SIZE_4KB                                      (4 * 1024)
#define STACK_SIZE_8KB                                      (8 * 1024)
#define STACK_SIZE_16KB                                     (16 * 1024)
#define STACK_SIZE_32KB                                     (32 * 1024)

#define TASK_GIFPLAYER_STACK_SIZE                           STACK_SIZE_8KB
#define TASK_GIFPLAYER_PRIORITY                             3
#define TASK_GIFPLAYER_CORE                                 0
#define TASK_GIFPLAYER_QUEUE_SIZE                           16