#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

uint64_t syscall0(uint64_t syscall_number);
uint64_t syscall1(uint64_t syscall_number, uint64_t args0);
uint64_t syscall2(uint64_t syscall_number, uint64_t args0, uint64_t args1);
uint64_t syscall3(uint64_t syscall_number, uint64_t args0, uint64_t args1,
                  uint64_t args2);
uint64_t syscall4(uint64_t syscall_number, uint64_t args0, uint64_t args1,
                  uint64_t args2, uint64_t args3);
uint64_t syscall5(uint64_t syscall_number, uint64_t args0, uint64_t args1,
                  uint64_t args2, uint64_t args3, uint64_t args4);
uint64_t syscall6(uint64_t syscall_number, uint64_t args0, uint64_t args1,
                  uint64_t args2, uint64_t args3, uint64_t args4,
                  uint64_t args5);

typedef enum {
  Success = 0,
  Unsuccessful = -1,
  InvalidRange = -2,
  FailedToAllocate = -3,
  NotFound = -4,
  AlreadyExists = -5,
  InvalidArguments = -6,
  NotSupported = -7,
  TimedOut = -8,
  PeerClosed = -9,
  ShouldWait = -10,
  BufferTooSmall = -11,
  TypeMismatch = -12,
} pxstatus_t;

typedef int64_t handle_t;

typedef struct {
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t r11;
  uint64_t r10;
  uint64_t r9;
  uint64_t r8;
  uint64_t rsi;
  uint64_t rdi;
  uint64_t rbp;
  uint64_t rdx;
  uint64_t rcx;
  uint64_t rbx;
  uint64_t rax;
  uint64_t reserved;
  uint64_t reserved1;
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
} __attribute__((packed)) px_context_t;

typedef struct {
  uint64_t r15;
  uint64_t r14;
  uint64_t r13;
  uint64_t r12;
  uint64_t rbx;
  uint64_t rbp;
} __attribute__((packed)) px_fork_context_t;

#define PX_LOG 1
#define PX_CLOSE 2
#define PX_WAIT_FOR_SINGLE_OBJECT 3
#define PX_WAIT_FOR_MULTIPLE_OBJECTS 4
#define PX_NEW_THREAD 5
#define PX_NEW_THREAD_GIVEN_CONTEXT 6
#define PX_GET_THREAD_CONTEXT 7
#define PX_SET_THREAD_CONTEXT 8
#define PX_TERMINATE_THREAD 9
#define PX_NEW_VMB 10
#define PX_MAP_VMB 11
#define PX_UNMAP_VMB 12
#define PX_NEW_PROCESS 13
#define PX_TERMINATE_PROCESS 14
#define PX_CLONE 15
#define PX_NEW_CHANNEL_PAIR 16
#define PX_CHANNEL_SEND 17
#define PX_CHANNEL_RECEIVE 18
#define PX_NEW_EVENT 19
#define PX_EVENT_TRIGGER 20

pxstatus_t px_log(const char *string) {
  return syscall1(PX_LOG, (uintptr_t)string);
}

pxstatus_t px_close(handle_t handle) { return syscall1(PX_CLOSE, handle); }

pxstatus_t px_new_thread(handle_t process_handle, handle_t *thread_handle,
                         uintptr_t ip, uintptr_t sp, size_t arg) {
  return syscall5(PX_NEW_THREAD, process_handle, (uintptr_t)thread_handle, ip,
                  sp, arg);
}

pxstatus_t px_new_thread_given_context(handle_t process_handle,
                                       handle_t *thread_handle, void *context) {
  return syscall3(PX_NEW_THREAD_GIVEN_CONTEXT, process_handle,
                  (uintptr_t)thread_handle, (uintptr_t)context);
}

void px_terminate_thread(void) { syscall0(PX_TERMINATE_THREAD); }

pxstatus_t px_new_process(handle_t *process_handle) {
  return syscall1(PX_NEW_PROCESS, (uintptr_t)process_handle);
}

pxstatus_t px_terminate_process(handle_t process_handle) {
  return syscall1(PX_TERMINATE_PROCESS, process_handle);
}

typedef enum {
  Anon = 0,
  Pager = 1,
} vmb_backing_t;

pxstatus_t px_new_vmb(handle_t *vmb_handle, size_t length,
                      vmb_backing_t backing) {
  return syscall3(PX_NEW_VMB, (uintptr_t)vmb_handle, length, backing);
}

#define VAR_FLAGS_READ (1 << 0)
#define VAR_FLAGS_WRITE (1 << 1)
#define VAR_FLAGS_EXECUTE (1 << 2)
#define VAR_FLAGS_COW (1 << 3)
#define VAR_FLAGS_SHARED (1 << 4)

pxstatus_t px_map_vmb(handle_t process_handle, handle_t vmb_handle,
                      uintptr_t base, size_t length, int protections) {
  return syscall5(PX_MAP_VMB, process_handle, vmb_handle, base, length,
                  protections);
}

pxstatus_t px_unmap_vmb(handle_t process_handle, uintptr_t base,
                        size_t length) {
  return syscall3(PX_UNMAP_VMB, process_handle, base, length);
}

#define WAIT_TIMEOUT_INFINITE (size_t)-1

pxstatus_t px_wait_for_single_object(handle_t object, size_t timeout) {
  return syscall2(PX_WAIT_FOR_SINGLE_OBJECT, object, timeout);
}

pxstatus_t px_wait_for_multiple_objects(handle_t *objects,
                                        size_t number_of_objects, bool wait_all,
                                        size_t timeout,
                                        handle_t *triggered_object) {
  return syscall5(PX_WAIT_FOR_MULTIPLE_OBJECTS, (uintptr_t)objects,
                  number_of_objects, wait_all, timeout,
                  (uintptr_t)triggered_object);
}

#define CLONE_COPY_ADDRESS_SPACE (1 << 0)
#define CLONE_COPY_HANDLES (1 << 1)

pxstatus_t px_clone(handle_t process_from, handle_t process_to, int32_t flags) {
  return syscall3(PX_CLONE, process_from, process_to, flags);
}

pxstatus_t px_new_channel_pair(handle_t *handles) {
  return syscall1(PX_NEW_CHANNEL_PAIR, (uintptr_t)handles);
}

pxstatus_t px_channel_send(handle_t channel_handle, void *buffer, size_t length,
                           handle_t *handles, size_t handle_count) {
  return syscall5(PX_CHANNEL_SEND, channel_handle, (uintptr_t)buffer, length,
                  (uintptr_t)handles, handle_count);
}

pxstatus_t px_channel_receive(handle_t channel_handle, void *buffer,
                              size_t *length, handle_t *handles,
                              size_t *handle_count) {
  return syscall5(PX_CHANNEL_RECEIVE, channel_handle, (uintptr_t)buffer,
                  (uintptr_t)length, (uintptr_t)handles,
                  (uintptr_t)handle_count);
}

pxstatus_t px_new_event(handle_t *event_handle) {
  return syscall1(PX_NEW_EVENT, (uintptr_t)event_handle);
}

pxstatus_t px_event_trigger(handle_t event_handle, bool state) {
  return syscall2(PX_EVENT_TRIGGER, event_handle, state);
}

#define STACK_ADDRESS 0x70000000000ULL
#define CODE_ADDRESS 0x40000000000ULL

#define RANDOM_ADDRESS 0xA0000000000ULL
#define RANDOM_COW_ADDRESS 0xB0000000000ULL

#define px_current_process() -1
#define px_current_thread() -2

extern void die(size_t err);

extern handle_t px_fork(void);

handle_t _px_fork(uintptr_t ip, uintptr_t sp, px_fork_context_t *fork_context) {
  handle_t new_process_handle = 0;
  pxstatus_t status = px_new_process(&new_process_handle);

  if (status != Success) {
    px_log("Failed to create process :(\r\n");
    die(status);
  }

  status = px_clone(px_current_process(), new_process_handle,
                    CLONE_COPY_HANDLES | CLONE_COPY_ADDRESS_SPACE);
  if (status != Success) {
    px_log("Failed to clone process :(\r\n");
    die(status);
  }

  px_context_t thread_context = {0};
  thread_context.rip = ip;
  thread_context.rsp = sp;
  thread_context.rax = 0;

  thread_context.rbx = fork_context->rbx;
  thread_context.rbp = fork_context->rbp;
  thread_context.r12 = fork_context->r12;
  thread_context.r13 = fork_context->r13;
  thread_context.r14 = fork_context->r14;
  thread_context.r15 = fork_context->r15;

  handle_t thread_handle = 0;
  status = px_new_thread_given_context(new_process_handle, &thread_handle,
                                       &thread_context);

  if (status != Success) {
    px_log("Failed to create thread :(\r\n");
    die(status);
  }

  return new_process_handle;
}

void _start(void) {
  px_log("Hello from userspace!\r\n");

  handle_t channel_pair[2] = {0};
  pxstatus_t status = px_new_channel_pair(channel_pair);

  if (status != Success) {
    px_log("Failed to create channel pair :(\r\n");
    die(status);
  }

  handle_t channel_handle = channel_pair[0];
  handle_t new_process_handle = px_fork();

  if (new_process_handle == 0) {
    channel_handle = channel_pair[1];
    px_log("Cloned: Hello I am the cloned process!\r\n");

    px_log("Cloned: Creating an event\r\n");

    handle_t event_handle = 0;
    status = px_new_event(&event_handle);
    if (status != Success) {
      px_log("Failed to create event :(\r\n");
      die(status);
    }
    px_event_trigger(event_handle, false);

    px_log("Cloned: Going to send some data through the channel\r\n");

    char data[] = "Hello from the cloned process!\r\n";
    status =
        px_channel_send(channel_handle, data, sizeof(data), &event_handle, 1);
    if (status != Success) {
      px_log("Failed to send to the channel :(\r\n");
      die(status);
    }

    px_wait_for_single_object(event_handle, WAIT_TIMEOUT_INFINITE);

    px_log("Cloned: Done! bye bye!\r\n");
    px_terminate_process(px_current_process());
  }

  px_log("Going to read from the channel\r\n");
  char buffer[512];
  size_t length_read = sizeof(buffer);

  handle_t event_handle = 0;
  size_t handle_count = 1;
  status = px_channel_receive(channel_pair[0], buffer, &length_read,
                              &event_handle, &handle_count);
  if (status != Success) {
    px_log("Failed to read channel :(\r\n");
    die(status);
  }

  buffer[length_read - 1] = '\0';
  px_log("Got data from the cloned process!\r\n");
  px_log(buffer);

  px_log("Triggering the event now\r\n");
  px_event_trigger(event_handle, true);

  px_log("Done!\r\n");
  for (;;)
    ;
}

#if 0
void _start(void) {
  px_log("Hello from userspace!\r\n");
  px_log("Creating a new process\r\n");

  pxstatus_t status = Success;

  handle_t stack_vmb_handle = 0;
  px_new_vmb(&stack_vmb_handle, 4096, Anon);

  if (status != Success) {
    px_log("Failed to create stack vmb :(\r\n");
    die(status);
  }

  handle_t code_vmb_handle = 0;
  px_new_vmb(&code_vmb_handle, 4096, Anon);

  if (status != Success) {
    px_log("Failed to create code vmb :(\r\n");
    die(status);
  }

  status = px_map_vmb(px_current_process(), code_vmb_handle, RANDOM_ADDRESS,
                      4096, VAR_FLAGS_READ | VAR_FLAGS_WRITE);

  if (status != Success) {
    px_log("Failed to map code vmb :(\r\n");
    die(status);
  }

  char message[] = "Hello from new process!\r\n";

  uint8_t shellcode[26 + sizeof(message)] = {
      0x48, 0x8D, 0x3D, 0x13, 0x00, 0x00, 0x00, // lea rdi, [rip+0x13]
      0xB8, 0x01, 0x00, 0x00, 0x00,             // mov eax, 1
      0x0F, 0x05,                               // syscall
      0xB8, 0x0B, 0x00, 0x00, 0x00,             // mov eax, 11
      0x6a, 0xff,                               // push -1
      0x5f,                                     // pop rdi
      0x0F, 0x05,                               // syscall
      0x0F, 0x0B,                               // ud2
  };

  __builtin_memcpy(&shellcode[26], message, sizeof(message));
  __builtin_memcpy((void *)RANDOM_ADDRESS, shellcode, sizeof(shellcode));

  handle_t process_handle = 0;
  status = px_new_process(&process_handle);

  if (status != Success) {
    px_log("Failed to create process :(\r\n");
    die(status);
  }

  status = px_map_vmb(process_handle, stack_vmb_handle, STACK_ADDRESS - 4096,
                      4096, VAR_FLAGS_READ | VAR_FLAGS_WRITE);

  if (status != Success) {
    px_log("Failed to map in stack in the new process :(\r\n");
    die(status);
  }

  status = px_map_vmb(process_handle, code_vmb_handle, CODE_ADDRESS, 4096,
                      VAR_FLAGS_READ | VAR_FLAGS_WRITE | VAR_FLAGS_EXECUTE);

  handle_t thread_handle = 0;
  status = px_new_thread(process_handle, &thread_handle, CODE_ADDRESS,
                         STACK_ADDRESS, 0);

  if (status != Success) {
    px_log("Failed to create thread :(\r\n");
    die(status);
  }

  // status = px_wait_for_single_object(process_handle, WAIT_TIMEOUT_INFINITE);

  handle_t handles[2] = {process_handle, thread_handle};
  handle_t triggered_object = 0;
  status = px_wait_for_multiple_objects(handles, 2, true, WAIT_TIMEOUT_INFINITE,
                                        &triggered_object);

  if (status != Success) {
    px_log("Failed to wait on objects :(\r\n");
    die(status);
  }

  status = px_unmap_vmb(px_current_process(), RANDOM_ADDRESS, 4096);
  if (status != Success) {
    px_log("Failed to unmap code vmb :(\r\n");
    die(status);
  }

  px_close(thread_handle);
  px_close(process_handle);
  px_close(stack_vmb_handle);
  px_close(code_vmb_handle);

  px_log("Done!\r\n");

  for (;;)
    ;
}
#endif
