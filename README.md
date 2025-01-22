
# cmdln

The **cmdln** C library provides a simple command-line interpreter. Commands with
various parameters can be registered, and upon successful parsing of a command
with parameters, a callback function is invoked.

### Registration Functions and Parameter Types

**Command with no arguments:**

    void add_command_noargs(const char *p_name, void (*p_handler)(void));

**Command with a boolean parameter:**

    void add_command_boolean(const char *p_name, void (*p_handler)(boolean_t));

**Command with a char parameter:**

    void add_command_char(const char *p_name, void (*p_handler)(char));

**Command with an int parameter:**

    void add_command_int(const char *p_name, void (*p_handler)(int));

**Command with char and int parameters:**

    void add_command_char_int(const char *p_name, void (*p_handler)(char, int));

**Command with a string parameter:**

    void add_command_string(const char *p_name, void (*p_handler)(const char *));

**Command with char and string parameters:**

    void add_command_char_string(const char *p_name, void (*p_handler)(char, const char *));

**Command with int and string parameters:**

    void add_command_int_string(const char *p_name, void (*p_handler)(int, const char *));

### Library Features

- Standardized API (for the AZTech framework).
