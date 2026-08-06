#define PY_SSIZE_T_CLEAN
#include <Python.h>

// BTMHA external declarations
extern void mha_init(char quiet);
extern void mha_cleanup(void);
extern void parse_line(char *line, char quiet);

// Python wrapper for mha_init
static PyObject* py_btmha_init(PyObject *self, PyObject *args) {
    int quiet = 1;
    if (!PyArg_ParseTuple(args, "|p", &quiet)) {
        return NULL;
    }
    mha_init((char)quiet);
    Py_RETURN_NONE;
}

// Python wrapper for mha_cleanup
static PyObject* py_btmha_cleanup(PyObject *self, PyObject *args) {
    mha_cleanup();
    Py_RETURN_NONE;
}

// Python wrapper for parse_line
static PyObject* py_btmha_parse(PyObject *self, PyObject *args) {
    const char *command;
    int quiet = 1;
    
    // Parse the string argument and optional quiet boolean
    if (!PyArg_ParseTuple(args, "s|p", &command, &quiet)) {
        return NULL;
    }
    
    // Create a mutable copy since parse_line might modify the string
    char *cmd_copy = strdup(command);
    if (!cmd_copy) {
        return PyErr_NoMemory();
    }
    
    parse_line(cmd_copy, (char)quiet);
    free(cmd_copy);
    
    Py_RETURN_NONE;
}

// Method definition array
static PyMethodDef BtmhaMethods[] = {
    {"init", py_btmha_init, METH_VARARGS, "Initialize the BTMHA engine. Optional arg: quiet (bool)"},
    {"cleanup", py_btmha_cleanup, METH_NOARGS, "Cleanup BTMHA resources."},
    {"parse", py_btmha_parse, METH_VARARGS, "Parse and evaluate a BTMHA command string."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

// Module definition structure
static struct PyModuleDef btmhamodule = {
    PyModuleDef_HEAD_INIT,
    "btmha",     /* name of module */
    "BTMHA Python API", /* module documentation, may be NULL */
    -1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    BtmhaMethods
};

// Initialization function for the module
PyMODINIT_FUNC PyInit_btmha(void) {
    return PyModule_Create(&btmhamodule);
}
