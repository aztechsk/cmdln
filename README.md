
# cmdln

The **cmdln** C library provides a simple command line interpreter. Commands with various parameters can be registered, and upon
successful parsing of a command with parameters, a callback function is invoked.

### Registration functions and parameter types


command **noargs**

    void add_command_noargs(const char *p_name, void (*p_handler)(void));

command **boolean**

    void add_command_boolean(const char *p_name, void (*p_handler)(boolean_t));

command **char**

    void add_command_char(const char *p_name, void (*p_handler)(char));

command **int**

    void add_command_int(const char *p_name, void (*p_handler)(int));

command **char** + **int**

    void add_command_char_int(const char *p_name, void (*p_handler)(char, int));

command **string**

    void add_command_string(const char *p_name, void (*p_handler)(const char *));

command **char** + **string**

    void add_command_char_string(const char *p_name, void (*p_handler)(char, const char *));

command **int** + **string**

    void add_command_int_string(const char *p_name, void (*p_handler)(int, const char *));

### Library features

- Standardized API (for the AZTech framework).
