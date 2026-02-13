# Flanterm

Flanterm is a fast and reasonably complete terminal emulator with support for
multiple output backends. Included is a fast framebuffer backend.

### Quick usage

To quickly set up and use a framebuffer Flanterm instance, it is possible to
use the `flanterm_fb_init()` function as such:
```c
#include <flanterm/flanterm.h>
#include <flanterm/backends/fb.h>

struct flanterm_context *ft_ctx = flanterm_fb_init(
        NULL,
        NULL,
        framebuffer_ptr, width, height, pitch,
        red_mask_size, red_mask_shift,
        green_mask_size, green_mask_shift,
        blue_mask_size, blue_mask_shift,
        NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, NULL,
        NULL, 0, 0, 1,
        0, 0,
        0
    );
```
Where `framebuffer_ptr, width, height, pitch` and `{red,green,blue}_mask_{size,shift}`
represent the corresponding info about the framebuffer to use for this given instance.

The meaning of the other arguments can be found in `backends/fb.h`.

To then print to the terminal instance, simply use the `flanterm_write()`
function on the given instance. For example:
```c
#include <flanterm/flanterm.h>

const char msg[] = "Hello world\n";

flanterm_write(ft_ctx, msg, sizeof(msg));
```
