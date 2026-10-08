# cmdln

Lightweight command-line parser for FreeRTOS-based embedded applications using the
AZTech framework.

The module allows applications to register named commands with predefined handler
signatures. A writable input line is parsed, validated and synchronously dispatched
to the matching application handler.

## Features

- Runtime registration of commands
- Eight supported command handler signatures
- Validation of boolean, character and integer parameters
- Quoted and unquoted string parameters
- Synchronous command dispatch
- Runtime command list via `cmdln_hlp()`
- Integration with the framework terminal input (`tin`) module

## Integration

The project configuration must be included before `cmdln.h`:

```c
#include "sysconf.h"
#include "cmdln.h"
```

The framework terminal input (`tin`) module must also be enabled (`TERMIN == 1`).
`CMDLN_PARSER` must be defined by the project configuration and set to `1` to
enable the API. The module uses `TERMIN_MAX_ROW_LENGTH` as the maximum accepted
command-line length and `CMDLN_STRING_DELIMITER` as the string delimiter.

A typical terminal input setup passes `parse_line()` as the input callback:

```c
init_tin(uart_rx_byte, uart_dev, parse_line);
```

Commands may be registered from task context during application initialization.
Registration is also supported from task context while parsing is active.

## Command Registration API

Each command is registered with a name and a handler whose signature defines the
expected parameters:

| Function | Handler signature | Parameters |
| --- | --- | --- |
| `add_command_noargs()` | `void handler(void)` | none |
| `add_command_boolean()` | `void handler(boolean_t)` | boolean |
| `add_command_char()` | `void handler(char)` | character |
| `add_command_int()` | `void handler(int)` | integer |
| `add_command_char_int()` | `void handler(char, int)` | character + integer |
| `add_command_string()` | `void handler(const char *)` | string |
| `add_command_char_string()` | `void handler(char, const char *)` | character + string |
| `add_command_int_string()` | `void handler(int, const char *)` | integer + string |

Example:

```c
static void set_mode(int mode)
{
    /* Application code. */
}

static void set_label(const char *label)
{
    /* Application code. */
}

add_command_int("mode", set_mode);
add_command_string("label", set_label);
add_command_noargs("hlp", cmdln_hlp);
```

Command names must be non-empty, must not contain an ASCII space character, and
must not exceed `TERMIN_MAX_ROW_LENGTH`. The name string is not copied by the
module and must remain valid and unchanged for the lifetime of the registration.
Registered commands are permanent; there is no unregister API.

Concurrent registration of different command names is supported. Concurrent
registration of the same command name is a programming error.

## Parameter Syntax

Arguments are separated by the ASCII space character (`' '`). Command names and
textual boolean values are case-sensitive.

### Boolean

Accepted values are:

- false: `0`, `off`, `false`
- true: `1`, `on`, `true`

Example:

```text
feature on
feature false
```

### Character

A character parameter must contain exactly one alphabetic character.

```text
channel A
```

### Integer

Integer parameters are parsed as base-10 values and must fit in the C `int` type.

```text
mode 3
offset -120
```

### String

A string without spaces may be passed without quotes:

```text
label sensor1
```

A string containing spaces must be enclosed by `CMDLN_STRING_DELIMITER`. With the
usual project configuration using `'` as the delimiter:

```text
label 'front sensor'
```

An empty quoted string is also valid:

```text
label ''
```

The string pointer passed to a handler points into the command input buffer. A
handler that needs to retain the value after it returns must copy the string.

## Parsing and Dispatch

```c
void parse_line(char *line);
```

`parse_line()` parses and dispatches one NUL-terminated command line. The supplied
buffer must be writable because parsing modifies it in place. Input longer than
`TERMIN_MAX_ROW_LENGTH` is not dispatched.

The function is intended to be called by one terminal/parser task and is not
reentrant. Command handlers are invoked synchronously in the context of the task
calling `parse_line()`.

Invalid parameter counts, invalid parameter values and unknown commands are
reported through the framework logging interface and are not dispatched to the
application handler.

## Command List

```c
void cmdln_hlp(void);
```

`cmdln_hlp()` prints the names of all registered commands. It can be exposed as a
normal command:

```c
add_command_noargs("hlp", cmdln_hlp);
```

The function must be called from task context.

## Command Acceptance Marker

The public constant:

```c
extern const char *const cmd_accp;
```

contains the command acceptance marker used by framework command handlers. An
application handler may print it before its command-specific output.

## Notes

- Invalid registration arguments, detected duplicate command names and allocation
  failures are treated as fatal framework errors.
- Passing `NULL` to `parse_line()` is a fatal programming error.
- The module is a command parser and dispatcher; terminal input/output transport is
  provided by the surrounding AZTech framework.
