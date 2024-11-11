// SPDX-License-Identifier: GPL-2.0-only
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>

#define UNVME_FIO_IOENGINE	"libunvmed-ioengine.so"

extern char **environ;

extern char *unvme_get_filepath(char *pwd, const char *filename);

static void unvmed_fio_reset(const char *libfio)
{
	void *handle;

	handle = dlopen(libfio, RTLD_NOLOAD);
	if (handle)
		dlclose(handle);

	handle = dlopen(UNVME_FIO_IOENGINE, RTLD_NOLOAD);
	if (handle)
		dlclose(handle);
}

int unvmed_run_fio(int argc, char *argv[], const char *libfio, const char *pwd)
{
	int (*main)(int, char *[], char *[]);
	char **__argv;
	int ret = 0;

	void *fio;
	void *ioengine;

	/*
	 * If the previous app handle has not been closed yet, close here
	 * rather than closing it from the pthread context.  It should be the
	 * same context where dlopen() actually happened.
	 */
	unvmed_fio_reset(libfio);

	/*
	* Load fio binary built as a shared obejct every time `unvme fio`
	* command is invoked.  By freshly reloading the fio code and data to
	* the memory makes fio run as a standalone application.
	*
	* Open fio shared object with RLTD_NOW | RTLD_GLOBAL to guarantee that
	* the following ioengine shared object can refer the fio symbols
	* properly.
	*/
	fio = dlopen(libfio, RTLD_NOW | RTLD_GLOBAL);
	if (!fio) {
		fprintf(stderr, "failed to load shared object '%s'.  "
				"Give proper path to 'unvme start --with-fio=<path/to/fio/so>'\n", libfio);
		return -1;
	}

	/*
	 * Once ioengine shared object is loaded into the current process
	 * context, constructor will be called registering the ioengine to the
	 * fio context.
	 */
	ioengine = dlopen(UNVME_FIO_IOENGINE, RTLD_LAZY);
	if (!ioengine) {
		fprintf(stderr, "failed to load ioengine %s\n", UNVME_FIO_IOENGINE);
		return -1;
	}

	main = dlsym(fio, "main");
	if (dlerror()) {
		fprintf(stderr, "failed to load 'main' symbol in fio. "
				"Maybe forgot to give 'unvme start --with-fio=<path/to/fio/so>'\n");
		return errno;
	}

	/*
	 * Put a default argument '--eta=always' to print output in stdio
	 * successfully.
	 */
	__argv = malloc(sizeof(char *) * (argc + 2));
	for (int i = 0; i < argc; i++) {
		/* job file path */
		if (argv[i][0] != '-')
			__argv[i] = unvme_get_filepath((char *)pwd, argv[i]);
		else
			__argv[i] = argv[i];
	}
	__argv[argc] = "--eta=always";
	__argv[argc + 1] = NULL;

	ret = main(argc + 1, __argv, environ);

	free(__argv);

	dlclose(fio);
	dlclose(ioengine);

	return ret;
}
